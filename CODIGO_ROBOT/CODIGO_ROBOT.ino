#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

/**
 * PROYECTO: ROBOT EXPLORADOR ESP32-CAM (MODO DIAGNÓSTICO)
 * OBJETIVO: Identificar el punto exacto de fallo en el reinicio.
 */

#define PAN_PIN 14
#define TILT_PIN 15
Servo panServo;
Servo tiltServo;

#define M_DER_IN1 12
#define M_DER_IN2 13
#define M_IZQ_IN1 1
#define M_IZQ_IN2 16
#define PIN_ENA 2
#define SPEED_CH 2

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char* ssid     = "Robot Explorador ESP32 CAM";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket wsCamera("/Camera");
AsyncWebSocket wsCarInput("/CarInput");
uint32_t cameraClientId = 0;

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html><html><body><h1>Diagnóstico OK</h1><script>alert("Conectado");</script></body></html>
)HTMLHOMEPAGE";

void moveCar(int cmd) {
  if(cmd==1){ digitalWrite(M_DER_IN1,1); digitalWrite(M_DER_IN2,0); digitalWrite(M_IZQ_IN1,1); digitalWrite(M_IZQ_IN2,0); }
  else if(cmd==2){ digitalWrite(M_DER_IN1,0); digitalWrite(M_DER_IN2,1); digitalWrite(M_IZQ_IN1,0); digitalWrite(M_IZQ_IN2,1); }
  else { digitalWrite(M_DER_IN1,0); digitalWrite(M_DER_IN2,0); digitalWrite(M_IZQ_IN1,0); digitalWrite(M_IZQ_IN2,0); }
}

void setup() {
  // 1. INICIO SERIAL INMEDIATO
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n##################################");
  Serial.println("INICIANDO DIAGNÓSTICO DE SOFTWARE");
  Serial.println("##################################");

  // 2. DESACTIVAR BROWNOUT
  Serial.print("Desactivando Brownout...");
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.println(" OK");

  // 3. CONFIGURACIÓN DE PINES
  Serial.print("Configurando Pines...");
  pinMode(M_DER_IN1, OUTPUT); pinMode(M_DER_IN2, OUTPUT);
  pinMode(M_IZQ_IN1, OUTPUT); pinMode(M_IZQ_IN2, OUTPUT);
  pinMode(PIN_ENA, OUTPUT);
  ledcSetup(SPEED_CH, 1000, 8);
  ledcAttachPin(PIN_ENA, SPEED_CH);
  ledcWrite(SPEED_CH, 0);
  Serial.println(" OK");

  // 4. SERVOS
  Serial.print("Configurando Servos...");
  panServo.attach(PAN_PIN); tiltServo.attach(TILT_PIN);
  panServo.write(90); tiltServo.write(90);
  Serial.println(" OK");

  // 5. WIFI
  Serial.print("Iniciando WiFi...");
  WiFi.softAP(ssid, password);
  Serial.print(" IP: "); Serial.println(WiFi.softAPIP());

  // 6. SERVIDOR WEB
  Serial.print("Configurando Servidor...");
  wsCamera.onEvent([](AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType t, void *a, uint8_t *d, size_t l){
    if(t==WS_EVT_CONNECT) cameraClientId = c->id(); else if(t==WS_EVT_DISCONNECT) cameraClientId = 0;
  });
  server.addHandler(&wsCamera);
  server.addHandler(&wsCarInput);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(200, "text/plain", "Modo Diagnóstico Activo"); });
  server.begin();
  Serial.println(" OK");

  // 7. CÁMARA (Punto más probable de fallo)
  Serial.print("Iniciando Cámara (QVGA)...");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_4; config.ledc_timer = LEDC_TIMER_2;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM; config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM; config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM; config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA; // RESOLUCIÓN MÍNIMA PARA ESTABILIDAD
  config.jpeg_quality = 20;           // MENOR CALIDAD PARA AHORRAR RAM
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("\nERROR CÁMARA: 0x%x", err);
  } else {
    Serial.println(" OK");
  }

  Serial.println("##################################");
  Serial.println("SETUP COMPLETADO SIN REINICIOS");
  Serial.println("##################################");
}

void loop() {
  wsCamera.cleanupClients();
  if (cameraClientId != 0) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (fb) {
      wsCamera.binary(cameraClientId, fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
  }
  delay(10);
}

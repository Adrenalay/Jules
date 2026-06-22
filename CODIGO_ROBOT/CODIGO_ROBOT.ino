#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include "soc/soc.h"           // Librería para el control del hardware del sistema (Brownout)
#include "soc/rtc_cntl_reg.h"  // Registro de control para desactivar el detector de bajo voltaje

/**
 * PROYECTO: ROBOT EXPLORADOR ESP32-CAM (VERSIÓN ESTABLE)
 * MODIFICACIÓN: Desactivado Brownout y optimizado buffers de cámara.
 */

// ===================== CONFIGURACIÓN DE SERVOS =====================
#define PAN_PIN 14
#define TILT_PIN 15
Servo panServo;
Servo tiltServo;

// ===================== CONFIGURACIÓN DE MOTORES =====================
#define M_DER_IN1 12
#define M_DER_IN2 13
#define M_IZQ_IN1 1
#define M_IZQ_IN2 16
#define PIN_ENA 2
#define SPEED_CH 2

// ===================== CONFIGURACIÓN CÁMARA (AI-THINKER) =====================
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
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<style>.btn {background:black; border-radius:20%; width:85px; height:85px; color:red; font-size:35px; border:none; margin:5px;}</style></head>
<body style="text-align:center; font-family:Arial; background:#f0f0f0;">
  <h2 style="background:black; color:white; padding:10px; border-radius:10px;">Robot VIGILANTE UCE</h2>
  <img id="cam" src="" style="width:100%; max-width:400px; border:3px solid black; background:white;">
  <table style="margin:auto">
    <tr><td></td><td><button class="btn" ontouchstart="s('MoveCar','1')" ontouchend="s('MoveCar','0')">▲</button></td><td></td></tr>
    <tr><td><button class="btn" ontouchstart="s('MoveCar','3')" ontouchend="s('MoveCar','0')">◀</button></td><td></td>
    <td><button class="btn" ontouchstart="s('MoveCar','4')" ontouchend="s('MoveCar','0')">▶</button></td></tr>
    <tr><td></td><td><button class="btn" ontouchstart="s('MoveCar','2')" ontouchend="s('MoveCar','0')">▼</button></td><td></td></tr>
  </table>
  <script>
    var wsC=new WebSocket('ws://'+location.hostname+'/Camera');
    var wsI=new WebSocket('ws://'+location.hostname+'/CarInput');
    wsC.onmessage=function(e){document.getElementById('cam').src=URL.createObjectURL(e.data);};
    function s(k,v){if(wsI.readyState==1)wsI.send(k+','+v);}
  </script></body></html>
)HTMLHOMEPAGE";

void moveCar(int cmd) {
  if(cmd==1){ digitalWrite(M_DER_IN1,1); digitalWrite(M_DER_IN2,0); digitalWrite(M_IZQ_IN1,1); digitalWrite(M_IZQ_IN2,0); }
  else if(cmd==2){ digitalWrite(M_DER_IN1,0); digitalWrite(M_DER_IN2,1); digitalWrite(M_IZQ_IN1,0); digitalWrite(M_IZQ_IN2,1); }
  else if(cmd==3){ digitalWrite(M_DER_IN1,1); digitalWrite(M_DER_IN2,0); digitalWrite(M_IZQ_IN1,0); digitalWrite(M_IZQ_IN2,1); }
  else if(cmd==4){ digitalWrite(M_DER_IN1,0); digitalWrite(M_DER_IN2,1); digitalWrite(M_IZQ_IN1,1); digitalWrite(M_IZQ_IN2,0); }
  else { digitalWrite(M_DER_IN1,0); digitalWrite(M_DER_IN2,0); digitalWrite(M_IZQ_IN1,0); digitalWrite(M_IZQ_IN2,0); }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // DESACTIVAR DETECTOR DE BROWNOUT (Previene reinicios por caídas de tensión)

  Serial.begin(115200);
  pinMode(M_DER_IN1, OUTPUT); pinMode(M_DER_IN2, OUTPUT);
  pinMode(M_IZQ_IN1, OUTPUT); pinMode(M_IZQ_IN2, OUTPUT);
  pinMode(PIN_ENA, OUTPUT);

  ledcSetup(SPEED_CH, 1000, 8);
  ledcAttachPin(PIN_ENA, SPEED_CH);
  ledcWrite(SPEED_CH, 150); // Velocidad inicial moderada para no pedir demasiada corriente al arranque

  panServo.attach(PAN_PIN); tiltServo.attach(TILT_PIN);
  panServo.write(90); tiltServo.write(90);

  WiFi.setSleep(false); // Evitar que el WiFi entre en modo de bajo consumo
  WiFi.softAP(ssid, password);

  wsCamera.onEvent([](AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType t, void *a, uint8_t *d, size_t l){
    if(t==WS_EVT_CONNECT) cameraClientId = c->id(); else if(t==WS_EVT_DISCONNECT) cameraClientId = 0;
  });
  wsCarInput.onEvent([](AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType t, void *a, uint8_t *d, size_t l){
    if(t==WS_EVT_DATA){
      String msg = String((char*)d).substring(0, l);
      int comma = msg.indexOf(',');
      if(comma != -1){
        String k = msg.substring(0, comma); int v = msg.substring(comma+1).toInt();
        if(k=="MoveCar") moveCar(v);
        else if(k=="Speed") ledcWrite(SPEED_CH, v);
        else if(k=="Pan") panServo.write(v);
        else if(k=="Tilt") tiltServo.write(v);
      }
    }
  });
  server.addHandler(&wsCamera); server.addHandler(&wsCarInput);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send_P(200, "text/html", htmlHomePage); });
  server.begin();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_4; config.ledc_timer = LEDC_TIMER_2;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM; config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM; config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM; config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000; // Reducido de 20MHz a 10MHz para mayor estabilidad
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 15; // Calidad ligeramente menor para reducir carga de CPU
  config.fb_count = 1;      // Usar solo 1 buffer para ahorrar RAM y corriente

  esp_err_t err = esp_camera_init(&config);
  if (err == ESP_OK) {
    sensor_t * s = esp_camera_sensor_get();
    s->set_vflip(s, 1); // Voltear imagen si es necesario
    s->set_hmirror(s, 1);
  }
  Serial.println("SISTEMA ESTABILIZADO");
}

void loop() {
  wsCamera.cleanupClients(); wsCarInput.cleanupClients();
  if (cameraClientId != 0) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (fb) { wsCamera.binary(cameraClientId, fb->buf, fb->len); esp_camera_fb_return(fb); }
  }
}

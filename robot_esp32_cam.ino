/*******************************************************
 *  PROYECTO: Robot Explorador con ESP32-CAM - MEJORADO
 *  AUTOR ORIGINAL: Electro T3D
 *  MODIFICACIONES: Jules (Mejora de diagnóstico y comunicación)
 *******************************************************/

#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>
#include <ESP32Servo.h>

// ===================== CONFIGURACIÓN DE SERVOS =====================
// MANTENIDO SIN CAMBIOS SEGÚN SOLICITUD
#define PAN_PIN 14
#define TILT_PIN 15
Servo panServo;
Servo tiltServo;

// ===================== CONFIGURACIÓN DE MOTORES =====================
struct MOTOR_PINS
{
  int pinEn;   // PWM velocidad
  int pinIN1;  // Dirección 1
  int pinIN2;  // Dirección 2
};

// NOTA: El pin 1 es TX. El pin 3 ha sido cambiado por el 16 para evitar conflictos seriales.
std::vector<MOTOR_PINS> motorPins =
{
  {2, 12, 13}, // Motor derecho
  {2, 1, 16},  // Motor izquierdo
};

#define LIGHT_PIN 4

// ===================== CONSTANTES DE MOVIMIENTO =====================
#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0
#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1
#define FORWARD 1
#define BACKWARD -1

// ===================== CONFIGURACIÓN PWM =====================
const int PWMFreq = 1000;
const int PWMResolution = 8;
const int PWMSpeedChannel = 2;
const int PWMLightChannel = 3;

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

// ===================== WIFI =====================
const char* ssid     = "Robot Explorador ESP32 CAM";
const char* password = "12345678";

// ===================== SERVIDOR WEB =====================
AsyncWebServer server(80);
AsyncWebSocket wsCamera("/Camera");
AsyncWebSocket wsCarInput("/CarInput");

uint32_t cameraClientId = 0;

// ===================== INTERFAZ WEB HTML =====================
const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
    .arrows { font-size:30px; color:red; }
    td.button { background-color:black; border-radius:25%; box-shadow: 5px 5px #888888; }
    td.button:active { transform: translate(5px,5px); box-shadow: none; }
    .noselect { -webkit-touch-callout: none; -webkit-user-select: none; user-select: none; }
    .slidecontainer { width: 100%; }
    .slider { -webkit-appearance: none; width: 100%; height: 15px; border-radius: 5px; background: #d3d3d3; outline: none; opacity: 0.7; transition: opacity .2s; }
    .slider:hover { opacity: 1; }
    .slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 25px; height: 25px; border-radius: 50%; background: red; cursor: pointer; }
    </style>
  </head>
  <body class="noselect" align="center" style="background-color:white">
    <h2 style="font-family: Arial; color:white; background-color:black; padding:8px; border-radius:10px;">
      Robot VIGILANTE UCE - POR FLEXI PETS
    </h2>
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr><img id="cameraImage" src="" style="width:400px;height:250px"></tr>
      <tr>
        <td></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","1")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8679;</span></td>
        <td></td>
      </tr>
      <tr>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","3")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8678;</span></td>
        <td class="button"></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","4")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8680;</span></td>
      </tr>
      <tr>
        <td></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","2")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8681;</span></td>
        <td></td>
      </tr>
      <tr/><tr/>
      <tr>
        <td style="text-align:left"><b>Velocidad:</b></td>
        <td colspan=2><input type="range" min="0" max="255" value="150" class="slider" id="Speed" oninput='sendButtonInput("Speed",value)'></td>
      </tr>
      <tr>
        <td style="text-align:left"><b>Luz:</b></td>
        <td colspan=2><input type="range" min="0" max="255" value="0" class="slider" id="Light" oninput='sendButtonInput("Light",value)'></td>
      </tr>
      <tr>
        <td style="text-align:left"><b>S. Horizontal:</b></td>
        <td colspan=2><input type="range" min="0" max="180" value="90" class="slider" id="Pan" oninput='sendButtonInput("Pan",value)'></td>
      </tr>
      <tr>
        <td style="text-align:left"><b>S. Vertical:</b></td>
        <td colspan=2><input type="range" min="0" max="180" value="90" class="slider" id="Tilt" oninput='sendButtonInput("Tilt",value)'></td>
      </tr>
    </table>
    <script>
      var webSocketCameraUrl = "ws:\/\/" + window.location.hostname + "/Camera";
      var webSocketCarInputUrl = "ws:\/\/" + window.location.hostname + "/CarInput";
      var websocketCamera, websocketCarInput;
      function initCameraWebSocket() {
        websocketCamera = new WebSocket(webSocketCameraUrl);
        websocketCamera.binaryType = 'blob';
        websocketCamera.onmessage = function(event) { document.getElementById("cameraImage").src = URL.createObjectURL(event.data); };
        websocketCamera.onclose = function() { setTimeout(initCameraWebSocket, 2000); };
      }
      function initCarInputWebSocket() {
        websocketCarInput = new WebSocket(webSocketCarInputUrl);
        websocketCarInput.onopen = function() {
          ["Speed", "Light", "Pan", "Tilt"].forEach(id => sendButtonInput(id, document.getElementById(id).value));
        };
        websocketCarInput.onclose = function() { setTimeout(initCarInputWebSocket, 2000); };
      }
      function sendButtonInput(key, value) { if(websocketCarInput.readyState == 1) websocketCarInput.send(key + "," + value); }
      window.onload = function() { initCameraWebSocket(); initCarInputWebSocket(); };
    </script>
  </body>
</html>
)HTMLHOMEPAGE";

// ===================== CONTROL DE MOTORES =====================
void rotateMotor(int motorNumber, int motorDirection)
{
  if (motorDirection == FORWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  }
  else if (motorDirection == BACKWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);
  }
  else
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  }
}

// ===================== MOVIMIENTO DEL ROBOT =====================
void moveCar(int inputValue)
{
  Serial.printf("Comando Movimiento: %d\n", inputValue);

  switch(inputValue)
  {
    case UP:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);
      break;
    case DOWN:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);
      break;
    case LEFT:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);
      break;
    case RIGHT:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);
      break;
    case STOP:
    default:
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP);
      break;
  }
}

// ===================== SERVIDOR WEB =====================
void handleRoot(AsyncWebServerRequest *request) { request->send_P(200, "text/html", htmlHomePage); }
void handleNotFound(AsyncWebServerRequest *request) { request->send(404, "text/plain", "No encontrado"); }

// ===================== WEBSOCKET CONTROLES =====================
void onCarInputWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_DATA)
  {
    std::string msg((char*)data, len);
    std::istringstream ss(msg);
    std::string key, value;
    getline(ss, key, ',');
    getline(ss, value, ',');
    int val = atoi(value.c_str());

    if (key == "MoveCar") moveCar(val);
    else if (key == "Speed") {
      ledcWrite(PWMSpeedChannel, val);
      Serial.printf("Velocidad seteada a: %d\n", val);
    }
    else if (key == "Light") ledcWrite(PWMLightChannel, val);
    else if (key == "Pan") panServo.write(val);
    else if (key == "Tilt") tiltServo.write(val);
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    moveCar(STOP);
  }
}

void onCameraWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT) cameraClientId = client->id();
  else if (type == WS_EVT_DISCONNECT) cameraClientId = 0;
}

// ===================== CONFIGURAR CÁMARA =====================
void setupCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_4;
  config.ledc_timer = LEDC_TIMER_2;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error al iniciar cámara: 0x%x\n", err);
  } else {
    Serial.println("Cámara iniciada correctamente.");
  }
}

// ===================== ENVÍO DE VIDEO =====================
void sendCameraPicture()
{
  if (cameraClientId == 0) return;
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) return;
  wsCamera.binary(cameraClientId, fb->buf, fb->len);
  esp_camera_fb_return(fb);
  while (true)
  {
    AsyncWebSocketClient * clientPointer = wsCamera.client(cameraClientId);
    if (!clientPointer || !(clientPointer->queueIsFull())) break;
    delay(1);
  }
}

// ===================== CONFIGURACIÓN DE PINES =====================
void setUpPinModes()
{
  // SERVOS: MANTENIDOS SEGÚN SOLICITUD
  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);
  panServo.write(90);
  tiltServo.write(90);

  // PWM MOTORES Y LUZ
  ledcSetup(PWMSpeedChannel, PWMFreq, PWMResolution);
  ledcSetup(PWMLightChannel, PWMFreq, PWMResolution);

  for (int i = 0; i < motorPins.size(); i++)
  {
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);
    pinMode(motorPins[i].pinEn, OUTPUT);
    ledcAttachPin(motorPins[i].pinEn, PWMSpeedChannel);
  }

  pinMode(LIGHT_PIN, OUTPUT);
  ledcAttachPin(LIGHT_PIN, PWMLightChannel);

  moveCar(STOP);
  Serial.println("Pines de motores configurados.");
}

// ===================== SETUP =====================
void setup()
{
  // MEJORA: Inicialización de Serial para diagnóstico
  Serial.begin(115200);
  Serial.println("\n--- ROBOT EXPLORADOR ESP32-CAM ---");

  setUpPinModes();

  Serial.print("Iniciando WiFi...");
  WiFi.softAP(ssid, password);
  Serial.println(" OK.");
  Serial.print("IP del Robot: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  wsCamera.onEvent(onCameraWebSocketEvent);
  server.addHandler(&wsCamera);
  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);
  server.begin();

  setupCamera();
  Serial.println("Servidor iniciado.");
}

// ===================== LOOP =====================
void loop()
{
  wsCamera.cleanupClients();
  wsCarInput.cleanupClients();
  sendCameraPicture();
}

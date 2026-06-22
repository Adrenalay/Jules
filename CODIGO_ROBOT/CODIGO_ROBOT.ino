/*******************************************************
 *  PROYECTO: Robot Explorador con ESP32-CAM - AUTOR: Electro T3D
 *  DESCRIPCIÓN: Versión Original con corrección de Pin 16 y Estabilidad WiFi.
 *******************************************************/

#include "esp_camera.h"      // Librería para controlar la cámara del ESP32-CAM
#include <Arduino.h>         // Librería base de Arduino (funciones principales)
#include <WiFi.h>            // Permite conectar el ESP32 a redes WiFi
#include <AsyncTCP.h>        // Comunicación TCP asíncrona (necesaria para el servidor web)
#include <ESPAsyncWebServer.h> // Crea el servidor web para controlar el robot desde el navegador
#include <iostream>          // Manejo de entrada/salida (uso interno, estilo C++)
#include <sstream>           // Manipulación de textos/strings (procesamiento de datos)
#include <ESP32Servo.h>      // Control de servomotores (pan y tilt de la cámara)
#include "soc/soc.h"           // Para desactivar brownout
#include "soc/rtc_cntl_reg.h"  // Para desactivar brownout

// ===================== CONFIGURACIÓN DE SERVOS =====================
#define PAN_PIN 14     // Servo horizontal
#define TILT_PIN 15    // Servo vertical
Servo panServo;
Servo tiltServo;

// ===================== CONFIGURACIÓN DE MOTORES =====================
struct MOTOR_PINS
{
  int pinEn;   // PWM velocidad
  int pinIN1;  // Dirección 1
  int pinIN2;  // Dirección 2
};

// Pines de motores (derecho e izquierdo)
// CAMBIO REALIZADO: Pin 3 -> 16 para evitar conflicto serial
std::vector<MOTOR_PINS> motorPins =
{
  {2, 12, 13}, // Motor derecho
  {2, 1, 16},  // Motor izquierdo
};

#define LIGHT_PIN 4  // LED frontal

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
const int PWMFreq = 1000;       // Frecuencia 1KHz
const int PWMResolution = 8;    // Resolución 8 bits
const int PWMSpeedChannel = 2;  // Canal velocidad motores
const int PWMLightChannel = 3;  // Canal luz LED

// ===================== CONFIGURACIÓN CÁMARA =====================
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
const char* ssid     = "Robot Explorador ESP32 CAM";     // Nombre de red
const char* password = "12345678";      // Contraseña

// ===================== SERVIDOR WEB =====================
AsyncWebServer server(80);
AsyncWebSocket wsCamera("/Camera");   // WebSocket para video
AsyncWebSocket wsCarInput("/CarInput"); // WebSocket para controles

uint32_t cameraClientId = 0;

// ===================== INTERFAZ WEB HTML ORIGINAL =====================
const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
    .arrows {
      font-size:30px;
      color:red;
    }
    td.button {
      background-color:black;
      border-radius:25%;
      box-shadow: 5px 5px #888888;
    }
    td.button:active {
      transform: translate(5px,5px);
      box-shadow: none;
    }

    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }

    .slidecontainer {
      width: 100%;
    }

    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 15px;
      border-radius: 5px;
      background: #d3d3d3;
      outline: none;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
    }

    .slider:hover {
      opacity: 1;
    }

    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 25px;
      height: 25px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    .slider::-moz-range-thumb {
      width: 25px;
      height: 25px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    </style>

  </head>
  <body class="noselect" align="center" style="background-color:white">

<h2 style="font-family: Arial; color:white; background-color:black; padding:8px; border-radius:10px;">
  Robot VIGILANTE UCE-
  POR FLEXI PETS
</h2>

    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr>
        <img id="cameraImage" src="" style="width:400px;height:250px"></td>
      </tr>
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
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="255" value="150" class="slider" id="Speed" oninput='sendButtonInput("Speed",value)'>
          </div>
        </td>
      </tr>
      <tr>
        <td style="text-align:left"><b>Luz:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="255" value="0" class="slider" id="Light" oninput='sendButtonInput("Light",value)'>
          </div>
        </td>
      </tr>
      <tr>
        <td style="text-align:left"><b>S. Horizontal:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Pan" oninput='sendButtonInput("Pan",value)'>
          </div>
        </td>
      </tr>
      <tr>
        <td style="text-align:left"><b>S. Vertical:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Tilt" oninput='sendButtonInput("Tilt",value)'>
          </div>
        </td>
      </tr>
    </table>

    <script>
      var webSocketCameraUrl = "ws:\/\/" + window.location.hostname + "/Camera";
      var webSocketCarInputUrl = "ws:\/\/" + window.location.hostname + "/CarInput";
      var websocketCamera;
      var websocketCarInput;

      function initCameraWebSocket()
      {
        websocketCamera = new WebSocket(webSocketCameraUrl);
        websocketCamera.binaryType = 'blob';
        websocketCamera.onopen    = function(event){};
        websocketCamera.onclose   = function(event){setTimeout(initCameraWebSocket, 2000);};
        websocketCamera.onmessage = function(event)
        {
          var imageId = document.getElementById("cameraImage");
          imageId.src = URL.createObjectURL(event.data);
        };
      }

      function initCarInputWebSocket()
      {
        websocketCarInput = new WebSocket(webSocketCarInputUrl);
        websocketCarInput.onopen    = function(event)
        {
          sendButtonInput("Speed", document.getElementById("Speed").value);
          sendButtonInput("Light", document.getElementById("Light").value);
          sendButtonInput("Pan", document.getElementById("Pan").value);
          sendButtonInput("Tilt", document.getElementById("Tilt").value);
        };
        websocketCarInput.onclose   = function(event){setTimeout(initCarInputWebSocket, 2000);};
        websocketCarInput.onmessage = function(event){};
      }

      function initWebSocket()
      {
        initCameraWebSocket ();
        initCarInputWebSocket();
      }

      function sendButtonInput(key, value)
      {
        var data = key + "," + value;
        websocketCarInput.send(data);
      }

      window.onload = initWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });
    </script>
  </body>
</html>
)HTMLHOMEPAGE";

// ===================== CONTROL DE MOTORES =====================
void rotateMotor(int motorNumber, int motorDirection)
{
  // Controla la dirección del motor
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
    // Motor detenido
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  }
}

// ===================== MOVIMIENTO DEL ROBOT =====================
void moveCar(int inputValue)
{
  Serial.printf("Valor recibido: %d\n", inputValue);

  switch(inputValue)
  {
    case UP: // Adelante
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);
      break;

    case DOWN: // Atrás
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);
      break;

    case LEFT: // Izquierda
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);
      break;

    case RIGHT: // Derecha
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);
      break;

    case STOP: // Detener
    default:
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP);
      break;
  }
}

// ===================== SERVIDOR WEB =====================
void handleRoot(AsyncWebServerRequest *request)
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "No encontrado");
}

// ===================== WEBSOCKET CONTROLES =====================
void onCarInputWebSocketEvent(AsyncWebSocket *server,
                      AsyncWebSocketClient *client,
                      AwsEventType type,
                      void *arg,
                      uint8_t *data,
                      size_t len)
{
  switch (type)
  {
    case WS_EVT_CONNECT:
      Serial.println("Cliente conectado");
      break;

    case WS_EVT_DATA:
    {
      // Recibir datos tipo: "Speed,150"
      std::string msg((char*)data, len);
      std::istringstream ss(msg);
      std::string key, value;

      getline(ss, key, ',');
      getline(ss, value, ',');

      int val = atoi(value.c_str());

      if (key == "MoveCar") moveCar(val);
      else if (key == "Speed") ledcWrite(PWMSpeedChannel, val);
      else if (key == "Light") ledcWrite(PWMLightChannel, val);
      else if (key == "Pan") panServo.write(val);
      else if (key == "Tilt") tiltServo.write(val);
    }
    break;
  }
}

// ===================== WEBSOCKET CÁMARA =====================
void onCameraWebSocketEvent(AsyncWebSocket *server,
                      AsyncWebSocketClient *client,
                      AwsEventType type,
                      void *arg,
                      uint8_t *data,
                      size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    cameraClientId = client->id(); // Guardar cliente activo
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    cameraClientId = 0;
  }
}

// ===================== CONFIGURAR CÁMARA =====================
void setupCamera()
{
  camera_config_t config;

  // Configuración de pines
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

  // Configuración de imagen
  config.xclk_freq_hz = 10000000; // Estabilidad: Reducido a 10MHz
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // Inicializar cámara
  esp_camera_init(&config);
}

// ===================== ENVÍO DE VIDEO =====================
void sendCameraPicture()
{
  if (cameraClientId == 0) return;

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) return;

  wsCamera.binary(cameraClientId, fb->buf, fb->len);
  esp_camera_fb_return(fb);

  // 🔥 ESTA PARTE ES LA CLAVE
  while (true)
  {
    AsyncWebSocketClient * clientPointer = wsCamera.client(cameraClientId);
    if (!clientPointer || !(clientPointer->queueIsFull()))
    {
      break;
    }
    delay(1);
  }
}

// ===================== CONFIGURACIÓN DE PINES =====================
void setUpPinModes()
{
  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);

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
}

// ===================== SETUP =====================
void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Desactivar brownout para estabilidad
  Serial.begin(115200);

  setUpPinModes();

  WiFi.setSleep(false); // ESTABILIDAD WIFI: Desactivar modo sueño
  WiFi.softAP(ssid, password); // Crear red WiFi

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  wsCamera.onEvent(onCameraWebSocketEvent);
  server.addHandler(&wsCamera);
  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);
  server.begin();
  setupCamera();
}

// ===================== LOOP =====================
void loop()
{
  wsCamera.cleanupClients();
  wsCarInput.cleanupClients();
  sendCameraPicture(); // Enviar video continuamente
}

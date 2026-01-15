/*
  Proyecto: Carro a Control Remoto con Bluetooth
  Autor: Jules
  Fecha: 2023-10-27

  Descripción:
  Este código controla un auto de dos ruedas utilizando un Arduino Nano, un driver L298N y un módulo Bluetooth HC-05.
  Recibe comandos a través del puerto serie desde una aplicación de celular para mover el auto.

  Comandos esperados:
  - 'F': Moverse hacia adelante
  - 'B': Moverse hacia atrás
  - 'L': Girar a la izquierda
  - 'R': Girar a la derecha
  - 'S': Detener los motores
*/

// --- Definición de Pines para el L298N ---

// Motor Izquierdo (Motor A - conectado a OUT1/OUT2)
const int ENA = 9;   // Pin de velocidad (PWM) para el Motor A
const int IN1 = 2;   // Pin de control 1 para el Motor A
const int IN2 = 3;   // Pin de control 2 para el Motor A

// Motor Derecho (Motor B - conectado a OUT3/OUT4)
const int ENB = 10;  // Pin de velocidad (PWM) para el Motor B
const int IN3 = 4;   // Pin de control 1 para el Motor B
const int IN4 = 5;   // Pin de control 2 para el Motor B

// --- Variables Globales ---
char command;         // Variable para almacenar el comando recibido
int speed = 200;      // Velocidad de los motores (0 a 255). Puedes ajustarla.

void setup() {
  // Inicializa la comunicación serie a 9600 bps (velocidad estándar del HC-05)
  Serial.begin(9600);

  // Configura todos los pines de control de motores como SALIDA
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Inicia con los motores detenidos
  stopMotors();
}

void loop() {
  // Verifica si hay datos disponibles en el buffer del puerto serie
  if (Serial.available() > 0) {
    // Lee el primer caracter disponible
    command = Serial.read();

    // Ejecuta una acción basada en el comando recibido
    switch (command) {
      case 'F':
        goForward();
        break;
      case 'B':
        goBackward();
        break;
      case 'R':
        turnRight();
        break;
      case 'L':
        turnLeft();
        break;
      case 'S':
        stopMotors();
        break;
    }
  }
}

// --- Funciones de Movimiento ---

void goForward() {
  // Motor derecho hacia adelante
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Motor izquierdo hacia adelante
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  // Establece la velocidad para ambos motores
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void goBackward() {
  // Motor derecho hacia atrás
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  // Motor izquierdo hacia atrás
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  // Establece la velocidad para ambos motores
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void turnRight() {
  // Motor izquierdo hacia adelante
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Motor derecho hacia atrás
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  // Establece la velocidad para ambos motores
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void turnLeft() {
  // Motor izquierdo hacia atrás
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  // Motor derecho hacia adelante
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  // Establece la velocidad para ambos motores
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void stopMotors() {
  // Apaga ambos motores
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  // Pone la velocidad en 0
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

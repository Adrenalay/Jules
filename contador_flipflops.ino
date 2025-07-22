// Definición de Pines
const int START_PIN = A0;
const int EMERGENCIA_PIN = A1;
const int SENSOR_PIN = A2;
const int MOTOR_PIN = 11;

// Pines para segmentos A-G (cátodo común)
const int pinesSegmento[7] = {2, 3, 4, 5, 6, 7, 8}; // A, B, C, D, E, F, G

// Pines de habilitación para los displays (activos en bajo)
const int PIN_ENABLE_DECENAS = 9;
const int PIN_ENABLE_UNIDADES = 10;

// Tabla de segmentos para números del 0 al 9
const bool tablaDigitos[10][7] = {
  {1, 1, 1, 1, 1, 1, 0}, // 0
  {0, 1, 1, 0, 0, 0, 0}, // 1
  {1, 1, 0, 1, 1, 0, 1}, // 2
  {1, 1, 1, 1, 0, 0, 1}, // 3
  {0, 1, 1, 0, 0, 1, 1}, // 4
  {1, 0, 1, 1, 0, 1, 1}, // 5
  {1, 0, 1, 1, 1, 1, 1}, // 6
  {1, 1, 1, 0, 0, 0, 0}, // 7
  {1, 1, 1, 1, 1, 1, 1}, // 8
  {1, 1, 1, 1, 0, 1, 1}  // 9
};

// Variables Globales del Sistema
bool sistemaActivo = false;
byte contadorHuevos = 0;
bool sensorEstadoAnterior = HIGH; // Asumimos PULLUP, estado inicial no presionado

// Variables para multiplexación
unsigned long tiempoAnteriorMultiplex = 0;
const int intervaloMultiplex = 5; // ms

void setup() {
  // Configurar pines de entrada con PULLUP interno
  pinMode(START_PIN, INPUT_PULLUP);
  pinMode(EMERGENCIA_PIN, INPUT_PULLUP);
  pinMode(SENSOR_PIN, INPUT_PULLUP);

  // Configurar pines de salida para motor y displays
  pinMode(MOTOR_PIN, OUTPUT);
  for (int i = 0; i < 7; i++) {
    pinMode(pinesSegmento[i], OUTPUT);
  }
  pinMode(PIN_ENABLE_DECENAS, OUTPUT);
  pinMode(PIN_ENABLE_UNIDADES, OUTPUT);

  // Inicializar estado de salidas
  digitalWrite(MOTOR_PIN, LOW);
  digitalWrite(PIN_ENABLE_DECENAS, HIGH); // Desactivado
  digitalWrite(PIN_ENABLE_UNIDADES, HIGH); // Desactivado

  Serial.begin(9600);
  Serial.println("Sistema inicializado. Esperando START.");
}

void loop() {
  // 1. Leer entradas
  bool startPresionado = (digitalRead(START_PIN) == LOW);
  bool emergenciaPresionada = (digitalRead(EMERGENCIA_PIN) == LOW);
  bool sensorActual = digitalRead(SENSOR_PIN);

  // 2. Lógica de control del sistema
  if (emergenciaPresionada) {
    sistemaActivo = false;
    Serial.println("EMERGENCIA: Sistema detenido.");
    delay(100); // Pequeño retardo para evitar múltiples detecciones
  }

  if (startPresionado && !sistemaActivo) {
    if (contadorHuevos >= 30) {
      contadorHuevos = 0; // Reiniciar conteo para un nuevo ciclo
      Serial.println("START: Nuevo ciclo. Contador reiniciado.");
    }
    sistemaActivo = true;
    Serial.println("START: Sistema activado.");
    delay(100); // Pequeño retardo para evitar múltiples detecciones
  }

  if (contadorHuevos >= 30) {
    sistemaActivo = false;
  }

  // 3. Lógica del motor
  if (sistemaActivo) {
    digitalWrite(MOTOR_PIN, HIGH);
  } else {
    digitalWrite(MOTOR_PIN, LOW);
  }

  // 4. Lógica del contador de huevos (detección de flanco descendente)
  if (sistemaActivo) {
    if (sensorActual == LOW && sensorEstadoAnterior == HIGH) {
      // Flanco detectado (objeto pasando por el sensor)
      contadorHuevos++;
      Serial.print("Huevo detectado. Conteo: ");
      Serial.println(contadorHuevos);
    }
    sensorEstadoAnterior = sensorActual;
  }

  // 5. Lógica de visualización (multiplexación)
  actualizarDisplays();
}

void mostrarDigito(byte digito) {
  if (digito > 9) return; // No mostrar nada si el dígito es inválido
  for (int i = 0; i < 7; i++) {
    digitalWrite(pinesSegmento[i], tablaDigitos[digito][i]);
  }
}

void actualizarDisplays() {
  byte decenas = contadorHuevos / 10;
  byte unidades = contadorHuevos % 10;

  // Mostrar decenas (con supresión de cero)
  digitalWrite(PIN_ENABLE_UNIDADES, HIGH); // Apagar unidades
  if (decenas > 0) {
    mostrarDigito(decenas);
    digitalWrite(PIN_ENABLE_DECENAS, LOW); // Encender decenas
  } else {
    digitalWrite(PIN_ENABLE_DECENAS, HIGH); // Mantener apagado si es cero
  }
  delay(intervaloMultiplex);

  // Mostrar unidades
  digitalWrite(PIN_ENABLE_DECENAS, HIGH); // Apagar decenas
  mostrarDigito(unidades);
  digitalWrite(PIN_ENABLE_UNIDADES, LOW); // Encender unidades
  delay(intervaloMultiplex);
}

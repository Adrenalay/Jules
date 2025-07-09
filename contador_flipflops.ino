// Contador Ascendente/Descendente con Flip-Flops JK, Reloj Interno y Display 7 Segmentos

// Parte 1: Estructuras Lógicas (Flip-Flop JK)
// Definición de la estructura para un Flip-Flop JK
struct FlipFlopJK {
  bool Q;        // Salida principal
  bool notQ;     // Salida complementaria (Q negada)
  bool prevClk;  // Estado anterior del reloj (para detectar flanco)
  bool CLR;      // Entrada Clear (asíncrona), activa en BAJO. Dejaremos en ALTO (inactiva).
};

// Función universal para actualizar un Flip-Flop JK
void actualizarFlipFlopJK(bool J, bool K, bool clk_signal, bool CLR_input, FlipFlopJK &ff) {
  if (CLR_input == LOW) { // Reseteo asíncrono si CLR es BAJO
    ff.Q = false;
    ff.notQ = true;
  } else {
    // Detectar flanco de subida del reloj (LOW -> HIGH)
    if (ff.prevClk == LOW && clk_signal == HIGH) {
      bool current_J = J;
      bool current_K = K;

      // Lógica interna del Flip-Flop JK
      if (!current_J && !current_K) { // J=0, K=0: Mantener
        // ff.Q = ff.Q; // No cambia
      } else if (!current_J && current_K) { // J=0, K=1: Reset
        ff.Q = false;
      } else if (current_J && !current_K) { // J=1, K=0: Set
        ff.Q = true;
      } else { // J=1, K=1: Toggle
        ff.Q = !ff.Q;
      }
      ff.notQ = !ff.Q;
    }
  }
  ff.prevClk = clk_signal; // Guardar el estado actual del reloj para la próxima detección de flanco
}

// Parte 3: Eliminación de Rebote para Botones
struct Boton {
  int pin;
  bool estadoEstable;
  bool estadoAnterior; // Para la lógica interna de la función antirrebote
  unsigned long ultimoCambio;
};

// Función antirrebote
bool leerBotonSinRebote(Boton &b, unsigned long retardo = 50) { // Aumentado a 50ms para más estabilidad
  bool lecturaActual = digitalRead(b.pin);

  if (lecturaActual != b.estadoAnterior) {
    b.ultimoCambio = millis();
  }

  if ((millis() - b.ultimoCambio) > retardo) {
    if (lecturaActual != b.estadoEstable) {
      b.estadoEstable = lecturaActual;
    }
  }
  b.estadoAnterior = lecturaActual;
  return b.estadoEstable;
}

// Parte 4: Visualización (Display 7 Segmentos)
// Pines para segmentos A–G (compartidos)
const int pinesSegmento[7] = {2, 3, 4, 5, 6, 7, 8}; // A, B, C, D, E, F, G

// Estructura para un display individual
struct Display7Segmentos {
  int digito;       // Número a mostrar (0–9)
  int pinEnable;    // Pin que activa este display (LOW para cátodo común)
};

// Tabla de segmentos para números del 0 al 9 (Cátodo Común: HIGH enciende segmento)
const bool tablaDigitos[10][7] = {
  {1, 1, 1, 1, 1, 1, 0}, // 0: A,B,C,D,E,F
  {0, 1, 1, 0, 0, 0, 0}, // 1: B,C
  {1, 1, 0, 1, 1, 0, 1}, // 2: A,B,D,E,G
  {1, 1, 1, 1, 0, 0, 1}, // 3: A,B,C,D,G
  {0, 1, 1, 0, 0, 1, 1}, // 4: B,C,F,G
  {1, 0, 1, 1, 0, 1, 1}, // 5: A,C,D,F,G
  {1, 0, 1, 1, 1, 1, 1}, // 6: A,C,D,E,F,G
  {1, 1, 1, 0, 0, 0, 0}, // 7: A,B,C
  {1, 1, 1, 1, 1, 1, 1}, // 8: A,B,C,D,E,F,G
  {1, 1, 1, 1, 0, 1, 1}  // 9: A,B,C,D,F,G
};


// --- Definiciones de Pines y Variables Globales ---

// Pines
const int PIN_BOTON_UPDOWN = 11;
const int PIN_ENABLE_DECENAS = 9;
const int PIN_ENABLE_UNIDADES = 10;

// Flip-Flops (FF0 a FF3 para un contador de 4 bits)
FlipFlopJK ff0, ff1, ff2, ff3;

// Reloj Interno (Parte 2)
bool clk = LOW; // Estado actual del reloj
unsigned long tiempo_anterior_reloj = 0; // Para la función millis()
unsigned long periodo_reloj = 500;   // Periodo del reloj en ms (500ms = 2Hz)

// Selector UP/DOWN (Parte 3)
Boton botonUpDown = {PIN_BOTON_UPDOWN, HIGH, HIGH, 0}; // Inicializar estadoEstable y Anterior en HIGH (PULLUP)
bool countUp = true; // true para contar hacia arriba, false para abajo
// bool estadoBotonPrevioEstable = HIGH; // Movido a static dentro de loop()

// Visualización (Parte 4)
Display7Segmentos displays[2]; // 0: Decenas, 1: Unidades
byte contador_valor_actual = 0; // Valor numérico del contador (0-15)

// Lógica Combinacional (Simulación de Compuertas)
bool AND_gate(bool a, bool b) {
  return a && b;
}

bool OR_gate(bool a, bool b) {
  return a || b;
}

bool NOT_gate(bool a) {
  return !a;
}

// --- Fin Definiciones ---

void setup() {
  // Inicializar Serial para depuración
  Serial.begin(9600);
  Serial.println("Iniciando Contador con Flip-Flops JK...");

  // Configuración de Pines
  // Segmentos del display (A-G)
  for (int i = 0; i < 7; i++) {
    pinMode(pinesSegmento[i], OUTPUT);
    digitalWrite(pinesSegmento[i], LOW); // Apagar todos los segmentos inicialmente
  }

  // Pines de habilitación de los displays (cátodo común, LOW para activar)
  pinMode(PIN_ENABLE_DECENAS, OUTPUT);
  pinMode(PIN_ENABLE_UNIDADES, OUTPUT);
  digitalWrite(PIN_ENABLE_DECENAS, HIGH);  // Desactivar display de decenas
  digitalWrite(PIN_ENABLE_UNIDADES, HIGH); // Desactivar display de unidades

  // Pin del botón UP/DOWN
  pinMode(botonUpDown.pin, INPUT_PULLUP); // Usar resistencia PULLUP interna

  // Inicialización de Variables Globales
  // Flip-Flops (Q=0, notQ=1, prevClk=LOW, CLR=HIGH (inactivo))
  ff0 = {false, true, LOW, HIGH};
  ff1 = {false, true, LOW, HIGH};
  ff2 = {false, true, LOW, HIGH};
  ff3 = {false, true, LOW, HIGH};

  // Reloj
  tiempo_anterior_reloj = millis();

  // Displays
  displays[0] = {0, PIN_ENABLE_DECENAS};  // Decenas
  displays[1] = {0, PIN_ENABLE_UNIDADES}; // Unidades

  Serial.println("Setup completado.");
}

// Función para generar el pulso de reloj interno (Parte 2)
void generar_reloj_interno() {
  if (millis() - tiempo_anterior_reloj >= periodo_reloj) {
    clk = !clk; // Invertir el estado del reloj
    tiempo_anterior_reloj = millis();
  }
}

// Función para mostrar un dígito en un display específico (Parte 4)
void mostrarDigitoEnDisplay(Display7Segmentos d) {
  // Apagar ambos displays primero para evitar "ghosting"
  digitalWrite(displays[0].pinEnable, HIGH);
  digitalWrite(displays[1].pinEnable, HIGH);

  // Validar que el dígito esté en el rango 0-9
  int digito_a_mostrar = d.digito;
  if (digito_a_mostrar < 0 || digito_a_mostrar > 9) {
    digito_a_mostrar = 0; // Mostrar 0 si está fuera de rango (o un error, ej. todos los segmentos)
  }

  // Configurar los segmentos según la tabla para el dígito actual
  for (int i = 0; i < 7; i++) {
    // Para cátodo común, un HIGH en el pin de segmento enciende el LED del segmento
    // si tablaDigitos[digito_a_mostrar][i] es true (1).
    digitalWrite(pinesSegmento[i], tablaDigitos[digito_a_mostrar][i] ? HIGH : LOW);
  }

  // Activar el display correcto (LOW para cátodo común)
  digitalWrite(d.pinEnable, LOW);
}


void loop() {
  // 1. Generar el pulso de reloj (Parte 2)
  generar_reloj_interno();

  // 2. Leer y procesar el botón UP/DOWN (Parte 3)
  static bool estadoBotonPrevioEstable = HIGH; // Para detectar el flanco del botón UP/DOWN
  bool estadoBotonActualEstable = leerBotonSinRebote(botonUpDown);
  // Detectar flanco descendente (botón presionado, ya que es INPUT_PULLUP)
  // Cambia el modo solo una vez por presión
  if (estadoBotonActualEstable == LOW && estadoBotonPrevioEstable == HIGH) {
    countUp = !countUp; // Invertir dirección del contador
    Serial.print("Modo cambiado a: ");
    Serial.println(countUp ? "ASCENDENTE" : "DESCENDENTE");
  }
  estadoBotonPrevioEstable = estadoBotonActualEstable; // Actualizar para la próxima iteración

  // Solo actuar en el flanco de subida del reloj principal `clk` para la lógica secuencial
  if (clk == HIGH && ff0.prevClk == LOW) { // Usamos ff0.prevClk como referencia, ya que todos usan el mismo clk
    Serial.println("Flanco de subida detectado.");
    // 3. Lógica Combinacional y Secuencial para Flip-Flops (Parte 1)
    bool J0, K0, J1, K1, J2, K2, J3, K3;

    // FF0 siempre togglea
    J0 = true; K0 = true;

    if (countUp) {
      // Lógica para contador ascendente
      J1 = ff0.Q;
      K1 = ff0.Q;
      J2 = AND_gate(ff0.Q, ff1.Q);
      K2 = AND_gate(ff0.Q, ff1.Q);
      J3 = AND_gate(AND_gate(ff0.Q, ff1.Q), ff2.Q);
      K3 = AND_gate(AND_gate(ff0.Q, ff1.Q), ff2.Q);
    } else {
      // Lógica para contador descendente
      // Para K = J = !Qn : el flip flop hace toggle si Qn es 0, y se mantiene si Qn es 1.
      // Esto significa que si Qn=0, Qn+1 = 1. Si Qn=1, Qn+1 = 1. No es lo que queremos para contar hacia abajo.
      // Para contar hacia abajo, el bit N debe cambiar cuando todos los bits anteriores (0 a N-1) son 0.
      // FF1 togglea si FF0 es 0 (es decir, !ff0.Q)
      // FF2 togglea si FF0 y FF1 son 0 (es decir, !ff0.Q AND !ff1.Q)
      // FF3 togglea si FF0, FF1 y FF2 son 0 (es decir, !ff0.Q AND !ff1.Q AND !ff2.Q)
      // Un flip-flop JK togglea si J=1 y K=1.
      J1 = NOT_gate(ff0.Q);
      K1 = NOT_gate(ff0.Q);
      J2 = AND_gate(NOT_gate(ff0.Q), NOT_gate(ff1.Q));
      K2 = AND_gate(NOT_gate(ff0.Q), NOT_gate(ff1.Q));
      J3 = AND_gate(AND_gate(NOT_gate(ff0.Q), NOT_gate(ff1.Q)), NOT_gate(ff2.Q));
      K3 = AND_gate(AND_gate(NOT_gate(ff0.Q), NOT_gate(ff1.Q)), NOT_gate(ff2.Q));
    }

    // Actualizar los Flip-Flops (solo en el flanco de subida del reloj `clk`)
    // El CLR se mantiene en HIGH (inactivo)
    actualizarFlipFlopJK(J0, K0, clk, HIGH, ff0);
    actualizarFlipFlopJK(J1, K1, clk, HIGH, ff1);
    actualizarFlipFlopJK(J2, K2, clk, HIGH, ff2);
    actualizarFlipFlopJK(J3, K3, clk, HIGH, ff3);

    // Actualizar el valor del contador a partir de las salidas Q de los flip-flops
    contador_valor_actual = (ff3.Q << 3) | (ff2.Q << 2) | (ff1.Q << 1) | ff0.Q;
    Serial.print("Q3 Q2 Q1 Q0: "); Serial.print(ff3.Q); Serial.print(ff2.Q); Serial.print(ff1.Q); Serial.println(ff0.Q);
    Serial.print("Contador: "); Serial.println(contador_valor_actual);

  } // Fin del bloque if (flanco de subida)

  // 4. Visualización (Parte 4) - Se ejecuta continuamente para el barrido
  displays[0].digito = (contador_valor_actual / 10) % 10; // Decenas
  displays[1].digito = contador_valor_actual % 10;      // Unidades

  // Multiplexado de displays
  mostrarDigitoEnDisplay(displays[0]); // Muestra decenas
  delay(5); // Tiempo visible para el display de decenas (ajustar para brillo/parpadeo)

  mostrarDigitoEnDisplay(displays[1]); // Muestra unidades
  delay(5); // Tiempo visible para el display de unidades

}

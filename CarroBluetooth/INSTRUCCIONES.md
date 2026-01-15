# Proyecto: Carro a Control Remoto con Arduino Nano y Bluetooth

¡Hola! Esta guía te ayudará a construir tu propio auto a control remoto. Sigue las instrucciones paso a paso para conectar todos los componentes correctamente.

## Componentes Necesarios

- Arduino Nano
- Módulo L298N (Driver para motores)
- 2 Motores DC (como los Pololu)
- Módulo Bluetooth HC-05
- Batería LiPo 2S (7.4V)
- Interruptor de palanca
- 1 LED (cualquier color)
- 1 Resistencia de 220 ohmios
- Cables de conexión (jumpers)
- Chasis para el auto
- Protoboard (opcional, para facilitar conexiones)

---

## Diagrama de Conexiones

A continuación se detalla cómo conectar cada componente. ¡Presta mucha atención a los pines!

### 1. Alimentación Principal

El L298N distribuirá la energía. La batería se conecta a la entrada de poder del L298N, y de ahí se alimenta el Arduino Nano.

- **Batería LiPo 2S (Terminal Positivo `+`)** -> **Pata central del Interruptor**.
- **Pata lateral del Interruptor** -> **Pin `+12V` del L298N**.
- **Batería LiPo 2S (Terminal Negativo `-`)** -> **Pin `GND` del L298N**.

**Importante**: Asegúrate de que el jumper de 5V esté puesto en el L298N. Esto permite que el L298N regule el voltaje y alimente al Arduino Nano a través de su pin de 5V.

### 2. Conexión del Arduino Nano al L298N

- **Pin `+5V` del L298N** -> **Pin `5V` del Arduino Nano**.
- **Pin `GND` del L298N** -> **Pin `GND` del Arduino Nano**.

### 3. Conexión de los Motores al L298N

- **Motor Izquierdo**: Conecta sus dos cables a los terminales `OUT1` y `OUT2` del L298N.
- **Motor Derecho**: Conecta sus dos cables a los terminales `OUT3` y `OUT4` del L298N.

La polaridad no es crítica en este punto; si un motor gira al revés, simplemente inviertes los cables en el código o en los terminales.

### 4. Conexión de Control (L298N al Arduino Nano)

Estos pines controlan la dirección y velocidad de los motores.

- **Pin `IN1` del L298N** -> **Pin `D2` del Arduino Nano**.
- **Pin `IN2` del L298N** -> **Pin `D3` del Arduino Nano**.
- **Pin `IN3` del L298N** -> **Pin `D4` del Arduino Nano**.
- **Pin `IN4` del L298N** -> **Pin `D5` del Arduino Nano**.

- **Pin `ENA` del L298N** -> **Pin `D9` del Arduino Nano** (PWM para control de velocidad).
- **Pin `ENB` del L298N** -> **Pin `D10` del Arduino Nano** (PWM para control de velocidad).

**Nota**: Asegúrate de remover los jumpers que puedan estar en `ENA` y `ENB` en el L298N.

### 5. Conexión del Módulo Bluetooth HC-05

- **Pin `VCC` del HC-05** -> **Pin `5V` del Arduino Nano**.
- **Pin `GND` del HC-05** -> **Pin `GND` del Arduino Nano**.
- **Pin `TXD` del HC-05** -> **Pin `RX` (D0) del Arduino Nano**.
- **Pin `RXD` del HC-05** -> **Pin `TX` (D1) del Arduino Nano**.

**¡ADVERTENCIA IMPORTANTE!**: Debes desconectar los cables de los pines `RX` y `TX` del Arduino Nano antes de subir el código. Si no lo haces, la carga fallará. Vuelve a conectarlos después de que el código se haya cargado.

### 6. Conexión del LED Indicador

Este LED se encenderá cuando el interruptor principal esté activado.

- **Resistencia de 220Ω** -> Conectada a la **pata positiva (ánodo)** del LED.
- El otro extremo de la **resistencia** -> **Pin `+5V` del L298N** (o cualquier punto que reciba 5V después del interruptor).
- **Pata negativa (cátodo) del LED** -> **Pin `GND` del L298N** (o cualquier punto GND).

---

## Aplicación para Celular y Configuración

Para controlar el auto, necesitarás una aplicación que pueda enviar caracteres a través de Bluetooth.

**Aplicación Recomendada**:
- Busca en la Play Store "Arduino Bluetooth Controller" o una similar. Te recomiendo **"Bluetooth RC Controller"** de "NEXT PROTOTYPES".

**Configuración de la App**:
1.  Conecta tu celular al módulo HC-05 (la contraseña por defecto suele ser `1234` o `0000`).
2.  Abre la aplicación y ve a la configuración de los botones.
3.  Asigna los siguientes caracteres a cada botón:
    - **Adelante**: `F`
    - **Atrás**: `B`
    - **Izquierda**: `L`
    - **Derecha**: `R`
    - **Detener**: `S` (o cualquier otro caracter que no uses).

¡Y listo! Con estas conexiones y configuración, estarás preparado para cargar el código y poner tu auto en movimiento.

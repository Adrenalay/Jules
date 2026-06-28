# Informe Final de Optimización: Robot Explorador ESP32-CAM

Se ha realizado una revisión integral y se han aplicado las optimizaciones solicitadas al sistema "VIGILANCIA-FLEXI PETS".

## 1. Interfaz de Usuario
*   **Encabezado Personalizado:** La página web ahora muestra con orgullo el nombre **"VIGILANCIA-FLEXI PETS"**.

## 2. Correcciones de Movimiento y Control
*   **Inversión de Giro:** Solucionado el problema de dirección (Izquierda/Derecha) mediante el intercambio de pines en la configuración.
*   **Inversión de Servo Pan:** Corregido el giro del servomotor horizontal aplicando una inversión lógica (`180 - val`).
*   **Prevención de Jitter:** Comunicación Serial desactivada para evitar interferencias con el motor en el pin TX (GPIO 1).

## 3. Optimizaciones de Estabilidad (Hardware ESP32-CAM)
*   **Protección contra Reinicios:** Desactivación del detector de Brownout.
*   **WiFi de Alta Respuesta:** Desactivado el modo sleep para eliminar el lag.
*   **Configuración de Cámara Estable:**
    *   Frecuencia XCLK a **10MHz**.
    *   Buffer de imagen único (`fb_count = 1`) para máxima estabilidad de RAM.

## 4. Instrucciones de Configuración en Arduino IDE
Para que el robot funcione correctamente, DEBES configurar el IDE de Arduino así:
1.  **Board:** "AI Thinker ESP32-CAM".
2.  **PSRAM:** "Disabled" (Obligatorio por el uso del GPIO 16).
3.  **Flash Mode:** "QIO".
4.  **Flash Frequency:** "80MHz".

---
*Optimizado para máxima estabilidad y respuesta en tiempo real.*

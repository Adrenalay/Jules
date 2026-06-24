# Informe Final de Optimización: Robot Explorador ESP32-CAM

Se ha realizado una revisión integral y se han aplicado correcciones quirúrgicas al código del sistema "FLEXI PETS".

## 1. Correcciones de Movimiento (NUEVO)

*   **Inversión de Giro:** Se ha solucionado el problema donde el robot giraba a la derecha al presionar izquierda y viceversa. Esto se logró intercambiando lógicamente los pines de los motores en la configuración `motorPins`. Ahora el comportamiento coincide con la interfaz web.
*   **Prevención de Jitter:** Se han comentado las funciones `Serial` en las secciones críticas. Debido a que el motor derecho utiliza el GPIO 1 (TX), cualquier intento de imprimir datos por el puerto serie causaría movimientos erráticos o ruidos en los motores.

## 2. Mejoras de Estabilidad (Implementadas)

*   **Protección Brownout:** Desactivada para prevenir reinicios accidentales por picos de tensión.
*   **WiFi de Baja Latencia:** Se ha desactivado el modo de ahorro de energía del WiFi (`WiFi.setSleep(false)`) para asegurar que el video y los comandos fluyan sin retrasos.
*   **Optimización de Cámara:**
    *   **XCLK a 10MHz:** Mayor estabilidad y menor calor.
    *   **Buffer Simple (fb_count = 1):** Liberación de memoria RAM crítica para el servidor web.

## 3. Advertencias Finales de Hardware

*   **Conflicto GPIO 16 (PSRAM):** El motor derecho utiliza el pin 16. **DEBES desactivar la PSRAM** en el menú `Tools -> PSRAM -> Disabled` del IDE de Arduino. Si no lo haces, el ESP32 se reiniciará al intentar girar.
*   **Energía:** Se recomienda alimentar los motores y el ESP32 con fuentes separadas o usar un condensador de **1000uF** para estabilizar la línea de 5V.

# COM480 - Sistema de Cinturón de Navegación Inteligente

Este repositorio contiene el firmware y el software para el prototipo de asistencia a la navegación utilizando hardware embebido y visión por computadora distribuida.

## Arquitectura del Repositorio
* `/esp32guiaI2C`: Código fuente para el ESP32-CAM (Servidor de video y control de actuadores).
* `/version2cinturon`: Código fuente para el Arduino Uno (Procesamiento de sensores ultrasónicos).

## equisitos e Instalación
1. Descargar e instalar **Arduino IDE** (versión 2.0 o superior recomendada).
2. Para el ESP32-CAM, añadir la URL de tarjetas en *Archivos -> Preferencias -> Gestor de URLs Adicionales*:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Ir al *Gestor de Tarjetas*, buscar **esp32** de Espressif Systems e instalar la versión correspondiente.

## onfiguración
* Abre el archivo `esp32guiaI2C.ino` y modifica las credenciales de red para la transmisión de video:
  ```cpp
  const char* ssid = "TU_SSID_WIFI";
  const char* password = "TU_CONTRASEÑA";

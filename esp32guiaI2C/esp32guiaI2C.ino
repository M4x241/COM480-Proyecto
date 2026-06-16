#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// CAMBIA ESTO POR TU WIFI
const char* ssid = "sfx";
const char* password = "87654321";

// Pines de comunicacion UART (Respetando tu correccion)
HardwareSerial SerialESP(1); 
#define RX_PIN 15 // Conectar al divisor del Pin 3 del Arduino
#define TX_PIN 14 // Conectar al Pin 2 del Arduino

// Configuracion del boton
#define BOTON_PIN 13 
bool rutActiva = false;
bool estadoAnteriorBoton = HIGH; // HIGH porque usaremos PULLUP interno

// Array hardcodeado de 10 movimientos (F=Frente, D=Derecha, I=Izquierda, S=Stop)
char rutaMapeada[10] = {'F', 'F', 'D', 'F', 'F', 'I', 'F', 'F', 'S', 'S'};
int indiceRuta = 0;
unsigned long ultimoTiempoEnvio = 0;

// Pines de la camara AI-Thinker
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

httpd_handle_t stream_httpd = NULL;

// Manejador de la pagina principal con el estado de la variable
static esp_err_t index_handler(httpd_req_t *req) {
  char html_response[512];
  const char* estadoRuta = rutActiva ? "ACTIVADO" : "DESACTIVADO";
  
  snprintf(html_response, sizeof(html_response),
    "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><meta http-equiv=\"refresh\" content=\"2\"><style>body{background-color:#121212;color:#ffffff;font-family:sans-serif;margin:0;display:flex;flex-direction:column;justify-content:center;align-items:center;height:100vh;}h1{color:#4CAF50;}span{font-weight:bold;color:%s;}</style></head><body><h1>Servidor Iniciado Correctamente</h1><p>Estado de la ruta activa: <span>%s</span></p></body></html>",
    rutActiva ? "#4CAF50" : "#F44336", estadoRuta
  );

  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, html_response, strlen(html_response));
}

// Manejador del stream (Mantenido segun estructura solicitada, pero sin bucle de transmision)
static esp_err_t stream_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/plain");
  return httpd_resp_send(req, "Stream deshabilitado", -1);
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  
  httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_handler, .user_ctx = NULL };
  httpd_uri_t stream_uri = { .uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL };
  
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri);
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void setup() {
  Serial.begin(115200);
  SerialESP.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); 

  pinMode(BOTON_PIN, INPUT_PULLUP); // Conectar boton entre GPIO 13 y GND

  // Configuracion de la camara
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM; config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM; config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM; config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM; config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  
  // Evita desbordamiento de memoria
  if(psramFound()){
    config.fb_count = 2;
  } else {
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error al iniciar camara 0x%x", err);
    return;
  }

  // Conexion WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi conectado.");
  Serial.print("Ver estado en: http://");
  Serial.println(WiFi.localIP());

  startCameraServer();
}

void loop() {
  // LECTURA DEL BOTON (Con pequeno debounce)
  bool estadoBoton = digitalRead(BOTON_PIN);
  if (estadoBoton == LOW && estadoAnteriorBoton == HIGH) {
    delay(50); // Antirrebote
    if (digitalRead(BOTON_PIN) == LOW) {
      rutActiva = !rutActiva; // Cambia el estado
      
      if (rutActiva) {
        Serial.println("MODO RUTA ACTIVADO - Enviando comandos...");
        indiceRuta = 0; // Reiniciamos la ruta
      } else {
        Serial.println("MODO RUTA DESACTIVADO - Devolviendo control a sensores.");
        SerialESP.print('X'); // 'X' le dice al Arduino que vuelva a la normalidad
      }
    }
  }
  estadoAnteriorBoton = estadoBoton;

  // ENVIO DE DATOS SI LA RUTA ESTA ACTIVA
  if (rutActiva) {
    unsigned long tiempoActual = millis();
    // Envia el siguiente paso cada 1000 milisegundos (1 segundo)
    if (tiempoActual - ultimoTiempoEnvio >= 1000) {
      
      if (indiceRuta < 10) {
        char movimiento = rutaMapeada[indiceRuta];
        SerialESP.print(movimiento); // Envia el caracter al Arduino
        Serial.print("Enviando paso "); Serial.print(indiceRuta); Serial.print(": "); Serial.println(movimiento);
        indiceRuta++;
      } else {
        // Si termina los 10 pasos, se queda enviando 'S' (Stop) o puedes apagar rutActiva
        SerialESP.print('S');
      }
      
      ultimoTiempoEnvio = tiempoActual;
    }
  }
}
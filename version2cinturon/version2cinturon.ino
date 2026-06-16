#include <SoftwareSerial.h>

// RX en Pin 2, TX en Pin 3 (recuerda el divisor de voltaje)
SoftwareSerial espSerial(2, 3); 

const int trigFrente = 9, echoFrente = 8;
const int trigDerecho = 11, echoDerecho = 10;
const int trigIzquierdo = 13, echoIzquierdo = 12;

const int motorFrente = 7;
const int motorDerecho = 6;
const int motorIzquierdo = 5;

unsigned long prevMillis[3] = {0, 0, 0};
bool motorState[3] = {LOW, LOW, LOW};

// Variable para controlar si manda el ESP32 o mandan los sensores
bool modoESP = false; 

void setup() {
  Serial.begin(115200);
  espSerial.begin(9600); // Comunicación con el ESP32
  
  pinMode(trigFrente, OUTPUT); pinMode(echoFrente, INPUT);
  pinMode(trigDerecho, OUTPUT); pinMode(echoDerecho, INPUT);
  pinMode(trigIzquierdo, OUTPUT); pinMode(echoIzquierdo, INPUT);
  
  pinMode(motorFrente, OUTPUT); pinMode(motorDerecho, OUTPUT); pinMode(motorIzquierdo, OUTPUT);
}

long medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH) * 0.034 / 2;
}

// Función para apagar todos los motores rápido
void apagarMotores() {
  digitalWrite(motorFrente, LOW);
  digitalWrite(motorDerecho, LOW);
  digitalWrite(motorIzquierdo, LOW);
  for(int i=0; i<3; i++) motorState[i] = LOW;
}

void loop() {
  // 1. LEER COMANDOS DEL ESP32-CAM
  if (espSerial.available()) {
    char comando = espSerial.read();
    
    if (comando == 'X') {
      modoESP = false; // Volver a modo sensores
      apagarMotores();
      Serial.println("Modo: SENSORES AUTONOMOS");
    } 
    else if (comando == 'F' || comando == 'D' || comando == 'I' || comando == 'S') {
      modoESP = true; // El ESP32 toma el control
      apagarMotores(); // Apagamos antes de ejecutar el nuevo movimiento
      
      Serial.print("Modo: ESP32 - Ejecutando: ");
      Serial.println(comando);
      
      if (comando == 'F') digitalWrite(motorFrente, HIGH);
      if (comando == 'D') digitalWrite(motorDerecho, HIGH);
      if (comando == 'I') digitalWrite(motorIzquierdo, HIGH);
      // Si es 'S' (Stop), ya se apagaron arriba, no hace nada
    }
  }

  // 2. LÓGICA DE SENSORES (Solo se ejecuta si modoESP es falso)
  if (!modoESP) {
    long dists[3] = {medirDistancia(trigFrente, echoFrente), medirDistancia(trigDerecho, echoDerecho), medirDistancia(trigIzquierdo, echoIzquierdo)};
    int motorPins[3] = {motorFrente, motorDerecho, motorIzquierdo};

    for (int i = 0; i < 3; i++) {
      unsigned long currentMillis = millis();
      
      if (dists[i] > 0 && dists[i] <= 15) {
        gestionarMotor(i, motorPins[i], 500, 500, currentMillis);
      } 
      else if (dists[i] > 15 && dists[i] <= 60) {
        gestionarMotor(i, motorPins[i], 500, 1000, currentMillis);
      } 
      else {
        digitalWrite(motorPins[i], LOW);
        motorState[i] = LOW;
      }
    }
  }
}

void gestionarMotor(int index, int pin, int tiempoOn, int tiempoOff, unsigned long current) {
  if (motorState[index] == LOW) { 
    if (current - prevMillis[index] >= tiempoOff) {
      motorState[index] = HIGH;
      prevMillis[index] = current;
      digitalWrite(pin, HIGH);
    }
  } else { 
    if (current - prevMillis[index] >= tiempoOn) {
      motorState[index] = LOW;
      prevMillis[index] = current;
      digitalWrite(pin, LOW);
    }
  }
}
/*
 * CÓDIGO TRANSMISOR PARA ARDUINO NANO + JOYSTICK
 * Lee los valores de un joystick y los envía vía nRF24L01+
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// --- Configuración del nRF24L01+ ---
#define CE_PIN  9
#define CSN_PIN 10
RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001"; // Misma dirección que en el receptor

// --- Configuración de los pines del Joystick ---
#define JOY_X_PIN A0
#define JOY_Y_PIN A1
#define JOY_SW_PIN 2

// --- Estructura para los datos ---
// Esta caja contendrá la información que enviaremos
struct DatosJoystick {
  int ejeX;
  int ejeY;
  bool boton;
};
DatosJoystick misDatos;

void setup() {
  Serial.begin(9600);
  
  // Configurar pines del joystick
  pinMode(JOY_SW_PIN, INPUT_PULLUP); // Usamos la resistencia PULLUP interna para el botón

  Serial.println("Iniciando Transmisor de Joystick...");

  if (!radio.begin()) {
    Serial.println("Módulo nRF no responde.");
    while (1) {}
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.openWritingPipe(address);
  radio.stopListening();
}

void loop() {
  // 1. Leer los datos del joystick y guardarlos en nuestra estructura
  misDatos.ejeX = analogRead(JOY_X_PIN);
  misDatos.ejeY = analogRead(JOY_Y_PIN);
  misDatos.boton = !digitalRead(JOY_SW_PIN); // Invertimos la lógica. 'true' cuando se presiona.

  // 2. Enviar la estructura de datos completa
  // Usamos sizeof(misDatos) para enviar exactamente el tamaño de nuestra "caja"
  bool ok = radio.write(&misDatos, sizeof(misDatos));

  if (ok) {
    // Imprimimos los datos que estamos enviando (opcional, para depuración)
    Serial.print("Enviado: X=");
    Serial.print(misDatos.ejeX);
    Serial.print(" | Y=");
    Serial.print(misDatos.ejeY);
    Serial.print(" | Botón=");
    Serial.println(misDatos.boton);
  } else {
    Serial.println("Fallo en el envío.");
  }
  
  //delay(100); // Enviamos datos 10 veces por segundo. Puedes ajustar esto.
}
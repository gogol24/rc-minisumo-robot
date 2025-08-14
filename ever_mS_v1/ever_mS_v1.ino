/*
 * CÓDIGO RECEPTOR PARA ESP32-S3
 * Recibe los datos del joystick desde el Nano y los muestra.
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <cstdint> // Incluir para los tipos de datos fijos

// --- Configuración del nRF24L01+ ---
#define NRF_CE_PIN  9
#define NRF_CSN_PIN 10
RF24 radio(NRF_CE_PIN, NRF_CSN_PIN);
const byte address[6] = "00001";

// --- Estructura para los datos del Joystick ---
struct DatosJoystick {
  int16_t ejeX;  // int16_t es SIEMPRE de 2 bytes (16 bits)
  int16_t ejeY;  // Igual que el 'int' del Arduino Nano
  bool boton;
};
DatosJoystick misDatos;


// =======================================================================
//          ▼▼▼▼▼▼▼ ESTA ES LA SECCIÓN QUE CAMBIAMOS ▼▼▼▼▼▼▼
// --- Configuración de pines para el driver TB6612FNG (NUEVA ASIGNACIÓN) ---
#define STBY_PIN 7

// Motor A (Izquierdo)
#define PWMA_PIN 4
#define AIN1_PIN 6 // Cambiado
#define AIN2_PIN 5 // Cambiado

// Motor B (Derecho)
#define PWMB_PIN 17
#define BIN1_PIN 15
#define BIN2_PIN 16

//          ▲▲▲▲▲▲ FIN DE LA SECCIÓN MODIFICADA ▲▲▲▲▲▲
// =======================================================================

// ---------- Anti-jitter en salidas ----------
const int DEADZONE_OUT = 10;   // zona muerta ±30
const int HYST         = 10;   // histéresis extra
const int MIN_DUTY     = 15;   // duty mínimo que mueve el motor

int prevA = 0, prevB = 0;      // recuerdan el último comando

int filtroSalida(int valor, int previo) {
  // 1. zona muerta absoluta
  //if (abs(valor) <= DEADZONE_OUT) return 0;

  // 2. histéresis: si estaba parado, exige un poco más
  if (previo == 0 && abs(valor) < (DEADZONE_OUT + HYST)) return 0;

  // 3. asegurar par mínimo pero conservando proporción
  int signo = (valor > 0) ? 1 : -1;
  int magn  = abs(valor);
  int duty  = map(magn, DEADZONE_OUT, 255, MIN_DUTY, 255);
  return signo * constrain(duty, 0, 255);

}


// --- Configuración del PWM (LEDC) ---
const int PWM_FREQ = 5000;    // Frecuencia en Hz para el PWM
const int PWM_RESOLUTION = 8; // Resolución de 8 bits (0-255)
const int PWM_CHANNEL_A = 0;  // Canal LEDC para motor A
const int PWM_CHANNEL_B = 1;  // Canal LEDC para motor B

// --- Función para controlar un motor ---
// Recibe un motor (canal PWM) y una velocidad (-255 a 255)
// ▼▼▼ FUNCIÓN CORREGIDA ▼▼▼
void moverMotor(int pinPwm, int pinIn1, int pinIn2, float velocidad) {
  velocidad = constrain(velocidad, -255, 255);

  if (velocidad > 0) {
    digitalWrite(pinIn1, HIGH);
    digitalWrite(pinIn2, LOW);
    ledcWrite(pinPwm, velocidad); // Escribir el valor PWM al PIN

  } else if (velocidad < 0) {
    digitalWrite(pinIn1, LOW);
    digitalWrite(pinIn2, HIGH);
    ledcWrite(pinPwm, -velocidad); // Escribir el valor PWM al PIN

  } else {
    digitalWrite(pinIn1, LOW);
    digitalWrite(pinIn2, LOW);
    ledcWrite(pinPwm, 0); // Detener el PWM en el PIN
  }
      Serial.printf("V:%4.0f", 
                  velocidad);
}


const float cuadrado=255.0f*255.0f;

void setup() {
  Serial.begin(115200);
  
  // --- Inicializar pines de los motores ---
  pinMode(STBY_PIN, OUTPUT);
  pinMode(AIN1_PIN, OUTPUT);
  pinMode(AIN2_PIN, OUTPUT);
  pinMode(BIN1_PIN, OUTPUT);
  pinMode(BIN2_PIN, OUTPUT);
  
  // --- Configurar los canales PWM con LEDC ---
  ledcAttach(PWMA_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(PWMB_PIN, PWM_FREQ, PWM_RESOLUTION);

  // Activar el driver
  digitalWrite(STBY_PIN, HIGH);
  
  // --- Inicializar la radio nRF24L01+ ---
  Serial.println("Iniciando Receptor y Controlador de Motores (Pines Actualizados)...");
  if (!radio.begin()) {
    Serial.println("Módulo nRF no responde. Deteniendo.");
    while (1) {}
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(0, address);
  radio.startListening();
  Serial.println("Sistema listo. Esperando datos del joystick...");
}

void loop() {
  if (radio.available()) {
    radio.read(&misDatos, sizeof(misDatos));

    // --- LÓGICA DE CONTROL DEL ROBOT ---

    // 1. Mapear los valores del joystick (0-1023) a un rango de control (-255 a 255)
    float cantidadAvance = map(misDatos.ejeY, 0, 1023, -255, 255);
    float cantidadGiro = map(misDatos.ejeX, 0, 1023, -255, 255);

    // 2. Aplicar una "zona muerta" (deadzone)
    if (fabs(cantidadAvance) < 10) cantidadAvance = 0;
    if (fabs(cantidadGiro) < 2)  cantidadGiro = 0;


    // 3. Mezclar las velocidades
    float velocidadMotorB=cantidadAvance;  
    float velocidadMotorA=cantidadAvance;
    float giro=(cantidadGiro/510);
    if (!cantidadAvance && cantidadGiro){
      velocidadMotorB=-cantidadGiro;  
      velocidadMotorA=cantidadGiro;
    } else if (giro>0){
      velocidadMotorB-=(velocidadMotorB*fabs(giro));
    } else if (giro<0){
      velocidadMotorA-=(velocidadMotorA*fabs(giro));
    }
    
    // 4. Imprimir los datos para depuración

    Serial.printf("JoyX:%4d Y:%4d  ->  A:%6.1f  B:%6.1f  |  Av:%5.0f Gi:%5.0f  giro:%+5.2f\n", 
                  misDatos.ejeX, misDatos.ejeY, velocidadMotorA, velocidadMotorB,cantidadAvance,cantidadGiro,giro);
    
    if (velocidadMotorB) velocidadMotorB=((fabs(velocidadMotorB)-40)*velocidadMotorB/fabs(velocidadMotorB));

    // 5. Enviar los comandos finales a los motores
    moverMotor(PWMB_PIN, BIN1_PIN, BIN2_PIN, velocidadMotorB);
    moverMotor(PWMA_PIN, AIN1_PIN, AIN2_PIN, velocidadMotorA);
    // después de llamar al motor…

  }
}
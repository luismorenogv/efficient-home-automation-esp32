#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

// Definir el pin conectado a GDO0 (Interrupción)
#define CC1101_GDO0_PIN 26
#define CC1101_SS_PIN 5  // Pin para CSN/SS
#define LED_DEBUGGING 2

int led_state = 0;

void setup() {
  // Iniciar comunicación serial
  Serial.begin(115200);
  while (!Serial);

  // Iniciar el módulo CC1101
  ELECHOUSE_cc1101.Init();
  
  // Ahora pasamos los cuatro pines: SCK, MISO, MOSI, SS
  ELECHOUSE_cc1101.setSpiPin(18, 19, 23, CC1101_SS_PIN); // SCK, MISO, MOSI, SS
  ELECHOUSE_cc1101.setGDO0(CC1101_GDO0_PIN);

  // Configurar el módulo CC1101 para recibir
  ELECHOUSE_cc1101.SetRx(); // Modo recepción

  pinMode(LED_DEBUGGING, OUTPUT);

  Serial.println("CC1101 RF Receiver Initialized at 433.92 MHz");
}

void loop() {
  // Verificar si hay datos recibidos
  if (ELECHOUSE_cc1101.CheckReceiveFlag()) {
    if (led_state == 0){
      digitalWrite(LED_DEBUGGING, HIGH);
      led_state = 1;
    }
    else{
      digitalWrite(LED_DEBUGGING, LOW);
      led_state = 0;
    }
    
    // Leer los datos recibidos
    byte data[128];
    byte len = ELECHOUSE_cc1101.ReceiveData(data); // Añadir el tamaño del buffer

    if (len > 0 && len <= 128) {  // Asegúrate de que los datos recibidos son válidos
      Serial.print("Received data: ");
      for (int i = 0; i < len; i++) {
        Serial.print(data[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    } else {
      Serial.println("Error: Data length out of bounds.");
    }
  }

  // Añadir un pequeño retardo
  delay(10);
}

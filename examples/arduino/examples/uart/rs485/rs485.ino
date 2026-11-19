#include <Arduino.h>
#include "hal/uart_types.h"

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);

  if (!Serial1.setPins(-1, -1, -1, RS485_DIR)) {
    Serial.println("RS485 direction pin configuration failed");
    return;
  }
  if (!Serial1.setMode(UART_MODE_RS485_HALF_DUPLEX)) {
    Serial.println("RS485 half-duplex mode configuration failed");
  }
}

void loop() {
  while (Serial1.available()) {
    Serial.write(Serial1.read());
  }
  while (Serial.available()) {
    Serial1.write(Serial.read());
  }
}

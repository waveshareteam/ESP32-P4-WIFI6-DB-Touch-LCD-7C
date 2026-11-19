#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RS232_RX, RS232_TX);
  Serial.println("RS232 serial bridge started");
}

void loop() {
  while (Serial2.available()) {
    Serial.write(Serial2.read());
  }
  while (Serial.available()) {
    Serial2.write(Serial.read());
  }
}

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

#define CE_PIN 9 
#define CSN_PIN 10 
RF24 radio(CE_PIN, CSN_PIN);

const byte addresses[][6] = {"00001", "00002"};

void setup() {
  Serial.begin(115200);
  radio.begin();
  radio.setAutoAck(true);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(125);
  radio.openWritingPipe(addresses[1]); // 00002
  radio.openReadingPipe(1, addresses[0]); // 00001
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    radio.stopListening();
    bool success = radio.write(command.c_str(), sizeof command + 1);
    radio.startListening();

    if (success) {
      Serial.print("Command sent: ");
      Serial.println(command);
    } else {
      Serial.println("Error: Failed to send command");
    }
  }

  if (radio.available()) {
    char receivedData[64];
    radio.read(receivedData, sizeof(receivedData));
    Serial.print("Received data: ");
    Serial.println(receivedData);
  }

  // Check if connection is established
  if (!radio.isChipConnected()) {
    Serial.println("Error: NRF24 module not connected");
  }
}
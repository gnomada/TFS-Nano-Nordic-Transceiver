#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <ArduinoJson.h>

const int RESET_PIN = 7;
int errorCount = 0;

void resetBoard()
{
  delay(1000);
  digitalWrite(RESET_PIN, LOW);
  delay(10);
}

class ErrorManager
{
  public:
    static void printError(int code, int attempt)
    {
      String description;
      String category;

      switch (code)
      {
        case 1:
          description = F("No data received");
          category = F("Conn");
          break;
        case 2:
          description = F("Another error description");
          category = F("Data");
          break;
        default:
          description = F("Unknown error");
          category = F("Unknown");
          break;
      }

      description += " (Attempt " + String(attempt) + ")";

      DynamicJsonDocument errorDoc(128);
      errorDoc["module"]["type"] = "transceptor";
      errorDoc["module"]["name"] = "nordic";
      errorDoc["error"]["code"] = code;
      errorDoc["error"]["category"] = category;
      errorDoc["error"]["description"] = description;

      String errorJson;
      serializeJson(errorDoc, errorJson);

      Serial.println(errorJson);

      errorCount++;

      if (errorCount >= 20)
      {
        DynamicJsonDocument resetDoc(64);
        resetDoc["module"]["type"] = "transceptor";
        resetDoc["module"]["name"] = "nordic";
        resetDoc["info"]["message"] = F("reset");

        String resetJson;
        serializeJson(resetDoc, resetJson);

        Serial.println(resetJson);

        resetBoard();
      }
    }
};

class TelemetryReceiver
{
  private:
    RF24 radio;
    const byte *address;
    int attemptCount;

  public:
    TelemetryReceiver(int cePin, int csnPin, const byte *address)
      : radio(cePin, csnPin), address(address), attemptCount(1) {}

  void begin()
  {
    radio.begin();
    radio.openReadingPipe(0, address);
    radio.startListening();
  }

  void printTelemetry()
  {
    if (radio.available())
    {
      char receivedData[32];
      radio.read(receivedData, sizeof(receivedData));

      // Create a JSON document
      DynamicJsonDocument doc(64);

      if (receivedData[0] == '\0')
      {
        ErrorManager::printError(2, attemptCount);
      }
      
      else
      {
        // Data received
        doc["module"]["type"] = "transceptor";
        doc["module"]["name"] = "nordic";
        doc["data"] = receivedData;

        // Serialize the JSON document to a string
        String jsonOutput;
        serializeJson(doc, jsonOutput);

        // Print the JSON string
        Serial.println(jsonOutput);
      }
    }
    
    else
    {
      ErrorManager::printError(1, attemptCount);
      delay(1000);
    }

    attemptCount++;
  }
};

const byte address[6] = "00001";
TelemetryReceiver telemetryReceiver(9, 10, address);

void setup()
{
  digitalWrite(RESET_PIN, HIGH);
  pinMode(RESET_PIN, OUTPUT);
  Serial.begin(115200);
  telemetryReceiver.begin();
}

void loop()
{
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); // Lee el comando hasta que se recibe un salto de línea
    command.trim(); // Elimina los espacios en blanco al inicio y al final del comando
    
    if (command.equals("connect")) {
      // Espera más comandos en un bucle
      while (true) {
        if (Serial.available() > 0) {
          String nextCommand = Serial.readStringUntil('\n');
          nextCommand.trim();
          
          Serial.println("connect");
          if (nextCommand.equals("disconnect")) {

            break;
          }

        }
      }
    }
  }

  // Procesa la recepción de telemetría como antes
  telemetryReceiver.printTelemetry();
}
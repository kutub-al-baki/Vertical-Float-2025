#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00500";

struct SensorData {
  uint16_t timestamp;
  float temperature;
  float pressure;
  float altitude;
};

SensorData receivedData;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW); // Or RF24_PA_MIN
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, address);
  radio.startListening();

  Serial.println("Receiver ready. Waiting for data...");
}

void loop() {
  if (radio.available()) {
    while (radio.available()) {
      radio.read(&receivedData, sizeof(SensorData));

      Serial.print("Time: ");
      Serial.print(receivedData.timestamp);
      Serial.print(" s | Temp: ");
      Serial.print(receivedData.temperature);
      Serial.print(" °C | Pressure: ");
      Serial.print(receivedData.pressure);
      Serial.print(" mbar | Alt: ");
      Serial.print(receivedData.altitude);
      Serial.println(" m");
    }
  }
}
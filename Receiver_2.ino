#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

struct SensorData {
  uint16_t timestamp;
  float temperature;
  float pressure;
  float altitude;
};

SensorData receivedData;

bool waitingForFake = false;
unsigned long fakeStartTime = 0;
const char* companyID = "PN06";
int diveCount = 1;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, address);
  radio.startListening();

  Serial.println("Receiver ready. Waiting for dives...");
}

void loop() {
  if (!waitingForFake && radio.available()) {
    radio.read(&receivedData, sizeof(SensorData));
    Serial.print("\n--- Dive ");
    Serial.print(diveCount);
    Serial.println(" Started ---");

    printFormatted(receivedData.timestamp, receivedData.pressure / 10.0, receivedData.altitude);

    fakeStartTime = millis();
    waitingForFake = true;
  }

  if (waitingForFake && millis() - fakeStartTime >= 225000UL) {
    Serial.println("----- Ready for dive data -----");
    simulateFakeDive();
    waitingForFake = false;
    diveCount++;
  }
}

void simulateFakeDive() {
  float depth = 2.0;
  float maxDepth = 17.58;
  float depthStep = (maxDepth - depth) / 44.0;

  for (int i = 0; i < 45; i++) {
    float randomVariation = random(-5, 6) / 100.0; // ±0.05m
    float simulatedDepth = depth + (depthStep * i) + randomVariation;
    float simulatedPressure = 101.3 + simulatedDepth * 0.0981; // rough water conversion

    printFormatted(i + 1, simulatedPressure, simulatedDepth);
    delay(1000);
  }

  Serial.print("--- Dive ");
  Serial.print(diveCount);
  Serial.println(" Complete ---\n");
}

void printFormatted(int seconds, float pressure_kPa, float depth_m) {
  int hours = (seconds / 3600) % 24;
  int minutes = (seconds / 60) % 60;
  int secs = seconds % 60;

  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, secs);

  Serial.print(companyID);
  Serial.print(" ");
  Serial.print(timeStr);
  Serial.print(" UTC ");
  Serial.print(pressure_kPa, 1);
  Serial.print(" kPa ");
  Serial.print(depth_m, 2);
  Serial.println(" m");

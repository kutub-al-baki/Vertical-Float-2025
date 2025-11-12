#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include <MS5837.h> // BlueRobotics Bar02 sensor library

#define IN1 6 // Motor control pins
#define IN2 7

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";
const int count = 0;

MS5837 sensor;

// Structure for storing and sending sensor data
struct SensorData {
  uint16_t timestamp;       // seconds since float started dive
  float temperature;        // °C
  float pressure;           // mbar
  float altitude;           // meters (depth from surface)
};

const int DATA_COUNT = 45;
SensorData dataLog[DATA_COUNT];

float altitudeOffset = 0.0;
bool calibrated = false;

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  Serial.begin(9600);
  Wire.begin();

  // Initialize sensor
  if (!sensor.init()) {
    Serial.println("BAR02 Sensor not found!");
    while (1); // Stop if sensor not found
  }

  sensor.setModel(MS5837::MS5837_30BA);
  sensor.setFluidDensity(997); // Freshwater

  // Initialize NRF24L01
  radio.begin();
  radio.setPALevel(RF24_PA_LOW); // Try RF24_PA_MIN if still unstable
  radio.setDataRate(RF24_250KBPS); // Lower speed, better range and reliability
  radio.openWritingPipe(address);
  radio.stopListening(); // Set as transmitter

  // Initial sensor reading and calibration
  sensor.read();
  altitudeOffset = sensor.depth();
  calibrated = true;

  SensorData initialData;
  initialData.timestamp = 0;
  initialData.temperature = sensor.temperature();
  initialData.pressure = sensor.pressure();
  initialData.altitude = 0.0;

  Serial.println("Initial Reading:");
  Serial.print("T: ");
  Serial.print(initialData.temperature);
  Serial.print(" C | P: ");
  Serial.print(initialData.pressure);
  Serial.print(" mbar | Alt: ");
  Serial.print(initialData.altitude);
  Serial.println(" m | Time: 0 s");

  // Send initial data
  if (!radio.write(&initialData, sizeof(SensorData))) {
    Serial.println("Initial data transmission failed!");
  } else {
    Serial.println("Initial data sent successfully.");
  }
}

void collectAndTransmitData() {
  Serial.println("Preparing for dive...");

  delay(180000);

  // Rotate motor clockwise to pull in water (sink)
  Serial.println("Motor rotating clockwise...");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  delay(6200);

  // Stop motor and wait 45s to collect data
  Serial.println("Sinking... Logging data for 45 seconds.");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  unsigned long startTime = millis();

  for (int i = 0; i < DATA_COUNT; i++) {
    sensor.read();
    dataLog[i].timestamp = (millis() - startTime) / 1000;
    dataLog[i].temperature = sensor.temperature();
    dataLog[i].pressure = sensor.pressure();
    dataLog[i].altitude = sensor.depth() - altitudeOffset;

    Serial.print("Logged [");
    Serial.print(i + 1);
    Serial.print("] T: ");
    Serial.print(dataLog[i].temperature);
    Serial.print(" C | P: ");
    Serial.print(dataLog[i].pressure);
    Serial.print(" mbar | Alt: ");
    Serial.print(dataLog[i].altitude);
    Serial.print(" m | Time: ");
    Serial.print(dataLog[i].timestamp);
    Serial.println(" s");

    delay(1000);
  }

  // Rotate motor anticlockwise to surface
  Serial.println("Motor rotating anticlockwise...");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  delay(7200);

  // Stop motor again (optionally wait to stabilize)
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  delay(5000);

  Serial.println("Transmitting collected data...");
  for (int i = 0; i < DATA_COUNT; i++) {
    if (!radio.write(&dataLog[i], sizeof(SensorData))) {
      Serial.print("Data [");
      Serial.print(i + 1);
      Serial.println("] transmission failed!");
    } else {
      Serial.print("Data [");
      Serial.print(i + 1);
      Serial.println("] sent.");
    }
    delay(20); // small delay to avoid congestion
  }

  Serial.println("Cycle complete.\n");
  delay(10000); // Wait before next cycle (optional)
}

void loop() {
  collectAndTransmitData(); // Repeat the float cycle
}
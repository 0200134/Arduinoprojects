#include "DHT.h"

// Define pin numbers
#define DHTPIN 2   // DHT11 sensor data pin connected to digital pin 2
#define LDRPIN A0  // Photoresistor connected to analog pin A0
#define DHTTYPE DHT11  // Define the type of DHT sensor

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

void setup() {
  Serial.begin(9600); // Begin serial communication at 9600 baud rate
  dht.begin(); // Initialize DHT sensor
  pinMode(LDRPIN, INPUT); // Set LDR pin as input
}

void loop() {
  // Read humidity and temperature from DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // Read light intensity from LDR
  int lightIntensity = analogRead(LDRPIN);

  // Check if any reads failed and exit early
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Print temperature, humidity, and light intensity values to the Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" *C, Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, Light Intensity: ");
  Serial.println(lightIntensity);

  delay(2000); // Wait a few seconds between measurements
}

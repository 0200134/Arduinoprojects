#include <Wire.h>
#include "DHT.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

// Pin for DHT11 sensor
#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  if (!bmp.begin()) {
    Serial.print("Could not find a valid BMP085 sensor, check wiring!");
    while (1);
  }
}

void loop() {
  // Read temperature and humidity from DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Read pressure from BMP180
  sensors_event_t event;
  bmp.getEvent(&event);

  if (event.pressure) {
    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    float altitude = bmp.pressureToAltitude(seaLevelPressure, event.pressure, temperature);
    
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" *C, Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Pressure: ");
    Serial.print(event.pressure);
    Serial.print(" hPa, Altitude: ");
    Serial.print(altitude);
    Serial.println(" m");
  } else {
    Serial.println("Failed to read from BMP sensor!");
  }

  delay(2000); // Wait a few seconds between measurements
}
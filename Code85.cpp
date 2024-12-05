#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>

// Define sensor pins
#define DHTPIN 2
#define DHTTYPE DHT22
#define RAIN_PIN A0

// Initialize sensors
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

void setup() {
  Serial.begin(9600);
  dht.begin();
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
}

void loop() {
  // Read data from DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if reading failed
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Read data from BMP280
  float pressure = bmp.readPressure();
  float altitude = bmp.readAltitude(1013.25); // Adjust as necessary for local forecast

  // Read data from rain sensor
  int rainValue = analogRead(RAIN_PIN);

  // Print readings to Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C\tHumidity: ");
  Serial.print(humidity);
  Serial.print(" %\tPressure: ");
  Serial.print(pressure / 100.0F);
  Serial.print(" hPa\tAltitude: ");
  Serial.print(altitude);
  Serial.print(" m\tRain: ");
  Serial.println(rainValue);
  
  // Wait a few seconds before next reading
  delay(2000);
}

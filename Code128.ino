#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <SD.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define WIND_SPEED_PIN 3
#define RAIN_GAUGE_PIN 4
#define SD_CS_PIN 10

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;
File logFile;

void setup() {
  Serial.begin(115200);
  dht.begin();
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1);
  }
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  
  logFile = SD.open("weather_log.txt", FILE_WRITE);
  if (!logFile) {
    Serial.println("Failed to open log file!");
    return;
  }
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float p = bmp.readPressure() / 100.0;
  float windSpeed = analogRead(WIND_SPEED_PIN) * (5.0 / 1023.0) * 2.237;
  int rainGauge = digitalRead(RAIN_GAUGE_PIN);

  if (isnan(h) || isnan(t) || isnan(p)) {
    Serial.println("Failed to read from sensors!");
    return;
  }

  logFile.print("Temperature: ");
  logFile.print(t);
  logFile.print(" C, Humidity: ");
  logFile.print(h);
  logFile.print(" %, Pressure: ");
  logFile.print(p);
  logFile.print(" hPa, Wind Speed: ");
  logFile.print(windSpeed);
  logFile.print(" mph, Rain Gauge: ");
  logFile.println(rainGauge);
  
  logFile.flush();
  delay(60000);  // Log data every minute
}
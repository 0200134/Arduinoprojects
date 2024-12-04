#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define BME_SDA D2
#define BME_SCL D1

Adafruit_BME280 bme;
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
const char* server = "http://your_server_endpoint";  // Replace with your server endpoint

void setup() {
  Serial.begin(115200);
  Wire.begin(BME_SDA, BME_SCL);
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(server);
    http.addHeader("Content-Type", "application/json");

    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;

    String payload = "{\"temperature\":";
    payload += temperature;
    payload += ",\"humidity\":";
    payload += humidity;
    payload += ",\"pressure\":";
    payload += pressure;
    payload += "}";

    int httpResponseCode = http.POST(payload);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
  delay(60000);  // Send data every minute
}
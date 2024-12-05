#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define CURRENT_SENSOR_PIN A0

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
const char* server = "http://your_server_endpoint";  // Replace with your server endpoint

void setup() {
  Serial.begin(115200);
  pinMode(CURRENT_SENSOR_PIN, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  int sensorValue = analogRead(CURRENT_SENSOR_PIN);
  float voltage = (sensorValue / 1023.0) * 5.0;
  float current = (voltage - 2.5) / 0.185;

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(server);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"current\":";
    payload += current;
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
  
  delay(300000);  // Update every 5 minutes
}

#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define SOIL_MOISTURE_SENSOR_PIN A0
#define RELAY_PIN 7

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
const char* server = "http://your_server_endpoint";  // Replace with your server endpoint

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int soilMoistureValue = analogRead(SOIL_MOISTURE_SENSOR_PIN);

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(server);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"temperature\":";
    payload += t;
    payload += ",\"humidity\":";
    payload += h;
    payload += ",\"soil_moisture\":";
    payload += soilMoistureValue;
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

  if (soilMoistureValue < 300) {  // Adjust threshold as needed
    digitalWrite(RELAY_PIN, HIGH);  // Turn on the pump
    Serial.println("Watering the plant");
  } else {
    digitalWrite(RELAY_PIN, LOW);  // Turn off the pump
    Serial.println("Soil moisture is sufficient");
  }
  
  delay(300000);  // Update every 5 minutes
}
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Constants for the sensors
#define DHTPIN 2
#define DHTTYPE DHT22
#define SOIL_MOISTURE_PIN A0
#define LDR_PIN A1
#define RELAY_PIN 3

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);

// Wi-Fi credentials
const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Initialize Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Set up web server
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/control", handleControl);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  server.send(200, "text/html", "<h1>Smart Plant Monitoring System</h1><ul><li><a href='/status'>System Status</a></li><li><a href='/control'>Control Watering</a></li></ul>");
}

void handleStatus() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int soilMoisture = analogRead(SOIL_MOISTURE_PIN);
  int lightLevel = analogRead(LDR_PIN);
  
  if (isnan(temp) || isnan(hum)) {
    server.send(200, "text/plain", "Failed to read from DHT sensor!");
    return;
  }
  
  String message = "Temperature: " + String(temp) + " °C\n";
  message += "Humidity: " + String(hum) + " %\n";
  message += "Soil Moisture: " + String(soilMoisture) + "\n";
  message += "Light Level: " + String(lightLevel);
  
  server.send(200, "text/plain", message);
}

void handleControl() {
  String state = server.arg("state");
  if (state == "on") {
    digitalWrite(RELAY_PIN, HIGH);
    server.send(200, "text/plain", "Watering started");
  } else if (state == "off") {
    digitalWrite(RELAY_PIN, LOW);
    server.send(200, "text/plain", "Watering stopped");
  } else {
    server.send(200, "text/plain", "Invalid command. Use 'on' or 'off'.");
  }
}

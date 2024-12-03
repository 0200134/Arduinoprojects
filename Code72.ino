#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define PIRPIN 3
#define RELAYPIN 4

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  pinMode(PIRPIN, INPUT);
  pinMode(RELAYPIN, OUTPUT);
  
  // Connect to Wi-Fi
  WiFi.begin("your-SSID", "your-PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
  // Set up web server
  server.on("/", handleRoot);
  server.on("/temperature", handleTemperature);
  server.on("/motion", handleMotion);
  server.on("/control", handleControl);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  server.send(200, "text/html", "<h1>Smart Home System</h1><ul><li><a href='/temperature'>Temperature & Humidity</a></li><li><a href='/motion'>Motion Status</a></li><li><a href='/control'>Control Relay</a></li></ul>");
}

void handleTemperature() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (isnan(temp) || isnan(hum)) {
    server.send(200, "text/plain", "Failed to read from DHT sensor!");
    return;
  }
  String message = "Temperature: " + String(temp) + " °C\n";
  message += "Humidity: " + String(hum) + " %";
  server.send(200, "text/plain", message);
}

void handleMotion() {
  int motion = digitalRead(PIRPIN);
  if (motion == HIGH) {
    server.send(200, "text/plain", "Motion detected!");
  } else {
    server.send(200, "text/plain", "No motion detected.");
  }
}

void handleControl() {
  String state = server.arg("state");
  if (state == "on") {
    digitalWrite(RELAYPIN, HIGH);
    server.send(200, "text/plain", "Relay is ON");
  } else if (state == "off") {
    digitalWrite(RELAYPIN, LOW);
    server.send(200, "text/plain", "Relay is OFF");
  } else {
    server.send(200, "text/plain", "Invalid command. Use 'on' or 'off'.");
  }
}
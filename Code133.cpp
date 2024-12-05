#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

// Pin Definitions
#define DHTPIN 2
#define DHTTYPE DHT22
#define RELAY1 3
#define RELAY2 4
#define PIRPIN 6
#define LDRPIN A0

// WiFi settings
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Sensor and server objects
DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);

// Functions to handle web server requests
void handleRootRequest(AsyncWebServerRequest *request) {
  String html = "<html><head><title>Home Automation</title></head><body><h1>Home Automation Control</h1>";
  html += "<p><a href=\"/relay1/on\">Turn Relay 1 ON</a></p>";
  html += "<p><a href=\"/relay1/off\">Turn Relay 1 OFF</a></p>";
  html += "<p><a href=\"/relay2/on\">Turn Relay 2 ON</a></p>";
  html += "<p><a href=\"/relay2/off\">Turn Relay 2 OFF</a></p>";
  html += "<p>Temperature: " + String(dht.readTemperature()) + " &#8451;</p>";
  html += "<p>Humidity: " + String(dht.readHumidity()) + " %</p>";
  html += "<p>Motion Detected: " + String(digitalRead(PIRPIN)) + "</p>";
  html += "<p>Light Level: " + String(analogRead(LDRPIN)) + "</p>";
  html += "</body></html>";
  request->send(200, "text/html", html);
}

void handleRelay1On(AsyncWebServerRequest *request) {
  digitalWrite(RELAY1, LOW);
  request->send(200, "text/html", "<html><body><p>Relay 1 is ON</p></body></html>");
}

void handleRelay1Off(AsyncWebServerRequest *request) {
  digitalWrite(RELAY1, HIGH);
  request->send(200, "text/html", "<html><body><p>Relay 1 is OFF</p></body></html>");
}

void handleRelay2On(AsyncWebServerRequest *request) {
  digitalWrite(RELAY2, LOW);
  request->send(200, "text/html", "<html><body><p>Relay 2 is ON</p></body></html>");
}

void handleRelay2Off(AsyncWebServerRequest *request) {
  digitalWrite(RELAY2, HIGH);
  request->send(200, "text/html", "<html><body><p>Relay 2 is OFF</p></body></html>");
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(PIRPIN, INPUT);
  dht.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Web server routes
  server.on("/", HTTP_GET, handleRootRequest);
  server.on("/relay1/on", HTTP_GET, handleRelay1On);
  server.on("/relay1/off", HTTP_GET, handleRelay1Off);
  server.on("/relay2/on", HTTP_GET, handleRelay2On);
  server.on("/relay2/off", HTTP_GET, handleRelay2Off);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // All handled by AsyncWebServer
}

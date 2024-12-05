#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

ESP8266WebServer server(80);

const int relayPin1 = D1;
const int relayPin2 = D2;
const int relayPin3 = D3;
const int relayPin4 = D4;

void handleRoot() {
  String html = "<html><body><h1>Home Automation System</h1>";
  html += "<p><a href=\"/relay1/on\">Relay 1 ON</a></p>";
  html += "<p><a href=\"/relay1/off\">Relay 1 OFF</a></p>";
  html += "<p><a href=\"/relay2/on\">Relay 2 ON</a></p>";
  html += "<p><a href=\"/relay2/off\">Relay 2 OFF</a></p>";
  html += "<p><a href=\"/relay3/on\">Relay 3 ON</a></p>";
  html += "<p><a href=\"/relay3/off\">Relay 3 OFF</a></p>";
  html += "<p><a href=\"/relay4/on\">Relay 4 ON</a></p>";
  html += "<p><a href=\"/relay4/off\">Relay 4 OFF</a></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleRelay1On() {
  digitalWrite(relayPin1, LOW);
  server.send(200, "text/html", "<p>Relay 1 ON</p><a href=\"/\">Go back</a>");
}

void handleRelay1Off() {
  digitalWrite(relayPin1, HIGH);
  server.send(200, "text/html", "<p>Relay 1 OFF</p><a href=\"/\">Go back</a>");
}

void handleRelay2On() {
  digitalWrite(relayPin2, LOW);
  server.send(200, "text/html", "<p>Relay 2 ON</p><a href=\"/\">Go back</a>");
}

void handleRelay2Off() {
  digitalWrite(relayPin2, HIGH);
  server.send(200, "text/html", "<p>Relay 2 OFF</p><a href=\"/\">Go back</a>");
}

void handleRelay3On() {
  digitalWrite(relayPin3, LOW);
  server.send(200, "text/html", "<p>Relay 3 ON</p><a href=\"/\">Go back</a>");
}

void handleRelay3Off() {
  digitalWrite(relayPin3, HIGH);
  server.send(200, "text/html", "<p>Relay 3 OFF</p><a href=\"/\">Go back</a>");
}

void handleRelay4On() {
  digitalWrite(relayPin4, LOW);
  server.send(200, "text/html", "<p>Relay 4 ON</p><a href=\"/\">Go back</a>");
}

void handleRelay4Off() {
  digitalWrite(relayPin4, HIGH);
  server.send(200, "text/html", "<p>Relay 4 OFF</p><a href=\"/\">Go back</a>");
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(relayPin3, OUTPUT);
  pinMode(relayPin4, OUTPUT);

  digitalWrite(relayPin1, HIGH);
  digitalWrite(relayPin2, HIGH);
  digitalWrite(relayPin3, HIGH);
  digitalWrite(relayPin4, HIGH);

  server.on("/", handleRoot);
  server.on("/relay1/on", handleRelay1On);
  server.on("/relay1/off", handleRelay1Off);
  server.on("/relay2/on", handleRelay2On);
  server.on("/relay2/off", handleRelay2Off);
  server.on("/relay3/on", handleRelay3On);
  server.on("/relay3/off", handleRelay3Off);
  server.on("/relay4/on", handleRelay4On);
  server.on("/relay4/off", handleRelay4Off);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

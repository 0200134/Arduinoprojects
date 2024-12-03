#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <SPIFFS.h>

// Pin Definitions
#define PIR_PIN 2
#define RELAY_PIN 8
#define SERVO_PIN 9

// Replace with your network credentials and server details
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "YOUR_MQTT_SERVER";
const char* weather_api_key = "YOUR_API_KEY"; // OpenWeatherMap API key
const char* weather_api_url = "http://api.openweathermap.org/data/2.5/weather?q=YOUR_CITY&appid=YOUR_API_KEY";

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myServo;

void setup() {
  Serial.begin(9600);
  SPIFFS.begin();
  myServo.attach(SERVO_PIN);
  lcd.begin();
  lcd.backlight();

  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);

  server.on("/", handleRoot);
  server.on("/weather", handleWeather);
  server.serveStatic("/capture", SPIFFS, "/capture.html");
  server.begin();
}

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == "home/security") {
    if (message == "ARM") {
      digitalWrite(RELAY_PIN, HIGH);
      myServo.write(90);
    } else if (message == "DISARM") {
      digitalWrite(RELAY_PIN, LOW);
      myServo.write(0);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ArduinoClient")) {
      client.subscribe("home/security");
    } else {
      delay(5000);
    }
  }
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Home Automation and Security System</h1>";
  html += "<p>Light: <a href=\"/light/on\">ON</a> <a href=\"/light/off\">OFF</a></p>";
  html += "<p>Servo: <a href=\"/servo/90\">90</a> <a href=\"/servo/0\">0</a></p>";
  html += "<p><a href=\"/weather\">Get Weather</a></p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleWeather() {
  HTTPClient http;
  http.begin(weather_api_url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    float temp = doc["main"]["temp"] - 273.15; // Kelvin to Celsius
    int humidity = doc["main"]["humidity"];
    
    String weatherData = "Temp: " + String(temp) + "C\n" + "Humidity: " + String(humidity) + "%";
    server.send(200, "text/plain", weatherData);
  } else {
    server.send(500, "text/plain", "Failed to retrieve weather data");
  }
  http.end();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  server.handleClient();

  int motion = digitalRead(PIR_PIN);
  if (motion == HIGH) {
    String data = "{\"motion\": true, \"time\": \"" + String(millis()) + "\"}";
    client.publish("home/motion", data.c_str());
    
    lcd.setCursor(0, 0);
    lcd.print("Motion Detected!");
    delay(5000);
  } else {
    lcd.clear();
  }

  delay(2000);
}
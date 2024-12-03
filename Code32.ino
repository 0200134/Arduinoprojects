#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP32_Face.h>  // Custom library for facial recognition (hypothetical)
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// Pin Definitions
#define PIR_PIN 33
#define RELAY_PIN 25
#define SERVO_PIN 26

// Replace with your network credentials and server details
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "YOUR_MQTT_SERVER";
const char* cloud_endpoint = "YOUR_CLOUD_ENDPOINT"; // Example: Azure, AWS, Google Cloud

WiFiClient espClient;
PubSubClient client(espClient);
AsyncWebServer server(80);
Adafruit_BME280 bme;
Servo myServo;

void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  
  myServo.attach(SERVO_PIN);

  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false);
  });
  
  server.begin();
  
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  
  FaceRecognition_Init(); // Hypothetical function to initialize facial recognition
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
    if (client.connect("ESP32Client")) {
      client.subscribe("home/security");
    } else {
      delay(5000);
    }
  }
}

void log_to_cloud(String data) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(cloud_endpoint);
    http.addHeader("Content-Type", "application/json");
    
    int httpResponseCode = http.POST(data);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  int motion = digitalRead(PIR_PIN);
  if (motion == HIGH) {
    // Example function call to recognize face (hypothetical)
    bool faceRecognized = FaceRecognition_Check(); 
    
    if (faceRecognized) {
      String data = "{\"motion\": true, \"face\": \"recognized\", \"time\": \"" + String(millis()) + "\"}";
      log_to_cloud(data);
      
      digitalWrite(RELAY_PIN, HIGH);
      lcd.setCursor(0, 0);
      lcd.print("Welcome Home!");
      delay(5000);
      digitalWrite(RELAY_PIN, LOW);
    } else {
      String data = "{\"motion\": true, \"face\": \"unknown\", \"time\": \"" + String(millis()) + "\"}";
      log_to_cloud(data);
      
      lcd.setCursor(0, 0);
      lcd.print("Intruder Alert!");
    }
    delay(5000);
  }
  
  float temp = bme.readTemperature();
  float humidity = bme.readHumidity();
  
  String weatherData = "{\"temperature\": " + String(temp) + ", \"humidity\": " + String(humidity) + "}";
  log_to_cloud(weatherData);
  
  delay(2000);
}
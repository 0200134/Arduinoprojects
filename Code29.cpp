#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define PIR_PIN 2
#define RELAY_PIN 8
#define SERVO_PIN 9
#define CAMERA_PIN 10

Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "YOUR_MQTT_SERVER";
const char* cloud_endpoint = "YOUR_CLOUD_ENDPOINT";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(9600);
  myServo.attach(SERVO_PIN);
  lcd.begin();
  lcd.backlight();
  
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(CAMERA_PIN, OUTPUT);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);
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

void send_to_cloud(String data) {
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

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int motion = digitalRead(PIR_PIN);
  if (motion == HIGH) {
    digitalWrite(CAMERA_PIN, HIGH); // Turn on camera (simulation)
    String data = "{\"motion\": true, \"time\": \"" + String(millis()) + "\"}";
    send_to_cloud(data);
    
    lcd.setCursor(0, 0);
    lcd.print("Motion Detected!");
    delay(5000); // Keep the message on display for 5 seconds
    digitalWrite(CAMERA_PIN, LOW); // Turn off camera
  } else {
    lcd.clear();
  }
  
  delay(2000);
}

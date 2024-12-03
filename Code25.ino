#include <DHT.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FirebaseESP8266.h>
#include <Ticker.h>

// WiFi credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Firebase credentials
#define FIREBASE_HOST "your_firebase_host"
#define FIREBASE_AUTH "your_firebase_auth_token"

// Define the pins for the sensors, actuators, and display
#define DHTPIN 2
#define DHTTYPE DHT11
#define PIR_PIN 3
#define LED_PIN 13
#define PHOTORESISTOR_PIN A0
#define RELAY_PIN 5
#define BUZZER_PIN 6

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);
FirebaseData firebaseData;
Ticker ticker;

int pirState = LOW; // Start with no motion detected
int val = 0;        // Variable to store PIR status
int lightLevel = 0; // Variable to store light sensor value

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize OLED display
  if(!display.begin(SSD1306_I2C_ADDRESS, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  // Start web server
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.begin();
  Serial.println("Web server started");

  // Schedule periodic updates
  ticker.attach(2, updateSensors);
}

void loop() {
  server.handleClient();
  delay(10); // Small delay to prevent overload
}

void updateSensors() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  val = digitalRead(PIR_PIN); // Read PIR sensor
  lightLevel = analogRead(PHOTORESISTOR_PIN); // Read light sensor

  // Update OLED display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(temp);
  display.print("C");
  display.setCursor(0, 10);
  display.print("Humidity: ");
  display.print(hum);
  display.print("%");
  display.setCursor(0, 20);
  display.print("Light: ");
  display.print(lightLevel);
  display.setCursor(0, 30);
  display.print("Motion: ");
  display.print(val == HIGH ? "Detected" : "None");
  display.display();

  // Control the LED based on light level
  digitalWrite(LED_PIN, lightLevel < 300 ? HIGH : LOW);

  // Control the relay based on temperature
  digitalWrite(RELAY_PIN, temp > 30 ? HIGH : LOW);

  // Activate buzzer if motion is detected
  if (val == HIGH) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Display motion detection status on Serial Monitor
  if (val == HIGH && pirState == LOW) {
    Serial.println("Motion detected!");
    pirState = HIGH;
  } else if (val == LOW && pirState == HIGH) {
    Serial.println("Motion ended!");
    pirState = LOW;
  }

  // Upload data to Firebase
  uploadDataToFirebase(temp, hum, lightLevel, pirState);
}

void handleRoot() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int lightLevel = analogRead(PHOTORESISTOR_PIN);

  String html = "<html><body>";
  html += "<h1>Smart Home System</h1>";
  html += "<p>Temperature: " + String(temp) + " C</p>";
  html += "<p>Humidity: " + String(hum) + " %</p>";
  html += "<p>Light Level: " + String(lightLevel) + "</p>";
  html += "<p><a href='/control?cmd=ledon'>Turn LED On</a></p>";
  html += "<p><a href='/control?cmd=ledoff'>Turn LED Off</a></p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleControl() {
  String command = server.arg("cmd");
  digitalWrite(LED_PIN, command == "ledon" ? HIGH : LOW);
  server.send(200, "text/plain", "OK");
}

void uploadDataToFirebase(float temp, float hum, int lightLevel, int pirState) {
  if (Firebase.pushFloat(firebaseData, "/temperature", temp)) {
    Serial.println("Temperature data uploaded successfully");
  } else {
    Serial.println("Failed to upload temperature data");
  }

  if (Firebase.pushFloat(firebaseData, "/humidity", hum)) {
    Serial.println("Humidity data uploaded successfully");
  } else {
    Serial.println("Failed to upload humidity data");
  }

  if (Firebase.pushInt(firebaseData, "/lightLevel", lightLevel)) {
    Serial.println("Light level data uploaded successfully");
  } else {
    Serial.println("Failed to upload light level data");
  }

  if (Firebase.pushInt(firebaseData, "/motion", pirState)) {
    Serial.println("Motion data uploaded successfully");
  } else {
    Serial.println("Failed to upload motion data");
  }
}
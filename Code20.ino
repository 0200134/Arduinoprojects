#include <DHT.h>
#include <LiquidCrystal.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SD.h>
#include <SPI.h>

// WiFi credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Define the pins for the DHT sensor, LCD, PIR sensor, LED, photoresistor, and SD card
#define DHTPIN 2
#define DHTTYPE DHT11
#define PIR_PIN 3
#define LED_PIN 13
#define PHOTORESISTOR_PIN A0
#define SD_CS_PIN 4

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
ESP8266WebServer server(80);

int pirState = LOW; // Start with no motion detected
int val = 0;        // Variable to store PIR status
int lightLevel = 0; // Variable to store light sensor value

void setup() {
  lcd.begin(16, 2);
  dht.begin();
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Start web server
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  val = digitalRead(PIR_PIN); // Read PIR sensor
  lightLevel = analogRead(PHOTORESISTOR_PIN); // Read light sensor

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print("C");
  
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(hum);
  lcd.print("%");

  // Control the LED based on light level
  if (lightLevel < 300) { // Adjust the threshold value as needed
    digitalWrite(LED_PIN, HIGH); // Turn on the LED if it's dark
  } else {
    digitalWrite(LED_PIN, LOW);  // Turn off the LED if it's bright
  }

  // Display motion detection status on Serial Monitor
  if (val == HIGH) { // Check if the PIR sensor is HIGH
    if (pirState == LOW) {
      Serial.println("Motion detected!");
      pirState = HIGH;
    }
  } else {
    if (pirState == HIGH) {
      Serial.println("Motion ended!");
      pirState = LOW;
    }
  }

  // Log data to SD card
  logData(temp, hum, lightLevel, pirState);

  // Handle web server requests
  server.handleClient();
  delay(2000);
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
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleControl() {
  String command = server.arg("cmd");
  if (command == "ledon") {
    digitalWrite(LED_PIN, HIGH);
  } else if (command == "ledoff") {
    digitalWrite(LED_PIN, LOW);
  }
  server.send(200, "text/plain", "OK");
}

void logData(float temp, float hum, int lightLevel, int pirState) {
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.print("Temperature: ");
    dataFile.print(temp);
    dataFile.print(" C, Humidity: ");
    dataFile.print(hum);
    dataFile.print(" %, Light Level: ");
    dataFile.print(lightLevel);
    dataFile.print(", Motion: ");
    dataFile.println(pirState == HIGH ? "Detected" : "Not detected");
    dataFile.close();
  } else {
    Serial.println("Error opening datalog.txt");
  }
}
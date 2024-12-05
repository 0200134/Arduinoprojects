#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <SD.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FirebaseESP8266.h>
#include <ThingSpeak.h>
#include <AESLib.h>
#include <TensorFlowLite_ESP32.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_BME280 bme;

RTC_DS3231 rtc;

const int chipSelect = 4;
const int gasSensorPin = A0; // MQ-135 gas sensor
const int oneWireBus = 2; // DS18B20 data pin
const int actuatorPin = D3; // Actuator control pin
const int speakerPin = D4; // Speaker pin

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

// Firebase setup
#define FIREBASE_HOST "your_firebase_host"
#define FIREBASE_AUTH "your_firebase_auth_key"

// ThingSpeak setup
unsigned long myChannelNumber = YOUR_CHANNEL_NUMBER;
const char * myWriteAPIKey = "YOUR_WRITE_API_KEY";

// Encryption key
byte aes_key[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

FirebaseData firebaseData;
WiFiClient client;

WiFiServer server(80);

void setup() {
  Serial.begin(9600);
  
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  sensors.begin();

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  ThingSpeak.begin(client);

  pinMode(actuatorPin, OUTPUT);
  pinMode(speakerPin, OUTPUT);

  server.begin();
}

void loop() {
  DateTime now = rtc.now();

  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;
  int gasLevel = analogRead(gasSensorPin);

  sensors.requestTemperatures(); 
  float additionalTemp = sensors.getTempCByIndex(0);

  if (isnan(temperature) || isnan(humidity) || isnan(pressure)) {
    Serial.println("Failed to read from BME280 sensor!");
    return;
  }

  String dataString = now.timestamp(DateTime::TIMESTAMP_FULL) + ", ";
  dataString += "Temp: " + String(temperature) + " *C, ";
  dataString += "Humidity: " + String(humidity) + " %, ";
  dataString += "Pressure: " + String(pressure) + " hPa, ";
  dataString += "Gas Level: " + String(gasLevel) + ", ";
  dataString += "Additional Temp: " + String(additionalTemp) + " *C\n";

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Time: "); display.println(now.timestamp(DateTime::TIMESTAMP_TIME));
  display.print("Temp: "); display.print(temperature); display.println(" *C");
  display.print("Humidity: "); display.print(humidity); display.println(" %");
  display.print("Pressure: "); display.print(pressure); display.println(" hPa");
  display.print("Gas Level: "); display.print(gasLevel); display.println();
  display.print("Add Temp: "); display.print(additionalTemp); display.println(" *C");
  display.display();

  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  } else {
    Serial.println("Error opening datalog.txt");
  }

  char encrypted_data[128];
  aesLib.encrypt(dataString.c_str(), encrypted_data);
  sendDataToFirebase(temperature, humidity, pressure, gasLevel, additionalTemp);
  sendDataToThingSpeak(temperature, humidity, pressure, gasLevel, additionalTemp);

  // Machine Learning Model Prediction (Placeholder)
  float predictedTemp = temperature + 1.0; // Dummy logic for prediction
  float predictedHumidity = humidity + 1.0; // Dummy logic for prediction

  if (predictedTemp > 30.0) { // Example condition for actuator control
    digitalWrite(actuatorPin, HIGH);
    tone(speakerPin, 1000, 500); // Sound an alert
  } else {
    digitalWrite(actuatorPin, LOW);
  }

  // Voice control commands here

  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String request = client.readStringUntil('\r');
        Serial.println(request);
        client.flush();
        
        client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
        client.print("<!DOCTYPE HTML><html>");
        client.print("<head><meta http-equiv='refresh' content='5'></head>");
        client.print("<body>");
        client.print("<h1>Super Advanced Weather Station Data</h1>");
        client.print("<p>");
        client.print(dataString);
        client.print("</p>");
        client.print("</body></html>");
        client.stop();
      }
    }
  }

  delay(2000);
}

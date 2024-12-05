#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <PubSubClient.h>
#include <AESLib.h>

// Define sensor pins
#define DHTPIN 2
#define DHTTYPE DHT22
#define RAIN_PIN A0
#define LIGHT_PIN A1
#define SOIL_PIN A2
#define SD_CS_PIN 4
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8

// Initialize sensors
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
File dataFile;

// Ethernet settings
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177);
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

// MQTT settings
const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic = "weather/station";

// AES encryption settings
AESLib aesLib;
const char* aes_key = "mysecretaeskey123"; // Must be 16, 24, or 32 bytes
const char* aes_iv = "myinitialvector12"; // Must be 16 bytes

void setup() {
  Serial.begin(9600);
  dht.begin();
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
  
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }

  // Initialize Ethernet
  Ethernet.begin(mac, ip);
  delay(1000); // Give the Ethernet shield a second to initialize
  Serial.println("Ethernet initialized");

  // Initialize MQTT
  mqttClient.setServer(mqtt_server, 1883);
}

void loop() {
  // Read data from DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if reading failed
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Read data from BMP280
  float pressure = bmp.readPressure();
  float altitude = bmp.readAltitude(1013.25); // Adjust as necessary for local forecast

  // Read data from other sensors
  int rainValue = analogRead(RAIN_PIN);
  int lightValue = analogRead(LIGHT_PIN);
  int soilValue = analogRead(SOIL_PIN);

  // Print readings to Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C\tHumidity: ");
  Serial.print(humidity);
  Serial.print(" %\tPressure: ");
  Serial.print(pressure / 100.0F);
  Serial.print(" hPa\tAltitude: ");
  Serial.print(altitude);
  Serial.print(" m\tRain: ");
  Serial.print(rainValue);
  Serial.print("\tLight: ");
  Serial.print(lightValue);
  Serial.print("\tSoil Moisture: ");
  Serial.println(soilValue);

  // Display data on TFT
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Weather Station");
  tft.setTextSize(1);
  tft.print("Temp: ");
  tft.print(temperature);
  tft.println(" C");
  tft.print("Humidity: ");
  tft.print(humidity);
  tft.println(" %");
  tft.print("Pressure: ");
  tft.print(pressure / 100.0F);
  tft.println(" hPa");
  tft.print("Altitude: ");
  tft.print(altitude);
  tft.println(" m");
  tft.print("Rain: ");
  tft.println(rainValue);
  tft.print("Light: ");
  tft.println(lightValue);
  tft.print("Soil Moisture: ");
  tft.println(soilValue);

  // Log data to SD card
  dataFile = SD.open("weather_data.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.print("Temperature: ");
    dataFile.print(temperature);
    dataFile.print(" °C\tHumidity: ");
    dataFile.print(humidity);
    dataFile.print(" %\tPressure: ");
    dataFile.print(pressure / 100.0F);
    dataFile.print(" hPa\tAltitude: ");
    dataFile.print(altitude);
    dataFile.print(" m\tRain: ");
    dataFile.print(rainValue);
    dataFile.print("\tLight: ");
    dataFile.print(lightValue);
    dataFile.print("\tSoil Moisture: ");
    dataFile.println(soilValue);
    dataFile.close();
  } else {
    Serial.println("Error opening weather_data.txt");
  }

  // Encrypt data
  String data = "Temperature: " + String(temperature) + " °C, Humidity: " + String(humidity) + " %, Pressure: " + String(pressure / 100.0F) + " hPa, Altitude: " + String(altitude) + " m, Rain: " + String(rainValue) + ", Light: " + String(lightValue) + ", Soil Moisture: " + String(soilValue);
  int data_size = data.length() + 1;
  byte encrypted[data_size];
  aesLib.encrypt((byte*)data.c_str(), encrypted, aes_key, 128, aes_iv);

  // Send encrypted data via MQTT
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  mqttClient.publish(mqtt_topic, encrypted, data_size);

  // Wait a few seconds before next reading
  delay(2000);
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("weatherStationClient")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

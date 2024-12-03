#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>
#include <ThingSpeak.h>

// Define sensor pins
#define DHTPIN 2
#define DHTTYPE DHT22
#define RAIN_PIN A0
#define LIGHT_PIN A1
#define SOIL_PIN A2
#define SD_CS_PIN 4

// Initialize sensors
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
File dataFile;

// Ethernet settings
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177);
EthernetClient client;

// ThingSpeak settings
unsigned long myChannelNumber = YOUR_CHANNEL_NUMBER;
const char * myWriteAPIKey = "YOUR_WRITE_API_KEY";
const char * server = "api.thingspeak.com";

void setup() {
  Serial.begin(9600);
  dht.begin();
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }

  // Initialize Ethernet
  Ethernet.begin(mac, ip);
  delay(1000); // Give the Ethernet shield a second to initialize
  Serial.println("Ethernet initialized");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
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

  // Read data from rain sensor
  int rainValue = analogRead(RAIN_PIN);
  
  // Read data from light sensor
  int lightValue = analogRead(LIGHT_PIN);
  
  // Read data from soil moisture sensor
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

  // Send data to ThingSpeak
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, pressure / 100.0F);
  ThingSpeak.setField(4, altitude);
  ThingSpeak.setField(5, rainValue);
  ThingSpeak.setField(6, lightValue);
  ThingSpeak.setField(7, soilValue);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  // Wait a few seconds before next reading
  delay(2000);
}
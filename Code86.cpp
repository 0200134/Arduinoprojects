#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>

// Define sensor pins
#define DHTPIN 2
#define DHTTYPE DHT22
#define RAIN_PIN A0
#define SD_CS_PIN 4

// Initialize sensors
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
File dataFile;

// Ethernet settings (fill in your details)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177);
EthernetClient client;
const char server[] = "example.com"; // Your server's URL

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
  Serial.println(rainValue);

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
    dataFile.println(rainValue);
    dataFile.close();
  } else {
    Serial.println("Error opening weather_data.txt");
  }

  // Send data via Ethernet (example: HTTP GET request to a server)
  if (client.connect(server, 80)) {
    client.print("GET /update?temp=");
    client.print(temperature);
    client.print("&hum=");
    client.print(humidity);
    client.print("&press=");
    client.print(pressure / 100.0F);
    client.print("&alt=");
    client.print(altitude);
    client.print("&rain=");
    client.print(rainValue);
    client.println(" HTTP/1.1");
    client.println("Host: example.com");
    client.println("Connection: close");
    client.println();
    client.stop();
  }

  // Wait a few seconds before next reading
  delay(2000);
}

#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SSD1306.h>
#include <AES.h>

// Define sensor pins
#define DHTPIN 2
#define DHTTYPE DHT22
#define RAIN_PIN 34
#define LIGHT_PIN 35
#define SOIL_PIN 32
#define SD_CS_PIN 5
#define OLED_RESET -1

// Initialize sensors
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
Adafruit_SSD1306 display(OLED_RESET);
File dataFile;

// Wi-Fi settings
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// AES encryption settings
AES aes;
byte aesKey[] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0xCF, 0x15, 0x88, 0x09, 0xCF, 0x4F};
byte aesIV[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

// Function to encrypt data
void encryptData(const String &data, byte* encryptedData) {
  aes.do_aes_encrypt((byte*)data.c_str(), data.length(), encryptedData, aesKey, 128, aesIV);
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  // Initialize display
  if(!display.begin(SSD1306_I2C_ADDRESS, OLED_RESET)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // AES initialization
  aes.set_key(aesKey, sizeof(aesKey));
  aes.set_iv(aesIV);
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

  // Display data on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Weather Station");
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");
  display.print("Pressure: ");
  display.print(pressure / 100.0F);
  display.println(" hPa");
  display.print("Altitude: ");
  display.print(altitude);
  display.println(" m");
  display.print("Rain: ");
  display.println(rainValue);
  display.print("Light: ");
  display.println(lightValue);
  display.print("Soil Moisture: ");
  display.println(soilValue);
  display.display();

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

  // Prepare data for encryption
  String data = "Temperature: " + String(temperature) + " °C, Humidity: " + String(humidity) + " %, Pressure: " + String(pressure / 100.0F) + " hPa, Altitude: " + String(altitude) + " m, Rain: " + String(rainValue) + ", Light: " + String(lightValue) + ", Soil Moisture: " + String(soilValue);
  byte encryptedData[data.length()];
  encryptData(data, encryptedData);

  // Send encrypted data via Wi-Fi
  WiFiClient client;
  if (client.connect("your_server.com", 80)) {
    client.print("GET /update?data=");
    client.write(encryptedData, sizeof(encryptedData));
    client.println(" HTTP/1.1");
    client.println("Host: your_server.com");
    client.println("Connection: close");
    client.println();
    client.stop();
  }

  // Wait a few seconds before next reading
  delay(2000);
}

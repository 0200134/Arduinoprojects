#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SSD1306.h>
#include <AESLib.h>
#include <TensorFlowLite.h>

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
AESLib aesLib;
const char* aes_key = "mysecretaeskey123"; // Must be 16, 24, or 32 bytes
const char* aes_iv = "myinitialvector12";  // Must be 16 bytes

// TensorFlow Lite settings
const uint8_t* model = /* Your TensorFlow Lite model here */;
tflite::MicroErrorReporter tfl_error_reporter;
tflite::MicroInterpreter* interpreter;
tflite::MicroMutableOpResolver<10> resolver;

void setup() {
  Serial.begin(9600);
  dht.begin();
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  // Initialize display
  if (!display.begin(SSD1306_I2C_ADDRESS, OLED_RESET)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
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
  aesLib.gen_iv(aes_iv); // Generate a random IV

  // TensorFlow Lite initialization
  static tflite::MicroModel micro_model(model, model_size, &tfl_error_reporter);
  interpreter = new tflite::MicroInterpreter(&micro_model, &resolver, tensor_arena, tensor_arena_size, &tfl_error_reporter);
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

  // Encrypt data
  String data = "Temperature: " + String(temperature) + " °C, Humidity: " + String(humidity) + " %, Pressure: " + String(pressure / 100.0F) + " hPa, Altitude: " + String(altitude) + " m, Rain: " + String(rainValue) + ", Light: " + String(lightValue) + ", Soil Moisture: " + String(soilValue);
  byte encrypted[32];
  aesLib.encrypt64(data.c_str(), encrypted, aes_key, aes_iv);

  // Send encrypted data via Wi-Fi
  WiFiClient client;
  if (client.connect("your_server.com", 80)) {
    client.print("GET /update?data=");
    client.write(encrypted, sizeof(encrypted));
    client.println(" HTTP/1.1");
    client.println("Host: your_server.com");
    client.println("Connection: close");
    client.println();
    client.stop();
  }

  // Use TensorFlow Lite for predictive analytics
  float input_data[7] = {temperature, humidity, pressure / 100.0F, altitude, rainValue, lightValue, soilValue};
  TfLiteTensor* input_tensor = interpreter->input(0);
  memcpy(input_tensor->data.f, input_data, 7 * sizeof(float));
  interpreter->Invoke();
  TfLiteTensor* output_tensor = interpreter->output(0);
  float prediction = output_tensor->data.f[0];

  Serial.print("Predicted Weather: ");
  Serial.println(prediction);

  // Wait a few seconds before next reading
  delay(2000);
}

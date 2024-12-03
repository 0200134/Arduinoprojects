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
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Define sensor pins
#define DHTPIN 2
#define DHTTYPE DHT22
#define RAIN_PIN 34
#define LIGHT_PIN 35
#define SOIL_PIN 32
#define SD_CS_PIN 5
#define OLED_RESET -1
#define FAN_PIN 14
#define HEATER_PIN 27
#define IRRIGATION_PIN 26

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

// Web server
AsyncWebServer server(80);

// HTML for web dashboard
const char* index_html = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Weather Station</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; }
    table { width: 100%; margin: 20px 0; }
    th, td { padding: 10px; text-align: center; border: 1px solid #ddd; }
    th { background-color: #f4f4f4; }
  </style>
</head>
<body>
  <h1>Weather Station</h1>
  <table>
    <tr><th>Sensor</th><th>Value</th></tr>
    <tr><td>Temperature</td><td id="temp">--</td></tr>
    <tr><td>Humidity</td><td id="humidity">--</td></tr>
    <tr><td>Pressure</td><td id="pressure">--</td></tr>
    <tr><td>Altitude</td><td id="altitude">--</td></tr>
    <tr><td>Rain</td><td id="rain">--</td></tr>
    <tr><td>Light</td><td id="light">--</td></tr>
    <tr><td>Soil Moisture</td><td id="soil">--</td></tr>
  </table>
  <h2>Control</h2>
  <label for="fan">Fan: </label>
  <input type="checkbox" id="fan"><br>
  <label for="heater">Heater: </label>
  <input type="checkbox" id="heater"><br>
  <label for="irrigation">Irrigation: </label>
  <input type="checkbox" id="irrigation"><br>
  <script>
    function updateData() {
      fetch("/getData")
        .then(response => response.json())
        .then(data => {
          document.getElementById("temp").innerText = data.temperature + " °C";
          document.getElementById("humidity").innerText = data.humidity + " %";
          document.getElementById("pressure").innerText = data.pressure + " hPa";
          document.getElementById("altitude").innerText = data.altitude + " m";
          document.getElementById("rain").innerText = data.rain;
          document.getElementById("light").innerText = data.light;
          document.getElementById("soil").innerText = data.soilMoisture;
        });
    }
    setInterval(updateData, 5000);
    document.getElementById("fan").addEventListener("change", () => {
      fetch("/control?fan=" + (document.getElementById("fan").checked ? "on" : "off"));
    });
    document.getElementById("heater").addEventListener("change", () => {
      fetch("/control?heater=" + (document.getElementById("heater").checked ? "on" : "off"));
    });
    document.getElementById("irrigation").addEventListener("change", () => {
      fetch("/control?irrigation=" + (document.getElementById("irrigation").checked ? "on" : "off"));
    });
  </script>
</body>
</html>)rawliteral";

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

  // Initialize actuators
  pinMode(FAN_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(IRRIGATION_PIN, OUTPUT);

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

  // Start web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  server.on("/getData", HTTP_GET, [](AsyncWebServerRequest *request){
    String data = getSensorData();
    request->send(200, "application/json", data);
  });
  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("fan")) {
      String fan = request->getParam("fan")->value();
      digitalWrite(FAN_PIN, fan == "on" ? HIGH : LOW);
    }
    if (request->hasParam("heater")) {
      String heater = request->getParam("heater")->value();
      digitalWrite(HEATER_PIN, heater == "on" ? HIGH : LOW);
    }
    if (request->hasParam("irrigation")) {
      String irrigation = request->getParam("irrigation")->value();
      digitalWrite(IRRIGATION_PIN, irrigation == "on" ? HIGH : LOW);
    }
    request->send(200, "text/plain", "Control updated");
  });
  server.begin();
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
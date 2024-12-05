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

// Sensor pins
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

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
Adafruit_SSD1306 display(OLED_RESET);
File dataFile;

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
AESLib aesLib;
const char* aes_key = "mysecretaeskey123";
const char* aes_iv = "myinitialvector12";
const uint8_t* model = /* Your TensorFlow Lite model here */;
tflite::MicroErrorReporter tfl_error_reporter;
tflite::MicroInterpreter* interpreter;
tflite::MicroMutableOpResolver<10> resolver;
AsyncWebServer server(80);

const char* index_html = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Weather Station</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>body { font-family: Arial; text-align: center; }</style>
</head>
<body>
  <h1>Weather Station</h1>
  <div id="data"></div>
  <script>
    function updateData() {
      fetch("/getData").then(response => response.json()).then(data => {
        document.getElementById("data").innerHTML = JSON.stringify(data);
      });
    }
    setInterval(updateData, 5000);
  </script>
</body>
</html>)rawliteral";

void setup() {
  Serial.begin(9600);
  dht.begin();
  bmp.begin();
  display.begin(SSD1306_I2C_ADDRESS, OLED_RESET);
  SD.begin(SD_CS_PIN);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(IRRIGATION_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  aesLib.gen_iv(aes_iv);
  static tflite::MicroModel micro_model(model, model_size, &tfl_error_reporter);
  interpreter = new tflite::MicroInterpreter(&micro_model, &resolver, tensor_arena, tensor_arena_size, &tfl_error_reporter);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  server.on("/getData", HTTP_GET, [](AsyncWebServerRequest *request){
    String data = getSensorData();
    request->send(200, "application/json", data);
  });
  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("fan")) digitalWrite(FAN_PIN, request->getParam("fan")->value() == "on" ? HIGH : LOW);
    if (request->hasParam("heater")) digitalWrite(HEATER_PIN, request->getParam("heater")->value() == "on" ? HIGH : LOW);
    if (request->hasParam("irrigation")) digitalWrite(IRRIGATION_PIN, request->getParam("irrigation")->value() == "on" ? HIGH : LOW);
    request->send(200, "text/plain", "Control updated");
  });
  server.begin();
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) return;
  float pressure = bmp.readPressure();
  float altitude = bmp.readAltitude(1013.25);
  int rainValue = analogRead(RAIN_PIN);
  int lightValue = analogRead(LIGHT_PIN);
  int soilValue = analogRead(SOIL_PIN);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Weather Station");
  display.printf("Temp: %.2f C\nHumidity: %.2f %%\nPressure: %.2f hPa\nAltitude: %.2f m\nRain: %d\nLight: %d\nSoil Moisture: %d\n",
    temperature, humidity, pressure / 100.0F, altitude, rainValue, lightValue, soilValue);
  display.display();

  dataFile = SD.open("weather_data.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.printf("Temp: %.2f C, Humidity: %.2f %%, Pressure: %.2f hPa, Altitude: %.2f m, Rain: %d, Light: %d, Soil Moisture: %d\n",
      temperature, humidity, pressure / 100.0F, altitude, rainValue, lightValue, soilValue);
    dataFile.close();
  }

  String data = String(temperature) + "," + String(humidity) + "," + String(pressure / 100.0F) + "," + String(altitude) + "," + String(rainValue) + "," + String(lightValue) + "," + String(soilValue);
  byte encrypted[32];
  aesLib.encrypt64(data.c_str(), encrypted, aes_key, aes_iv);

  WiFiClient client;
  if (client.connect("your_server.com", 80)) {
    client.printf("GET /update?data=%s HTTP/1.1\r\nHost: your_server.com\r\nConnection: close\r\n\r\n", encrypted);
    client.stop();
  }

  float input_data[7] = {temperature, humidity, pressure / 100.0F, altitude, rainValue, lightValue, soilValue};
  TfLiteTensor* input_tensor = interpreter->input(0);
  memcpy(input_tensor->data.f, input_data, sizeof(input_data));
  interpreter->Invoke();
  float prediction = interpreter->output(0)->data.f[0];
  Serial.printf("Predicted Weather: %.2f\n", prediction);

  delay(2000);
}

String getSensorData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float pressure = bmp.readPressure() / 100.0F;
  float altitude = bmp.readAltitude(1013.25);
  int rainValue = analogRead(RAIN_PIN);
  int lightValue = analogRead(LIGHT_PIN);
  int soilValue = analogRead(SOIL_PIN);

  StaticJsonDocument<256> json;
  json["temperature"] = temperature;
  json["humidity"] = humidity;
  json["pressure"] = pressure;
  json["altitude"] = altitude;
  json["rain"] = rainValue;
  json["light"] = lightValue;
  json["soilMoisture"] = soilValue;
  String data;
  serializeJson(json, data);
  return data;
}

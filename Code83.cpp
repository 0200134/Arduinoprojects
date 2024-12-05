#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TensorFlowLite.h>
#include "model.h"  // Include your trained AI model's header file
#include <MQ135.h>
#include <ThingSpeak.h>

// Pin definitions
#define ONE_WIRE_BUS 15
#define DHTPIN 14
#define DHTTYPE DHT22
#define MQ135PIN A0
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Constants
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
BH1750 lightMeter;
MQ135 mq135(MQ135PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Wi-Fi credentials
const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";

// Firebase credentials
#define FIREBASE_HOST "your-firebase-database.firebaseio.com"
#define FIREBASE_AUTH "your-firebase-auth-token"

// ThingSpeak credentials
unsigned long myChannelNumber = YOUR_CHANNEL_ID;
const char* myWriteAPIKey = "YOUR_WRITE_API_KEY";

// Weather API credentials
const char* weather_api_key = "your-weather-api-key";
const char* weather_api_url = "http://api.openweathermap.org/data/2.5/weather?q=your-city&appid=your-weather-api-key";

// Firebase Data object
FirebaseData firebaseData;

// TensorFlow Lite Micro
constexpr int kTensorArenaSize = 2 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

// ThingSpeak
WiFiClient  client;

void setup() {
  Serial.begin(115200);

  // Initialize sensors, display, and Firebase
  ds18b20.begin();
  dht.begin();
  bmp.begin(0x76);
  lightMeter.begin();
  display.begin(SSD1306_I2C_ADDRESS, OLED_RESET);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  ThingSpeak.begin(client);

  display.display();
  delay(2000);
  display.clearDisplay();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Set up TensorFlow Lite model
  tflite::MicroMutableOpResolver<6> resolver;
  resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED, tflite::ops::micro::Register_FULLY_CONNECTED());
  resolver.AddBuiltin(tflite::BuiltinOperator_RELU, tflite::ops::micro::Register_RELU());
  resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX, tflite::ops::micro::Register_SOFTMAX());

  const tflite::Model* model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model version does not match schema");
    while (1);
  }

  static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.println("AllocateTensors() failed");
    while (1);
  }

  input = interpreter->input(0);
  output = interpreter->output(0);
}

void loop() {
  // Read sensor data
  ds18b20.requestTemperatures();
  float temperature = ds18b20.getTempCByIndex(0);
  float humidity = dht.readHumidity();
  float bmp_temperature = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0F;
  float lightLevel = lightMeter.readLightLevel();
  float airQuality = mq135.getPPM();

  // Display data on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");
  display.print("Hum: ");
  display.print(humidity);
  display.println(" %");
  display.print("BMP Temp: ");
  display.print(bmp_temperature);
  display.println(" C");
  display.print("Pressure: ");
  display.print(pressure);
  display.println(" hPa");
  display.print("Light: ");
  display.print(lightLevel);
  display.println(" lx");
  display.print("Air Quality: ");
  display.print(airQuality);
  display.println(" ppm");
  display.display();

  // Send data to ThingSpeak
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, bmp_temperature);
  ThingSpeak.setField(4, pressure);
  ThingSpeak.setField(5, lightLevel);
  ThingSpeak.setField(6, airQuality);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  // Log data to Firebase
  if (Firebase.ready()) {
    Firebase.setFloat(firebaseData, "/ds18b20_temp", temperature);
    Firebase.setFloat(firebaseData, "/humidity", humidity);
    Firebase.setFloat(firebaseData, "/bmp_temp", bmp_temperature);
    Firebase.setFloat(firebaseData, "/pressure", pressure);
    Firebase.setFloat(firebaseData, "/light", lightLevel);
    Firebase.setFloat(firebaseData, "/air_quality", airQuality);
  }

  // AI-based predictions and anomaly detection
  input->data.f[0] = temperature;
  input->data.f[1] = humidity;
  input->data.f[2] = bmp_temperature;
  input->data.f[3] = pressure;
  input->data.f[4] = lightLevel;
  input->data.f[5] = airQuality;

  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    Serial.println("Invoke failed");
    return;
  }

  // Process AI model output (detect anomalies and predict trends)
  float anomaly_score = output->data.f[0];
  float predicted_temperature = output->data.f[1];  // Assuming the second output is the predicted value

  Serial.print("Anomaly Score: ");
  Serial.println(anomaly_score);
  Serial.print("Predicted Temperature: ");
  Serial.println(predicted_temperature);

  if (anomaly_score > 0.7) {
    // Send notifications through ThingSpeak
    String postStr = api_key;
    postStr += "&field1=";
    postStr += String(anomaly_score);
    postStr += "\r\n\r\n";

    client.connect("api.thingspeak.com", 80);
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + String(writeAPIKey) + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");

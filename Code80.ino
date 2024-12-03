#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TensorFlowLite.h>
#include "model.h"  // Include your trained AI model's header file
#include <BlynkSimpleEsp32.h>

// Pin definitions
#define ONE_WIRE_BUS 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Constants
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Wi-Fi credentials
const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";

// Firebase credentials
#define FIREBASE_HOST "your-firebase-database.firebaseio.com"
#define FIREBASE_AUTH "your-firebase-auth-token"

// Blynk credentials
char auth[] = "your-Blynk-auth-token";

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

void setup() {
  Serial.begin(115200);

  // Initialize sensor, display, and Firebase
  sensors.begin();
  display.begin(SSD1306_I2C_ADDRESS, OLED_RESET);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  display.display();
  delay(2000);
  display.clearDisplay();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  Blynk.begin(auth, ssid, password);

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
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  // Display data on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Temperature: ");
  display.print(temperature);
  display.println(" C");
  display.display();

  // Send data to Blynk
  Blynk.virtualWrite(V0, temperature);

  // Log data to Firebase
  if (Firebase.ready()) {
    if (Firebase.setFloat(firebaseData, "/temperature", temperature)) {
      Serial.println("Data sent to Firebase");
    } else {
      Serial.println("Failed to send data to Firebase");
    }
  }

  // AI-based anomaly detection
  input->data.f[0] = temperature;
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
    Blynk.notify("Temperature anomaly detected!");
  }

  // Automate actions based on predictions (example: turning on a cooling system)
  if (predicted_temperature > 30.0) {
    // Add your code to activate cooling system
    Serial.println("Activating cooling system...");
  }

  // Fetch external weather data
  if ((millis() % 60000) == 0) {  // Fetch every 60 seconds
    fetchWeatherData();
  }

  delay(1000);
}

void fetchWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(weather_api_url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);

      // Parse JSON
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      float external_temp = doc["main"]["temp"];
      float external_humidity = doc["main"]["humidity"];

      Serial.print("External Temp: ");
      Serial.println(external_temp);
      Serial.print("External Humidity: ");
      Serial.println(external_humidity);

      // Compare with internal temperature
      if (abs(temperature - external_temp) > 5.0) {
        Blynk.notify("Significant temperature difference detected!");
      }

      Blynk.virtualWrite(V1, external_temp);
      Blynk.virtualWrite(V2, external_humidity);
    } else {
      Serial.println("Error fetching weather data");
    }
    http.end();
  }
}
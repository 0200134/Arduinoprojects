#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESP32WebServer.h>
#include <WiFi.h>
#include <TensorFlowLite.h>
#include "model.h"  // Include your trained AI model's header file

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT dht(15, DHT22);
ESP32WebServer server(80);

// Wi-Fi credentials
const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";

// TensorFlow Lite Micro
constexpr int kTensorArenaSize = 2 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  pinMode(5, OUTPUT); // Relay pin

  // Initialize OLED display
  if (!display.begin(SSD1306_I2C_ADDRESS, OLED_RESET)) {
    Serial.println("SSD1306 allocation failed");
    while (1);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  // Initialize Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Set up web server
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/control", handleControl);
  server.begin();
  Serial.println("Web server started");

  // Set up the AI model
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
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int current = analogRead(A0);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Display data on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Temp: "); display.print(temperature); display.println(" C");
  display.print("Hum: "); display.print(humidity); display.println(" %");
  display.print("Current: "); display.print(current); display.println(" A");
  display.display();

  // Set model input (example: temperature, humidity, current)
  input->data.f[0] = temperature;
  input->data.f[1] = humidity;
  input->data.f[2] = current;

  // Run inference
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    Serial.println("Invoke failed");
    return;
  }

  // Process AI model output (example: control relay based on prediction)
  float prediction = output->data.f[0];
  if (prediction > 0.5) {
    digitalWrite(5, HIGH);
  } else {
    digitalWrite(5, LOW);
  }

  delay(1000);
  server.handleClient();
}

void handleRoot() {
  server.send(200, "text/html", "<h1>Smart Energy Management System</h1><ul><li><a href='/status'>System Status</a></li><li><a href='/control'>Control Relay</a></li></ul>");
}

void handleStatus() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int current = analogRead(A0);
  
  String message = "Temperature: " + String(temperature) + " °C\n";
  message += "Humidity: " + String(humidity) + " %\n";
  message += "Current: " + String(current) + " A";
  
  server.send(200, "text/plain", message);
}

void handleControl() {
  String state = server.arg("state");
  if (state == "on") {
    digitalWrite(5, HIGH);
    server.send(200, "text/plain", "Relay is ON");
  } else if (state == "off") {
    digitalWrite(5, LOW);
    server.send(200, "text/plain", "Relay is OFF");
  } else {
    server.send(200, "text/plain", "Invalid command. Use 'on' or 'off'.");
  }
}
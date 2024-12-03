#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/kernels/micro_ops.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <ESP32_Microphone.h>
#include <WiFi.h>
#include <ESP32HTTPClient.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define DHTPIN 5
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

constexpr int tensor_arena_size = 16 * 1024;
uint8_t tensor_arena[tensor_arena_size];

const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
const char* mqtt_server = "your_MQTT_server";

WiFiClient espClient;
PubSubClient client(espClient);

const int LIGHT_PIN = 2;
const int FAN_PIN = 4;
const int LOCK_PIN = 13;
const int MOTION_SENSOR_PIN = 34;
const int ROOM_COUNT = 3;

Servo doorServo;

struct Room {
  int lightPin;
  int fanPin;
  int tempSensorPin;
  int motionSensorPin;
};

Room rooms[ROOM_COUNT] = {
  {2, 4, 34, 32},  // Room 1
  {5, 18, 33, 25}, // Room 2
  {19, 21, 26, 27} // Room 3
};

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  initializeModel();

  ESP32_Microphone::begin();
  dht.begin();

  for (int i = 0; i < ROOM_COUNT; i++) {
    pinMode(rooms[i].lightPin, OUTPUT);
    pinMode(rooms[i].fanPin, OUTPUT);
    pinMode(rooms[i].tempSensorPin, INPUT);
    pinMode(rooms[i].motionSensorPin, INPUT);
  }

  doorServo.attach(LOCK_PIN);
  doorServo.write(0); // Initial position: Door unlocked

  connectToMQTT();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  handleVoiceCommands();
  readSensors();

  delay(2000);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void initializeModel() {
  model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model provided is schema version");
    return;
  }

  static tflite::MicroMutableOpResolver<10> micro_op_resolver;
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_DEPTHWISE_CONV_2D, tflite::ops::micro::Register_DEPTHWISE_CONV_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D, tflite::ops::micro::Register_CONV_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED, tflite::ops::micro::Register_FULLY_CONNECTED());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX, tflite::ops::micro::Register_SOFTMAX());

  static tflite::MicroInterpreter static_interpreter(model, micro_op_resolver, tensor_arena, tensor_arena_size, error_reporter);
  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.println("AllocateTensors() failed");
    return;
  }

  input = interpreter->input(0);
  output = interpreter->output(0);
}

void handleVoiceCommands() {
  if (ESP32_Microphone::isAvailable()) {
    int16_t* samples = ESP32_Microphone::getSamples();
    for (int i = 0; i < input->dims->data[1]; ++i) {
      input->data.f[i] = samples[i];
    }

    if (interpreter->Invoke() != kTfLiteOk) {
      Serial.println("Invoke failed");
      return;
    }

    int command = getMaxOutputIndex();
    executeCommand(command);
  }
}

int getMaxOutputIndex() {
  int max_index = -1;
  float max_value = -1;
  for (int i = 0; i < output->dims->data[1]; ++i) {
    if (output->data.f[i] > max_value) {
      max_value = output->data.f[i];
      max_index = i;
    }
  }
  return max_index;
}

void executeCommand(int command) {
  switch (command) {
    case 0:
      controlLight(true, 0); // Turn on light in Room 0
      sendToCloud("Light On in Room 0");
      break;
    case 1:
      controlLight(false, 0); // Turn off light in Room 0
      sendToCloud("Light Off in Room 0");
      break;
    case 2:
      controlFan(true, 0); // Turn on fan in Room 0
      sendToCloud("Fan On in Room 0");
      break;
    case 3:
      controlFan(false, 0); // Turn off fan in Room 0
      sendToCloud("Fan Off in Room 0");
      break;
    case 4:
      controlLock(true);
      sendToCloud("Door Locked");
      break;
    case 5:
      controlLock(false);
      sendToCloud("Door Unlocked");
      break;
    default:
      Serial.println("Unknown command");
      break;
  }
}

void readSensors() {
  for (int i = 0; i < ROOM_COUNT; i++) {
    float temperature = analogRead(rooms[i].tempSensorPin) * (3.3 / 4095.0) * 100.0;
    bool motionDetected = digitalRead(rooms[i].motionSensorPin);

    Serial.print("Room ");
    Serial.print(i);
    Serial.print(" - Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C  Motion: ");
    Serial.println(motionDetected ? "Detected" : "Not Detected");

    if (temperature > 30.0) {
      controlFan(true, i);
    } else {
      controlFan(false, i);
    }

    if (motionDetected) {
      controlLight(true, i);
    } else {
      controlLight(false, i);
    }

    sendToCloudData(i, temperature, motionDetected);
  }
}

void controlLight(bool state, int room) {
  digitalWrite(rooms[room].lightPin, state ? HIGH : LOW);
  client.publish(("home/room" + String(room) + "/light/status").c_str(), state ? "ON" : "OFF");
}

void controlFan(bool state, int room) {
  digitalWrite(rooms[room].fanPin, state ? HIGH : LOW);
  client.publish(("home/room" + String(room) + "/fan/status").c_str(), state ? "ON" : "OFF");
}

void controlLock(bool state) {
  if (state) {
    doorServo.write(90); // Lock position
    client.publish("home/lock/status", "LOCKED");
  } else {
    doorServo.write(0); // Unlock position
    client.publish("home/lock/status", "UNLOCKED");
  }
}

void sendToCloud(const char* message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://example.com/update");
    http.addHeader("Content-Type", "application/json");

    String payload = String("{\"message\": \"") + message + "\"}";
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.println("Error in sending HTTP POST");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void sendToCloudData(int room, float temperature, bool motionDetected) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://example.com/data");
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument doc(256);
    doc["room"] = room;
    doc["temperature"] = temperature;
    doc["motion"] = motionDetected;

    String payload;
    serializeJson(doc[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/YU-LIN-LIN/mbed07/tree/5e46c75898fec1ccfedcc0c3942f9aff6aff8727/src%2Fmodel_deploy%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "1")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/yixuan0760/mbed07/tree/aa7bdf19287dd868b9ccd20b41669c7024c656cb/src%2Fmodel_deploy%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "2")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/leetoby1215/midterm_project/tree/c6eea65af4c6d00d41c59479c197fd3a0b56f5ac/src%2Fmodel_deploy%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "3")
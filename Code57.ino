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

const int LIGHT_PIN = 2;
const int FAN_PIN = 4;
const int LOCK_PIN = 13;
const int MOTION_SENSOR_PIN = 34;

Servo doorServo;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

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

  ESP32_Microphone::begin();
  dht.begin();

  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(LOCK_PIN, OUTPUT);
  pinMode(MOTION_SENSOR_PIN, INPUT);

  doorServo.attach(LOCK_PIN);
  doorServo.write(0); // Initial position: Door unlocked
}

void loop() {
  if (ESP32_Microphone::isAvailable()) {
    int16_t* samples = ESP32_Microphone::getSamples();
    for (int i = 0; i < input->dims->data[1]; ++i) {
      input->data.f[i] = samples[i];
    }

    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
      Serial.println("Invoke failed");
      return;
    }

    float max_value = -1;
    int max_index = -1;
    for (int i = 0; i < output->dims->data[1]; ++i) {
      if (output->data.f[i] > max_value) {
        max_value = output->data.f[i];
        max_index = i;
      }
    }

    switch (max_index) {
      case 0:
        Serial.println("Command: Turn on the light");
        controlLight(true);
        sendToCloud("Light On");
        break;
      case 1:
        Serial.println("Command: Turn off the light");
        controlLight(false);
        sendToCloud("Light Off");
        break;
      case 2:
        Serial.println("Command: Turn on the fan");
        controlFan(true);
        sendToCloud("Fan On");
        break;
      case 3:
        Serial.println("Command: Turn off the fan");
        controlFan(false);
        sendToCloud("Fan Off");
        break;
      case 4:
        Serial.println("Command: Lock the door");
        controlLock(true);
        sendToCloud("Door Locked");
        break;
      case 5:
        Serial.println("Command: Unlock the door");
        controlLock(false);
        sendToCloud("Door Unlocked");
        break;
      default:
        Serial.println("Unknown command");
        break;
    }
  }

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  bool motionDetected = digitalRead(MOTION_SENSOR_PIN);

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C  Humidity: ");
  Serial.print(humidity);
  Serial.print("%  Motion: ");
  Serial.println(motionDetected ? "Detected" : "Not Detected");

  if (temperature > 30.0) {
    controlFan(true);
  } else {
    controlFan(false);
  }

  if (motionDetected) {
    controlLight(true);
  } else {
    controlLight(false);
  }

  delay(2000);
}

void controlLight(bool state) {
  digitalWrite(LIGHT_PIN, state ? HIGH : LOW);
}

void controlFan(bool state) {
  digitalWrite(FAN_PIN, state ? HIGH : LOW);
}

void controlLock(bool state) {
  if (state) {
    doorServo.write(90); // Lock position
  } else {
    doorServo.write(0); // Unlock position
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
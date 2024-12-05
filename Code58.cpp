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

Servo doorServo;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

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

  connectToMQTT();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

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
  client.publish("home/light/status", state ? "ON" : "OFF");
}

void controlFan(bool state) {
  digitalWrite(FAN_PIN, state ? HIGH : LOW);
  client.publish("home/fan/status", state ? "ON" : "OFF");
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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic, "home/commands") == 0) {
    if (strncmp((char*)payload, "light_on", length) == 0) {
      controlLight(true);
    } else if (strncmp((char*)payload, "light_off", length) == 0) {
      controlLight(false);
    } else if (strncmp((char*)payload, "fan_on", length) == 0) {
      controlFan(true);
    } else if (strncmp((char*)payload, "fan_off", length) == 0) {
      controlFan(false);
    } else if (strncmp((char*)payload, "lock", length) == 0) {
      controlLock(true);
    } else if (strncmp((char*)payload, "unlock", length) == 0) {
      controlLock(false);
    }
  }
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("home/commands");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("home/commands");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/YU-LIN-LIN/mbed07/tree/5e46c75898fec1ccfedcc0c3942f9aff6aff8727/src%2Fmodel_deploy%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "1")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/yixuan0760/mbed07/tree/aa7bdf19287dd868b9ccd20b41669c7024c656cb/src%2Fmodel_deploy%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "2")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/leetoby1215/midterm_project/tree/c6eea65af4c6d00d41c59479c197fd3a0b56f5ac/src%2Fmodel_deploy%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "3")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/MariuszKrzanowski/iot-on-premises/tree/841ac7f3b62e74bbd04e585e4828528a38663e82/src%2FArduino%2FPromiscuousModeForwarder%2Fsrc%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "4")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/brico-labs/fingerprintReader/tree/b9728ba51fe430f9e43702751f04bd78aca97864/hardware%2FfingerprintSensor%2Fsrc%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "5")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/AndreasHuth/WifiMqttOta/tree/37ca2919c18447d8b2babf1db7c32898b4b987fb/src%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "6")

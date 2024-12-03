#include <TensorFlowLite.h>
#include "model.h"  // Include your trained model's header file
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// Create an accelerometer object
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Define model input and output parameters
constexpr int kTensorArenaSize = 2 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Gesture Recognition...");

  // Initialize the accelerometer
  if (!accel.begin()) {
    Serial.println("No ADXL345 detected!");
    while (1);
  }
  accel.setRange(ADXL345_RANGE_16_G);

  // Set up the model
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
  // Read accelerometer data
  sensors_event_t event;
  accel.getEvent(&event);
  
  // Set model input (assuming 3-axis accelerometer data)
  input->data.f[0] = event.acceleration.x;
  input->data.f[1] = event.acceleration.y;
  input->data.f[2] = event.acceleration.z;

  // Run inference
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    Serial.println("Invoke failed");
    return;
  }

  // Process output
  int gesture_id = -1;
  float max_value = -1.0;
  for (int i = 0; i < output->dims->data[1]; i++) {
    float value = output->data.f[i];
    if (value > max_value) {
      max_value = value;
      gesture_id = i;
    }
  }

  // Print the detected gesture
  Serial.print("Detected gesture ID: ");
  Serial.println(gesture_id);

  delay(500);
}
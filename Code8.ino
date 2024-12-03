#include <TensorFlowLite.h>
#include "model.h"  // Include your trained voice command model's header file
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Servo.h>

// Create an object for the microphone
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Create servo objects
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;
Servo servo5;

// Define model input and output parameters
constexpr int kTensorArenaSize = 2 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Voice-Controlled Robotic Arm...");

  // Initialize servos
  servo1.attach(9);
  servo2.attach(10);
  servo3.attach(11);
  servo4.attach(12);
  servo5.attach(13);

  // Initialize the accelerometer (if needed)
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
  // Read microphone data
  // This is a placeholder, replace with actual microphone reading code
  float microphone_value = analogRead(A0) / 1023.0;

  // Set model input
  input->data.f[0] = microphone_value;

  // Run inference
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    Serial.println("Invoke failed");
    return;
  }

  // Process output
  int command_id = -1;
  float max_value = -1.0;
  for (int i = 0; i < output->dims->data[1]; i++) {
    float value = output->data.f[i];
    if (value > max_value) {
      max_value = value;
      command_id = i;
    }
  }

  // Execute the detected command
  switch (command_id) {
    case 0:
      // Command 0: Move arm up
      servo1.write(90);
      break;
    case 1:
      // Command 1: Move arm down
      servo1.write(0);
      break;
    case 2:
      // Command 2: Open gripper
      servo5.write(90);
      break;
    case 3:
      // Command 3: Close gripper
      servo5.write(0);
      break;
    // Add more commands as needed
    default:
      break;
  }

  delay(500);
}
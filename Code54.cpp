#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/kernels/micro_ops.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

constexpr int tensor_arena_size = 2 * 1024;
uint8_t tensor_arena[tensor_arena_size];

const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

void setup() {
  Serial.begin(9600);
  if (!accel.begin()) {
    Serial.println("Could not find a valid ADXL345 sensor, check wiring!");
    while (1);
  }

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

void loop() {
  sensors_event_t event;
  accel.getEvent(&event);
  
  input->data.f[0] = event.acceleration.x;
  input->data.f[1] = event.acceleration.y;
  input->data.f[2] = event.acceleration.z;

  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    Serial.println("Invoke failed");
    return;
  }

  int gesture_index = output->data.f[0];

  switch (gesture_index) {
    case 0:
      Serial.println("Gesture 1 detected");
      break;
    case 1:
      Serial.println("Gesture 2 detected");
      break;
    case 2:
      Serial.println("Gesture 3 detected");
      break;
  }
  delay(200);
}

#include <TensorFlowLite.h>
#include <NewPing.h>
#include <Servo.h>
#include "model.h"  // Your converted model

#define TRIGGER_PIN_LEFT  9
#define ECHO_PIN_LEFT     8
#define TRIGGER_PIN_RIGHT 7
#define ECHO_PIN_RIGHT    6
#define MAX_DISTANCE      200

NewPing sonarLeft(TRIGGER_PIN_LEFT, ECHO_PIN_LEFT, MAX_DISTANCE);
NewPing sonarRight(TRIGGER_PIN_RIGHT, ECHO_PIN_RIGHT, MAX_DISTANCE);

Servo leftServo;
Servo rightServo;

// TensorFlow Lite setup
tflite::MicroErrorReporter tflErrorReporter;
tflite::Model* model = tflite::GetModel(g_model);
tflite::MicroInterpreter* interpreter;

void setup() {
  Serial.begin(115200);

  // Initialize servos
  leftServo.attach(10);  // Attach servo to pin 10
  rightServo.attach(11); // Attach servo to pin 11

  // Set up TensorFlow Lite model
  static tflite::MicroMutableOpResolver<10> micro_op_resolver;
  tflite::MicroInterpreter static_micro_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, &tflErrorReporter);

  interpreter = &static_micro_interpreter;
  interpreter->AllocateTensors();
}

void loop() {
  // Get distance measurements
  unsigned int distanceLeft = sonarLeft.ping_cm();
  unsigned int distanceRight = sonarRight.ping_cm();

  // Prepare input tensor
  float input[2] = {static_cast<float>(distanceLeft), static_cast<float>(distanceRight)};
  TfLiteTensor* input_tensor = interpreter->input(0);
  memcpy(input_tensor->data.f, input, 2 * sizeof(float));

  // Run inference
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    Serial.println("Invoke failed");
    return;
  }

  // Process output tensor
  TfLiteTensor* output_tensor = interpreter->output(0);
  float safePathScore = output_tensor->data.f[0];

  // Make decision based on model output
  if (safePathScore > 0.5) {
    // Move forward
    leftServo.write(90);  // Set servo to neutral
    rightServo.write(90); // Set servo to neutral
  } else {
    // Turn to avoid obstacle
    if (distanceLeft < distanceRight) {
      leftServo.write(0);   // Turn right
      rightServo.write(180); // Turn right
    } else {
      leftServo.write(180);  // Turn left
      rightServo.write(0);   // Turn left
    }
  }

  delay(100); // Wait before next loop
}
#include <TensorFlowLite.h>
#include <Arduino_ML.h>
#include "model.h"  // Your converted model

// Setup TensorFlow Lite model
tflite::MicroErrorReporter tflErrorReporter;
tflite::Model* model = tflite::GetModel(g_model);
tflite::MicroInterpreter* interpreter;

// Set up audio input
#include <PDM.h>

// Audio recording parameters
constexpr int SAMPLE_RATE = 16000;
constexpr int NUM_CHANNELS = 1;
constexpr int RECORD_TIME = 1; // seconds

// Buffer to hold audio samples
int16_t audioBuffer[SAMPLE_RATE * NUM_CHANNELS * RECORD_TIME];

// Audio recording callback function
void onAudioDataAvailable(int16_t* data, uint32_t size) {
  memcpy(audioBuffer, data, size * sizeof(int16_t));
}

void setup() {
  Serial.begin(9600);

  // Start PDM microphone
  if (!PDM.begin(NUM_CHANNELS, SAMPLE_RATE)) {
    Serial.println("Failed to start PDM!");
    while (1);
  }

  // Set up PDM callback function
  PDM.onReceive(onAudioDataAvailable);

  // Set up the TensorFlow Lite model
  static tflite::MicroMutableOpResolver<10> micro_op_resolver;
  tflite::MicroInterpreter static_micro_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, &tflErrorReporter);

  interpreter = &static_micro_interpreter;

  // Allocate memory for model's input and output tensors
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.println("AllocateTensors() failed");
    while (1);
  }
}

void loop() {
  // Wait for audio input
  if (PDM.available()) {
    // Copy audio samples to TensorFlow Lite input tensor
    TfLiteTensor* input_tensor = interpreter->input(0);
    memcpy(input_tensor->data.int16, audioBuffer, input_tensor->bytes);

    // Run inference
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
      Serial.println("Invoke failed");
      while (1);
    }

    // Process output tensor
    TfLiteTensor* output_tensor = interpreter->output(0);
    int recognized_digit = -1;
    for (int i = 0; i < output_tensor->dims->data[1]; i++) {
      if (output_tensor->data.f[i] > 0.5) {
        recognized_digit = i;
        break;
      }
    }

    // Print recognized digit
    if (recognized_digit != -1) {
      Serial.print("Recognized Digit: ");
      Serial.println(recognized_digit);
    } else {
      Serial.println("No digit recognized");
    }
  }
}
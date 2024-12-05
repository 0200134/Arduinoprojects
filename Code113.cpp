#include <Arduino.h>
#include <TensorFlowLite.h>
#include <PDM.h>
#include "audio_provider.h"
#include "feature_provider.h"
#include "model.h"
#include "recognize_commands.h"
#include "ring_buffer.h"

// Config
constexpr int kTensorArenaSize = 10 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

// Globals
FeatureProvider* feature_provider = nullptr;
RecognizeCommands* recognizer = nullptr;
TfLiteTensor* output = nullptr;
int32_t previous_time = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM!");
    while (1) {}
  }

  Serial.println("Setting up...");
  
  // Create the feature provider
  static FeatureProvider static_feature_provider(kFeatureElementCount, g_features);
  feature_provider = &static_feature_provider;

  // Initialize TensorFlow Lite
  tflite::MicroErrorReporter error_reporter;
  tflite::MicroInterpreter interpreter(g_model, g_op_resolver, tensor_arena, kTensorArenaSize, &error_reporter);
  interpreter.AllocateTensors();

  output = interpreter.output(0);

  // Set up the recognizer
  static RecognizeCommands static_recognizer(output);
  recognizer = &static_recognizer;
}

void loop() {
  const int32_t current_time = millis();

  // Fetch audio features
  int how_many_new_slices = 0;
  feature_provider->PopulateFeatureData(error_reporter, previous_time, current_time, &how_many_new_slices);

  if (how_many_new_slices == 0) {
    return;
  }

  // Run inference
  TfLiteStatus invoke_status = interpreter.Invoke();
  if (invoke_status != kTfLiteOk) {
    error_reporter.Report("Invoke failed");
    return;
  }

  // Recognize the command
  RecognizeCommands::Result result;
  recognizer->ProcessLatestResults(error_reporter, current_time, &result);

  // Print the recognized command
  if (result.found_command[0] != '\0' && result.score > 0.5) {
    Serial.print("Heard command: ");
    Serial.println(result.found_command);
  }

  previous_time = current_time;
}

#include <TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

// Define model data (replace with your model data)
extern const unsigned char model_data[];

// Globals, used for compatibility with Arduino-style sketches.
namespace {
  tflite::MicroErrorReporter micro_error_reporter;
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;

  // Create an area of memory to use for input, output, and intermediate arrays.
  constexpr int kTensorArenaSize = 2 * 1024;
  uint8_t tensor_arena[kTensorArenaSize];
} // namespace

void setup() {
  // Set up error reporting.
  error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure.
  model = tflite::GetModel(model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // Pull in all the operation implementations we need.
  tflite::MicroMutableOpResolver<10> micro_op_resolver;
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_ADD, tflite::ops::micro::Register_ADD());
  // Add other operations your model needs here

  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }

  // Obtain pointers to the model's input and output tensors.
  input = interpreter->input(0);
  output = interpreter->output(0);
}

void loop() {
  // Put your main code here, to run repeatedly:
  // E.g., read sensor data and fill input tensor, run inference, etc.

  // Dummy example filling input tensor
  input->data.f[0] = 1.0;
  input->data.f[1] = 2.0;

  // Run the model on this input and make sure it succeeds.
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed on input: %f, %f",
                         input->data.f[0], input->data.f[1]);
    return;
  }

  // Extract the output and print it
  float output_value = output->data.f[0];
  Serial.println(output_value);
}
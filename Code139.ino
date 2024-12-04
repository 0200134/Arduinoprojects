#include <TensorFlowLite.h>
#include <Arduino_OV7670.h>
#include <Arduino_LSM9DS1.h>
#include <Arduino_MPU6050.h>
#include <Arduino_Sound.h>
#include <NewPing.h>
#include <Servo.h>
#include "object_recognition_model.h"  // Your object recognition model
#include "voice_command_model.h"       // Your voice command model
#include "slam_model.h"                // Your SLAM model

#define TRIGGER_PIN  9
#define ECHO_PIN     8
#define MAX_DISTANCE 200

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
Arduino_OV7670 camera;
LSM9DS1 imu;
MPU6050 mpu;
Servo leftServo;
Servo rightServo;

// TensorFlow Lite setup
tflite::MicroErrorReporter tflErrorReporter;
tflite::Model* objectModel = tflite::GetModel(g_object_recognition_model);
tflite::Model* voiceModel = tflite::GetModel(g_voice_command_model);
tflite::Model* slamModel = tflite::GetModel(g_slam_model);
tflite::MicroInterpreter* objectInterpreter;
tflite::MicroInterpreter* voiceInterpreter;
tflite::MicroInterpreter* slamInterpreter;

void setup() {
  Serial.begin(115200);

  // Initialize sensors
  camera.begin(QVGA, RGB565);
  if (!imu.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  if (!mpu.begin(MPU6050_SCALE_2000DPS, MPU6050_RANGE_2G)) {
    Serial.println("Failed to initialize MPU6050!");
    while (1);
  }
  if (!AudioIn.begin(16000)) {
    Serial.println("Failed to start microphone!");
    while (1);
  }

  // Initialize servos
  leftServo.attach(10);  // Attach servo to pin 10
  rightServo.attach(11); // Attach servo to pin 11

  // Set up TensorFlow Lite models
  static tflite::MicroMutableOpResolver<10> micro_op_resolver;
  tflite::MicroInterpreter static_object_interpreter(
      objectModel, micro_op_resolver, tensor_arena, kTensorArenaSize, &tflErrorReporter);
  tflite::MicroInterpreter static_voice_interpreter(
      voiceModel, micro_op_resolver, tensor_arena, kTensorArenaSize, &tflErrorReporter);
  tflite::MicroInterpreter static_slam_interpreter(
      slamModel, micro_op_resolver, tensor_arena, kTensorArenaSize, &tflErrorReporter);

  objectInterpreter = &static_object_interpreter;
  voiceInterpreter = &static_voice_interpreter;
  slamInterpreter = &static_slam_interpreter;

  objectInterpreter->AllocateTensors();
  voiceInterpreter->AllocateTensors();
  slamInterpreter->AllocateTensors();
}

void loop() {
  // Capture image
  uint8_t* image = camera.captureImage();

  // Get IMU data
  imu.read();
  float ax = imu.calcAccel(imu.ax);
  float ay = imu.calcAccel(imu.ay);
  float az = imu.calcAccel(imu.az);
  float gx = imu.calcGyro(imu.gx);
  float gy = imu.calcGyro(imu.gy);
  float gz = imu.calcGyro(imu.gz);

  // Get distance measurements
  unsigned int distance = sonar.ping_cm();

  // Prepare input tensor for object recognition
  TfLiteTensor* objectInputTensor = objectInterpreter->input(0);
  memcpy(objectInputTensor->data.uint8, image, objectInputTensor->bytes);

  // Run object recognition inference
  TfLiteStatus objectInvokeStatus = objectInterpreter->Invoke();
  if (objectInvokeStatus != kTfLiteOk) {
    Serial.println("Object recognition invoke failed");
    return;
  }

  // Process object recognition output tensor
  TfLiteTensor* objectOutputTensor = objectInterpreter->output(0);
  int recognizedObject = -1;
  float maxScore = 0.0;
  for (int i = 0; i < objectOutputTensor->dims->data[1]; i++) {
    if (objectOutputTensor->data.f[i] > maxScore) {
      maxScore = objectOutputTensor->data.f[i];
      recognizedObject = i;
    }
  }

  // Print recognized object
  Serial.print("Recognized Object: ");
  Serial.println(recognizedObject);

  // Capture audio
  int audioBuffer[16000];
  int bytesRead = AudioIn.read(audioBuffer, sizeof(audioBuffer));

  // Prepare input tensor for voice command recognition
  TfLiteTensor* voiceInputTensor = voiceInterpreter->input(0);
  memcpy(voiceInputTensor->data.int16, audioBuffer, voiceInputTensor->bytes);

  // Run voice command recognition inference
  TfLiteStatus voiceInvokeStatus = voiceInterpreter->Invoke();
  if (voiceInvokeStatus != kTfLiteOk) {
    Serial.println("Voice command recognition invoke failed");
    return;
  }

  // Process voice command recognition output tensor
  TfLiteTensor* voiceOutputTensor = voiceInterpreter->output(0);
  int recognizedCommand = -1;
  float maxScore = 0.0;
  for (int i = 0; i < voiceOutputTensor->dims->data[1]; i++) {
    if (voiceOutputTensor->data.f[i] > maxScore) {
      maxScore = voiceOutputTensor->data.f[i];
      recognizedCommand = i;
    }
  }

  // Print recognized command
  Serial.print("Recognized Command: ");
  Serial.println(recognizedCommand);

  // Prepare input tensor for SLAM
  float input[10] = {ax, ay, az, gx, gy, gz, (float)distance, (float)recognizedObject, (float)recognizedCommand, 0}; // Adjust accordingly
  TfLiteTensor* slamInputTensor = slamInterpreter->input(0);
  memcpy(slamInputTensor->data.f, input, 10 * sizeof(float));

  // Run SLAM inference
  TfLiteStatus slamInvokeStatus = slamInterpreter->Invoke();
  if (slamInvokeStatus != kTfLiteOk) {
    Serial.println("SLAM invoke failed");
    return;
  }

  // Process SLAM output tensor
  TfLiteTensor* slamOutputTensor = slamInterpreter->output(0);
  float navigationCommand = slamOutputTensor->data.f[0];

  // Print navigation command
  Serial.print("Navigation Command: ");
  Serial.println(navigationCommand);

  // Make decision based on SLAM output
  if (navigationCommand > 0.5) {
    Serial.println("Moving forward.");
    leftServo.write(90);  // Set servo to neutral
    rightServo.write(90); // Set servo to neutral
  } else {
    Serial.println("Avoiding obstacle.");
    leftServo.write(0);   // Turn
    rightServo.write(180); // Turn
  }

  delay(100); // Wait before next loop
}
#include <TensorFlowLite.h>
#include <Arduino_OV7670.h>
#include <Arduino_LSM9DS1.h>
#include <Arduino_MPU6050.h>
#include <NewPing.h>
#include <Servo.h>
#include "slam_model.h"  // Your SLAM model

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
tflite::Model* model = tflite::GetModel(g_slam_model);
tflite::MicroInterpreter* interpreter;

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

  // Prepare input tensor for SLAM
  float input[10] = {ax, ay, az, gx, gy, gz, (float)distance, 0, 0, 0}; // Adjust accordingly
  TfLiteTensor* input_tensor = interpreter->input(0);
  memcpy(input_tensor->data.f, input, 10 * sizeof(float));

  // Run SLAM inference
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    Serial.println("SLAM invoke failed");
    return;
  }

  // Process SLAM output tensor
  TfLiteTensor* output_tensor = interpreter->output(0);
  float navigationCommand = output_tensor->data.f[0];

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

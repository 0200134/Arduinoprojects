#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TinyGPS++.h>
#include <Servo.h>
#include <WiFi.h>
#include <ESP32WebServer.h>

// Define constants
#define TRIG_PIN 6
#define ECHO_PIN 7
#define MOTOR_PIN_1 9
#define MOTOR_PIN_2 10
#define MOTOR_PIN_3 11
#define MOTOR_PIN_4 12

Adafruit_MPU6050 mpu;
TinyGPSPlus gps;
Servo motor1, motor2, motor3, motor4;
ESP32WebServer server(80);

// Wi-Fi credentials
const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";

void setup() {
  Serial.begin(115200);
  
  // Initialize IMU
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  // Initialize GPS
  Serial1.begin(9600);

  // Initialize motors
  motor1.attach(MOTOR_PIN_1);
  motor2.attach(MOTOR_PIN_2);
  motor3.attach(MOTOR_PIN_3);
  motor4.attach(MOTOR_PIN_4);

  // Initialize ultrasonic sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Set up web server
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/control", handleControl);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  // Read IMU data
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Read GPS data
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // Read ultrasonic sensor
  long duration, distance;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;

  // Display data on serial monitor
  Serial.print("IMU: ");
  Serial.print(a.acceleration.x);
  Serial.print(", ");
  Serial.print(a.acceleration.y);
  Serial.print(", ");
  Serial.println(a.acceleration.z);
  Serial.print("GPS: ");
  Serial.print(gps.location.lat(), 6);
  Serial.print(", ");
  Serial.println(gps.location.lng(), 6);
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Control motors based on data
  // Add logic for autonomous navigation and obstacle avoidance
  
  delay(500);
  server.handleClient();
}

void handleRoot() {
  server.send(200, "text/html", "<h1>Autonomous Drone System</h1><ul><li><a href='/status'>System Status</a></li><li><a href='/control'>Control Motors</a></li></ul>");
}

void handleStatus() {
  String message = "IMU: " + String(a.acceleration.x) + ", " + String(a.acceleration.y) + ", " + String(a.acceleration.z) + "\n";
  message += "GPS: " + String(gps.location.lat(), 6) + ", " + String(gps.location.lng(), 6) + "\n";
  message += "Distance: " + String(distance) + " cm\n";
  
  server.send(200, "text/plain", message);
}

void handleControl() {
  String command = server.arg("command");
  if (command == "start") {
    // Start motors
    motor1.writeMicroseconds(1500);
    motor2.writeMicroseconds(1500);
    motor3.writeMicroseconds(1500);
    motor4.writeMicroseconds(1500);
    server.send(200, "text/plain", "Motors started");
  } else if (command == "stop") {
    // Stop motors
    motor1.writeMicroseconds(1000);
    motor2.writeMicroseconds(1000);
    motor3.writeMicroseconds(1000);
    motor4.writeMicroseconds(1000);
    server.send(200, "text/plain", "Motors stopped");
  } else {
    server.send(200, "text/plain", "Invalid command");
  }
}
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <MPU6050_light.h>
#include <EdgeImpulseModel.h> // Include the model header file generated by Edge Impulse

#define DHTPIN 2 // Pin where the DHT11 is connected
#define DHTTYPE DHT11
#define PIRPIN 3 // Pin where the PIR sensor is connected
#define UVPIN A4 // Pin where the UV sensor is connected
#define GASPIN A5 // Pin where the MQ-2 gas sensor is connected

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
MPU6050 mpu(Wire);
const int tempSensorPin = A0;
const int lightSensorPin = A1;
const int soundSensorPin = A2;
const int airQualitySensorPin = A3;

volatile int pirState = LOW; // PIR motion sensor state
bool pirTriggered = false; // PIR sensor interrupt flag

unsigned long previousMillis = 0;
const long interval = 200; // Sampling interval in milliseconds

struct SensorData {
  float temperature;
  float humidity;
  float temperatureDHT;
  int lightLevel;
  int soundLevel;
  int airQualityLevel;
  float uvLevel;
  float gasLevel;
  float pressure;
  float accelX;
  float accelY;
  float accelZ;
  float gyroX;
  float gyroY;
  float gyroZ;
} data;

void setup() {
  Serial.begin(9600);
  pinMode(PIRPIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIRPIN), pirISR, CHANGE); // Setup interrupt for PIR sensor
  dht.begin();
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
  Wire.begin();
  mpu.begin();
  mpu.calcGyroOffsets(true);
  
  // Initialize the Edge Impulse model
  if (!ei_impulse_start()) {
    Serial.println("Model init failed!");
    while (1);
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    collectSensorData();
    if (isnan(data.humidity) || isnan(data.temperatureDHT)) {
      Serial.println("DHT sensor error!");
      return;
    }
    runModel();
    printResults();
    pirTriggered = false; // Reset PIR flag
  }
}

void collectSensorData() {
  data.temperature = analogRead(tempSensorPin) * 0.48828125;
  data.humidity = dht.readHumidity();
  data.temperatureDHT = dht.readTemperature();
  data.lightLevel = analogRead(lightSensorPin);
  data.soundLevel = analogRead(soundSensorPin);
  data.airQualityLevel = analogRead(airQualitySensorPin);
  data.uvLevel = analogRead(UVPIN) * (5.0 / 1023.0); // Convert to voltage for UV sensor
  data.gasLevel = analogRead(GASPIN) * (5.0 / 1023.0); // Convert to voltage for gas sensor
  data.pressure = bmp.readPressure() / 100.0F; // Read barometric pressure in hPa
  
  mpu.update();
  data.accelX = mpu.getAccX();
  data.accelY = mpu.getAccY();
  data.accelZ = mpu.getAccZ();
  data.gyroX = mpu.getGyroX();
  data.gyroY = mpu.getGyroY();
  data.gyroZ = mpu.getGyroZ();
}

void runModel() {
  ei_impulse_features[0] = data.temperature;
  ei_impulse_features[1] = data.humidity;
  ei_impulse_features[2] = data.lightLevel;
  ei_impulse_features[3] = data.soundLevel;
  ei_impulse_features[4] = data.airQualityLevel;
  ei_impulse_features[5] = pirState;
  ei_impulse_features[6] = data.uvLevel;
  ei_impulse_features[7] = data.pressure;
  ei_impulse_features[8] = data.gasLevel;
  ei_impulse_features[9] = data.accelX;
  ei_impulse_features[10] = data.accelY;
  ei_impulse_features[11] = data.accelZ;
  ei_impulse_features[12] = data.gyroX;
  ei_impulse_features[13] = data.gyroY;
  ei_impulse_features[14] = data.gyroZ;
  
  ei_impulse_result_t result = ei_impulse_run();
}

void printResults() {
  Serial.print("LM35 Temp: "); Serial.print(data.temperature); Serial.print("°C, ");
  Serial.print("DHT11 Temp: "); Serial.print(data.temperatureDHT); Serial.print("°C, ");
  Serial.print("Humidity: "); Serial.print(data.humidity); Serial.print("%, ");
  Serial.print("Light: "); Serial.print(data.lightLevel); Serial.print(", ");
  Serial.print("Sound: "); Serial.print(data.soundLevel); Serial.print(", ");
  Serial.print("Air Quality: "); Serial.print(data.airQualityLevel); Serial.print(", ");
  Serial.print("Motion: "); Serial.print(pirTriggered ? "Detected" : "Not Detected"); Serial.print(", ");
  Serial.print("UV: "); Serial.print(data.uvLevel); Serial.print("V, ");
  Serial.print("Pressure: "); Serial.print(data.pressure); Serial.print("hPa, ");
  Serial.print("Gas: "); Serial.print(data.gasLevel); Serial.print("V, ");
  Serial.print("Accel (X, Y, Z): ("); Serial.print(data.accelX); Serial.print(", "); Serial.print(data.accelY); Serial.print(", "); Serial.print(data.accelZ); Serial.print("), ");
  Serial.print("Gyro (X, Y, Z): ("); Serial.print(data.gyroX); Serial.print(", "); Serial.print(data.gyroY); Serial.print(", "); Serial.print(data.gyroZ); Serial.print(")");
  Serial.print(" - Prediction: "); Serial.println(result.classification[0].label);
}

void pirISR() {
  pirState = digitalRead(PIRPIN);
  pirTriggered = true; // Set PIR flag on interrupt
}
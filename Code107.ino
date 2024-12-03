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
  // Efficient sensor reading
  float temperature = analogRead(tempSensorPin) * 0.48828125;
  float humidity = dht.readHumidity();
  float temperatureDHT = dht.readTemperature();
  int lightLevel = analogRead(lightSensorPin);
  int soundLevel = analogRead(soundSensorPin);
  int airQualityLevel = analogRead(airQualitySensorPin);
  float uvLevel = analogRead(UVPIN) * (5.0 / 1023.0); // Convert to voltage for UV sensor
  float gasLevel = analogRead(GASPIN) * (5.0 / 1023.0); // Convert to voltage for gas sensor
  float pressure = bmp.readPressure() / 100.0F; // Read barometric pressure in hPa

  mpu.update();
  float accelX = mpu.getAccX();
  float accelY = mpu.getAccY();
  float accelZ = mpu.getAccZ();
  float gyroX = mpu.getGyroX();
  float gyroY = mpu.getGyroY();
  float gyroZ = mpu.getGyroZ();

  // Check DHT11 errors
  if (isnan(humidity) || isnan(temperatureDHT)) {
    Serial.println("DHT sensor error!");
    delay(500);
    return;
  }

  // Prepare features and run model
  ei_impulse_features[0] = temperature;
  ei_impulse_features[1] = humidity;
  ei_impulse_features[2] = lightLevel;
  ei_impulse_features[3] = soundLevel;
  ei_impulse_features[4] = airQualityLevel;
  ei_impulse_features[5] = pirState;
  ei_impulse_features[6] = uvLevel;
  ei_impulse_features[7] = pressure;
  ei_impulse_features[8] = gasLevel;
  ei_impulse_features[9] = accelX;
  ei_impulse_features[10] = accelY;
  ei_impulse_features[11] = accelZ;
  ei_impulse_features[12] = gyroX;
  ei_impulse_features[13] = gyroY;
  ei_impulse_features[14] = gyroZ;
  ei_impulse_result_t result = ei_impulse_run();

  // Print results efficiently
  Serial.print("LM35 Temp: "); Serial.print(temperature); Serial.print("°C, ");
  Serial.print("DHT11 Temp: "); Serial.print(temperatureDHT); Serial.print("°C, ");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.print("%, ");
  Serial.print("Light: "); Serial.print(lightLevel); Serial.print(", ");
  Serial.print("Sound: "); Serial.print(soundLevel); Serial.print(", ");
  Serial.print("Air Quality: "); Serial.print(airQualityLevel); Serial.print(", ");
  Serial.print("Motion: "); Serial.print(pirTriggered ? "Detected" : "Not Detected"); Serial.print(", ");
  Serial.print("UV: "); Serial.print(uvLevel); Serial.print("V, ");
  Serial.print("Pressure: "); Serial.print(pressure); Serial.print("hPa, ");
  Serial.print("Gas: "); Serial.print(gasLevel); Serial.print("V, ");
  Serial.print("Accel (X, Y, Z): ("); Serial.print(accelX); Serial.print(", "); Serial.print(accelY); Serial.print(", "); Serial.print(accelZ); Serial.print("), ");
  Serial.print("Gyro (X, Y, Z): ("); Serial.print(gyroX); Serial.print(", "); Serial.print(gyroY); Serial.print(", "); Serial.print(gyroZ); Serial.print(")");
  Serial.print(" - Prediction: "); Serial.println(result.classification[0].label);

  pirTriggered = false; // Reset PIR flag

  delay(200); // Reduced delay to 200 ms
}

void pirISR() {
  pirState = digitalRead(PIRPIN);
  pirTriggered = true; // Set PIR flag on interrupt
}
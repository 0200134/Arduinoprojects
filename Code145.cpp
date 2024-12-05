#include <Arduino.h>
#include <EdgeImpulse.h>
#include <EdgeImpulseModel.h>
#include <Arduino_LSM9DS1.h> // Library for the accelerometer and gyroscope
#include <Arduino_HTS221.h>  // Library for the temperature sensor
#include <Arduino_APDS9960.h> // Library for the light sensor

#define FIXED_POINT_PRECISION 1000

// Kalman filter variables
float kalman_temp = 0.0;
float kalman_ax = 0.0, kalman_ay = 0.0, kalman_az = 0.0;
float kalman_gyro_x = 0.0, kalman_gyro_y = 0.0, kalman_gyro_z = 0.0;
float kalman_light = 0.0;

// Kalman filter functions
float kalmanFilter(float newValue, float* prevValue, float* error, float Q, float R);

void setup() {
    Serial.begin(115200);

    if (!IMU.begin()) {
        Serial.println("Failed to initialize IMU!");
        while (1);
    }

    if (!HTS.begin()) {
        Serial.println("Failed to initialize temperature sensor!");
        while (1);
    }

    if (!APDS.begin()) {
        Serial.println("Failed to initialize light sensor!");
        while (1);
    }

    EdgeImpulseModel.begin();
}

void loop() {
    float temp, light;
    int ax, ay, az;
    int gx, gy, gz;
    static int32_t normalized_temp, normalized_ax, normalized_ay, normalized_az, normalized_light;
    static int32_t normalized_gyro_x, normalized_gyro_y, normalized_gyro_z;

    // Read sensor data
    temp = HTS.readTemperature();
    IMU.readAcceleration(ax, ay, az);
    IMU.readGyroscope(gx, gy, gz);
    light = APDS.readLight();

    // Apply Kalman filter
    kalman_temp = kalmanFilter(temp, &kalman_temp, 1.0, 1.0, 0.1);
    kalman_ax = kalmanFilter(ax, &kalman_ax, 1.0, 1.0, 0.1);
    kalman_ay = kalmanFilter(ay, &kalman_ay, 1.0, 1.0, 0.1);
    kalman_az = kalmanFilter(az, &kalman_az, 1.0, 1.0, 0.1);
    kalman_gyro_x = kalmanFilter(gx, &kalman_gyro_x, 1.0, 1.0, 0.1);
    kalman_gyro_y = kalmanFilter(gy, &kalman_gyro_y, 1.0, 1.0, 0.1);
    kalman_gyro_z = kalmanFilter(gz, &kalman_gyro_z, 1.0, 1.0, 0.1);
    kalman_light = kalmanFilter(light, &kalman_light, 1.0, 1.0, 0.1);

    // Normalize sensor data using fixed-point arithmetic
    normalized_temp = (int32_t)(kalman_temp * FIXED_POINT_PRECISION);
    normalized_ax = (int32_t)((kalman_ax + 4.0) * FIXED_POINT_PRECISION / 8.0);
    normalized_ay = (int32_t)((kalman_ay + 4.0) * FIXED_POINT_PRECISION / 8.0);
    normalized_az = (int32_t)((kalman_az + 4.0) * FIXED_POINT_PRECISION / 8.0);
    normalized_gyro_x = (int32_t)((kalman_gyro_x + 250.0) * FIXED_POINT_PRECISION / 500.0);
    normalized_gyro_y = (int32_t)((kalman_gyro_y + 250.0) * FIXED_POINT_PRECISION / 500.0);
    normalized_gyro_z = (int32_t)((kalman_gyro_z + 250.0) * FIXED_POINT_PRECISION / 500.0);
    normalized_light = (int32_t)(kalman_light * FIXED_POINT_PRECISION / 1024.0);

    // Run the model with sensor data
    int32_t result = EdgeImpulseModel.predict(
        normalized_temp, normalized_ax, normalized_ay, normalized_az,
        normalized_gyro_x, normalized_gyro_y, normalized_gyro_z, normalized_light
    );

    // Output result (convert back to floating-point for display)
    Serial.print("Prediction: ");
    Serial.println((float)result / FIXED_POINT_PRECISION);

    // Delay for stability
    delay(100);
}

// Kalman filter function
float kalmanFilter(float newValue, float* prevValue, float* error, float Q, float R) {
    *error += Q;
    float K = *error / (*error + R);
    *prevValue = *prevValue + K * (newValue - *prevValue);
    *error = (1 - K) * *error;
    return *prevValue;
}

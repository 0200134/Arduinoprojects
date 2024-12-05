#include <Arduino.h>
#include <EdgeImpulse.h>
#include <EdgeImpulseModel.h>
#include <Arduino_LSM9DS1.h> // Library for the accelerometer
#include <Arduino_HTS221.h>  // Library for the temperature sensor

#define FIXED_POINT_PRECISION 1000

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

    EdgeImpulseModel.begin();
}

void loop() {
    float temp;
    int ax, ay, az;
    static int32_t normalized_temp, normalized_ax, normalized_ay, normalized_az;

    // Read temperature data
    temp = HTS.readTemperature();

    // Read accelerometer data
    IMU.readAcceleration(ax, ay, az);

    // Normalize sensor data using fixed-point arithmetic
    normalized_temp = (int32_t)(temp * FIXED_POINT_PRECISION);
    normalized_ax = (int32_t)((ax + 4.0) * FIXED_POINT_PRECISION / 8.0);
    normalized_ay = (int32_t)((ay + 4.0) * FIXED_POINT_PRECISION / 8.0);
    normalized_az = (int32_t)((az + 4.0) * FIXED_POINT_PRECISION / 8.0);

    // Run the model with sensor data
    int32_t result = EdgeImpulseModel.predict(normalized_temp, normalized_ax, normalized_ay, normalized_az);

    // Output result (convert back to floating-point for display)
    Serial.print("Prediction: ");
    Serial.println((float)result / FIXED_POINT_PRECISION);

    // Delay for stability
    delay(100);
}

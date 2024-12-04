#include <Arduino.h>
#include <EdgeImpulse.h>
#include <EdgeImpulseModel.h>

// Fixed-point precision
#define FIXED_POINT_PRECISION 1000

void setup() {
    Serial.begin(115200);
    EdgeImpulseModel.begin();
}

void loop() {
    static const uint8_t sensor_pin = A0;
    static int16_t sensor_value;

    // Read sensor data
    sensor_value = analogRead(sensor_pin);

    // Normalize sensor data using fixed-point arithmetic
    int32_t normalized_value = (sensor_value * FIXED_POINT_PRECISION) / 1023;

    // Run the model (assume model can handle fixed-point)
    int32_t result = EdgeImpulseModel.predict(normalized_value);

    // Output result (convert back to floating-point for display)
    Serial.println((float)result / FIXED_POINT_PRECISION);

    // Delay for stability
    delay(100);
}
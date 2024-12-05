#include <Arduino.h>
#include <EdgeImpulse.h>
#include <EdgeImpulseModel.h>

void setup() {
    Serial.begin(115200);
    EdgeImpulseModel.begin();
}

void loop() {
    static const uint8_t sensor_pin = A0;
    static int16_t sensor_value;

    // Read sensor data
    sensor_value = analogRead(sensor_pin);

    // Normalize sensor data
    float normalized_value = (float)sensor_value / 1023.0;

    // Run the model
    float result = EdgeImpulseModel.predict(normalized_value);

    // Output result
    Serial.println(result);

    // Delay for stability
    delay(100);
}

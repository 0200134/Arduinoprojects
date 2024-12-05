#include <Arduino.h>
#include <EdgeImpulse.h>

void setup() {
    Serial.begin(115200);
    // Initialize Edge Impulse
    EdgeImpulse.begin();
}

void loop() {
    // Collect data from sensors
    float sensor_value = analogRead(A0);
    // Send data to Edge Impulse
    EdgeImpulse.sendData(sensor_value);
    delay(100);
}

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino_LSM9DS1.h>
#include <Arduino_HTS221.h>
#include <Arduino_APDS9960.h>

#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
#define MQTT_BROKER "your_mqtt_broker"
#define MQTT_PORT 1883
#define MQTT_TOPIC "your_mqtt_topic"
#define DEVICE_ID "your_device_id"
#define FIXED_POINT_PRECISION 1000

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    client.setServer(MQTT_BROKER, MQTT_PORT);

    if (!client.connected()) {
        Serial.println("Connecting to MQTT broker...");
        if (client.connect(DEVICE_ID)) {
            Serial.println("Connected to MQTT broker");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            delay(5000);
        }
    }

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
}

void loop() {
    float temp, light;
    int ax, ay, az;

    // Read sensor data
    temp = HTS.readTemperature();
    IMU.readAcceleration(ax, ay, az);
    light = APDS.readLight();

    // Normalize sensor data
    int32_t normalized_temp = (int32_t)(temp * FIXED_POINT_PRECISION);
    int32_t normalized_ax = (int32_t)((ax + 4.0) * FIXED_POINT_PRECISION / 8.0);
    int32_t normalized_ay = (int32_t)((ay + 4.0) * FIXED_POINT_PRECISION / 8.0);
    int32_t normalized_az = (int32_t)((az + 4.0) * FIXED_POINT_PRECISION / 8.0);
    int32_t normalized_light = (int32_t)(light * FIXED_POINT_PRECISION / 1024.0);

    // Create JSON object
    StaticJsonDocument<200> doc;
    doc["temperature"] = normalized_temp;
    doc["acceleration_x"] = normalized_ax;
    doc["acceleration_y"] = normalized_ay;
    doc["acceleration_z"] = normalized_az;
    doc["light"] = normalized_light;

    char buffer[256];
    serializeJson(doc, buffer);

    // Publish data to MQTT
    client.publish(MQTT_TOPIC, buffer);
    client.loop();

    // Delay for stability
    delay(1000);
}
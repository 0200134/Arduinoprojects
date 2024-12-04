#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Pin Definitions
#define DHTPIN 2
#define DHTTYPE DHT22
#define RELAY1 3
#define RELAY2 4
#define PIRPIN 6
#define LDRPIN A0

// WiFi and MQTT Server Settings
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "mqtt.example.com";

// Objects
DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(PIRPIN, INPUT);
  dht.begin();

  setupWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
}

void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  if (strcmp(topic, "home/relay1") == 0) {
    handleRelayCommand(RELAY1, message);
  } else if (strcmp(topic, "home/relay2") == 0) {
    handleRelayCommand(RELAY2, message);
  }
}

void handleRelayCommand(int relayPin, String command) {
  if (command == "on") {
    digitalWrite(relayPin, LOW); // Relay module is active LOW
  } else if (command == "off") {
    digitalWrite(relayPin, HIGH);
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ArduinoClient")) {
      Serial.println("connected");
      client.subscribe("home/relay1");
      client.subscribe("home/relay2");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int pirState = digitalRead(PIRPIN);
  int lightLevel = analogRead(LDRPIN);

  if (!isnan(temperature) && !isnan(humidity)) {
    publishSensorData("home/temperature", temperature);
    publishSensorData("home/humidity", humidity);
  }

  publishSensorData("home/motion", pirState);
  publishSensorData("home/light", lightLevel);

  delay(2000); // Publish data every 2 seconds
}

void publishSensorData(const char* topic, float value) {
  char payload[8];
  dtostrf(value, 1, 2, payload);
  client.publish(topic, payload);
}

void publishSensorData(const char* topic, int value) {
  char payload[8];
  itoa(value, payload, 10);
  client.publish(topic, payload);
}
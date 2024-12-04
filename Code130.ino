#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define RELAY1 3
#define RELAY2 4

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "mqtt.example.com";

void setup() {
  Serial.begin(115200);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  dht.begin();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
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

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  if (strcmp(topic, "home/relay1") == 0) {
    if (message == "on") {
      digitalWrite(RELAY1, LOW); // Relay module is active LOW
    } else if (message == "off") {
      digitalWrite(RELAY1, HIGH);
    }
  } else if (strcmp(topic, "home/relay2") == 0) {
    if (message == "on") {
      digitalWrite(RELAY2, LOW);
    } else if (message == "off") {
      digitalWrite(RELAY2, HIGH);
    }
  }
}

void reconnect() {
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
    reconnect();
  }
  client.loop();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  char tempString[8];
  dtostrf(temperature, 1, 2, tempString);
  client.publish("home/temperature", tempString);

  char humString[8];
  dtostrf(humidity, 1, 2, humString);
  client.publish("home/humidity", humString);

  delay(2000); // Publish data every 2 seconds
}
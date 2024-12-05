#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
const char* mqtt_server = "your_MQTT_server_IP";

WiFiClient espClient;
PubSubClient client(espClient);

const int relayPin1 = D1;
const int relayPin2 = D2;
const int relayPin3 = D3;
const int relayPin4 = D4;

void setup() {
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(relayPin3, OUTPUT);
  pinMode(relayPin4, OUTPUT);
  digitalWrite(relayPin1, HIGH);
  digitalWrite(relayPin2, HIGH);
  digitalWrite(relayPin3, HIGH);
  digitalWrite(relayPin4, HIGH);

  Serial.begin(115200);
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
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  if (String(topic) == "home/relay1") {
    if (message == "ON") {
      digitalWrite(relayPin1, LOW);
    } else {
      digitalWrite(relayPin1, HIGH);
    }
  }
  if (String(topic) == "home/relay2") {
    if (message == "ON") {
      digitalWrite(relayPin2, LOW);
    } else {
      digitalWrite(relayPin2, HIGH);
    }
  }
  if (String(topic) == "home/relay3") {
    if (message == "ON") {
      digitalWrite(relayPin3, LOW);
    } else {
      digitalWrite(relayPin3, HIGH);
    }
  }
  if (String(topic) == "home/relay4") {
    if (message == "ON") {
      digitalWrite(relayPin4, LOW);
    } else {
      digitalWrite(relayPin4, HIGH);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe("home/relay1");
      client.subscribe("home/relay2");
      client.subscribe("home/relay3");
      client.subscribe("home/relay4");
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
}

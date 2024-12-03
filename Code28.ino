#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define LIGHT_SENSOR A0
#define RELAY_PIN 8
#define SERVO_PIN 9

DHT dht(DHTPIN, DHTTYPE);
Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "YOUR_MQTT_SERVER";

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);

void setup() {
  Serial.begin(9600);
  dht.begin();
  myServo.attach(SERVO_PIN);
  lcd.begin();
  lcd.backlight();
  
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);
  
  server.on("/", handleRoot);
  server.begin();
}

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == "home/light") {
    if (message == "ON") {
      digitalWrite(RELAY_PIN, HIGH);
    } else if (message == "OFF") {
      digitalWrite(RELAY_PIN, LOW);
    }
  } else if (String/servo") {
    myServo.write(message.toInt());
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ArduinoClient")) {
      client.subscribe("home/light");
      client.subscribe("home/servo");
    } else {
      delay(5000);
    }
  }
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Home Automation System</h1>";
  html += "<p>Light: <a href=\"/light/on\">ON</a> <a href=\"/light/off\">OFF</a></p>";
  html += "<p>Servo: <a href=\"/servo/90\">90</a> <a href=\"/servo/0\">0</a></p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  server.handleClient();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int lightLevel = analogRead(LIGHT_SENSOR);
  
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(t);
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(h);
  lcd.print(" %");

  if (lightLevel < 300) {
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }

  if (t > 25) {
    myServo.write(90);
  } else {
    myServo.write(0);
  }

  delay(2000);
}
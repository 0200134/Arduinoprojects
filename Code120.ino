#include <ESP8266WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>

#define WIFI_SSID         "your_SSID"
#define WIFI_PASS         "your_PASSWORD"
#define APP_KEY           "your_APP_KEY"
#define APP_SECRET        "your_APP_SECRET"
#define SWITCH_ID         "your_SWITCH_ID"

#define RELAY_PIN         D1

bool onPowerState(const String &deviceId, bool &state) {
  digitalWrite(RELAY_PIN, state ? LOW : HIGH);
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];
  mySwitch.onPowerState(onPowerState);

  SinricPro.begin(APP_KEY, APP_SECRET);
}

void loop() {
  SinricPro.handle();
}
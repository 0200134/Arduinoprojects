const int LIGHT_PIN = 2;
const int FAN_PIN = 4;
const int TEMP_SENSOR_PIN = 34;

void setup() {
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  float temperature = analogRead(TEMP_SENSOR_PIN) * (3.3 / 4095.0) * 100.0;
  Serial.println(temperature);

  if (temperature > 30.0) {
    controlFan(true);
  } else {
    controlFan(false);
  }

  delay(1000);
}

void controlLight(bool state) {
  digitalWrite(LIGHT_PIN, state ? HIGH : LOW);
}

void controlFan(bool state) {
  digitalWrite(FAN_PIN, state ? HIGH : LOW);
}

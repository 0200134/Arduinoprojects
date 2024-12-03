#include <DHT.h>           // Library for temperature and humidity sensor
#include <Servo.h>         // Library for servo motor control
#include <Wire.h>          // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD display

#define DHTPIN 2          // Pin where DHT sensor is connected
#define DHTTYPE DHT22     // DHT 22 (AM2302)
#define LIGHT_SENSOR A0   // Pin for light sensor
#define RELAY_PIN 8       // Pin for relay module
#define SERVO_PIN 9       // Pin for servo motor

DHT dht(DHTPIN, DHTTYPE);
Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);
  dht.begin();
  myServo.attach(SERVO_PIN);
  lcd.begin();
  lcd.backlight();
  
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
}

void loop() {
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

  // Light control based on sensor data
  if (lightLevel < 300) { // If it's dark
    digitalWrite(RELAY_PIN, HIGH); // Turn on light
  } else {
    digitalWrite(RELAY_PIN, LOW); // Turn off light
  }
  
  // Temperature control using servo
  if (t > 25) { // If temperature is too high
    myServo.write(90); // Adjust servo to cool down
  } else {
    myServo.write(0); // Adjust servo to normal position
  }
  
  delay(2000); // Delay between each loop
}
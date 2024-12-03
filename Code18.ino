#include <DHT.h>
#include <LiquidCrystal.h>

// Define the pins for the DHT sensor, LCD, PIR sensor, LED, and photoresistor
#define DHTPIN 2
#define DHTTYPE DHT11
#define PIR_PIN 3
#define LED_PIN 13
#define PHOTORESISTOR_PIN A0

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
int pirState = LOW; // Start with no motion detected
int val = 0;        // Variable to store PIR status
int lightLevel = 0; // Variable to store light sensor value

void setup() {
  lcd.begin(16, 2);
  dht.begin();
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  val = digitalRead(PIR_PIN); // Read PIR sensor
  lightLevel = analogRead(PHOTORESISTOR_PIN); // Read light sensor

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print("C");
  
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(hum);
  lcd.print("%");

  // Control the LED based on light level
  if (lightLevel < 300) { // Adjust the threshold value as needed
    digitalWrite(LED_PIN, HIGH); // Turn on the LED if it's dark
  } else {
    digitalWrite(LED_PIN, LOW);  // Turn off the LED if it's bright
  }

  // Display motion detection status on Serial Monitor
  if (val == HIGH) { // Check if the PIR sensor is HIGH
    if (pirState == LOW) {
      Serial.println("Motion detected!");
      pirState = HIGH;
    }
  } else {
    if (pirState == HIGH) {
      Serial.println("Motion ended!");
      pirState = LOW;
    }
  }

  delay(2000);
}
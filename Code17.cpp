#include <DHT.h>
#include <LiquidCrystal.h>

// Define the pins for the DHT sensor and LCD
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Define the pin for the PIR motion sensor
const int PIR_PIN = 3;
int pirState = LOW; // Start with no motion detected
int val = 0;        // Variable to store the PIR status

void setup() {
  lcd.begin(16, 2);
  dht.begin();
  pinMode(PIR_PIN, INPUT);
  Serial.begin(9600);
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  val = digitalRead(PIR_PIN); // Read PIR sensor

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print("C");
  
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(hum);
  lcd.print("%");

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

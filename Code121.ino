#include <SoftwareSerial.h>

#define PIR_PIN 2
SoftwareSerial gsm(7, 8);  // RX, TX

void setup() {
  pinMode(PIR_PIN, INPUT);
  gsm.begin(9600);
  Serial.begin(9600);
  delay(1000);
  gsm.println("AT");
  delay(1000);
  gsm.println("AT+CMGF=1");  // Set SMS mode to text
  delay(1000);
}

void loop() {
  int pirState = digitalRead(PIR_PIN);
  if (pirState == HIGH) {
    Serial.println("Motion detected!");
    sendSMS("Motion detected at your home!");
    delay(10000);  // Wait 10 seconds before sending another SMS
  }
  delay(500);
}

void sendSMS(String message) {
  gsm.println("AT+CMGS=\"+1234567890\"");  // Replace with your phone number
  delay(1000);
  gsm.println(message);
  delay(1000);
  gsm.println((char)26);  // ASCII code for CTRL+Z
  delay(1000);
  Serial.println("SMS sent");
}
#include <IRremote.h>
#include <Servo.h>

// Pin definitions
const int RECV_PIN = 2;
const int LED_PIN = 13;
const int SERVO_PIN = 9;

// Create IR receiver and servo objects
IRrecv irrecv(RECV_PIN);
decode_results results;
Servo myServo;

void setup() {
  Serial.begin(9600);
  irrecv.enableIRIn();  // Start the IR receiver
  pinMode(LED_PIN, OUTPUT);
  myServo.attach(SERVO_PIN);
}

void loop() {
  if (irrecv.decode(&results)) {
    // Print the received value to the Serial Monitor
    Serial.println(results.value, HEX);

    // Handle the received code
    switch (results.value) {
      case 0xFF629D:  // Replace with your remote's code for "forward"
        digitalWrite(LED_PIN, HIGH);  // Turn LED on
        break;
      case 0xFFA857:  // Replace with your remote's code for "reverse"
        digitalWrite(LED_PIN, LOW);   // Turn LED off
        break;
      case 0xFF22DD:  // Replace with your remote's code for "left"
        myServo.write(0);             // Turn servo to 0 degrees
        break;
      case 0xFFC23D:  // Replace with your remote's code for "right"
        myServo.write(180);           // Turn servo to 180 degrees
        break;
    }
    irrecv.resume();  // Receive the next value
  }
}
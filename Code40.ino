#include <DHT.h>

// Define the DHT sensor pin and type
#define DHTPIN 2
#define DHTTYPE DHT11

// Initialize the DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Define the LED pin
#define LEDPIN 13

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(LEDPIN, OUTPUT);
}

void loop() {
  // Read temperature and humidity
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Print the readings to the Serial Monitor
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");

  // Control the LED based on the temperature reading
  if (temperature > 25) {
    digitalWrite(LEDPIN, HIGH); // Turn on the LED
  } else {
    digitalWrite(LEDPIN, LOW);  // Turn off the LED
  }

  // Wait a few seconds between measurements.
  delay(2000);
}
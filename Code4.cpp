#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include "DHT.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET    -1  // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Define pins
#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  if (!bmp.begin()) {
    Serial.print("Could not find a valid BMP085 sensor, check wiring!");
    while (1);
  }

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();
}

void loop() {
  // Read humidity and temperature from DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // Read pressure from BMP180
  sensors_event_t event;
  bmp.getEvent(&event);

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Clear the display
  display.clearDisplay();
  
  // Display temperature
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" *C");
  
  // Display humidity
  display.setCursor(0, 10);
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");
  
  // Display pressure and altitude
  if (event.pressure) {
    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    float altitude = bmp.pressureToAltitude(seaLevelPressure, event.pressure, temperature);
    
    display.setCursor(0, 20);
    display.print("Pressure: ");
    display.print(event.pressure);
    display.println(" hPa");
    
    display.setCursor(0, 30);
    display.print("Altitude: ");
    display.print(altitude);
    display.println(" m");
  } else {
    display.setCursor(0, 20);
    display.println("Pressure read failed!");
  }
  
  display.display();
  delay(2000); // Wait a few seconds between measurements
}

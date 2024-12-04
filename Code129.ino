#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(9600);
  dht.begin();

  if (!bmp.begin()) {
    Serial.print("Could not find a valid BMP180 sensor, check wiring!");
    while (1);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  display.clearDisplay();
  
  // Read Temperature and Humidity
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // Read Pressure
  sensors_event_t event;
  bmp.getEvent(&event);
  float pressure = event.pressure;

  // Display on OLED
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");

  display.setCursor(0, 10);
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");

  display.setCursor(0, 20);
  display.print("Pressure: ");
  display.print(pressure);
  display.println(" hPa");

  display.display();
  
  // Wait 2 seconds before next reading
  delay(2000);
}
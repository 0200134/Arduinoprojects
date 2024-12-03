#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include "DHT.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SD.h>
#include <ESP8266WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

const int chipSelect = 4; // SD card module select pin
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

WiFiServer server(80);

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1);
  }
  
  if(!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  server.begin();
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  sensors_event_t event;
  bmp.getEvent(&event);

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  String dataString = "";
  if (event.pressure) {
    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    float altitude = bmp.pressureToAltitude(seaLevelPressure, event.pressure, temperature);
    
    dataString += "Temperature: " + String(temperature) + " *C, ";
    dataString += "Humidity: " + String(humidity) + " %, ";
    dataString += "Pressure: " + String(event.pressure) + " hPa, ";
    dataString += "Altitude: " + String(altitude) + " m\n";
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Temp: "); display.print(temperature); display.println(" *C");
    display.print("Humidity: "); display.print(humidity); display.println(" %");
    display.print("Pressure: "); display.print(event.pressure); display.println(" hPa");
    display.print("Altitude: "); display.print(altitude); display.println(" m");
    display.display();

    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
    } else {
      Serial.println("Error opening datalog.txt");
    }
  } else {
    Serial.println("Failed to read from BMP sensor!");
  }

  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String request = client.readStringUntil('\r');
        Serial.println(request);
        client.flush();
        
        client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
        client.print("<!DOCTYPE HTML><html>");
        client.print("<head><meta http-equiv='refresh' content='5'></head>");
        client.print("<body>");
        client.print("<h1>Weather Station Data</h1>");
        client.print("<p>");
        client.print(dataString);
        client.print("</p>");
        client.print("</body></html>");
        client.stop();
      }
    }
  }
  delay(2000);
}
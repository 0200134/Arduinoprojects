#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <SD.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_BME280 bme;

RTC_DS3231 rtc;

const int chipSelect = 4;

const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

// IFTTT setup
const char* serverName = "http://maker.ifttt.com/trigger/event_name/with/key/your_IFTTT_key";

WiFiServer server(80);

// Machine Learning Model Predict Function Placeholder
float predictTemperature(float temperature, float humidity, float pressure) {
  // Placeholder for the actual model
  return temperature + 0.5; // Dummy prediction logic
}

float predictHumidity(float temperature, float humidity, float pressure) {
  // Placeholder for the actual model
  return humidity + 1; // Dummy prediction logic
}

void setup() {
  Serial.begin(9600);
  
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
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
  DateTime now = rtc.now();

  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  if (isnan(temperature) || isnan(humidity) || isnan(pressure)) {
    Serial.println("Failed to read from BME280 sensor!");
    return;
  }

  float predictedTemp = predictTemperature(temperature, humidity, pressure);
  float predictedHum = predictHumidity(temperature, humidity, pressure);

  String dataString = now.timestamp(DateTime::TIMESTAMP_FULL) + ", ";
  dataString += "Temp: " + String(temperature) + " *C, ";
  dataString += "Humidity: " + String(humidity) + " %, ";
  dataString += "Pressure: " + String(pressure) + " hPa, ";
  dataString += "Predicted Temp: " + String(predictedTemp) + " *C, ";
  dataString += "Predicted Hum: " + String(predictedHum) + " %\n";

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Time: "); display.println(now.timestamp(DateTime::TIMESTAMP_TIME));
  display.print("Temp: "); display.print(temperature); display.println(" *C");
  display.print("Humidity: "); display.print(humidity); display.println(" %");
  display.print("Pressure: "); display.print(pressure); display.println(" hPa");
  display.print("Pred Temp: "); display.print(predictedTemp); display.println(" *C");
  display.print("Pred Hum: "); display.print(predictedHum); display.println(" %");
  display.display();

  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  } else {
    Serial.println("Error opening datalog.txt");
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");
    
    String jsonPayload = "{\"value1\":\"" + String(temperature) + "\", \"value2\":\"" + String(humidity) + "\", \"value3\":\"" + String(pressure) + "\"}";
    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();
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
        client.print("<h1>Super Advanced Weather Station Data</h1>");
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
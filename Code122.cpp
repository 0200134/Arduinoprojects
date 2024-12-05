#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ThingSpeak.h>

#define DHTPIN D4
#define DHTTYPE DHT22

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
unsigned long myChannelNumber = YOUR_CHANNEL_NUMBER;
const char* myWriteAPIKey = "YOUR_API_KEY";

DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;

void setup() {
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  ThingSpeak.begin(client);
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  ThingSpeak.setField(1, t);
  ThingSpeak.setField(2, h);
  int responseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (responseCode == 200) {
    Serial.println("Data successfully sent to Thingspeak");
  } else {
    Serial.println("Failed to send data to Thingspeak");
  }
  
  delay(300000);  // Update every 5 minutes
}

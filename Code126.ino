#include <SD.h>
#include <Wire.h>
#include <Adafruit_VC0706.h>

#define CAMERA_RX 2
#define CAMERA_TX 3
#define PIR_PIN 4
#define SD_CS_PIN 10

File imgFile;
Adafruit_VC0706 camera(&Serial1);

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  
  Serial1.begin(38400);
  if (!camera.begin()) {
    Serial.println("Camera not found!");
    return;
  }
  
  camera.setImageSize(VC0706_640x480);
  Serial.println("Camera ready");
}

void loop() {
  if (digitalRead(PIR_PIN) == HIGH) {
    Serial.println("Motion detected!");
    captureImage();
    delay(10000);
  }
}

void captureImage() {
  if (!camera.takePicture()) {
    Serial.println("Failed to capture image!");
    return;
  }
  
  uint16_t imgSize = camera.frameLength();
  char filename[15];
  sprintf(filename, "img%03d.jpg", millis() % 1000);
  
  imgFile = SD.open(filename, FILE_WRITE);
  if (!imgFile) {
    Serial.println("Failed to open file!");
    return;
  }
  
  Serial.print("Saving image: ");
  Serial.println(filename);

  while (imgSize > 0) {
    uint8_t *buffer;
    uint8_t bytesToRead = min(32, imgSize);
    buffer = camera.readPicture(bytesToRead);
    imgFile.write(buffer, bytesToRead);
    imgSize -= bytesToRead;
  }
  
  imgFile.close();
  Serial.println("Image saved");
}
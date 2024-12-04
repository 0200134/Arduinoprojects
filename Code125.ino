#include <Adafruit_Fingerprint.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <Servo.h>

#define fingerprintRX 2
#define fingerprintTX 3
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);
Servo myServo;

const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; // Connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4, 3, 2}; // Connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  Serial.begin(9600);
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor detected!");
  } else {
    Serial.println("Fingerprint sensor not found :(");
    while (1);
  }
  
  myServo.attach(10);
  lcd.begin(16, 2);
  lcd.print("Enter Password:");
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    lcd.setCursor(0, 1);
    lcd.print(key);
    if (key == 'A') {
      lcd.clear();
      lcd.print("Scan Finger:");
      delay(2000);
      if (getFingerprintID() == 1) {
        lcd.clear();
        lcd.print("Access Granted");
        myServo.write(90);  // Unlock
        delay(5000);
        myServo.write(0);  // Lock
        lcd.clear();
        lcd.print("Enter Password:");
      } else {
        lcd.clear();
        lcd.print("Access Denied");
        delay(2000);
        lcd.clear();
        lcd.print("Enter Password:");
      }
    }
  }
}

int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/IsaqueNunes/espeletronicjavelin/tree/de9ab756793de1cff8f8121e1736814ac11e5068/Requests.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "1")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/goeckenils/esp32WithPost/tree/66d4113aa7ab40bd18b3c49c0045e5eb51711312/src%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "2")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/trendy77/CromwellTSMARTPad/tree/c6f34fd0ffe2c569d40b31ab03f236b11ef1365f/sketches%2Fdhtbasic%2F.build%2Fdhtbasic%2Fsketch%2Fdhtbasic.ino.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "3")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/tanakornHope/aircondition-controller/tree/e2a427c41cf6d8f61c13bfce06368f4c35492f64/src%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "4")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/sdsxpln/IoT-57/tree/5515c6d7297289bbffa582ad12051f90e64f2cd9/CoPrime_RedStrip.c?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "5")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/IshwarAnjana/CC3200_Project/tree/e79c3306b80cb23e935975b59618afabe8416e25/App_CC3200%2FCode.c?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "6")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/ildoddo/aurora/tree/985735bd812c6d5d366c3ffb8dcaae6552f3498c/src%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "7")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/aleiei/ESP32-CAM-RTSP/tree/3a34d008cdca43d6cf22f46c8919ce7ac632ee21/src%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "8")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/KillerSpartan/Snake/tree/f64ff7bc592109f4f67155b42f5a6d3aa26a1b86/Snake%20Definitive%20Edition%2Fsrc%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "9")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/nmhlog/ProyeksSistemTertanam/tree/2bcd4861f9139c3c83b6ca7121d04081a001593e/src%2FmainEsp32v2.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "10")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/spademarck2020/schooltool/tree/534d08d6e15a75c42afd67a1fd1ed012243a35a8/esp32%2FA103esp32.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "11")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/Queegmire/thermoprobe/tree/4a3f5604a4f9d5e5b21aaa207213633bb4c90cfc/examples%2Fmqtt%2Fsrc%2Fmain.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "12")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/kibbek/Arduino_Lampa/tree/01d9d4d7874326b2b3cb887a8c8ea4a5ab164573/ESP8266WiFi.c?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "13")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/ajurasz/ajurasz.github.io/tree/d121ae8916559cdda220827085f6a7ffc33b86ba/src%2Fpages%2F2016-12-15-homemade-central-heating-temperature-monitoring%2Findex.md?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "14")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/MShinskiy/university-course-work/tree/deec7c914b8d0517d84bfa8aa8774a4baf785f0f/CE323%2FAssignment1%2FTask2.c?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "15")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/pkm774/kernel_asus_sdm660_4.19/tree/dbbbbb7628c96a159998f668a67866748f491c41/drivers%2Fisdn%2FmISDN%2Fdsp_dtmf.c?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "16")[43dcd9a7-70db-4a1f-b0ae-981daa162054](https://github.com/w3ichen/Old-Work/tree/bd17f55761aae12c74d0777271d8b4a6ee23de06/Arduino-alarm.cpp?citationMarker=43dcd9a7-70db-4a1f-b0ae-981daa162054 "17")
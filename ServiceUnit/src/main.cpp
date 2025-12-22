#include <Arduino.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoWebsockets.h>

#define BAUD_RATE 9600
#define buttonInput 23

#define workingLED 21

#define SD_MOSI     23
#define SD_MISO     19
#define SD_SCLK     18
#define SD_CS       5

#define txPin 26
#define rxPin 27

#define ClockPin 34 

const String ssid = "ESP32-Access-Point";
const String password = "123456789";
websockets::WebsocketsClient ws;

unsigned long previousMillis = 0;
unsigned long interval = 30000;

void initWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  bool connected = ws.connect("ws://192.168.4.1/ws");

  if (connected) {
    Serial.println("WebSocket connected");
  } else {
    Serial.println("WebSocket connection failed");
  }

  // Message handler
  ws.onMessage([](websockets::WebsocketsMessage message) {
    Serial.print("Received: ");
    Serial.println(message.data());

    if (message.data() == "WORKING_ON") {
      digitalWrite(workingLED, HIGH);
    }
  });

  ws.onEvent([](websockets::WebsocketsEvent event, String data) {
    if (event == websockets::WebsocketsEvent::ConnectionClosed) {
      Serial.println("WebSocket disconnected");
    }
  });
}

//Set the pin modes
void setPinModes() {
  pinMode(txPin, OUTPUT);
  pinMode(rxPin, INPUT);
  
  pinMode(workingLED, OUTPUT);
  pinMode(ClockPin, INPUT);
}

//initialize SD card for use
void initSD() {

  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card MOUNT FAIL");
  } else {
    Serial.println("SD Card MOUNT SUCCESS");
    Serial.println("");

    uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    String str = "SDCard Size: " + String(cardSize) + "MB";
    Serial.println(str);
    
    uint8_t cardType = SD.cardType();
    if(cardType == CARD_NONE) {
      Serial.println("No SD card attached");
    }

    //print card type
    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if(cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }
  }
}

//End SD
void endSD() {
  SD.end();
  SPI.end();
}

//Write file to SD
bool writeToFile(String filepath, String content, bool append) {
  File file;
  if (append == true) {
    file = SD.open(filepath, FILE_APPEND);
  } else {
    file = SD.open(filepath, FILE_WRITE);
  }
  
  if (!file) {
    return false;
  }

  file.print(content);

  file.close();

  return true;
}

File getFileForRead(String filepath) {
  File file = SD.open(filepath, FILE_READ);

  return file;
}


void setup() {
  Serial.begin(BAUD_RATE);
  
  setPinModes();
  
  digitalWrite(BUILTIN_LED, LOW);
  
  //initialize SD setup 
  initSD();
  //initialize wifi connection
  initWiFi();
  
  digitalWrite(BUILTIN_LED, HIGH);
 
  Serial.println("Setup complete");
}

void loop() {
  // put your main code here, to run repeatedly:
  ws.poll();

}

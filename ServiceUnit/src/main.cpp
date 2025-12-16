#include <Arduino.h>
#include <WiFi.h>

#define BAUD_RATE 9600
#define buttonInput 23

#define onoffLED 19
#define workingLED 21

#define txPin 26
#define rxPin 27

#define ClockPin 34 

const String ssid = "ESP32-Access-Point";
const String password = "123456789";

unsigned long previousMillis = 0;
unsigned long interval = 30000;

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void setPinModes() {
  pinMode(txPin, OUTPUT);
  pinMode(rxPin, INPUT);
  
  pinMode(onoffLED, OUTPUT);
  pinMode(workingLED, OUTPUT);
  pinMode(ClockPin, INPUT);
}

void setup() {
  Serial.begin(BAUD_RATE);
  
  setPinModes();
  
  digitalWrite(onoffLED, HIGH);
  
  digitalWrite(workingLED, LOW);
  
  initWiFi();
  digitalWrite(workingLED, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
}

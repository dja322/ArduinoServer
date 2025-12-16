#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "..\.pio\libdeps\esp32doit-devkit-v1\EspSoftwareSerial\src\SoftwareSerial.h"

#define BAUD_RATE 9600
#define buttonInput 23

#define onoffLED 19
#define workingLED 21

#define txPin 26
#define rxPin 27

#define ClockPin 34

// HTML content
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Web Page</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h1>Hello from ESP32</h1>
  <p>This page is hosted directly on the ESP32.</p>
  <button onclick="sendCommand()">Click Me</button>
</body>


<script>
function sendCommand() {
  fetch('/button')
    .then(response => response.text())
    .then(data => alert(data));
}
</script>
</html>
)rawliteral";

//Set up Proxy serial
//SoftwareSerial CCUSerial =  SoftwareSerial(proxy_rxPin, proxy_wxPin);

SoftwareSerial CCUSerial;

String ssid = "ESP32-Access-Point";
String password = "123456789";
String ExternalWiFissid = "";
String ExternalWiFiPassword = "";

// Set web server port number to 80
WebServer server(80);

// Variable to store the HTTP request
String header;

void setPinModes() {
  pinMode(buttonInput, INPUT_PULLUP);

  pinMode(txPin, OUTPUT);
  pinMode(rxPin, INPUT);

  pinMode(onoffLED, OUTPUT);
  pinMode(workingLED, OUTPUT);

  pinMode(ClockPin, INPUT);
}

bool sendSerialMessage(SoftwareSerial* serial, String buffer) {

  uint8_t size = buffer.length();
  uint8_t currentByte = 0;
  uint8_t bytesSent = 0;

  serial->write(size);

  while(currentByte < size) {
    bytesSent = serial->write(buffer[currentByte]);
    if (bytesSent > 0) {
      currentByte++;
    }
  }
  return true;
}

void handleDataRequest() {
  if (server.hasArg("value")) {
    String val = server.arg("value");
    server.send(200, "text/plain", "Received: " + val);
  } else {
    server.send(400, "text/plain", "Missing value");
  }
}

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

bool working = false;

void handleButton() {
  Serial.println("Button pressed!");
  server.send(200, "text/plain", "Button received");
  working = true;
}

IPAddress IP;

void initWifi() {
  WiFi.mode(WIFI_AP_STA);
  IP = WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  WiFi.begin(ExternalWiFissid, ExternalWiFiPassword);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());

  // Define HTTP api route
  server.on("/data", HTTP_GET, handleDataRequest);
  // Route for root page
  server.on("/", handleRoot);
  //handle button response
  server.on("/button", handleButton);
  server.begin();
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(BAUD_RATE);
  CCUSerial.begin(BAUD_RATE, EspSoftwareSerial::SWSERIAL_8N1, rxPin, txPin);

  setPinModes();

  digitalWrite(onoffLED, HIGH);

  initWifi();

  Serial.println(WiFi.softAPIP()); // Primary access point
  Serial.println(WiFi.softAPSubnetMask());
  Serial.println(WiFi.softAPBroadcastIP());
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(WiFi.gatewayIP());

  delay(1000); //give time for CCU to load up
  sendSerialMessage(&CCUSerial, WiFi.softAPIP().toString());

}

void loop() {
 
  if (digitalRead(buttonInput) == LOW || working == true) {
    digitalWrite(workingLED, HIGH);
  } else {

    digitalWrite(workingLED, LOW);
  }

  server.handleClient();


  

  //delay(1000); //measured in miliseconds
}

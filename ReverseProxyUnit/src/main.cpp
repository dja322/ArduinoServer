#include <Arduino.h>
#include <WiFi.h>

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#include "..\.pio\libdeps\esp32doit-devkit-v1\EspSoftwareSerial\src\SoftwareSerial.h"

#define BAUD_RATE 9600
#define buttonInput 23

#define onoffLED 19
#define workingLED 21

#define txPin 26
#define rxPin 27

#define ClockPin 34

#define MAX_CLIENTS 10

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

//Set up Software serial for CentralControlunit
SoftwareSerial CCUSerial;
uint32_t clients[MAX_CLIENTS];
int numClients = 0;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String ssid = "ESP32-Access-Point";
String password = "123456789";

bool working = false;

IPAddress IP;

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

void onWsEvent(AsyncWebSocket *server,
               AsyncWebSocketClient *client,
               AwsEventType type,
               void *arg,
               uint8_t *data,
               size_t len) {

  if (type == WS_EVT_CONNECT) {
    Serial.printf("WS client %u connected\n", client->id());
    clients[numClients] = client->id();
    numClients++;
  }

  if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WS client %u disconnected\n", client->id());
  }

  if (type == WS_EVT_DATA) {
    String msg = "";
    for (size_t i = 0; i < len; i++) {
      msg += (char)data[i];
    }
    Serial.println("WS received: " + msg);
  }
}

void initWifi() {
  WiFi.mode(WIFI_AP);
  IP = WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(IP);

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Route for root page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });
  //handle button response

  server.on("/button", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Button pressed from browser");
    working = true;

    // PUSH to all ESP32 clients
    ws.textAll("WORKING_ON");

    request->send(200, "text/plain", "Button received");
  });

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


  //delay(1000); //measured in miliseconds
}

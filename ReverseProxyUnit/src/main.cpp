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

  <!-- Existing button -->
  <button onclick="sendCommand()">Click Me</button>

  <hr>

  <!-- Text input -->
  <h3>Send Text to ESP32</h3>
  <input type="text" id="textInput" placeholder="File name">
  <button onclick="sendText()">Send Text</button>

  <hr>

  <!-- File upload -->
  <h3>Send File to ESP32</h3>
  <input type="file" id="fileInput">
  <button onclick="sendFile()">Upload File</button>

</body>

<script>
/* Existing command */
function sendCommand() {
  fetch('/button')
    .then(response => response.text())
    .then(data => alert(data));
}

/* Send text as JSON */
function sendText() {
  const text = document.getElementById('textInput').value;
  // send plain text body instead of JSON
  fetch('/sendfilename', {
    method: 'POST',
    headers: {
      'Content-Type': 'text/plain'
    },
    body: text
  })
  .then(response => response.text())
  .then(data => alert(data));
}

/* Send file using multipart/form-data */
function sendFile() {
  const fileInput = document.getElementById('fileInput');
  const file = fileInput.files[0];

  if (!file) {
    alert('Please select a file first');
    return;
  }

  const formData = new FormData();
  formData.append('file', file);

  fetch('/uploadfile', {
    method: 'POST',
    body: formData
  })
  .then(response => response.text())
  .then(data => alert(data));
}
</script>
</html>
)rawliteral";

//Set up Software serial for CentralControlunit
SoftwareSerial CCUSerial;
uint32_t clients[MAX_CLIENTS];
bool newClient = false;
int numClients = 0;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String ssid = "ESP32-Access-Point";
String password = "123456789";

bool working = false;
bool readingFile = false;

IPAddress IP;

String ws_data = "";

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

void requestFile(uint32_t clientID, String filename) {
  ws.text(clientID, "F_REQUEST_FILE");
  ws.text(clientID, filename);
  ws.text(clientID, "F_END");
}

void sendFile(uint32_t clientID, String filename, String data) {
  const size_t CHUNK_SIZE = 512;
  
  ws.text(clientID, "F_SEND_FILE");
  ws.text(clientID, filename);
  for (size_t i = 0; i < data.length(); i += CHUNK_SIZE) {
    ws.text(clientID, data.substring(i, i + CHUNK_SIZE));
    delay(5); // Show wifi
  }

  ws.text(clientID, "F_END");
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
    String str_data = "ID:" + client->id();
    sendSerialMessage(&CCUSerial, str_data);
  }

  if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WS client %u disconnected\n", client->id());
  }

  if (type == WS_EVT_DATA) {
    String msg = "";
    for (size_t i = 0; i < len; i++) {
      msg += (char)data[i];
    }

    if (msg == "F_DELIVERY") {
      readingFile = true;
    } else if (msg == "F_END") {
      readingFile = false;
    } else if (readingFile) {
      ws_data += msg;
    }
    Serial.println("WS received: " + ws_data);
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

    working = false;
  });

   // Handle POST where the client sends plain text in the body.
  server.on("/sendfilename", HTTP_POST,
    [](AsyncWebServerRequest *request){
      // response will be sent from the body callback when full body received
    },
    nullptr,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      static String body = ""; // simple buffer for one request at a time
      if (index == 0) body = ""; // start of body

      body.concat((const char*)data, len);

      if (index + len == total) { // full body received
        Serial.printf("Received body: %s\n", body.c_str());
        if (numClients > 0) {
          requestFile(clients[0], body);
        }
        request->send(200, "text/plain", String("Requested: ") + body);
        readingFile = true;
      }

      //wait for atleast 3 seconds for data to be received
      unsigned long start = millis();
      while (ws_data.length() == 0 && millis() - start < 3000) {
        delay(10);
       }

      request->send(200, "text/plain", ws_data);
      ws_data = "";
    }
  );

  static String uploadBuffer = "";
  server.on("/uploadfile", HTTP_POST, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "Upload received");
    },
  [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if (index == 0) {
      uploadBuffer = ""; // start of new upload
      Serial.printf("Start upload: %s\n", filename.c_str());
    }

    // Append received chunk to buffer
    uploadBuffer.concat((const char*)data, len);

    if (final) {
      Serial.printf("Upload finished: %s, size=%u\n", filename.c_str(), uploadBuffer.length());
      // Send the collected file contents as a string to the websocket client
      if (numClients > 0) {
        sendFile(clients[0], filename, uploadBuffer);
      }
      uploadBuffer = ""; // clear buffer
    }
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
  String data = "IP:" + WiFi.softAPIP().toString();
  sendSerialMessage(&CCUSerial, data);

}

void loop() {
 
  if (digitalRead(buttonInput) == LOW || working == true) {
    digitalWrite(workingLED, HIGH);
  } else {
    digitalWrite(workingLED, LOW);
  }
}

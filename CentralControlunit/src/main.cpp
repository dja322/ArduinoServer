#include <Arduino.h>
#include <SoftwareSerial.h>
#include <time.h>
#include "..\lib\LiquidCrystal\src\LiquidCrystal.h"


#define proxy_rxPin 5 //proxy read pin
#define proxy_wxPin 6 //proxy write pin
#define service_rxPin 3 //Service unit 1 read pin
#define service_wxPin 4 //Service unit 2 write pin
#define service2_rxPin 2 //Service unit 1 read pin
#define service2_wxPin A0 //Service unit 2 write pin

#define X_Pin A3 // > 800 going down, < 200 going up
#define Y_Pin A4 // > 800 going right, < 200 going left
#define Joystick_Switch_Pin A5 //HIGH is unpressed, LOW is pressed

#define BAUD_RATE 9600

//Set up Proxy serial
SoftwareSerial ProxySerial =  SoftwareSerial(proxy_rxPin, proxy_wxPin);

//Set up Service unit 1 serial
SoftwareSerial Service1Serial =  SoftwareSerial(service_rxPin, service_wxPin);

//Set up Service unit 2 serial
SoftwareSerial Service2Serial =  SoftwareSerial(service2_rxPin, service2_wxPin);

//LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
LiquidCrystal lcd(A1, A2, 10, 9, 8, 7);

int UserCursorLocation[2] = {0,0};

bool joystickAllowInput = true;

String readMessage(SoftwareSerial* serial) {
  uint8_t size = int(serial->read());
  uint8_t bytesReceived = 0;
  String message = "";
  char in_char = '\0';

  while(bytesReceived < size) {
    if (ProxySerial.available() > 0) {
      in_char = serial->read();

      if (in_char < 32 || in_char > 122) {
        message += '\0';
        break;
      }

      message += in_char;
      if (message[bytesReceived] == '\0') {
        break;
      }
      bytesReceived++;
    }
  }
  return message;
}

void setPinModes() {
  pinMode(proxy_rxPin, INPUT);
  pinMode(proxy_wxPin, OUTPUT);
  pinMode(service_rxPin, INPUT);
  pinMode(service_wxPin, OUTPUT);
  pinMode(service2_rxPin, INPUT);
  pinMode(service2_wxPin, OUTPUT);
  pinMode(Joystick_Switch_Pin, INPUT);
}

void setup() {

  // Set the baud rate for the Serial port
  Serial.begin(BAUD_RATE);
  
  //set pin modes
  setPinModes();
  digitalWrite(Joystick_Switch_Pin, HIGH);

  // Set the baud rate for the Software serials
  ProxySerial.begin(BAUD_RATE);
  Service1Serial.begin(BAUD_RATE);
  Service2Serial.begin(BAUD_RATE);

  lcd.begin(16, 2);
  lcd.blink();

  ProxySerial.listen();
}

String message = "";
void loop() {
  // put your main code here, to run repeatedly

  lcd.setCursor(UserCursorLocation[0], UserCursorLocation[1]);

  if (joystickAllowInput) {
    if (digitalRead(Joystick_Switch_Pin) == LOW) {

      joystickAllowInput = false;
    } else if (analogRead(X_Pin) > 800) { //down
      UserCursorLocation[1] -= 1;

      if (UserCursorLocation[1] < 0) {
        UserCursorLocation[1] = 0;
      }

      joystickAllowInput = false;
      
    } else if (analogRead(X_Pin) < 200) { //up
      UserCursorLocation[1] += 1;

      if (UserCursorLocation[1] > 1) {
        UserCursorLocation[1] = 1;
      }

      joystickAllowInput = false;
    }
    else if (analogRead(Y_Pin) < 200) { //left
      UserCursorLocation[0] -= 1;

      if (UserCursorLocation[0] < 0) {
        UserCursorLocation[0] = 0;
      }

      joystickAllowInput = false;
      
    } else if (analogRead(Y_Pin) > 800) { //right
      UserCursorLocation[0] += 1;

      if (UserCursorLocation[0] > 15) {
        UserCursorLocation[0] = 15;
      }

      joystickAllowInput = false;
    }
  } else if (analogRead(Y_Pin) < 800 && analogRead(Y_Pin) > 200 &&
             analogRead(X_Pin) < 800 && analogRead(X_Pin) > 200 &&
             digitalRead(Joystick_Switch_Pin) == HIGH) {
    joystickAllowInput = true;
  }


  if (ProxySerial.available() > 0) {
    // Also echo to USB serial for debugging
    message = readMessage(&ProxySerial);
    Serial.print("Received from ESP: ");
    Serial.println(message);

    if (message != "") {
      lcd.setCursor(0,0);
    }
    lcd.print(message);
  }

  
}

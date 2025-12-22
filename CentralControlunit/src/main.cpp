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
#define MENU_SIZE 10 //Size of menu

#define LCD_HEIGHT 2
#define LCD_WIDTH 16

#define STRING_EQUAL 0

#define BAUD_RATE 9600

//Set up Proxy serial
SoftwareSerial ProxySerial =  SoftwareSerial(proxy_rxPin, proxy_wxPin);

//Set up Service unit 1 serial
SoftwareSerial Service1Serial =  SoftwareSerial(service_rxPin, service_wxPin);

//Set up Service unit 2 serial
SoftwareSerial Service2Serial =  SoftwareSerial(service2_rxPin, service2_wxPin);

//LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
LiquidCrystal lcd(A1, A2, 10, 9, 8, 7);

//Set users position for cursor
int UserCursorLocation[2] = {0,0};

//Ensure joystick has distinct input
bool joystickAllowInput = true;

//Menu Char
String menu[MENU_SIZE] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
unsigned char menuPosition = 0;
unsigned char connectedDevices = 0;

//Read a serial message
String readMessage(SoftwareSerial* serial) {
  uint8_t size = int(serial->read());
  uint8_t bytesReceived = 0;
  String message = "";
  char in_char = '\0';

  while(bytesReceived < size) {
    if (serial->available() > 0) {
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

//set pin modes
void setPinModes() {
  pinMode(proxy_rxPin, INPUT);
  pinMode(proxy_wxPin, OUTPUT);
  pinMode(service_rxPin, INPUT);
  pinMode(service_wxPin, OUTPUT);
  pinMode(service2_rxPin, INPUT);
  pinMode(service2_wxPin, OUTPUT);
  pinMode(Joystick_Switch_Pin, INPUT);
}


/*
  Process joystick inputs, debounce inputs and updates user cursor locations
  Returns bool based on if joystick was pushed
*/
bool processJoyStick() {
  lcd.setCursor(UserCursorLocation[0], UserCursorLocation[1]);
  bool buttonPushed = false;

  if (joystickAllowInput) {
    if (digitalRead(Joystick_Switch_Pin) == LOW) {

      joystickAllowInput = false;
      buttonPushed = true;

    } else if (analogRead(X_Pin) > 800) { //down
      UserCursorLocation[1] -= 1;

      if (UserCursorLocation[1] < 0) {
        UserCursorLocation[1] = 0;

        if (menuPosition > 0) {
          menuPosition--;
        }
      }
      joystickAllowInput = false;
    } else if (analogRead(X_Pin) < 200) { //up
      UserCursorLocation[1] += 1;
      if (UserCursorLocation[1] > LCD_HEIGHT - 1) {
        UserCursorLocation[1] = LCD_HEIGHT - 1;

        if (menuPosition < MENU_SIZE - LCD_HEIGHT) {
          menuPosition++;
        }
      }

      joystickAllowInput = false;
    } else if (analogRead(Y_Pin) < 200) { //left
      UserCursorLocation[0] -= 1;

      if (UserCursorLocation[0] < 0) {
        UserCursorLocation[0] = 0;
      }

      joystickAllowInput = false;
    } else if (analogRead(Y_Pin) > 800) { //right
      UserCursorLocation[0] += 1;

      if (UserCursorLocation[0] > LCD_WIDTH - 1) {
        UserCursorLocation[0] = LCD_WIDTH - 1;
      }

      joystickAllowInput = false;
    }
  } else if (analogRead(Y_Pin) < 800 && analogRead(Y_Pin) > 200 &&
             analogRead(X_Pin) < 800 && analogRead(X_Pin) > 200 &&
             digitalRead(Joystick_Switch_Pin) == HIGH) {
    joystickAllowInput = true;
  }
  return buttonPushed;
}

//Reads a serial message then processes its content, return true if valid message
bool processSerialMessage() {

  // Also echo to USB serial for debugging
  String message = readMessage(&ProxySerial);
  Serial.print("Received from ESP: ");
  Serial.println(message);

  if (strncmp(message.c_str(), "IP:", strlen("IP:")) == STRING_EQUAL) {
    if (message != "") {
      menu[0] = message; 
    }
  } else if (strncmp(message.c_str(), "ID:", strlen("ID:")) == STRING_EQUAL) {
    if (message != "") {
      menu[2 + connectedDevices] = message; 
      if (connectedDevices + 1 < MENU_SIZE) {
        connectedDevices++;
      }
      menu[1] = "Devices Con:" + connectedDevices;
    }
  } else {
    return false;
  }

  return true;
}

/*
  Processes menu action, if action successful return true else false
*/
bool processMenuClick() {
  return true;
}

/*
  Update LCD to have proper menu contents
*/
void updateMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(menu[menuPosition]);
  lcd.setCursor(0,1);
  lcd.print(menu[menuPosition+1]);
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

  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  lcd.blink();

  ProxySerial.listen();
}

void loop() {
  // put your main code here, to run repeatedly

  if (processJoyStick()) {
    processMenuClick();
  }

  if (joystickAllowInput == false) {
    updateMenu();
  }

  if (ProxySerial.available() > 0) {
    processSerialMessage();
    updateMenu();
  }
  
}

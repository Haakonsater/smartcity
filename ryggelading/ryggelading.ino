#include <Zumo32U4.h>
//#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "mqtt.h"
#include <EEPROM.h>  //for å lagre og skrive data som ikke slettes ved shut down må man skrive til eeprom hvert byte kan bare holde verdier fra 0-255
#include "mqtt.h"
#include <PubSubClient.h>


#define RXD2 22
#define TXD2 26

int leftmotor;
int rightmotor;

Zumo32U4Motors motors;
Zumo32U4ButtonC buttonC;

//#define BUTTON_PIN 13;

//Pushbutton buttonC(BUTTON_PIN);


const int buttonPin = 27;
int ledPin = 19;
unsigned long previousMillis;
int buttonState = 0;
int singlePress = 0;
int batteryLevel = 15;
const long interval = 1000;
int ledState = LOW;  // ledState used to set the LED

/*
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
*/
void setup() {
  /*
  setup_wifi();
  client.setServer("172.20.10.3", 1883);
  client.setCallback(callback);
  */

  Serial.begin(9600);
  buttonC.waitForButton();
}

unsigned long currentMillis = millis();
void loop() {
  /*
  //Blinke del
  if (batteryLevel <= 20) {
    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;

      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }

      // set the LED with the ledState of the variable:
      digitalWrite(ledPin, ledState);
    }
  }
  delay(10);
  */


  // Dobble click del
  click();
}

void click() {
  if (!buttonC.isPressed() == HIGH && buttonState == 0) {
    unsigned long currentMillis = millis();

    //two presses within 500ms
    if (currentMillis - previousMillis < 500) {
      singlePress = 0;
      //Do double press action
      Serial.println("Nødlading");

      // Kjører i revers for nødlading i 2 sec lader opp 25% av batteriet
      int speed = -100;
      motors.setLeftSpeed(speed);
      motors.setRightSpeed(speed);
      batteryLevel = batteryLevel + 25;
      delay(2);
      /*
      char batterylvl_str[8];
      dtostrf(batteryLevel, 1, 2, batterylvl_str);
      client.publish("batteryLevel", batteryLevel);
      */


    } else {
      singlePress = 1;
      delay(10);
    }

    previousMillis = currentMillis;
    buttonState = 1;
  } else if (!buttonC.isPressed()) {
    buttonState = 0;
  }

  //check if 500ms have passed with no second button press
  if (singlePress == 1 && millis() - previousMillis > 500) {
    singlePress = 0;
    //DO single press stuff
    Serial.println("Single press");
    delay(10);
  }
}
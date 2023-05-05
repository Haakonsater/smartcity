#include <WiFi.h>          // Library for wifi
#include <PubSubClient.h>  // Library for mqtt
#include "mqtt.h"          // Include header file
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

Servo myservo;

#define SS_PIN 5                  /*Slave Select Pin*/
#define RST_PIN 0                 /*Reset Pin for RC522*/
#define LED_G 12                  /*Pin 8 for LED*/
MFRC522 mfrc522(SS_PIN, RST_PIN); /*Create MFRC522 initialized*/

// koster 200 for å komme seg gjennom bummen
int bum_pris = 200;
int currency = 5000;
//pins used for trafficlight
const int button = 13;
const int Y_led = 26;
const int R_led = 33;
const int G_led = 32;
//sends a value to the car to stop
int charge = 0;
unsigned long pestmils = 0;
const int streetLightPin = 27;
unsigned long prevtime = 0;

enum trafikkLys { green,
                  yellow,
                  red,
                  red_yellow,
                  red_yellow_green,
                  yellow_green };  //different states for the trafficlight
trafikkLys tilstand = green;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

void setup() {
  pinMode(streetLightPin, OUTPUT);
  pinMode(button, INPUT);
  pinMode(Y_led, OUTPUT);
  pinMode(R_led, OUTPUT);
  pinMode(G_led, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  Serial.setTimeout(10);
  SPI.begin(); /*SPI communication initialized*/
  myservo.attach(14);

  mfrc522.PCD_Init();     /*RFID sensor initialized*/
  pinMode(LED_G, OUTPUT); /*LED Pin set as output*/
  Serial.println("Put your card to the reader...");
  Serial.println();
  //connecting the client to ip-adress on the RPI and which port we want to talk through
  client.setServer("192.168.137.85", 1883);
  //connecting the callback function to the client to be further used in client.loop
  client.setCallback(callback);
  myservo.write(0);
}




void loop() {
  if (!client.connected()) {  //if mqtt is not connected try again
    reconnect();
  }
  client.loop();  //holds connection and checks if new information is recived
  trafikk_lys();
  RFIDREAD();
}
void RFIDREAD() {
  /*Look for the RFID Card*/
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  /*Select Card*/
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  char Charge[8];
  charge = 1;
  sprintf(Charge, "%d", charge);
  client.publish("charging", Charge);

  /*Show UID for Card/Tag on serial monitor*/
  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  if ((content.substring(1) == "21 B3 1F 26")&&(millis()-pestmils>1000)) /*UID for the Card/Tag we want to give access Replace with your card UID*/
  {
   
    
    Serial.println("Authorized access"); /*Print message if UID match with the database*/
    Serial.println();

    currency = currency - bum_pris;

    char str_curr[8];
    dtostrf(currency, 1, 2, str_curr);
    client.publish("currency", str_curr);


    myservo.write(180);
    charge = 1;
    sprintf(Charge, "%d", charge);
    client.publish("charging", Charge);
    Serial.println("chargeinffnfnfn");
    delay(4000);

      digitalWrite(LED_G, LOW);
      
      myservo.write(0);
      charge = 0;
      sprintf(Charge, "%d", charge);
      client.publish("charging", Charge);
      Serial.println(currency);
    pestmils=millis();
    
  } else {
    Serial.println(" Access denied"); /*If UID do not match print message*/
  }
}


void trafikk_lys() {
  switch (tilstand) {
    case green:
      
      digitalWrite(G_led, HIGH);
      
      if ((digitalRead(button) == HIGH) && (millis() - prevtime > 10000)) {
        client.publish("trafficLight", "green");

        if (millis() - prevtime > 14000) {
          tilstand = yellow;
          prevtime = millis();
        }
      }
      break;

    case yellow:
      if (millis() - prevtime > 5000) {
        client.publish("trafficLight", "yellow");

        digitalWrite(Y_led, HIGH);

        if (millis() - prevtime >= 8000) {
          digitalWrite(G_led, LOW);
          tilstand = red;
          client.publish("trafficLight", "red");
          prevtime = millis();
        }
      }
      break;

    case red:
      if (millis() - prevtime > 1000) {
        digitalWrite(R_led, HIGH);
      }
      if (millis() - prevtime > 3000) {
        digitalWrite(Y_led, LOW);
      }
      if (millis() - prevtime > 8000) {
        prevtime = millis();
        tilstand = red_yellow_green;
      }
      break;

    case red_yellow_green:
      if (millis() - prevtime > 1000) {
        digitalWrite(Y_led, HIGH);
      }
      if (millis() - prevtime > 4000) {
        client.publish("trafficLight", "yellow");

        digitalWrite(R_led, LOW);
      }
      if (millis() - prevtime > 5000) {
        tilstand = yellow_green;
        prevtime = millis();
      }
      break;

    case yellow_green:
      if (millis() - prevtime > 2000) {
        digitalWrite(G_led, HIGH);
      }
      if (millis() - prevtime > 4000) {
        digitalWrite(Y_led, LOW);
      }
      if (millis() - prevtime > 6000) {
        tilstand = green;

        client.publish("trafficLight", "green");

        prevtime = millis();
      }
      break;
  }
}

void reconnect() {

  while (!client.connected()) {  //while we're not connected
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESPsmart")) {  // if we're connected
      Serial.println("connected");
      // Subscribe to topics
      client.subscribe("trafficLight");
      client.subscribe("streetLyte");
      client.subscribe("currency");
    } else {  //if failed try again in 5 seconds
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  messageTemp;
  Serial.println("");

  //deciding which button is pushed
  if (String(topic) == "full charge") {

  } else if (String(topic) == "batteryLevel") {
  } else if (String(topic) == "batteryHealth") {
  } else if (String(topic) == "esp32/batteryCycle") {
  } else if (String(topic) == "charge percent") {
  } else if (String(topic) == "currency") {
    int currency = messageTemp.toInt();
  } else if (String(topic) == "streetLyte") {

    if (String(messageTemp) == "ON") {
      digitalWrite(streetLightPin, HIGH);
      Serial.println("streetlight on");
    } else {
      Serial.println("streetlight off");
      digitalWrite(streetLightPin, LOW);
    }
  }
}
//Kilde: https://www.jameco.com/Jameco/workshop/TechTip/temperature-measurement-ntc-thermistors.html (13.03.23)

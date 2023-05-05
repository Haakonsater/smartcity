#include <Wire.h>
#include <Zumo32U4.h>
#include <ArduinoJson.h>
Zumo32U4Buzzer buzzer;
Zumo32U4Encoders encoders;
Zumo32U4ButtonC buttonC;
Zumo32U4Motors motors;
Zumo32U4LineSensors lineSensors;
Zumo32U4IMU imu;
// Inkluderer kode fra mappene
#include "TurnSensor.h"
#include "GridMovement.h"
#include "Addresses.h"

String trafficLight;

int battery_stage = 0;
//variabler som skal til nodered
unsigned long serialMillis = 0;
unsigned long prevtime;
unsigned long jsonmillis = 0;
int leftCount = 0;
int rightCount = 0;
int counts = 0;
int battery_service = 0;


String lastStartPoint;
String lastEndPoint;


void setup() {

  Serial1.begin(9600);
  Serial.begin(9600);

  gridMovementSetup();

  buttonC.waitForButton();
  

  delay(1000);

}

void loop() {

  taxidriver();
  lowbattery();
  sendData();
  readData();
  setLocations();
 
  
}

// Når kunde bestiller adresse, lagres variablene til disse.
void setLocations(){
  if (startPoint.length() > 5){
    currentStartPoint = startPoint;
  }
  if (endPoint.length() > 7){
    currentEndPoint = endPoint;
  }
}

///////Kode til SW batteri og kommunikasjon til ESP32/////////

void readData() {
  if (Serial1.available()) {
    // Read the incoming data from the serial port if any bytes are available
    String jsonString = Serial1.readStringUntil('\n');
    if (jsonString.length() == 0) {  //if length of string is 0-error

      Serial.println("JSON parsing error:empty in");
      return;
    }
    // Parse the JSON string into a DynamicJsonDocument object
    DynamicJsonDocument jsonDoc(100);
    DeserializationError error = deserializeJson(jsonDoc, jsonString);  // deserialize the Json string
    charge = jsonDoc["charge"].as<int>();                               //convert to int
    battery_service = jsonDoc["battery_stage"].as<int>();
    startPoint = jsonDoc["startPoint"].as<String>();  // convert to string and save in variable
    endPoint = jsonDoc["endPoint"].as<String>();
    trafficLight = jsonDoc["trafficLight"].as<String>();
    Serial.println(trafficLight);
    Serial.println(endPoint);
  }
}

void sendData() {
  if (millis() - jsonmillis > 1000) {  // if 1 second has passed since last iteration
    DynamicJsonDocument jsonDoc(100);  // Create a new DynamicJsonDocument object
    // Add the leftcount and rightcount values to the JSON object
    jsonDoc["leftcount"] = encoders.getCountsAndResetLeft();
    jsonDoc["rightcount"] = encoders.getCountsAndResetRight();
    jsonDoc["taxometer"] = taxometer;
    // Serialize the JSON object to a string
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    // Send the JSON string over UART
    Serial1.println(jsonString);
    jsonmillis = millis();  // resets counter
  }
}
void lowbattery() {
  if (battery_service < 1) {
    ledYellow(0);          //led are off
    ledRed(0);             //led are off
    buzzer.stopPlaying();  // stop playing any previous sound
  } else if ((battery_service == 1) && (millis() - prevtime > 5000)) {
    needCharging = true;
    buzzer.playFrequency(1000, 1000, 10);  // play first tone
    delay(200);
    ledYellow(1);  // turn on yellow LED
    prevtime = millis();
  }
  if ((battery_service == 2) && (millis() - prevtime > 5000)) {
    needCharging = true;
    motors.setSpeeds(0, 0);
    ledYellow(0);                        //turns of yellow led
    buzzer.playFrequency(1000, 200, 8);  // play first tone
    delay(300);
    buzzer.playFrequency(1000, 200, 8);  // play second tone
    ledRed(1);                           // turn on red LED
    prevtime = millis();
    delay(300);
  }
}

//////////////////////

// En funksjon som forteller retninger ved Blindvei

void roadDeadEnd() {
  goStraight();
  deadEnd();
  goRight();
  stateFollowLine = 1;
  goStraight();
}
/*
 * Dette er hovedfunksjonen for adressene til hele banen
 * Hver case er en adresse der nye funksjoner gis til roboten.
 * Programmet vil alltid starte ved adressen Start der den
 * står stille fram til den får ny adresse fra kunde. Den
 * bytter deretter til Midjostasjonen der den starter. 
 * Hver addresse som bruker kan slutte reisen sin på har en 
 * if-setning som sender funksjonen til case FinnNyeKunder 
 * der den venter fram til nye kunder bestiller eller 
 * om lading trens, da sendes den til ladestasjonen. 
 * Hver adresse har også ulike funksjoner som forteller 
 * hvilke retninger den skal ta ved neste vinkelrette teipbit, 
 * og den går så videre til neste funksjon eller case.
 */


void taxidriver(){

  switch (address){
    case Start:
      motors.setSpeeds(0,0);
      
      delay(1000);
      //sendes null når ingen adresse valgt
      if (currentStartPoint == "null"){
        motors.setSpeeds(0,0);
      } else if (currentEndPoint.length()>5){        
        buzzer.playFrequency(1000, 200, 12);
        delay(1000);
        
        address = Midjostasjonen;
        lastStartPoint = currentStartPoint;
        lastEndPoint = currentEndPoint;        

        
      }
      break;

    case FinnNyeKunder:
      
      if (needCharging == true){
        currentEndPoint = "InngangLadestasjon";
      }
      
      else if (currentStartPoint != lastStartPoint && currentEndPoint != lastEndPoint){
        buzzer.playFrequency(1600, 200, 12);
        delay(3000);
        buzzer.playFrequency(1200, 200, 12);
        address = lastAddress;
        lastEndPoint = currentEndPoint;
        lastStartPoint = currentStartPoint;

        currentMillis = millis(); 
        while(millis() < currentMillis + period1){   
        }
       
      }
      else{
        
        motors.setSpeeds(0,0);
        break;
      }
      
      address = lastAddress;
      
      break;

    case Midjostasjonen:

      if (currentStartPoint == "Midjostasjonen"){
        currentMillis = millis();

        while (millis() < currentMillis + period1) {
        }

        //currentEndPoint = endPoint;
      } else if(currentEndPoint == "Midjostasjonen"){
        
        delay(1000);
        lastAddress = address;
        address = FinnNyeKunder;
        break;
      }

      else if (currentEndPoint == "InngangLadestasjon"){
        speedLimit = 200;
        goRight();
        
        address = InngangLadestasjon;

        break;
      }
      speedLimit = 200;
      
      goStraight();
  
      address = InngangLadestasjon;

      break;      

    case InngangLadestasjon:

      stateFollowLine = 0;
      
      if (currentEndPoint == "InngangLadestasjon"){
        goToCharging();  
        
        break;
        
      } else{
        speedLimit = 300;
        
        goStraight();
        address = Blindvei;
        break;
      }

    case Blindvei:

      if (currentStartPoint == "Blindvei"){
        currentMillis = millis();

        while (millis() < currentMillis + period1) {
        }

        
      } else if(currentEndPoint == "Blindvei"){
        
        lastAddress = address;
        address = FinnNyeKunder;

        
        break;
      }
      speedLimit = 200;
      roadDeadEnd();

      address = nidarosdomen;

      break;

    case nidarosdomen:

      if (currentStartPoint == "nidarosdomen"){
        currentMillis = millis();

        while (millis() < currentMillis + period1) {
        }

        
      } else if(currentEndPoint == "nidarosdomen"){
        
        lastAddress = address;
        address = FinnNyeKunder;

        
        break;
      }
      for(int i = 0; i < 4; i++){
        goStraight();
      }
      address = Fartsgrense400;
      break;

    case Fartsgrense400:

      speedLimit = 400;

      goStraight();
      address = Boligfelt;
      break;
      
    case Boligfelt:
      speedLimit = 200;
      
      stateFollowLine = 0;
      goLeft();

      //Bilen må ta et valg på hvilken retning den skal ta
      if (currentEndPoint == "Midjoveien1" || currentStartPoint == "Midjoveien1") {
        address = Midjoveien1;
      } else if (currentEndPoint == "Midjoveien2" || currentStartPoint == "Midjoveien2") {
        address = Midjoveien2;
      } else if (currentEndPoint == "Midjoveien3" || currentStartPoint == "Midjoveien3") {
        address = Midjoveien3;
      } else if (currentEndPoint == "Midjoveien4" || currentStartPoint == "Midjoveien4") {
        address = Midjoveien4;
      } else if (currentEndPoint == "Midjoveien5" || currentStartPoint == "Midjoveien5") {
        address = Midjoveien5;
      } else if (currentEndPoint == "Midjoveien6" || currentStartPoint == "Midjoveien6") {
        address = Midjoveien6;
      } else if (currentEndPoint == "Midjoveien7" || currentStartPoint == "Midjoveien7") {
        address = Midjoveien7;
      } else if (currentEndPoint == "Midjoveien8" || currentStartPoint == "Midjoveien8") {
        address = Midjoveien8;
      } else {
        address = Snarvei;
      }

      stateFollowLine = 1;
      break;

    case Midjoveien1:
      midjoVeien1();
      
      lastAddress = address;
      address = FinnNyeKunder;

      currentMillis = millis();

      while (millis() < currentMillis + period1) {
      }

      if (stateHouse1 == 0){
        
        address = Svingen;

      }

      break;
    case Midjoveien2:
      midjoVeien2();
      
      lastAddress = address;
      address = FinnNyeKunder;

      currentMillis = millis();

      while (millis() < currentMillis + period1) {
      }

      if (stateHouse2 == 0){
        
        address = Svingen;

      }

      break;
    case Midjoveien3:
      midjoVeien3();
      
      lastAddress = address;
      address = FinnNyeKunder;

      currentMillis = millis();

      while (millis() < currentMillis + period1) {
      }

      if (stateHouse3 == 0){
        
        address = Svingen;

      }

      break;
      
    case Midjoveien4:
      midjoVeien4();
      
      lastAddress = address;
      address = FinnNyeKunder;

      currentMillis = millis();

      while (millis() < currentMillis + period1) {
      }

      if (stateHouse4 == 0){
        
        address = Svingen;

      }

      break;
    case Midjoveien5:
     midjoVeien5();
      
      lastAddress = address;
      address = FinnNyeKunder;

      currentMillis = millis();

      while (millis() < currentMillis + period1) {
      }

      if (stateHouse5 == 0){
        
        address = Svingen;

      }

      break;
    case Midjoveien6:
      midjoVeien6();
      
      lastAddress = address;
      address = FinnNyeKunder;

      currentMillis = millis();

      while (millis() < currentMillis + period1) {
      }

      if (stateHouse6 == 0){
        
        address = Svingen;

      }

      break;
    case Midjoveien7:
      midjoVeien7();
      
      lastAddress = address;
      address = FinnNyeKunder;

      currentMillis = millis();

      while (millis() < currentMillis + period1) {
      }

      if (stateHouse7 == 0){
        
        address = Svingen;

      }

      break;
    case Midjoveien8:
      midjoVeien8();
      
      lastAddress = address;
      address = FinnNyeKunder;

      currentMillis = millis();

      while (millis() < currentMillis + period1) {
      }

      if (stateHouse8 == 0){
        
        address = Svingen;

      }

      break;
    case Snarvei:
      goRight();

      stateFollowLine = 0;
      
      goRight();    
      goLeft();

      stateFollowLine = 1;
      
      goStraight();
      address = Bom;
      break;

    case Svingen:
    
      stateFollowLine = 1; // Setter opp vanskelighetsgraden på linjefølgingen
      goStraight();
      address = Lyskryss;
      break;
      
    case Lyskryss:

      if (trafficLight == "yellow" || trafficLight == "red"){
        motors.setSpeeds(0,0);
        
        break;
      }

      else {
        //Her kjører bilen på et stykke uten teip. Setter en viss fart
        //fram til den detekterer en ny linje
        while (!aboveLine(0) || !aboveLine(4)) {
          motors.setSpeeds(200, 200);
          readSensors();
        }
        buzzer.playFrequency(1600, 200, 12);
        currentMillis = millis();
        while (millis() < currentMillis + intersectionDelay) {
        }
        address = SluttLyskryss;
        break;
      }

      case SluttLyskryss:

        speedLimit = 200;
        stateFollowLine = 0;
        goStraight();
        
        goLeft();
        stateFollowLine = 1;
        goStraight();
        address = Bom;
      break;

    case Bom:

      motors.setSpeeds(0,0);
      // variabelen charge settes til 1 når bommen er åpnet
      if (charge == 0){
        break;
      }

      else if (charge == 1){
        while (millis() < currentMillis + period1) {
        }
        speedLimit = 300;
        goStraight();
        address = Gloshaugen;
        
      }
      break;

    case Gloshaugen:

      if (currentStartPoint == "Gloshaugen"){
        currentMillis = millis();

        while (millis() < currentMillis + period1) {
        }

        //currentEndPoint = endPoint;
      } else if(currentEndPoint == "Gloshaugen"){
        
        lastAddress = address;
        address = FinnNyeKunder;

        break;
      }

      stateFollowLine = 0;
      goStraight();
      address = UtgangLadestasjon;
      break;

    case UtgangLadestasjon:
      speedLimit = 300;
      goStraight();
      address = Midjostasjonen;
      break;
  }
}

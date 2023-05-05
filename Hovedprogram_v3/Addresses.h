/* Her ligger koden for alle adressene i boligfeltet 
 * i tillegg til koden som kjører bilen til lading
 * de fleste kodene er ganske like, det som skiller
 * dem er hvilke retinger de skal ta i ulike kryss.
 * Kommenterer dermed bare for den ene boligen
 */

#pragma once
#include <Wire.h>

// En oversikt over alle adresser på banen der
// det skjer en endring i banen eller fartsgrense
enum Address {
  Start,
  FinnNyeKunder,
  Midjostasjonen,
  InngangLadestasjon,
  Blindvei,
  nidarosdomen,
  Fartsgrense400,
  Boligfelt,
  Midjoveien1,
  Midjoveien2,
  Midjoveien3,
  Midjoveien4,
  Midjoveien5,
  Midjoveien6,
  Midjoveien7,
  Midjoveien8,
  Snarvei,
  Svingen,
  Lyskryss,
  SluttLyskryss,
  Bom,
  Gloshaugen,
  UtgangLadestasjon,
};
Address address = Svingen;
Address lastAddress = address;

int stateHouse1 = 0;
int stateHouse2 = 0;
int stateHouse3 = 0;
int stateHouse4 = 0;
int stateHouse5 = 0;
int stateHouse6 = 0;
int stateHouse7 = 0;
int stateHouse8 = 0;
int stateCharging = 0;

int period1 = 3000;
int period2 = 8000;
int charge = 0;
int taxometer;

String startPoint = "";
String currentStartPoint = startPoint;
String endPoint = "";
String currentEndPoint = endPoint;

bool needCharging = false;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;

// Tar bilen til den 1. boligen
void midjoVeien1() {
  switch (stateHouse1) {
    case 0:
      //Finner veien til bolig 1 ved å ta ulike retninger ved kryssene
      goStraight();

      // Endrer vanskelighetsgradden slik at densvinger når bare én
      // av sidesensorene registrerer linje
      stateFollowLine = 0;  
                            
      goLeft();
      stateFollowLine = 1;
      goRight();
      foundEnd();  // Fant boligen og stopper opp her
      taxometer = 0;
      
      stateHouse1 = 1;
      break;
    case 1:
      //Kjører ut fra boligfelt
      deadEnd();  // Snur bilen 180 grader for å kjøre videre
      goRight();
      goStraight();
      stateFollowLine = 0;
      goRight();
      
      goStraight();
      address = Svingen;
      stateHouse1 = 0;
      break;
  }
}
void midjoVeien2() {
  switch (stateHouse2) {
    case 0:
      //Finner veien til bolig 2
      goStraight();
      stateFollowLine = 0;
      goLeft();
      stateFollowLine = 1;
      goLeft();
      foundEnd();
      
      taxometer = 0;
      
      stateHouse2 = 1;
      break;
    case 1:
      //Kjører ut fra boligfelt
      deadEnd();
      goLeft();
      goStraight();
      stateFollowLine = 0;
      goRight();
      
      goStraight();
      address = Svingen;
      stateHouse2 = 0;
      break;
  }
}
void midjoVeien3() {
  switch (stateHouse3) {
    case 0:
      //Finner veien til bolig 3
      goStraight();
      stateFollowLine = 0;
      goLeft();
      stateFollowLine = 0;
      goStraight();
      goRight();
      foundEnd();
      stateHouse3 = 1;
      break;
    case 1:
      //Kjører ut fra boligfelt
      deadEnd();
      goRight();
      stateFollowLine = 0;
      goRight();
      
      goStraight();
      address = Svingen;
      stateHouse3 = 0;
      break;
  }
}

void midjoVeien4() {
  switch (stateHouse4) {
    case 0:
      //Finner veien til bolig 4
      goStraight();
      stateFollowLine = 0;
      goLeft();
      stateFollowLine = 1;
      goStraight();
      goLeft();
      foundEnd();
      
      taxometer = 0;
      stateHouse4 = 1;
      break;
    case 1:
      //Kjører ut fra boligfelt
      deadEnd();
      goLeft();
      stateFollowLine = 0;
      goRight();
      
      goStraight();
      address = Svingen;
      stateHouse4 = 0;
      break;
  }
}

void midjoVeien5() {
  switch (stateHouse5) {
    case 0:
      //Finner veien til bolig 1
      goStraight();
      stateFollowLine = 0;
      goStraight();
      goLeft();
      stateFollowLine = 1;
      goRight();
      foundEnd();
      
      taxometer = 0;
      
      stateHouse5 = 1;
      break;
    case 1:
      //Kjører ut fra boligfelt
      deadEnd();
      goRight();
      goStraight();
      goRight();
      address = Svingen;
      stateHouse5 = 0;
      break;
  }
}
void midjoVeien6() {
  switch (stateHouse6) {
    case 0:
      //Finner veien til bolig 6
      goStraight();
      stateFollowLine = 0;
      goStraight();
      goLeft();
      stateFollowLine = 1;
      goLeft();
      foundEnd();
      
      taxometer = 0;
      
      stateHouse6 = 1;
      break;
    case 1:
      deadEnd();
      goLeft();
      goStraight();
      goRight();
      address = Svingen;
      stateHouse6 = 2;
      break;
  }
}
void midjoVeien7() {
  switch (stateHouse7) {
    case 0:
      //Finner veien til bolig 7
      goStraight();
      stateFollowLine = 0;
      goStraight();
      goLeft();
      stateFollowLine = 1;
      goStraight();
      goRight();
      foundEnd();
      
      taxometer = 0;
      
      stateHouse7 = 1;
      break;
    case 1:
      deadEnd();
      goRight();
      goRight();
      address = Svingen;
      stateHouse7 = 2;
      break;
  }
}
void midjoVeien8() {
  switch (stateHouse8) {
    case 0:
      //Finner veien til bolig 8
      goStraight();
      stateFollowLine = 0;
      goStraight();
      goLeft();
      stateFollowLine = 1;
      goStraight();
      goLeft();
      foundEnd();
      
      taxometer = 0;
      
      stateHouse8 = 1;
      break;
    case 1:
      deadEnd();
      goLeft();
      goRight();
      address = Svingen;
      stateHouse8 = 2;
      break;
  }
}
void goToCharging() {
  switch (stateCharging) {
    case 0:
      //Kjører til ladestasjon fra
      
      goRight();
      goLeft();
      foundEnd();
      stateCharging = 1;
      break;
    case 1:
      //Variabel charge settes til 2 når lading er fullført,
      //kjører deretter videre
      if (charge == 2) {
        deadEnd();
        stateCharging = 2;
      } else{
        motors.setSpeeds(0,0);
      }
      break;
    case 2:
      //Kjører ut på banen igjen og til Midjostasjonen
      needCharging = false;
      goLeft();
      goRight();
      goRight();
      currentEndPoint = "Midjostasjonen";
      address = UtgangLadestasjon;
      stateCharging = 0;
      break;
  }
}

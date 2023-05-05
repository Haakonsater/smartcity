
#include <WiFi.h>          // Library for wifi
#include <PubSubClient.h>  // Library for mqtt
#include "mqtt.h"          // Include header file

#define BUTTON_PIN_BITMASK 0x200000000
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5           /* Time ESP32 will go to sleep (in seconds) */

//termistor
const int thermPin = 34;
//pin for lightsensor
const int photoResistorPin = 35;
//pin for street lights
const int streetLightPin = 27;
enum StreetLight { OFF,
                   ON };
//variable for street light state
StreetLight streetLight = OFF;
//dimness when street lights are turned on/off
int criticallight = 3000;
//timer for street lights
int streetLightTimer = millis();

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];


RTC_DATA_ATTR int bootCount = 0;  //counts every time new wakeup
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

long previousMillis = 0;
long termMillis = 0;

void setup() {
  Serial.begin(115200);
  //gatelys
  pinMode(photoResistorPin, INPUT);
  pinMode(streetLightPin, OUTPUT);
  setup_wifi();
  //connecting the client to ip-adress on the RPI and which port we want to talk through
  client.setServer("192.168.137.85", 1883);
  //connecting the callback function to the client to be further used in client.loop
  client.setCallback(callback);
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();
  //
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}

void temp() {

  int thermValue = analogRead(thermPin);

  // Konverter analog verdi til temperatur
  float thermVoltage = thermValue * (3.3 / 4095);              // Finner spenningen. 3.3V er ESP32s referansespenning, 4095 er maks ADC-verdi
  float thermResistance = ((3.3 / thermVoltage) - 1) * 10000;  // Beregner motstanden til termistoren. 10000 er motstand til termistoren
  float steinhart = thermResistance / 10000;

  //Bruker Steinhart-hart-formelen til å regne temperaturen i celcius
  steinhart = log(steinhart);
  steinhart /= 3950;                 // B-verdi for termistoren fra datablad (3950)
  steinhart += 1.0 / (25 + 273.15);  // 25 er referansetemperaturen i Celsius
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;  // Konverter til Celsius

  if (millis() - termMillis > 1000) {
    // Serial.print("Temperatur: ");
    //Serial.print(steinhart);
    //Serial.println(" C");
    termMillis = millis();  //restets time counter
    //creates an array to hold information
    char temperaTure[8];
    dtostrf(steinhart, 1, 2, temperaTure);      //function that turns float to string
    client.publish("temprature", temperaTure);  //sends the string through mqtt
  }
}


void loop() {
  if (!client.connected()) {  //if mqtt is not connected try again
    reconnect();
  }
  client.loop();  //holds connection and checks if new information is recived
  temp();
  street_light();


  Serial.println("going to sleep");
  delay(1000);
  esp_deep_sleep_start();
}
void street_light() {
  //gatelys
  //updating streetlights
  if (millis() - streetLightTimer > 1000) {
    //resetting street light timer
    streetLightTimer = millis();
    //reading the lightlevel
    int dimness = analogRead(photoResistorPin);
    //Serial.println(dimness);
    char StreetLIGHT[8];
    sprintf(StreetLIGHT, "%d", dimness);
    client.publish("streetLight", StreetLIGHT);

    switch (streetLight) {

      //if the street light are off: check if they should be turned on
      case OFF:
        if (dimness < criticallight) {
          //digitalWrite(streetLightPin, HIGH);
          streetLight = ON;
          client.publish("streetLyte", "ON");
        }
      //if the street lights are on: check if they should be turned off
      case ON:
        if (dimness > criticallight) {
          //digitalWrite(streetLightPin, LOW);
          streetLight = OFF;
          client.publish("streetLyte", "OFF");
        }
    }
  }
}
void reconnect() {

  while (!client.connected()) {  //while we're not connected
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESPsensor")) {  // if we're connected
      Serial.println("connected");

    } else {  //if failed try again in 5 seconds
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void callback(char* topic, byte* message, unsigned int length) {  // function that converts recived information
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);  // what topic was recieved
  Serial.print(". Message: ");
  String messageTemp = "";            //declares a string to hold information
  for (int i = 0; i < length; i++) {  //converts char array to complete string
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
}
//Kilde: https://www.jameco.com/Jameco/workshop/TechTip/temperature-measurement-ntc-thermistors.html (13.03.23)
//https://randomnerdtutorials.com/esp32-timer-wake-up-deep-sleep/
//https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/
//https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#_CPPv431esp_sleep_disable_wakeup_source18esp_sleep_source_t

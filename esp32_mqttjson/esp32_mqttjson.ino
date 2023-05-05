#include <WiFi.h>          // Library for wifi
#include <PubSubClient.h>  // Library for mqtt
#include <Wire.h>          // Library for I2C communication
#include "mqtt.h"          // Include header file
#include <ArduinoJson.h>   // Library for Json-used to convert data to string
#include <Preferences.h>   //for å lagre og skrive data som ikke slettes ved strømbrudd
//pins for serial communication between the zumo and esp: UART

#define RXD2 16
#define TXD2 17
Preferences preferences;
// Critical information about the battery
float battery_health = 1;    //helsetilstanden på batteriet 0-100%. høyere jo bedre
float battery_level = 40;    // ladeprosent for batteriet 0-100%
float battery_cycles = 100;  //antall batteriet har blitt ladet


// Time tracking
unsigned long drainMillis = 0;
unsigned long millis70 = 0;
unsigned long prevTime = 0;
unsigned long serialMillis = 0;
unsigned long millis60 = 0;
unsigned long jsonmillis = 0;
unsigned long taxtime = 0;

// information to be transmitted or recieved
float money = 5000;
float charge = 0;
int leftcount = 0;
int rightcount = 0;
float counts = 0;
float distance_travelled = 0;
int over_70 = 0;
int overM = 0;
int battery_stage = 0;
float current_speed = 0;
// Information recieved from website sent to zumo
String startPoint;
String endPoint;
String trafficLight;
String laststartPoint;
String lastendPoint;

int miljo_points = 1000;

int taxometer = 0;
int meters_to_pay = 0;
uint8_t barrier = 0;
int under_5;
uint8_t lastEepromHealth = 0;
float L_distance = 0;
//constant that is calculated by the wheels circumference to find m/s
const float meter_per_click = 0.026;  // adjust this value depending on your setup



WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];


void setup() {
  //initializes flash-memory with the size of 1 byte (the size we want to access)

  //starting the serial monitor
  Serial.begin(9600);

  //EEPROM.write(addrhp, 100); //used if we want to reset the battery_healt to 100%
  //EEPROM.commit();
  //Initializing/starting the UART-communication between the zumo32U4 and esp-32
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);  // creating a Uart communication between the zumo and esp with wires
  //Connecting to wifi,function defined in mqtt.h
  setup_wifi();
  //connecting the client to ip-adress on the RPI and which port we want to talk through
  client.setServer("192.168.137.85", 1883);
  //connecting the callback function to the client to be further used in client.loop
  client.setCallback(callback);
  //reads off battery_health through memory
  preferences.begin("my-app", false);
  battery_health = preferences.getInt("battery_health", 100);  // Retrieve the battery_health value from preferences
  preferences.end();
}

void battery_update() {  //this function updates battery_health to the flash-memory
  //uses casting to convert float to uint8_t with scaling factor of 255
  int eepromhealth = battery_health;
  if (lastEepromHealth != eepromhealth) {  //only writes if battery_health has changed
    preferences.begin("my-app", false);
    preferences.putInt("battery_health", eepromhealth);
    preferences.end();
    lastEepromHealth = eepromhealth;  //last value is new value
  }
}
void readData() {
  //if any bytes are available on the UART-port(information recieved)
  if (Serial2.available()) {
    // Read the incoming data from the serial port
    String jsonString = Serial2.readStringUntil('\n');
    //if length of string is 0-return
    if (jsonString.length() == 0) {
      Serial.println("JSON parsing error:empty in");
      return;
    }
    // Parse the JSON string into a DynamicJsonDocument object
    DynamicJsonDocument jsonDoc(100);
    DeserializationError error = deserializeJson(jsonDoc, jsonString);
    //three different topics recieved from the zumo, converting them from string toint
    leftcount = jsonDoc["leftcount"].as<int>();  //leftcount= string leftcount holding information
    rightcount = jsonDoc["rightcount"].as<int>();
    taxometer = jsonDoc["taxometer"].as<int>();
  }
}
void taxo_meter() {
  if ((taxometer = 1) && (millis() - taxtime > 100)) {
    meters_to_pay += current_speed;
    taxtime = millis();
  }
  if (taxometer = 0) {
    money += meters_to_pay;
    meters_to_pay = 0;
  }
}


void sendDestination() {
  //creates a new DynamicJsonDoc to hold information about the endpoint and startpoint for the zumo
  DynamicJsonDocument DestinationDoc(200);
  DestinationDoc["startPoint"] = startPoint;
  DestinationDoc["endPoint"] = endPoint;
  String Destination;                          //creates the string that holds endpoint and startpoint
  serializeJson(DestinationDoc, Destination);  //information is formatted for Json
  Serial2.println(Destination);                //prints information transmitted through UART
  Serial.println(Destination);
}
void sendData() {
  //if 1 second has passed
  if (millis() - jsonmillis > 1000) {
    // Create a new DynamicJsonDocument object
    DynamicJsonDocument jsonDoc1(100);
    //topics transmitted through UART
    jsonDoc1["charge"] = charge;
    jsonDoc1["battery_health"] = battery_health;
    jsonDoc1["battery_stage"] = battery_stage;
    jsonDoc1["trafficLight"] = trafficLight;
    String infoString;

    serializeJson(jsonDoc1, infoString);  //Serialize the JSON object to a string

    Serial2.println(infoString);  //Send the JSON string over UART

    jsonmillis = millis();  //resets the counter so 1 second is now 0 second since last iteration
  }
}
void battery_mqtt_update() {  //takes the updated information from battery and currency and publishes them through mqtt

  if ((millis() - serialMillis > 1000) && (charge < 1)) {  // if 1 second has passed and the zumo is not currently charging
    // Battery is getting drained because of usage
    battery_drain();
    serialMillis = millis();  //resets the counter

    char batteryLevel[8];  // creates an character array
    //function that converts floats to strings, parameters(float,min width of string,decimal places,char array)
    dtostrf(battery_level, 1, 2, batteryLevel);
    //publishes the new values over to the topic /battery_level
    client.publish("batteryLevel", batteryLevel);

    //same as above and below
    char batteryHealth[8];
    dtostrf(battery_health, 1, 2, batteryHealth);
    client.publish("batteryHealth", batteryHealth);

    char batteryCycles[8];
    dtostrf(battery_cycles, 1, 2, batteryCycles);
    client.publish("batteryCycles", batteryCycles);

    char account_balance[8];
    dtostrf(money, 1, 2, account_balance);
    client.publish("currency", account_balance);
    Serial.println(account_balance);
  }
}
void getClicks() {  // converts counts on the zumo and converts it cm and cm/s
  readData();       // reads UART data from zumo
  if (millis() - prevTime >= 1000) {
    // average of counts this second
    counts = (leftcount + rightcount) / 2;
    // converts counts to cm this second
    current_speed = counts * meter_per_click;
    // absolute length travelled, using abs to get positive values if the zumo is reversing
    distance_travelled += abs(current_speed);
    prevTime = millis();  //resets counter

    if (current_speed > 1) {  //if speed is valid,
      //updates the new values to node-red through mqtt
      char distanceTravelled[8];
      dtostrf(distance_travelled, 1, 2, distanceTravelled);
      client.publish("distanceTravelled", distanceTravelled);

      char currentSpeed[8];
      dtostrf(current_speed, 1, 2, currentSpeed);
      client.publish("currentSpeed", currentSpeed);
    }
  }
}
void over_70tracker() {                // tracks output over 70% per 60 seconds
  if ((0.7 * 69) < current_speed) {    // if current speed is higher that 70% of max speed
    if (millis() - millis70 > 1000) {  // Checks if it has gone 1 second
      millis70 = millis();             //resets counter
      over_70++;                       //adds 1 second to over_70
    }
  }
  if (millis() - millis60 > 60000) {  // after 60 sec, over_70 is published to node-red
    char over70[8];
    dtostrf(over_70, 1, 0, over70);
    client.publish("over70", over70);
    miljo_points -= over_70;  //removes enviroment points if you are over 70% speed

    char miljoPoints[8];
    dtostrf(miljo_points, 1, 0, miljoPoints);
    client.publish("enviromentPoints", miljoPoints);
    over_70 = 0;          // over_70 is reset for new 60s interval
    millis60 = millis();  // resets counter
  }
}
void under5() {                                // tracks each time 5% barrier is passed
  if ((battery_level < 5) && (barrier = 1)) {  // adds 1 to under_5 each time battery level is under 5
    under_5++;
    barrier = 0;
  } else if (battery_level > 5) {  //barrier stops the counting so it only adds 1 time it is passed
    barrier = 1;                   // battery_health has to go above 5% to add to under_5 again
  }
}

void battery_drain() {  // continously drains battery by usage
  under5();             // updates information
  over_70tracker();

  if ((millis() - drainMillis >= 10000) && (battery_level > 0) && (battery_health > 0) && (current_speed>1) && (current_speed<100)) {  // if 1 second has passed, battery_level and battery_health is above 0

    float i = (distance_travelled - L_distance) / 10;                     //variable defined by distance traveled
    float y = (battery_cycles) / 25 + (under_5) / 10 + (over_70) / 100;  //variable that increases the more the zumo is used

    battery_level = battery_level - (i - i / battery_health) / 40;  //battery level losing energy based on distance and the current state of the battery health

    battery_health =battery_health- (y +i / 20) / 100;                   //battery health losing points faster if it has been long time since battery service
    L_distance = distance_travelled;                       //last distance is the new distance
    drainMillis = millis();                                //resets counter
    if ((random(1, 100) > 99) && (battery_health > 50)) {  // 1% chance and if battery_health is above 50
      battery_health -= 50;                                // battery health loses 50 points
    }
  }
  if (battery_level < 0) {  // realistic check so level cant be negative
    battery_level = 0;
  }
  if (battery_health < 0) {  // realistic check so health cant be negative
    battery_health = 0;
  }
}

void lowBattery() {          // stages where the low battery will effect the zumo
  if (battery_level > 20) {  //if batterylevel is above level 1 nothing happens
    battery_stage = 0;
  }
  if ((battery_level >= 10) && (battery_level < 20)) {
    battery_stage = 1;  //battery level is under level 1 but above level 0, warning stage
  }
  if (battery_level < 10) {  //battery_stage=2; //battery level is under level 0, critically low
    battery_stage = 2;
  }
  if (battery_level < 10) {
    battery_stage = 3;
  }
}


void loop() {

  if (!client.connected()) {  //if mqtt is not connected try again
    reconnect();
  }
  client.loop();  //holds connection and checks if new information is recived
  
  getClicks();
  sendData();
  battery_mqtt_update();
 
  lowBattery();
  taxo_meter();
  battery_update();
  
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
  Serial.println("");
  if ((strcmp(topic, "currency") == 0)) {  //checks if topic is currency
    money = messageTemp.toFloat();         //if topic is currency, convert string to float
  }
  if ((strcmp(topic, "charging") == 0)) {  // same as above, and below
    charge = messageTemp.toFloat();
    
  }
  if ((strcmp(topic, "batteryLevel") == 0)) {
    battery_level = messageTemp.toFloat();
  }
  if ((strcmp(topic, "batteryHealth") == 0)) {
    battery_health = messageTemp.toFloat();
  }
  if ((strcmp(topic, "startPoint") == 0)) {
    startPoint = messageTemp; 
    sendDestination();
       //sends the destination immediately through UART
      laststartPoint = startPoint;
    
  }
  if ((strcmp(topic, "endPoint") == 0)) {
    endPoint = messageTemp;
    sendDestination();
       //sends the destination immediately through UART
      lastendPoint = endPoint;
    
  }
  if ((strcmp(topic, "trafficLight") == 0)) {
    trafficLight=messageTemp;
    
  }
  if ((strcmp(topic, "environmentPoints") == 0)) {
    miljo_points = messageTemp.toInt();
  }
}
void reconnect() {

  while (!client.connected()) {                     //while we're not connected
    Serial.print("Attempting MQTT connection...");  // Attempt to connect

    if (client.connect("ESP32Client")) {  // if we're connected
      Serial.println("connected");
      // Subscribe to topics
      client.subscribe("currency");
      client.subscribe("charging");
      client.subscribe("batteryHealth");
      client.subscribe("batteryLevel");
      client.subscribe("startPoint");
      client.subscribe("endPoint");
      client.subscribe("batteryReplacement");
      client.subscribe("trafficLight");
      client.subscribe("environmentPoints");

    } else {  //if failed try again in 5 seconds
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/  //MQTT
//https://randomnerdtutorials.com/esp32-flash-memory/  //EEPROM
//https://github.com/nosknut/arduino-course-v2023/blob/main/IELET1002/ArduinoHackathon2/ExampleCode/WebServerSerial/WebServerSerial.ino  //json
//https://github.com/nosknut/arduino-course-v2023/blob/main/IELET1002/ArduinoHackathon2/ExampleCode/Router/Router.ino  //json
//https://pastebin.com/0KzRtCf7 //UART
//https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
//https://github.com/nosknut/arduino-course-v2023/blob/main/IELET1002/ArduinoHackathon2/ExampleCode/WebServerSerial/WebServerSerial.ino

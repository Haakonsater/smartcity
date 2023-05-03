#include <WiFi.h>
#include <WebServer.h>
#include "utilities.h"

// SSID & Password
const char *ssid = "iphone";
const char *password = "gruppec29";
char line1[100];

//client for publishing mqtt
const char* mqtt_server = "172.20.10.7";
WiFiClient espClient;
PubSubClient client(espClient);

//server for html
WebServer server(80);  // Object of WebServer(HTTP port, 80 is defult)

void setup() {
  Serial.begin(115200);
  Serial.println("Try Connecting to ");
  Serial.println(ssid);

  // Connect to your wi-fi modem
  WiFi.begin(ssid, password);

  // Check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected successfully");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());  //Show ESP32 IP on serial

  //mqtt
  client.setServer(mqtt_server, 1883);  

  //webserver
  server.on("/", handle_root);
  server.on("/handle-order", handleOrderPath);
  server.begin();
  Serial.println("HTTP server started");
  delay(100);  
}

void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  server.handleClient();
}

// HTML & CSS contents which display on web server
String HTML = "<!DOCTYPE html>\
<html>\
<style>\
h1{font-family: Arial}\
h2{font-family: Arial}\
select{font-family: Arial;width: 60; height: 20}\
</style>\
<body>\
<h1>Midjo taxi AS</h1>\
<div>\
<form action='/handle-order'>\
<div>\
<h2>Velg startpunkt</h2>\
<select name='startPoint' required>\
<option value=\"nidarosdommen\">nidarosdommen</option>\
<option value=\"Midjostasjonen\">Midjostasjonen</option>\
<option value=\"Blindvei\">Blindvei</option>\
<option value=\"Midjobroen\">Midjobroen</option>\
<option value=\"Midjoveien1\">Midjoveien1</option>\
<option value=\"Midjoveien2\">Midjoveien2</option>\
<option value=\"Midjoveien3\">Midjoveien3</option>\
<option value=\"Midjoveien4\">Midjoveien4</option>\
<option value=\"Midjoveien5\">Midjoveien5</option>\
<option value=\"Midjoveien6\">Midjoveien6</option>\
<option value=\"Midjoveien7\">Midjoveien7</option>\
<option value=\"Midjoveien8\">Midjoveien8</option>\
<option value=\"Midjoveien8\">Gloshaugen</option>\
</select>\
</div>\
<div>\
<h2>Velg sluttpunkt</h2>\
<select name='endPoint' required>\
<option value=\"ladestasjon\">Ladestasjon</option>\
<option value=\"bom\">bom</option>\
<option value=\"nidarosdommen\">nidarosdommen</option>\
</select>\
</div>\
<div>\
<input type=\"submit\" value=\"bestill\" onClick=\"fetch(location.href).then(()=>location.reload())\">\
</div>\
</form>\
</div>\
</body>\
</html>";

// Handle root url (/)
void handle_root() {
  server.send(200, "text/html", HTML);
}


//Handle an order
void handleOrderPath(){
    if(server.hasArg("startPoint") && server.hasArg("endPoint")){
      String startPoint = server.arg("startPoint");
      String endPoint = server.arg("endPoint");
    }

    server.sendHeader("Location", "/");
    // Respond to the http request with the status code 200
    // which means everything is ok and the request was
    // successful
    server.send(200);
    client.publish("startPoint", startPoint);
    client.publish("endPoint", endPoint);
}



void readHeader(WiFiClient client)
{
  // read first line of header
  char ch;
  int i = 0;
  while (ch != -1)
  {
    if (client.available())
    {
      ch = client.read();
      line1[i] = ch;
      i ++;
    }
  }
  line1[i] = '\0'; 
  Serial.println(line1);
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect

//--------------------------------------------------
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


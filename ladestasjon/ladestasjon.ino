#include <SPI.h>
#include <MFRC522.h>
#include "loading.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "mqtt.h"
#include <EEPROM.h>


#define OLED_RESET 4
#define SS_PIN 5                  /*Slave Select Pin*/
#define RST_PIN 0                 /*Reset Pin for RC522*/
#define LED_G 12                  /*Pin 8 for LED*/
MFRC522 mfrc522(SS_PIN, RST_PIN); /*Create MFRC522 initialized*/
Adafruit_SSD1306 display(OLED_RESET);

float batteryHealth =100;                        //helsetilstanden på batteriet 0-100%. høyere jo bedre
float batteryLevel =0;                         // ladeprosent for batteriet 0-100%
int batteryCycle;                           //antall batteriet har blitt ladet
float chargingspeed = batteryHealth / 100;  // Hastigheten batteriet lades med
char messageTemp;
float percentCharge;  // Tom variabel for hva bilen skal lade til
float p2;             // Tom variabel for hva bilen skal lade til brukt i funksjon
int environmentPoints = 1000;


int saldo= 5000;             // Saldo som skal leses av fra zumo
int mpris = 400;       // Pris for midjolading
int npris = 600;       //Pris for normal lading
int hpris = 800;       // Pris for hurtig lading
int bpris = 2000;      // Pris for å bytte batteri
int spris = 3000;      // pris for full serfivice
int newsaldo;          // Ny saldo
char str_charging[8];  // Global string som brukes til å sende data

// Tilstandene til ladestasjonen
enum Knapper {
  Midjolading,
  Normallading,
  Hurtiglading,
  Batteribytte,
  Service,
  Prosentlading,
  Ingen,
};

Knapper knapper;  // Definerer enum som knapper


// bitmap av Arne Midjo
const unsigned char myBitmap[] PROGMEM = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xf0, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x0e, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xc0, 0x0f, 0xff, 0xff, 0xc7, 0xff, 0xff, 0xf8, 0x07, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0x80, 0x7f, 0xff, 0xff, 0xf3, 0xff, 0xff, 0xf8, 0x01, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0x00, 0x7f, 0xff, 0xff, 0xf9, 0xff, 0xff, 0xf8, 0x00, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0x00, 0x7f, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xf8, 0x00, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xfe, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xfe, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x7f, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xfc, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x7f, 0x00, 0x00, 0xff,
  0xff, 0xff, 0xf8, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0xff,
  0xff, 0xff, 0xf0, 0x00, 0x3f, 0x3f, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x03, 0xff,
  0xff, 0xff, 0xf0, 0x00, 0x23, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x3f, 0xff,
  0xff, 0xff, 0xf8, 0x04, 0x17, 0xe7, 0xf0, 0x3f, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x03, 0xff,
  0xff, 0xff, 0xf8, 0x07, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0xff,
  0xff, 0xff, 0xfc, 0x01, 0xff, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x01, 0xff,
  0xff, 0xff, 0xfc, 0x01, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x07, 0xff,
  0xff, 0xff, 0xff, 0x00, 0xfe, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0xff,
  0xff, 0xff, 0xfe, 0x00, 0x3c, 0x00, 0x7b, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0xff,
  0xff, 0xff, 0xfc, 0x00, 0xc0, 0x00, 0x1e, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x3f, 0xff,
  0xff, 0xff, 0xf0, 0x02, 0xc0, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x7f,
  0xff, 0xff, 0xc0, 0x03, 0x00, 0x1f, 0xf7, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x1f,
  0xff, 0xfc, 0x00, 0x01, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff,
  0xff, 0x80, 0x00, 0x00, 0x0c, 0x00, 0xff, 0xbf, 0xff, 0xff, 0xfc, 0x00, 0x07, 0xff, 0xff, 0xff,
  0xf8, 0x00, 0x00, 0x00, 0x07, 0xf5, 0xfe, 0x00, 0x7f, 0xff, 0xe0, 0x00, 0x1f, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf0, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x0e, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};



WiFiClient espClient;            // Koble seg opp til internett
PubSubClient client(espClient);  // MQTT for å publishe og subscribe
long lastMsg = 0;
char msg[50];



void setup() {  // Setup for systemet

  // Ladestasjon kode
  Serial.setTimeout(10);                      // Venter 10ms for seriel kommunikasjon.
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // inisialiserer kommunikasjon med I2C addr 0x3D (for OLED 128x64)
  Serial.begin(9600);                         // Starter seriell kommunikasjon
  SPI.begin();                                // SPI kommunikasjon start
  mfrc522.PCD_Init();                         // RFID sensor start

  setup_wifi();                                        // WiFi setup
  client.setServer("192.168.137.85", 1883);               // IP adressen til brokeren og porten
  client.setCallback(callback);                        // Callback funksjon for å kommunisere
  display.clearDisplay();                              // Resetter display
  display.drawBitmap(0, 0, myBitmap, 128, 64, WHITE);  // Bilde av midjo på skjermen
  display.display();                                   // Setter bilde opp på display
  knapper = Ingen;                                     // Startverdien til knapper der den ikke gjør noe
}
// Callback funksjon
void callback(char* topic, byte* message, unsigned int length) {

  // Lager en string av dataen mottatt
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  messageTemp;

  // Kodesnutt for å registrere hvilken knapper som blir presset på nettsiden
  if (String(topic) == "percentCharge") {
    percentCharge = messageTemp.toFloat();  // Setter mottatt string til float
    knapper = Prosentlading;                // Setter tilstanden til kanapper som prosentlading
  }
  if (strcmp(topic, "batteryLevel") == 0) {
    batteryLevel = messageTemp.toFloat();  // Mottatt data fra string til float
  }
  if (strcmp(topic, "batteryHealth") == 0) {
    batteryHealth = messageTemp.toFloat();  // Mottatt data fra string til float
    chargingspeed = batteryHealth / 100;    // Definerer hastigheten til ladingen utifra batteryHealth
  }
  if (strcmp(topic, "batteryCycle") == 0) {
    batteryCycle = messageTemp.toFloat();  // Mottatt data fra string til float
  }
  if (strcmp(topic, "currency") == 0) {
    saldo = messageTemp.toFloat();  // Mottatt data fra string til float
  }
  if (strcmp(topic, "normalCharge") == 0) {
    knapper = Normallading;  //Setter tilstanden til knappen til normallading
  }
  if (strcmp(topic, "midjoCharge") == 0) {
    knapper = Midjolading;  // Setter tilstanden til knapper til midjolading
  }
  if (strcmp(topic, "speedCharge") == 0) {
    knapper = Hurtiglading;  // Setter tilstanden til knapper som hurtiglading
  }
  if (strcmp(topic, "batteryReplacement") == 0) {
    knapper = Batteribytte;  // Setter tilstanden til knapper som batteribytte
  }
  if (strcmp(topic, "batteryService") == 0) {
    knapper = Service;  // Setter tilstanden til knapper som Service
  }
  if (strcmp(topic, "environmentPoints") == 0) {
    environmentPoints = messageTemp.toFloat();  // Mottatt data fra string til float
  }
}


void loop() {
  if (!client.connected()) {  // Hvis ikke jeg er til MQTT, reconnect
    reconnect();
  }
  client.loop();  // Holder kontakten med MQTT



  if (!mfrc522.PICC_IsNewCardPresent()) {  // Registrerer hvis en ny NFC tag
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {  // Velger gitte NFC tag
    return;
  }

  // Legger til NFT tag i en lesbar string
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  // Sjekker om det er riktig NFC tag som blir tappet
  content.toUpperCase();
  if (content.substring(1) != "21 B3 1F 26")  // Ser om riktig NFC tagen er registrert
  {
    display.clearDisplay();
    display.println(" Access denied");  // Skriver feilmelding til OLED display hvis NFC ikke gjenkjennes
    display.display();
    delay(2000);  // Viser melding i 2 sekunder
    // Bilde av Midjo vises igjen
    display.clearDisplay();
    display.drawBitmap(0, 0, myBitmap, 128, 64, WHITE);
    display.display();

    return;
  }


  // Sjekker om NFC taggen er godkjent og at man har fått inn signal fra nettsiden
  if (content.substring(1) == "21 B3 1F 26" && knapper != Ingen) {
    client.publish("identification", "GruppeC29");  // Leverer gruppenummer som id til nodered


    // Cleaner OLED skjermen
    display.clearDisplay();

    // Switchcase for alle alle mulige valg på ladestasjonen
    switch (knapper) {

      case Midjolading:  // Kasen for Midjolading

        if (lavsaldo(saldo, mpris)) {  // Sjekker om man har nok på konto for å lade
          delay(2000);
          break;
        }
        p2 = 100;                                  // Setter lading til 100%
        senddata("charging", 1);                   // Sender info om at bilen lader og at den ikke skal kjøre
        charge(mpris, 1, p2);                      // Loading bar funksjonen
        senddata("batteryLevel", p2);              // Forteller at bilen er fulladet
        senddata("currency", newsaldo);            // Sender data til skyen med oppdatert saldo
        senddata("batteryHealth", batteryHealth);  // Sender oppdatert versjon av batterihelsen
        batteryCycle = batteryCycle + 1;           // Legger til +1 til antall ganger bilen har ladet
        senddata("batteryCycle", batteryCycle);    // Sender data om batterihelsen
        envPoint(20);                              // Miljøpoent sendes til skyen
        knapper = Ingen;                           // Resetter switchcasen
        break;

      case Normallading:               // Normallading valg
        if (lavsaldo(saldo, npris)) {  // Sjekker om man har nok på konto for å lade
          delay(2000);
          break;
        }
        p2 = 100;                                  // Setter lading til 100%
        senddata("charging", 1);                   // Sender info om at bilen lader
        charge(npris, 2, p2);                      // Loading funksjon
        senddata("batteryLevel", p2);              // Forteller at bilen er fulladet
        senddata("currency", newsaldo);            // Sender data til skyen med oppdatert saldo
        senddata("batteryHealth", batteryHealth);  // Sender oppdatering av baterihelsen til skyen
        batteryCycle = batteryCycle + 1;           // Legger til +1 for antall ganger batteriet er ladet
        senddata("batteryCycle", batteryCycle);    // Sender antall ganger batteriet er ladet
        envPoint(0);                               // Miljøpoent sendes til skyen
        knapper = Ingen;                           // Resetter switchcasen
        break;

      case Hurtiglading:               // Casen for hurtiglading
        if (lavsaldo(saldo, hpris)) {  // Sjekker om man har nok på konto for å lade
          delay(2000);
          break;
        }
        p2 = 100;                                  // Setter lading til 100%
        senddata("charging", 1);                   //Forteller bilen at den lades
        charge(hpris, 3, p2);                      // Hurtig lading
        senddata("batteryLevel", p2);              // Sender batteri nivå
        senddata("currency", newsaldo);            // Sender ny saldo
        senddata("batteryHealth", batteryHealth);  // Sender oppdatering om batterihelsen
        batteryCycle = batteryCycle + 1;           // Legger til +1 til antall ganger batteriet er ladet
        senddata("batteryCycle", batteryCycle);    // Sender data om batteryCycles
        envPoint(-10);                             // Miljøpoent sendes til skyen
        knapper = Ingen;                           // Resetter switchcasen

        break;

      case Batteribytte:
        if (lavsaldo(saldo, bpris)) {  // Sjekker om man har nok på konto for å bytte batteri
          delay(2000);
          break;
        }
        senddata("charging", 1);                 //Forteller bilen at den bytter batteri
        loading(1, 0, 100);                      // Loading baren på OLED skjermen
        newsaldo = saldo - bpris;                // Trekker ifra prisen fra kontoen
        batteryCycle = batteryCycle + 1;         // Legger tin +1 på batterycycle
        senddata("currency", newsaldo);          // Ny saldo opplasting
        senddata("batteryCycle", batteryCycle);  // Sender ny data på batterycycle
        senddata("batteryHealth", 100);          // Laster opp oppdatert batteri helse til bilen og skyen
        envPoint(-15);                           // Miljøpoent sendes til skyen
        knapper = Ingen;                         // Resetter switchcasen
        break;

      case Service:                    // casen for full service av bilen
        if (lavsaldo(saldo, spris)) {  // Sjekker om man har nok på konto for full service
          delay(2000);
          break;
        }
        p2 = 100;                                // Setter lading til 100%
        senddata("charging", 1);                 //Forteller bilen at den bytter batteri
        loading(0.75, 0, p2);                    // loadingbaren på OLEDskjermen
        newsaldo = saldo - spris;                // trekker ifra pris fra konto
        batteryCycle = batteryCycle + 1;         // Legger til +1 på hvor mange ganger bilen har ladet
        senddata("batteryHealth", p2);           // Sender batteri helse
        senddata("currency", newsaldo);          // Sender ny saldo
        senddata("batteryCycle", batteryCycle);  // Sender hvor mange ganger batteriet er skiftet
        envPoint(-20);                           // Miljøpoent sendes til skyen
        knapper = Ingen;                         // Resetter casen
        break;

      case Prosentlading:              // Casen for valgbar prosentlading
        if (lavsaldo(saldo, npris)) {  // Sjekker om man har nok på konto for å lade
          delay(2000);
          break;
        }

        p2 = percentCharge;                        // Setter lading til valgt prosent
        senddata("charging", 1);                   //Forteller bilen at den lades
        charge(npris, 3, p2);                      //Prosentlading med normal pris
        senddata("batteryLevel", p2);              // Sender batteri nivå til nettsiden
        senddata("currency", newsaldo);            // Sender ny saldo
        senddata("batteryHealth", batteryHealth);  // Sender batterihelse
        batteryCycle = batteryCycle + 1;           // Legger til 1+ for antall ganger batteriet er ladet
        senddata("batteryCycle", batteryCycle);    // Sender oppdatert verson av antall ganger batteriet er ladet
        knapper = Ingen;                           // Resetter switchcasen
        break;
      case Ingen:  // Vente casen der den ikke gjør noe
        break;
      default:  // Default hvis annet skjer
        break;
    }
    delay(10);  // 10ms delay for at koden ikke skal bli overkjørt
  }
  delay(500); // Delay siden 2 ikke alltid blir sendt i koden under
  senddata("charging", 2);  // Sender data om at bilen er ferdig med å lade
  delay(2000); // Sender ikke alltid 0
  senddata("charging",0); // viser at den er klar til å kjøre
  // Setter opp oled displayet for å vise ny saldo på konto
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Din saldo er:");
  display.print(newsaldo);
  display.display();
  delay(2000);
  // Midjo på oled skjerm
  display.clearDisplay();
  display.drawBitmap(0, 0, myBitmap, 128, 64, WHITE);
  display.display();

  saldo = newsaldo;  // oppdaterer saldoen som skal sendes
  return;
}

// connection funksjon for å koble seg opp med MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe til topicsene som vi bruker
      client.subscribe("batteryLevel");
      client.subscribe("currency");
      client.subscribe("batteryHealth");
      client.subscribe("environmentPoints"); 
      // Knapper fra nodered
      client.subscribe("midjoCharge");
      client.subscribe("normalCharge");
      client.subscribe("speedCharge");
      client.subscribe("batteryService");
      client.subscribe("batteryReplacement");
      client.subscribe("percentCharge");

    } else {
      // Printer feilmelding
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Funksjon for å vise loading bar på oled skjermen
void loading(float n, float p1, float p2) {
  display.setTextSize(2);

  //  loadbar L(10, 5, 100, 15);
  loadbar L(0, 20, 100, 30);  // Setter opp loadbaren fra egendefinert library
  while (p1 <= p2) {
    L.draw(p1, display);  // Tegner inn i rektangelet som er loadbaren
    p1 += n;              // Teller som legger til hastighetstallet n hver gang
    display.display();    // Oppdaterer skjermen hver runde
    delay(100);
    // Legger denne inn i loopen siden hvis batteryhealth er lav så vil den bruke såpass lang tid at espen dissconencter med serveren
    if (!client.connected()) {  // Reconnecter til MQTT.
      reconnect();
    }
    client.loop();
  }
  p1 = 0;               // Resetter p0 verdien til 0%
  L.draw(p2, display);  // Tegner opp loadbaren med nye prosenten
  display.display();
}

// Funksjon for å sjekke om man har nok penger for å utføre transaksjonen
bool lavsaldo(float saldo, float pris) {
  if (saldo - pris < 0) {

    // Visen på OLED skjermen at du har for lite saldo og viser hvor mye du har.
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Saldo for lav");
    display.println("Din saldo er: ");
    display.print(newsaldo + "kr");
    display.display();
    delay(3000);  // Viser meldingen i 3 sekunder
    return true;
  } else {
    return false;
  }
}


// Funksjon fpr å sende data til serveren
void senddata(char* topic, int data) {
  char datastring[8];                 // Maks 8 desimaler for datastringen
  sprintf(datastring, "%d", data);    // Gjør om float til string
  client.publish(topic, datastring);  // Sender stringen til valgt topic
}


void envPoint(int points) {
  environmentPoints += points;
  char envpoint_str[8];                               // Maks 8 desimaler for datastringen
  sprintf(envpoint_str, "%d", environmentPoints);     // Gjør om float til string
  client.publish("environmentPoints", envpoint_str);  // Sender stringen til valgt topic
}

// Funksjon for loadingbaren og saldo
void charge(int chargePrice, float speed, float percent) {
  loading(speed * chargingspeed, batteryLevel, percent);  // Loading funksjon fra egendefinert library loading.h.                                                       //  Input er hastigheten til loadingbar, som reduserers med chargingspeed, batterinivåete til bilen og hva den lades til
  newsaldo = saldo - chargePrice;                         // Oppdaterer saldo

  if (!client.connected()) {  // Sjekker om man trenger å reconnecte til srveren
    reconnect();
  }
  client.loop();
}
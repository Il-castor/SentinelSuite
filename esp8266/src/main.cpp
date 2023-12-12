#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <MFRC522.h>
#include <stdio.h>
#include <ArduinoJson.h>
#include "secrets.h"

#define SS_PIN D8   
#define RST_PIN D0 

#define RED_LED D3
#define GREEN_LED D2
#define BLU_LED D1
#define BUZZER_PIN D4

/*RFID Sensor Scheme:
* Vcc --> 3V3
* RST (Reset) --> D0
* GND (Ground) --> GND
* MISO (Master Input Slave Output) --> D6
* MOSI (Master Output Slave Input) --> D7
* SCK (Serial Clock) --> D5
* SS/SDA (Slave select) --> D8
*/

MFRC522 rfid(SS_PIN, RST_PIN); 
const int RFID_LENGTH = 4;



const char *MQTT_SERVER = "raspberrypi.local";
WiFiClient espClient;
PubSubClient client(espClient);

const char *SUBSCRIBE_TOPIC = "esp8266/configurazione";

int frequency;
int freqDuration;
//False enable RFID Reader, True disable RFID Reader 
bool isReader = true;

void wifi_connection()
{
  WiFi.begin(SSID, PASSWORD);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.println(WiFi.localIP());
}
// variabile dove mandare rfid letto
String sendTopic;
// variabile dove sottoscrivermi per vedere se quello che ho mandato è giusto o no
String subscribeTo;

void setColor(int R, int G, int B)
{
  analogWrite(RED_LED, R);
  analogWrite(GREEN_LED, G);
  analogWrite(BLU_LED, B);
}

void stateLed(String pl2)
{
  if (pl2.equals("1"))
  { // accendo led verde
    setColor(0, 255, 0);
    
    tone(BUZZER_PIN, frequency);
    delay(3000);
   
    noTone(BUZZER_PIN);
    setColor(0, 0, 0);
  }
  else if (pl2.equals("0"))
  { // accendo colore rosso
    setColor(255, 0, 0);
    delay(3000);
    setColor(0, 0, 0);
  }
  else if (pl2.equals("-1")){ //disable RFID reader 
    isReader = false;
    setColor(255, 0, 0);
    }
  else if (pl2.equals("-2")){
    isReader = true;
    setColor(0, 255, 0);
    delay(1000);
    setColor(0, 0, 0);
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String pl;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (unsigned int i = 0; i < length; i++)
  {
    // Serial.print((char)payload[i]);
    pl += (char)payload[i];
  }

  // Ho payload come stringa
  Serial.println(pl);

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, pl);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  // Leggo la configurazione
  if (((String)topic).equals(SUBSCRIBE_TOPIC))
  {
    String board_topic = doc["esp32"].as<String>();
    // Serial.println("doc " + board_topic);
    // client.subscribe(board_topic.c_str());
    // Serial.println("Mi sono sottoscritto al topic: "+ board_topic);

    sendTopic = doc["send-topic-data"].as<String>();
    subscribeTo = doc["subscribe"].as<String>();
    client.subscribe(subscribeTo.c_str());
    frequency = doc["buzzer"][0];
    freqDuration = doc["buzzer"][1];

    Serial.println("Ecco quello che ho parsato");
    Serial.println("sendTopic = " + String(sendTopic));
    Serial.println("subscribeTo = " + String(subscribeTo));
    Serial.println("frequency = " + String(frequency));
    Serial.println("freqDuration = " + (String)freqDuration);
  }
  else if (((String)topic).equals(subscribeTo))
  {
    //Serial.println("MAMMA");
    String pl2;
    for (unsigned int i = 0; i < length; i++)
    {
      // Serial.print((char)payload[i]);
      pl2 += (char)payload[i];
    }

    Serial.println(pl2);
    stateLed(pl2);
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");

      client.subscribe(SUBSCRIBE_TOPIC);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
 	Serial.begin(115200);
 	SPI.begin(); // Init SPI bus
 	rfid.PCD_Init(); // Init MFRC522
 	Serial.println();
 	Serial.print(F("Reader :"));
 	rfid.PCD_DumpVersionToSerial();

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BLU_LED, OUTPUT);

  wifi_connection();
  client.setServer(MQTT_SERVER, 1883);

  client.setCallback(callback);

  if (client.connect("esp8266-client"))
  {
    Serial.println("Connesso al broker MQTT");
    client.subscribe(SUBSCRIBE_TOPIC);
    Serial.println("Sottoscritto al topic");
  }
  else
    Serial.println("Connessione al broker MQTT fallita");

  Serial.println("Appoggia la tessera grazie");
  //setColor(255, 0, 0);
 	
}

void byteArrayToHexString(unsigned char *byteArray, int length, char *hexString)
{
  for (int i = 0; i < length; i++)
  {
    sprintf(&hexString[i * 2], "%02x", byteArray[i]);
  }
}

void readNfc()
{
  if (rfid.PICC_IsNewCardPresent())
  { // new tag is available
    if (rfid.PICC_ReadCardSerial())
    { // NUID has been readed
 	    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

      char hexString[4 * 2 + 1]; // Each byte represented by 2 characters + null terminator

      // Convert byte array to hexadecimal string
      byteArrayToHexString(rfid.uid.uidByte, RFID_LENGTH, hexString);

      client.publish(sendTopic.c_str(), hexString);

      for (int i = 0; i < rfid.uid.size; i++)
      {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(rfid.uid.uidByte[i], HEX);
      }

      Serial.println();
    }
    rfid.PICC_HaltA();      // halt PICC
    rfid.PCD_StopCrypto1(); // stop encryption on PCD

  }
}

void loop() {

 if (!client.connected())
    reconnect();

  client.loop();
  if (isReader)
    readNfc();
    

  
}

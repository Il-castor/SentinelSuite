#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "secrets.h"

#define MQTT_SERVER "raspberrypi.local"
#define MQTT_PORT 1883

#define POWER_PIN 25  // connected to sensor's VCC pin
#define SIGNAL_PIN 34 // for water level sensor

#define GREEN_LED 17
#define RED_LED 16
#define BLUE_LED 18

#define DO_PIN 27 // ESP32's pin GPIO13 connected to DO pin of the flame sensor

#define DHTPIN 4

#define DHTTYPE DHT11 // DHT 11
// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

const char *SUBSCRIBE_TOPIC = "esp32/configurazione";

String temperature_topic;
int timeToSendDataTempHum = 2000;
int timeToSendDataFlameWater = 5000;
String humidity_topic;
String temperature_subscribe;
String humidity_subscribe;
String isCelsius; // only C => Celsius or F for Fahrenheit

String sendWater, sendFire, subFire, waterPump;
int treshold = 300; // default value

WiFiClient espClient;
PubSubClient client(espClient);

void setColor(int red, int green, int blue)
{
  analogWrite(RED_LED, red);
  analogWrite(GREEN_LED, green);
  analogWrite(BLUE_LED, blue);
}

void stateLedTemperature(String pl2)
{
  switch (pl2.toInt())
  {
  case 0:
    setColor(255, 255, 255); // white
    break;
  case 1:
    setColor(128, 0, 128); // purple
    break;
  case 2:
    setColor(0, 255, 0); // green
    break;
  case 3:
    setColor(255, 0, 0); // red
    break;
  default:
    break;
  }
}

void stateLedHumidity(String pl2)
{
  switch (pl2.toInt())
  {
  case 0:
    setColor(0, 0, 0); // black => ok, no deum
    break;
  case 1:
    setColor(255, 165, 0); // orange => deum on
    break;
  default:
    break;
  }
}

void pumpWater(bool action)
{
  if (action)
  {
    Serial.println("ACCENDO LA POMPA");
    setColor(0, 0, 255); // blue
  }
  else
  {
    Serial.println("Spengo la pompa");
    setColor(255, 36, 86); // rose
  }
}

int lastValue;
void callback(char *topic, byte *payload, unsigned int length)
{
  String pl;
  Serial.println("lenght: " + (String)length);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (unsigned int i = 0; i < length; i++)
  {
    pl += (char)payload[i];
  }

  Serial.println(pl);
  DynamicJsonDocument doc(4096);

  DeserializationError error = deserializeJson(doc, pl);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  Serial.println();

  if (((String)topic).equals(SUBSCRIBE_TOPIC))
  {
    timeToSendDataTempHum = doc["timeToSendDataTempHum"].as<int>();
    timeToSendDataFlameWater = doc["timeToSendDataFlameWater"].as<int>();
    temperature_topic = doc["temperature_topic"].as<String>();

    isCelsius = doc["unit"].as<String>();
    temperature_subscribe = doc["temperature_subscribe"].as<String>();
    client.subscribe(temperature_subscribe.c_str(), 1);
    humidity_topic = doc["humidity_topic"].as<String>();
    humidity_subscribe = doc["humidity_subscribe"].as<String>();
    client.subscribe(humidity_subscribe.c_str(), 1);

    sendWater = doc["sendWater"].as<String>();
    treshold = doc["treshold"].as<int>();
    waterPump = doc["waterPump"].as<String>();
    sendFire = doc["sendFire"].as<String>();
    subFire = doc["subFire"].as<String>();
    client.subscribe(subFire.c_str()); // mi sottoscrivo al topic in caso in cui debba spegnere l'incendio

    client.subscribe(waterPump.c_str());

    Serial.println("Ecco quello che ho parsato");
    Serial.println("temperature topic = " + String(temperature_topic));
    Serial.println("temperature subscribe = " + String(temperature_subscribe));
    Serial.println("humidity topic " + String(humidity_topic));
    Serial.println("humidity subscribe = " + (String)humidity_subscribe);
    Serial.println(sendWater);
    Serial.println(treshold);
    Serial.println(waterPump);
    Serial.println(sendFire);
    Serial.println(subFire);
  }
  else if (((String)topic).equals(temperature_subscribe))
  {
    String pl2;
    for (int i = 0; i < length; i++)
    {
      pl2 += (char)payload[i];
    }
    Serial.println(pl2);
    stateLedTemperature(pl2);
  }
  else if (((String)topic).equals(humidity_subscribe))
  {
    String pl3;
    for (int i = 0; i < length; i++)
    {
      pl3 += (char)payload[i];
    }
    Serial.println(pl3);
    stateLedHumidity(pl3);
  }
  else if (((String)topic).equals(waterPump))
  {
    String pl3;
    for (int i = 0; i < length; i++)
    {
      pl3 += (char)payload[i];
    }
    if (pl3.toInt() == lastValue)
      return;
    else if (pl3.equals("1"))
      // bisogna spegnere l'incendio
      pumpWater(true);
    else
      pumpWater(false);
  }
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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

      client.subscribe(SUBSCRIBE_TOPIC, 1);
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

const float maxDiff = 1.0;
bool checkBound(float newValue, float prevValue)
{
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;

int value = 0;
int flame_state = 0;

void setup()
{
  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(DO_PIN, INPUT);
  pinMode(POWER_PIN, OUTPUT);

  setColor(0, 0, 255);

  digitalWrite(POWER_PIN, LOW); // turn the water sensor OFF

  setup_wifi();
  client.setServer(MQTT_SERVER, 1883);

  client.setCallback(callback);

  if (client.connect("esp32-client"))
  {
    Serial.println("Connesso al broker MQTT");
    client.subscribe(SUBSCRIBE_TOPIC, 1);
    Serial.println("Sottoscritto al topic");
  }
  else
    Serial.println("Connessione al broker MQTT fallita");

  dht.begin();

  Serial.println("I'm ready!");
}

void tempHumidity()
{
  long now = millis();
  if (now - lastMsg > timeToSendDataTempHum)
  {
    lastMsg = now;
    float newTemp;
    if (isCelsius.equals("F"))
      newTemp = dht.readTemperature(true);
    else if (isCelsius.equals("C"))
      newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();

    if (isnan(newTemp) || isnan(newHum))
      return;

    if (checkBound(newTemp, temp))
    {
      temp = newTemp;
      Serial.print("New temperature:");
      Serial.println(String(temp).c_str());
      client.publish(temperature_topic.c_str(), String(temp).c_str());
    }

    if (checkBound(newHum, hum))
    {
      hum = newHum;
      Serial.print("New humidity:");
      Serial.println(String(hum).c_str());
      client.publish(humidity_topic.c_str(), String(hum).c_str());
    }
  }
}

void flameWater()
{

  static long prevMillis = 0;
  if ((millis() - prevMillis) >= timeToSendDataFlameWater)
  {
    prevMillis = millis();
    digitalWrite(POWER_PIN, HIGH);
    delay(1000);
    value = analogRead(SIGNAL_PIN); // read the analog value from sensor
    delay(500);
    digitalWrite(POWER_PIN, LOW);
    Serial.println("Value of water: " + String(value));
    if (value > treshold)
    {
      Serial.println("ATTENZIONE");
      // pubblic on if i detect water leakage
      client.publish(sendWater.c_str(), "on");
    }
    else
    {
      Serial.println("No water leak");
      // pubblic on if i detect water leakage
      client.publish(sendWater.c_str(), "off");
    }
    flame_state = digitalRead(DO_PIN);
    // Serial.println(flame_state);
    if (flame_state == HIGH)
    {
      Serial.println("Flame dected => The fire is detected");
      client.publish(sendFire.c_str(), "1");
    }
    else
    {
      Serial.println("No flame dected => The fire is NOT detected");
      client.publish(sendFire.c_str(), "0");
    }
  }
}

void loop()
{
  if (!client.connected())
    reconnect();

  client.loop();

  tempHumidity();
  flameWater();
}

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <LoRa.h>
#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);
#define MQTT_SERVER "mqtt.ucll-labo.be"
#define MQTT_PORT 1883
#define MQTT_USER "ucll"
#define MQTT_PASSWORD "demo"
const String MQTT_CLIENTID = "ESP32-" + String(random(0xffffff), HEX);
#define MQTT_MESSAGE_TIMER 15000
unsigned long lastMessageMillis = 0;

#define SSID "TP-Link_42C4"
#define PASSWORD "66243359"
#define LEDPIN 25

#define SS 18
#define RST 23
#define DI0 26
#define BAND 863.5E6

boolean hasLora = false;

void onLoraReceive(int packetSize)
{
  if (packetSize == 0)
    return;

  // read packet
  String message = "";
  for (int i = 0; i < packetSize; i++)
  {
    message += (char)LoRa.read();
  }
  Serial.println("Received LoRa message : " + message);

  // check if message is valid json
  bool hasError = false;
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error)
  {
    hasError = true;
    Serial.print("Invalid data fprmat : deserializeJson() failed : ");
    Serial.println(error.f_str());
  }
  else
  {
    String user = doc["user"];
    String message = doc["message"];
    user.trim();
    message.trim();
    if (user.length() == 0)
    {
      hasError = true;
      Serial.println("Invalid data format : user value is missing");
    }

    if (message.length() == 0)
    {
      hasError = true;
      Serial.println("Invalid data format : message value is missing");
    }

    if (!hasError)
    {
      String topic = "Loragw/" + user;
      if (mqttClient.publish( topic.c_str() , message.c_str() , true))
      {
        Serial.println("Published MQTT message to topic ("+topic+") : " + message);
      }
      else
      {
        Serial.println("Failed to send MQTT message to topic ("+topic+") : " + message);
      }
    }
  }
}

void connectMQTT()
{
  Serial.println("Connecting to MQTT Server ... ");
  mqttClient.setKeepAlive(5);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  if (!mqttClient.connect(MQTT_CLIENTID.c_str(), MQTT_USER, MQTT_PASSWORD))
  {
    Serial.print("Connection failed ! state = ");
    Serial.println(mqttClient.state());
  }
  else
  {
    Serial.print("Connected to MQTT Server with ClientID : ");
    Serial.println(MQTT_CLIENTID);
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);

  // config WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  // config LoRa
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND))
  {
    Serial.println("LoRa initialisation failed!");
    hasLora = false;
  }
  else
  {
    Serial.println("LoRa initialised");
    hasLora = true;
  }

  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(8);
  LoRa.setCodingRate4(6);
  LoRa.setPreambleLength(8);
  LoRa.enableCrc();
  LoRa.setTxPower(14);
  Serial.println("LoRa communication parameters set");
}

void loop()
{

  onLoraReceive(LoRa.parsePacket());

  if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(LEDPIN, HIGH);
    if (!mqttClient.state() == MQTT_CONNECTED)
    {
      connectMQTT();
    }
    else
    {
      mqttClient.loop();
    }
  }
  else
  {
    digitalWrite(LEDPIN, LOW);
  }
}

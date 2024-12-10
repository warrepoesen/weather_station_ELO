#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <LoRa.h>
#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);
#define MQTT_SERVER "k106.ucll-labo.be"
#define MQTT_PORT 1883
#define MQTT_USER "project"
#define MQTT_PASSWORD "eloict1234"

#define SSID "ProjectNetwork" // Enter your WiFi name
#define PASSWORD "eloict1234" // Enter WiFi password
#define LEDPIN 25

#define SS 18
#define RST 23
#define DI0 26
#define BAND 8635E5

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
    char buf[1000];
    String t = doc["t"];
    String topic = doc["topic"];
    t.trim();
    topic.trim();

    if (t.length() == 0)
    {
      hasError = true;
      Serial.println("Invalid data format : user value is missing");
    }

    if (t == "i") //if info topic
    {
      String userName = doc["name"];
      String userPassword = doc["password"];
      userName.trim();
      userPassword.trim();
      JsonDocument doc1;
      doc1["name"] = userName;
      doc1["password"] = userPassword;
      serializeJson(doc1, buf);
    }
    if (t == "g") //if gps topic
    {
      double latitude = doc["latitude"];
      double longitude = doc["longitude"];
      JsonDocument doc1;
      doc1["latitude"] = latitude;
      doc1["longitude"] = longitude;
      serializeJson(doc1, buf);
    }
    if (t == "m") //if measure topic
    {

      float temperature = doc["temperature(C)"];
      float humidity = doc["humidity(%)"];
      float pressure = doc["pressure(HPa)"];
      float windspeed = doc["windspeed(Km/h)"];
      String windDirection = doc["winddirection()"];
      windDirection.trim();

      JsonDocument doc1;
      if (temperature)
      {
        doc1["temperature(C)"] = temperature;
        doc1["humidity(%)"] = humidity;
        doc1["pressure(HPa)"] = pressure;
      }
      if (windspeed)
      {
        doc1["windspeed(Km/h)"] = windspeed;
      }
      if (windDirection)
      {
        doc1["winddirection()"] = windDirection;
      }
      serializeJson(doc1, buf);
    }

    if (!hasError)
    {

      if (mqttClient.publish(topic.c_str(), buf, true))
      {
        Serial.println("Published MQTT message to topic (" + topic + ") : " + message);
      }
      else
      {
        Serial.println("Failed to send MQTT message to topic (" + topic + ") : " + message);
      }
    }
  }
}

void connectMQTT()
{
  Serial.println("Connecting to MQTT Server ... ");
  mqttClient.setKeepAlive(5);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  String client_id = "esp32-client-";
  client_id += String(WiFi.macAddress());
  if (!mqttClient.connect(client_id.c_str(), MQTT_USER, MQTT_PASSWORD))
  {
    Serial.print("Connection failed ! state = ");
    Serial.println(mqttClient.state());
  }
  else
  {
    Serial.print("Connected to MQTT Server with ClientID : ");
    Serial.println(client_id);
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

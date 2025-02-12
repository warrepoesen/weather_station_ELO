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
#define BAND 8654E5

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

    if (t == "i") // if info topic
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
    if (t == "g") // if gps topic
    {
      double latitude = doc["latitude"];
      double longitude = doc["longitude"];
      JsonDocument doc1;
      doc1["latitude"] = latitude;
      doc1["longitude"] = longitude;
      serializeJson(doc1, buf);
    }
    if (t == "m") // if measure topic
    {

      float temperature = doc["temperature(C)"];
      float humidity = doc["humidity(%)"];
      float pressure = doc["pressure(HPa)"];
      float windspeed = doc["windspeed(Km/h)"];
      String windDirection = doc["winddirection()"];
      float PM1 = doc["PM1(µg/m³)"];
      float PM2_5 = doc["PM2.5(µg/m³)"];
      float PM10 = doc["PM10(µg/m³)"];
      double battery = doc["battery(%)"];
      windDirection.trim();

      temperature = int(round(temperature * 100.00)) / 100.00; // round everything to 2 numbers
      humidity = int(round(humidity * 100.00)) / 100.00;
      pressure = int(round(pressure * 100.00)) / 100.00;
      PM1 = int(round(PM1 * 100.00)) / 100.00;
      PM2_5 = int(round(PM2_5 * 100.00)) / 100.00;
      PM10 = int(round(PM10 * 100.00)) / 100.00;
      windspeed = int(round(windspeed * 100.00)) / 100.00;
      battery = int(round(battery * 100.00)) / 100.00;

      JsonDocument doc1;
      if (temperature)
      {
        doc1["temperature(C)"] = temperature;
        doc1["humidity(%)"] = humidity;
        doc1["pressure(HPa)"] = pressure;
      }
      if (PM1)
      {
        doc1["PM1(µg/m³)"] = PM1;
        doc1["PM2.5(µg/m³)"] = PM2_5;
        doc1["PM10(µg/m³)"] = PM10;
      }
      if (windspeed)
      {
        doc1["windspeed(Km/h)"] = windspeed;
      }
      if (windDirection != "null")
      {
        doc1["winddirection()"] = windDirection;
      }

      doc1["battery(%)"] = battery;
      serializeJson(doc1, buf);
    }

    if (!hasError)
    {

      if (mqttClient.publish(topic.c_str(), buf, true))
      {
        Serial.println("Published MQTT message to topic (" + topic + ") : " + buf);
      }
      else
      {
        Serial.println("Failed to send MQTT message to topic (" + topic + ") : " + buf);
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

  LoRa.setSignalBandwidth(250E3);
  LoRa.setSpreadingFactor(8);
  LoRa.setCodingRate4(5);
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

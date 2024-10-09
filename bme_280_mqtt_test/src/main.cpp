/*********
  Complete project details at https://randomnerdtutorials.com
*********/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi
const char *ssid = "ProjectNetwork"; // Enter your WiFi name
const char *password = "eloict1234"; // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "www.lukaslipkens.be";
const char *topic = "stationTest";
const char *mqtt_username = "station";
const char *mqtt_password = "Elo-Ict-2024";
const int mqtt_port = 1883;

//globale variabelen
float temperature;
float humidity;
float pressure;

WiFiClient espClient;
PubSubClient client(espClient);

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C
// Adafruit_BME280 bme(BME_CS); // hardware SPI
// Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

unsigned long delayTime = 5000;

void setup()
{
  Serial.begin(9600);
  // Connecting to a Wi-Fi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  client.setServer(mqtt_broker, mqtt_port);

  while (!client.connected())
  {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("Public EMQX MQTT broker connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  Serial.println(F("BME280 test"));
  bool status;

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin(0x76);
  if (!status)
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1)
      ;
  }

  Serial.println("-- Default Test --");
  

  Serial.println();
}

void printValues()
{
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");

  // Convert temperature to Fahrenheit
  /*Serial.print("Temperature = ");
  Serial.print(1.8 * bme.readTemperature() + 32);
  Serial.println(" *F");*/

  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.println();
}
void readValues()
{
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;

}

void publishValues()
{
  JsonDocument doc;
  doc["id"] = "testWarre";
  doc["timestamp"]="tijd";
  doc["temperature(C)"] = temperature;
  doc["humidity(%)"]= humidity;
  doc["pressure(HPa)"]=pressure;
  char buf[1000];
  serializeJson(doc,buf);
  client.publish(topic,buf);
}

void loop()
{
  readValues();
  publishValues();
  delay(delayTime);
  
}
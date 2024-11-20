
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "sensor.h"
#include "gps.h"
#include "sleep.h"
#include "mac.h"

// WiFi
#define SSID "ProjectNetwork" // Enter your WiFi name
#define PASSWORD "eloict1234" // Enter WiFi password
WiFiClient espClient;

// MQTT Broker
#define MQTT_SERVER "k106.ucll-labo.be"
#define MQTT_PORT 1883
#define MQTT_USER "project"
#define MQTT_PASSWORD "eloict1234"

PubSubClient client(espClient);



// gps
extern RTC_DATA_ATTR double longitude;
extern RTC_DATA_ATTR double latitude;

// globale variabelen

 // I2C
// Adafruit_BME280 bme(BME_CS); // hardware SPI
// Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

unsigned long delayTime = 5000;







void publishValues()
{
  JsonDocument doc;

  if (bme.checkConnection(0x76))
  {
    doc["temperature(C)"] = temperature;
    doc["humidity(%)"] = humidity;
    doc["pressure(HPa)"] = pressure;
  }

  if (windSpeed > 0) // check if value exists
  {
    doc["windspeed(Km/h) "] = windSpeed;
  }
  doc["battery(%)"] = (69.69);
  char buf[1000];
  serializeJson(doc, buf);
  if (bme.checkConnection(0x76) | (windSpeed > 0))
  {
    client.publish(measureTopic, buf, true);
  }
}

void setup()
{
  Serial.begin(115200);
  
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                 " Seconds");

  // zet mosfet aan

  // Connecting to a Wi-Fi network
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  client.setServer(MQTT_SERVER, MQTT_PORT);

  while (!client.connected())
  {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), MQTT_USER, MQTT_PASSWORD))
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
  readMacAddress();

  if (bootCount == 1)
  {
    readGPS();
    delay(500);
    char buf[1000];
    publishGPS(buf);
    client.publish(gpsTopic,buf,true);
  }

  bme.begin(0x76);

  Serial.println("-- Default Test --");
  readValues();
  publishValues();
  // mosfet uitzetten
  Serial.println("Going to sleep now");
  delay(500);
  Serial.flush();
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}
void loop()
{
  readValues();
  publishValues();
  delay(delayTime);
}

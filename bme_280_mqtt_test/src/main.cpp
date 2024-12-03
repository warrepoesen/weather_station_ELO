
#include <WiFi.h>
#include <PubSubClient.h>

#include "sensor.h"
#include "gps.h"
#include "sleep.h"
#include "mac.h"
#include "AP.h"

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
bool gps_on = false;


void standardFunction()
{

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
  if (bootCount == 2)
  {
    Serial.println("Naam: " + String(userName));
    Serial.println("Wachtwoord: " + String(userPassword));
    char buf0[1000];
    serializeInfo(buf0);
    client.publish(infoTopic, buf0, true);
  }

  if (bootCount == 2 && gps_on)
  {
    readGPS();
    delay(500);
    char buf1[1000];
    serializeGPS(buf1);
    client.publish(gpsTopic, buf1, true);
  }

  bme.begin(0x76);

  Serial.println("-- Default Test --");
  readValues();
  if (bme.checkConnection(0x76) | (windSpeed > 0))
  {
    char buf2[1000];
    serializeValues(buf2);
    client.publish(measureTopic, buf2, true);
  }

  // mosfet uitzetten
}

void setup()
{

  Serial.begin(115200);

  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();

  if (!ap_setup) // if setup = true dont do this
  {
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

    standardFunction();

    Serial.println("Going to sleep now");
    delay(500);
    Serial.flush();
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
  }

  AP_setup(); // setup= true ==> this happens otherwise it will already sleep here.
}
void loop()
{

  AP_loop();
  if (!ap_setup) // if setup = true dont do this
  {
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

    Serial.println("Going to sleep now");
    delay(500);
    Serial.flush();
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
  }
}

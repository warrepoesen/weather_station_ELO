
#include <WiFi.h>
#include <PubSubClient.h>
#include <LoRa.h>

#include "sensor.h"
#include "gps.h"
#include "sleep.h"
#include "mac.h"
#include "AP.h"

//// WiFi
#define SSID "ProjectNetwork" // Enter your WiFi name
#define PASSWORD "eloict1234" // Enter WiFi password
WiFiClient espClient;

//// MQTT Broker
// #define MQTT_SERVER "k106.ucll-labo.be"
// #define MQTT_PORT 1883
// #define MQTT_USER "project"
// #define MQTT_PASSWORD "eloict1234"
//
// PubSubClient client(espClient);

// lora
#define SS 18
#define RST 23
#define DI0 26
#define BAND 8654E5

boolean hasLora = false;
unsigned long beforeLora;
unsigned long afterLora;

// gps
bool gps_on = false;

void sendLoraMessage(String message)
{
  if (hasLora)
  {
    LoRa.beginPacket();
    LoRa.print(message);
    beforeLora = millis();
    LoRa.endPacket();
    afterLora = millis();

    Serial.println("Message sent : " + message + "\n\r Time: " + (afterLora - beforeLora));
  }
}

void standardFunction()
{

  // zet mosfet aan

  WiFi.begin(SSID, PASSWORD); // is required to read mac addres
  readMacAddress();

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
  Serial.println("LoRa configuration done");

  if (bootCount == 2 && ap_complete)
  {
    Serial.println("Naam: " + String(userName));
    Serial.println("Wachtwoord: " + String(userPassword));
    char buf0[1000];
    serializeInfo(buf0);
    sendLoraMessage(buf0);
  }

  if (bootCount == 5 && gps_on)
  {
    readGPS();
    delay(500);
    char buf1[1000];
    serializeGPS(buf1);
    sendLoraMessage(buf1);
  }
  if (bootCount <= 10 && gps_on)
  {
    char buf2[1000];
    serializeGPS(buf2);
    sendLoraMessage(buf2);
  }

  bme.begin(0x76);
  alpha_setupSPI();

  Serial.println("-- Default Test --");
  readValues();
  if (bme.checkConnection(0x76) or (windSpeed >= 0) or windConnected) // check if any sensor is connected
  {
    char buf3[1000];
    serializeValues(buf3);
    sendLoraMessage(buf3);
  }

  // mosfet uitzetten
}

void setup()
{

  Serial.begin(115200);
  pinMode(REED_PIN, INPUT_PULLUP);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)REED_PIN, 0);

  esp_sleep_wakeup_cause_t wakeupReason = print_wakeup_reason();

  if (wakeupReason == ESP_SLEEP_WAKEUP_EXT0)
  {
    tipCount++;
    Serial.printf("Wakker geworden door reedcontact! Aantal tips: %d\n", tipCount);
  }
  else if (wakeupReason == ESP_SLEEP_WAKEUP_TIMER)
  {
    ++bootCount;
    totalRainfall = tipCount * WATERPERTIP;
    //Serial.println("--- Regenmeter ---");
    //Serial.printf("Aantal tips: %d\n", tipCount);
    //Serial.printf("Totaal gevallen regen: %.2f mm\n", totalRainfall);
    tipCount = 0;
  }
  else
  {
    Serial.println("Eerste opstart of onbekende wake-up reden.");
  }
  
  Serial.println("Boot number: " + String(bootCount));
  

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

  AP_setup(); // ap_setup= true ==> this happens otherwise it will already sleep here.
}
void loop()
{

  AP_loop();
  if (!ap_setup) // if setup = true dont do this
  {
    esp_sleep_enable_timer_wakeup(1 * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(1) + " Seconds");

    Serial.println("Going to sleep now");
    delay(500);
    Serial.flush();
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
  }
}

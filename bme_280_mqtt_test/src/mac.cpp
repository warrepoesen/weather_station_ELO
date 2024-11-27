#include <Arduino.h> 

#include "mac.h"

char measureTopic[256];
char gpsTopic[256];
char infoTopic[256];


void readMacAddress()
{
  uint8_t baseMac[6];
  esp_err_t ret = esp_base_mac_addr_get(baseMac);
  if (ret == ESP_OK)
  {
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  baseMac[0], baseMac[1], baseMac[2],
                  baseMac[3], baseMac[4], baseMac[5]);
    sprintf(measureTopic, "weatherstations/station_%02x%02x%02x%02x%02x%02x/measurement",
            baseMac[0], baseMac[1], baseMac[2],
            baseMac[3], baseMac[4], baseMac[5]);
    Serial.println(measureTopic);
    sprintf(gpsTopic, "weatherstations/station_%02x%02x%02x%02x%02x%02x/location",
            baseMac[0], baseMac[1], baseMac[2],
            baseMac[3], baseMac[4], baseMac[5]);
    Serial.println(gpsTopic);
    sprintf(infoTopic, "weatherstations/station_%02x%02x%02x%02x%02x%02x/info",
            baseMac[0], baseMac[1], baseMac[2],
            baseMac[3], baseMac[4], baseMac[5]);
    Serial.println(gpsTopic);

  }
  else
  {
    Serial.println("Failed to read MAC address");
  }
}



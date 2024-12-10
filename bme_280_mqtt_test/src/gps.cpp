#include "gps.h"
#include <TinyGPSPlus.h>
#include <ArduinoJson.h>

#include "mac.h"

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);
RTC_DATA_ATTR double longitude;
RTC_DATA_ATTR double latitude;

void readGPS()
{

    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
    Serial.println("Serial 2 started at 9600 baud rate");

    unsigned long startTime = millis();
    //unsigned long timeout = 1000000; // 1000-second timeout for GPS connection

    while (true) // Keep looping until we have valid coordinates or timeout
    {
        // Check for timeout
        /*if (millis() - startTime > timeout)
        {
            Serial.println("GPS connection timed out.");
            break; // Exit the loop if timeout is reached
        }*/

        // Process GPS data for 1000 milliseconds (1 second) each loop
        unsigned long readStart = millis();
        while (millis() - readStart < 1000)
        {
            while (gpsSerial.available() > 0)
            {
                gps.encode(gpsSerial.read());
            }

            // Read and print GPS data if it has been updated
            if (gps.location.isUpdated())
            {
                latitude = gps.location.lat();
                longitude = gps.location.lng();

                Serial.print("Time in UTC: ");
                Serial.println(String(gps.date.year()) + "/" + String(gps.date.month()) + "/" + String(gps.date.day()) + "," +
                               String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()));
                Serial.println("");
                Serial.print("Latitude: ");
                Serial.println(latitude);
                Serial.print("Longitude: ");
                Serial.println(longitude);
            }
        }

        // Check if both latitude and longitude are non-zero
        if (latitude != 0 && longitude != 0)
        {
            Serial.println("Valid GPS coordinates received.");
            break; // Exit the loop if valid coordinates are obtained
        }
        else
        {
            Serial.println("Waiting for valid GPS coordinates...");
            delay(500); // Wait before retrying to avoid excessive looping
        }
    }
}


void serializeGPS(char * buf)
{

  if (gps.location.lat() && gps.location.lng() != 0)
  {
    JsonDocument doc;
    doc["t"]="g";
    doc["topic"]=gpsTopic;
    doc["latitude"] = latitude;
    doc["longitude"] = longitude;

    
    serializeJson(doc, buf,1000);
   
  }
}

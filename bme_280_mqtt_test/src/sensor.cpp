#include "sensor.h"

#include <Adafruit_BME280.h>
#include <Wire.h>
#include <ArduinoJson.h>

float temperature;
float humidity;
float pressure;
float windSpeed;

Adafruit_BME280 bme;

float readWindSpeed()
{
  int sensorVal = analogRead(WINDSPEED_SENSOR_PIN);
  float voltage = sensorVal * (3.3 / 4095.0); // waarde op schaal zetten

  float currentmA = (voltage / WINDSPEED_SENSOR_RESISTOR) * 1000; // stroom in mA

  if (currentmA < 4)
  {
    return (-1); // Stroom mag niet lager zijn dan 4 mA want kan niet dus return NAN
  }
  else
  {
    windSpeed = (currentmA - 4) * (108 / 16); // sensor kan tot 30m/s = 108 km/h
    return windSpeed;
  }
}
void readValues()
{
  if (bme.checkConnection(0x76))
  {
    temperature = bme.readTemperature();
    humidity = bme.readHumidity();
    pressure = bme.readPressure() / 100.0F;
  }

  windSpeed = readWindSpeed();
}

void serializeValues(char * buf)
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
  
  serializeJson(doc, buf,sizeof(buf));
  
}
#include "sensor.h"

#include <Adafruit_BME280.h>
#include <Wire.h>
#include <ArduinoJson.h>

float temperature;
float humidity;
float pressure;
float windSpeed;
String windDirection;
bool windConnected;

Adafruit_BME280 bme;

float readWindSpeed()
{
  pinMode(WINDSPEED_SENSOR_PIN,INPUT);
  int sensorVal = analogRead(WINDSPEED_SENSOR_PIN);
  
  float voltage = sensorVal * (3.3 / 4095.0); // waarde op schaal zetten

  float currentmA = (voltage / WINDSPEED_SENSOR_RESISTOR) * 1000; // stroom in mA

  if (currentmA < 2)
  {
    return (-1); // Stroom mag niet lager zijn dan 4 mA want kan niet dus return NAN maar dit is niet werkelijk 4 dus we zetten de cutof of 2
  }
  else
  {
    windSpeed = (currentmA - 3) * (108 / 16); // sensor kan tot 30m/s = 108 km/h
    if (windSpeed < 0)
      windSpeed=0;
    return windSpeed;
  }
}

bool determineWindDirection()
{
  pinMode(WINDDIRECTION_SENSOR_PIN, INPUT);
  float currentmA, voltage;
  String result;
  float sensorVal = analogRead(WINDDIRECTION_SENSOR_PIN);
  voltage = sensorVal/ 4095.0 * 3.3;
  currentmA = (voltage/ WINDDIRECTION_SENSOR_RESISTOR) * 1000;

  if (currentmA == 0.0)
  {
    //result = "Windrichting is offline"; kan nog eens dienen voor recupel
    return 0;
  }

  else if (currentmA >= 17.7 || currentmA < 4.2)
  {
    result = "Noord (N)";
  }

  else if (currentmA >= 4.2 && currentmA < 6)
  {
    result = "Noord-Oost (NE)";
  }

  else if (currentmA >= 6 && currentmA < 8)
  {
    result = "Oost (E)";
  }

  else if (currentmA >= 8 && currentmA < 9.8)
  {
    result = "Zuid-Oost (SE)";
  }

  else if (currentmA >= 9.8 && currentmA < 11.8)
  {
    result = "Zuid (S)";
  }

  else if (currentmA >= 11.8 && currentmA < 13.4)
  {
    result = "Zuid-West (SW)";
  }

  else if (currentmA >= 13.4 && currentmA < 15.4)
  {
    result ="West (W)";
  }

  else if (currentmA >= 15.4 && currentmA < 17.7)
  {
    result = "Noord-West (NW)";
  }

  else{
    result = "Er is iets mis";
   
  }
  windDirection = result;
  
  return 1;
  
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
  windConnected = determineWindDirection();
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

  if (windSpeed >= 0) // check if value exists
  {
    doc["windspeed(Km/h) "] = windSpeed;
  }
  if(windConnected)
  {
    doc["winddirection()"]= windDirection;
    
  }
  doc["battery(%)"] = (69.69);
  
  serializeJson(doc, buf,1000);
  
}
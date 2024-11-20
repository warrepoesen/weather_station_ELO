#define WINDSPEED_SENSOR_PIN 36       // data wind
#define WINDSPEED_SENSOR_RESISTOR 120 // weerstandswaarde ingeven wind
#define SEALEVELPRESSURE_HPA (1013.25)
#include "Adafruit_BME280.h"

#pragma once 
extern Adafruit_BME280 bme;

extern float temperature;
extern float humidity;
extern float pressure;
extern float windSpeed;


float readWindSpeed();
void readValues();
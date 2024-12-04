
#include "Adafruit_BME280.h"

#define WINDSPEED_SENSOR_PIN 36       // data wind
#define WINDSPEED_SENSOR_RESISTOR 120 // weerstandswaarde ingeven wind
#define SEALEVELPRESSURE_HPA (1013.25)
#define WINDDIRECTION_SENSOR_PIN 39
#define WINDDIRECTION_SENSOR_RESISTOR 121



#pragma once 
extern Adafruit_BME280 bme;

extern float temperature;
extern float humidity;
extern float pressure;
extern float windSpeed;
extern String windDirection;
extern bool windConnected;

bool determineWindDirection();
float readWindSpeed();
void readValues();//reads all values and puts them in above variables.
void serializeValues(char * buf); // serializes a json doc with all the above variables into a buf to prep for mqtt
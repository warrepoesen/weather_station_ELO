
#include "Adafruit_BME280.h"

#define WINDSPEED_SENSOR_PIN 36       // data wind
#define WINDSPEED_SENSOR_RESISTOR 120 // weerstandswaarde ingeven wind
#define SEALEVELPRESSURE_HPA (1013.25)
#define WINDDIRECTION_SENSOR_PIN 39
#define WINDDIRECTION_SENSOR_RESISTOR 121
#define WATERPERTIP  0.165
#define REED_PIN 14 //reed contact pin

#define PIN_CS 21  // Chip Select (GPIO 21 gekozen uit de bovenste rijen)
#define PIN_MISO 34 // Master In Slave Out
#define PIN_MOSI 23 // Master Out Slave In
#define PIN_SCK 22  // Serial Clock
#define SPI_FREQ 1000000  



#pragma once 
extern Adafruit_BME280 bme;

extern float temperature;
extern float humidity;
extern float pressure;
extern float windSpeed;
extern String windDirection;
extern bool windConnected;
extern float PM1;
extern float PM2_5;
extern float PM10;


extern RTC_DATA_ATTR int tipCount; 
extern RTC_DATA_ATTR unsigned long lastWakeTime;
extern float totalRainfall;

void alpha_setupSPI();
bool alpha_on();
bool alpha_off();
float calculate_float(uint8_t val0, uint8_t val1, uint8_t val2, uint8_t val3);
void readPmData();

bool determineWindDirection();
float readWindSpeed();
void readValues();//reads all values and puts them in above variables.
void serializeValues(char * buf); // serializes a json doc with all the above variables into a buf to prep for mqtt

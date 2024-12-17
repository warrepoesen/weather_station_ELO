#include "sensor.h"

#include <Adafruit_BME280.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <SPI.h>

#include "mac.h"

float temperature;
float humidity;
float pressure;
float windSpeed;
String windDirection;
bool windConnected;
float battery;
float PM1;
float PM2_5;
float PM10;
float totalRainfall;

RTC_DATA_ATTR int tipCount = 0; //pluvio 
RTC_DATA_ATTR unsigned long lastWakeTime = 0;

Adafruit_BME280 bme;

void alpha_setupSPI()
{

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);

  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  Serial.println("\n");
  delay(1000);
}

bool alpha_on()
{

  byte command[] = {0x03, 0x00};
  byte inData[2];
  int expected[] = {243, 3};
  bool OPC = 0;

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  SPI.beginTransaction(SPISettings(300000, MSBFIRST, SPI_MODE1));
  digitalWrite(PIN_CS, LOW);
  inData[0] = SPI.transfer(command[0]);
  delay(10);
  inData[1] = SPI.transfer(command[1]);
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();

  if (inData[0] == expected[0] & inData[1] == expected[1])
  {
    OPC = 1;
  }

  return OPC;
}

bool alpha_off()
{
  byte command[] = {0x03, 0x01};
  byte inData[2];
  int expected[] = {243, 3};
  bool OPC = 1;

  Serial.println("Turning OPC off...");
  SPI.beginTransaction(SPISettings(300000, MSBFIRST, SPI_MODE1));
  digitalWrite(PIN_CS, LOW);
  inData[0] = SPI.transfer(command[0]);
  delay(10);
  inData[1] = SPI.transfer(command[1]);
  digitalWrite(PIN_CS, HIGH);

  if (inData[0] == expected[0] & inData[1] == expected[1])
  {
    OPC = 0;
    Serial.println("gelukt");
    SPI.end();
  }
  else
  {
    OPC = 1;
    Serial.println("niet gelukt");
    delay(3000);
  }

  return OPC;
}

float calculate_float(uint8_t val0, uint8_t val1, uint8_t val2, uint8_t val3)
{
  union
  {
    uint8_t b[4];
    float val;
  } u;

  u.b[0] = val0;
  u.b[1] = val1;
  u.b[2] = val2;
  u.b[3] = val3;

  return u.val;
}

void readPmData()
{
  byte command[] = {0x32, 0x00};

  uint8_t data[12];

  digitalWrite(PIN_CS, LOW);
  SPI.transfer(command[0]);
  digitalWrite(PIN_CS, HIGH);
  delay(12);

  digitalWrite(PIN_CS, LOW);

  for (int i = 0; i < 12; i++)
  {
    data[i] = SPI.transfer(command[1]);
    delay(4);
  }

  digitalWrite(PIN_CS, HIGH);

  PM1 = calculate_float(data[0], data[1], data[2], data[3]) / 10;
  PM2_5 = calculate_float(data[4], data[5], data[6], data[7]) / 10;
  PM10 = calculate_float(data[8], data[9], data[10], data[11]) / 10;

  Serial.println("PM-waarden:");
  Serial.print("PM1: ");
  Serial.print(PM1);
  Serial.println(" µg/m³");

  Serial.print("PM2.5: ");
  Serial.print(PM2_5);
  Serial.println(" µg/m³");

  Serial.print("PM10: ");
  Serial.print(PM10);
  Serial.println(" µg/m³");
}

float readWindSpeed()
{
  pinMode(WINDSPEED_SENSOR_PIN, INPUT);
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
      windSpeed = 0;
    return windSpeed;
  }
}

bool determineWindDirection()
{
  pinMode(WINDDIRECTION_SENSOR_PIN, INPUT);
  float currentmA, voltage;
  String result;
  float sensorVal = analogRead(WINDDIRECTION_SENSOR_PIN);
  voltage = sensorVal / 4095.0 * 3.3;
  currentmA = (voltage / WINDDIRECTION_SENSOR_RESISTOR) * 1000;

  if (currentmA == 0.0)
  {
    // result = "Windrichting is offline"; kan nog eens dienen voor recupel
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
    result = "West (W)";
  }

  else if (currentmA >= 15.4 && currentmA < 17.7)
  {
    result = "Noord-West (NW)";
  }

  else
  {
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
  if (alpha_on()) // enables the sensor en retuns true if succes otherwise false
  {
    readPmData(); // reads data and puts it in the global variables
  }

  windSpeed = readWindSpeed();
  windConnected = determineWindDirection();
}

void serializeValues(char *buf)
{
  battery = 69.69;
  JsonDocument doc;
  doc["t"] = "m";
  doc["topic"] = measureTopic;
  temperature = int(round(temperature * 100.00)) / 100.00; // round everything to 2 numbers
  humidity = int(round(humidity * 100.00)) / 100.00;
  pressure = int(round(pressure * 100.00)) / 100.00;
  PM1 = int(round(PM1 * 100.00)) / 100.00;
  PM2_5 = int(round(PM2_5 * 100.00)) / 100.00;
  PM10 = int(round(PM10 * 100.00)) / 100.00;
  windSpeed = int(round(windSpeed * 100.00)) / 100.00;
  battery = int(round(battery * 100.00)) / 100.00;

  if (bme.checkConnection(0x76))
  {
    doc["temperature(C)"] = temperature;
    doc["humidity(%)"] = humidity;
    doc["pressure(HPa)"] = pressure;
  }

  if (windSpeed >= 0) // check if value exists
  {
    doc["windspeed(Km/h)"] = windSpeed;
  }
  if (windConnected)
  {
    doc["winddirection()"] = windDirection;
  }
  if (alpha_on)
  {
    doc["PM1(µg/m³)"] = PM1;
    doc["PM2.5(µg/m³)"] = PM2_5;
    doc["PM10(µg/m³)"] = PM10;
  }
  doc["rainfall(mm)"]=totalRainfall;
  doc["battery(%)"] = battery;

  serializeJson(doc, buf, 1000);
}
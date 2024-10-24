/*********
  Complete project details at https://randomnerdtutorials.com
*********/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi
#define SSID "ProjectNetwork" // Enter your WiFi name
#define PASSWORD "eloict1234" // Enter WiFi password
WiFiClient espClient;

// MQTT Broker
#define MQTT_SERVER "k106.ucll-labo.be"
#define MQTT_PORT 1883
#define MQTT_USER "project"
#define MQTT_PASSWORD "eloict1234"
char topic[256];
PubSubClient client(espClient);

//sleep 
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */
RTC_DATA_ATTR int bootCount = 0;

// globale variabelen
float temperature;
float humidity;
float pressure;
float windSpeed;
#define WINDSPEED_SENSOR_PIN 36     // data wind
#define WINDSPEED_SENSOR_RESISTOR 120 // weerstandswaarde ingeven wind

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C
// Adafruit_BME280 bme(BME_CS); // hardware SPI
// Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

unsigned long delayTime = 5000;


void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void readMacAddress()
{
  uint8_t baseMac[6];
  esp_err_t ret = esp_base_mac_addr_get(baseMac);
  if (ret == ESP_OK)
  {
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  baseMac[0], baseMac[1], baseMac[2],
                  baseMac[3], baseMac[4], baseMac[5]);
    sprintf(topic, "weatherstations/data/stations/station_%02x%02x%02x%02x%02x%02x/measurement",
            baseMac[0], baseMac[1], baseMac[2],
            baseMac[3], baseMac[4], baseMac[5]);
    Serial.println(topic);
  }
  else
  {
    Serial.println("Failed to read MAC address");
  }
}
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
    float windSpeed = (currentmA - 4) * (108 / 16); // sensor kan tot 30m/s = 108 km/h
    return windSpeed;
  }
}
void printValues()
{
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");

  // Convert temperature to Fahrenheit
  /*Serial.print("Temperature = ");
  Serial.print(1.8 * bme.readTemperature() + 32);
  Serial.println(" *F");*/

  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.println();
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

void publishValues()
{
  JsonDocument doc;
  

  if (bme.checkConnection(0x76))
  {
    doc["temperature(C)"] = temperature;
    doc["humidity(%)"] = humidity;
    doc["pressure(HPa)"] = pressure;
  }

  if (windSpeed >0) // check if value is not NAN
  {
    doc["windspeed(Km/h) "] = windSpeed;
  }

  char buf[1000];
  serializeJson(doc, buf);
  if (bme.checkConnection(0x76) | (windSpeed > 0))
  {
    client.publish(topic, buf, true);
  }
}

void setup()
{
  Serial.begin(115200);
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");

  //zet mosfet aan

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
  bme.begin(0x76);
  

  Serial.println("-- Default Test --");
  readValues();
  publishValues();
  //mosfet uitzetten
  Serial.println("Going to sleep now");
  delay(500);
  Serial.flush(); 
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}
void loop()
{
  readValues();
  publishValues();
  delay(delayTime);
}

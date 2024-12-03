
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>

#include "sensor.h"
#include "gps.h"
#include "sleep.h"
#include "mac.h"

// WiFi
#define SSID "ProjectNetwork" // Enter your WiFi name
#define PASSWORD "eloict1234" // Enter WiFi password
WiFiClient espClient;

// Instellen van SSID en wachtwoord voor het access point
#define AP_SSID "ESP32_AP"
#define AP_PASSWORD "12345678"
WiFiServer server(80);
// HTML webpagina in PROGMEM voor betere prestaties
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP32 Web Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h1>Voer je gegevens in</h1>
  <form action="/get">
    Naam: <input type="text" name="name"><br>
    Wachtwoord: <input type="password" name="password"><br>
    <input type="submit" value="Verzenden">
  </form>
</body>
</html>)rawliteral";

RTC_DATA_ATTR char userName[64] = "";
RTC_DATA_ATTR char userPassword[64] = "";
String inputName;
String inputPassword;

// MQTT Broker
#define MQTT_SERVER "k106.ucll-labo.be"
#define MQTT_PORT 1883
#define MQTT_USER "project"
#define MQTT_PASSWORD "eloict1234"

PubSubClient client(espClient);

// gps
bool gps_on = false;

// setup
RTC_DATA_ATTR bool ap_setup = true;

void serializeInfo(char *buf)
{
  JsonDocument doc;
  doc["name"] = inputName;
  doc["password"] = inputPassword;
  serializeJson(doc, buf, sizeof(buf));
}
void standardFunction()
{

  // zet mosfet aan

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
  if (bootCount == 2)
  {
    Serial.println("Naam: " + String(userName));
    Serial.println("Wachtwoord: " + String(userPassword));
    char buf0[1000];
    serializeInfo(buf0);
    client.publish(infoTopic,buf0,true );
  }

  if (bootCount == 2 && gps_on)
  {
    readGPS();
    delay(500);
    char buf1[1000];
    serializeGPS(buf1);
    client.publish(gpsTopic, buf1, true);
  }

  bme.begin(0x76);

  Serial.println("-- Default Test --");
  readValues();
  if (bme.checkConnection(0x76) | (windSpeed > 0))
  {
    char buf2[1000];
    serializeValues(buf2);
    client.publish(measureTopic, buf2, true);
  }

  // mosfet uitzetten
}


void AP_setup()
{
  // Configureren van een statische IP om netwerkvertraging te verminderen
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  // Starten van het access point
  WiFi.softAP(AP_SSID, AP_PASSWORD, 1, false, 1); // Maximaal 1 clients toegestaan

  Serial.print("Access Point gestart. IP-adres: ");
  Serial.println(WiFi.softAPIP());

  server.begin();

  // Initialiseer mDNS
  if (MDNS.begin("meetstation"))
  { // Hostnaam instellen
    Serial.println("mDNS responder gestart. Toegang via http://meetstation.local");
  }
  else
  {
    Serial.println("mDNS responder starten mislukt!");
  }
}
void AP_loop()
{
  WiFiClient client = server.available();

  if (client)
  {
    Serial.println("Nieuwe client verbonden.");
    bool currentLineIsBlank = true;
    String header = "";

    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        header += c;

        // Detecteer einde van HTTP-verzoek
        if (c == '\n' && currentLineIsBlank)
        {
          // Verwerk GET-verzoek
          if (header.startsWith("GET /get?"))
          {
            int nameStart = header.indexOf("name=") + 5;
            int nameEnd = header.indexOf('&', nameStart);
            int passwordStart = header.indexOf("password=") + 9;

            inputName = header.substring(nameStart, nameEnd);
            inputPassword = header.substring(passwordStart, header.indexOf(' ', passwordStart));

            // // Decodeer URL-encoding (spaties, speciale tekens)
            // inputName.replace("+", " ");
            // inputPassword.replace("+", " ");
            // inputName.replace("%20", " ");
            // inputPassword.replace("%20", " ");

            Serial.println("Naam: " + inputName);
            Serial.println("Wachtwoord: " + inputPassword);
            inputName.toCharArray(userName, sizeof(userName));
            inputPassword.toCharArray(userPassword, sizeof(userPassword));
            ap_setup = false;
          }

          // Stuur HTML-response
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close");
          client.println();
          client.println(index_html);

          break;
        }

        if (c == '\n')
        {
          currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
          currentLineIsBlank = false;
        }
      }
    }

    // Verbreek de verbinding met de client
    client.stop();
    Serial.println("Client verbinding gesloten.");
  }
}

void setup()
{

  Serial.begin(115200);

  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();

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

  AP_setup(); // setup= true ==> this happens otherwise it will already sleep here.
}
void loop()
{

  AP_loop();
  if (!ap_setup) // if setup = true dont do this
  {
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

    

    Serial.println("Going to sleep now");
    delay(500);
    Serial.flush();
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
  }
}

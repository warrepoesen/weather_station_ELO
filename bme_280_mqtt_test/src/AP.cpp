#include "AP.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

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

// setup
RTC_DATA_ATTR bool ap_setup = true;
RTC_DATA_ATTR bool ap_complete = false;


void serializeInfo(char *buf)
{
  JsonDocument doc;
  doc["topic"]="info";
  doc["name"] = userName;
  doc["password"] = userPassword;
  serializeJson(doc, buf,1000);
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
  String inputName;
  String inputPassword;
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

           

            Serial.println("Naam: " + inputName);
            Serial.println("Wachtwoord: " + inputPassword);
            inputName.toCharArray(userName, sizeof(userName));
            inputPassword.toCharArray(userPassword, sizeof(userPassword));
            ap_setup = false;
            ap_complete = true;
            // Stuur HTTP-redirect terug naar de root URL
            client.println("HTTP/1.1 303 See Other");
            client.println("Location: /");
            client.println("Connection: close");
            client.println();
            
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
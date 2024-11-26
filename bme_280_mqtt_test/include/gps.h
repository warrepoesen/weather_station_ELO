#include <esp_attr.h>
#define RXD2 19 // aangepaste pinnen om UART2 te gebruiken
#define TXD2 23
#define GPS_BAUD 9600

extern RTC_DATA_ATTR double longitude;
extern RTC_DATA_ATTR double latitude;

void serializeGPS(char * buf);
void readGPS();
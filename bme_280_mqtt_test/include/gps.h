#define RXD2 19 // aangepaste pinnen om UART2 te gebruiken
#define TXD2 23
#define GPS_BAUD 9600

void publishGPS(char * buf);
void readGPS();
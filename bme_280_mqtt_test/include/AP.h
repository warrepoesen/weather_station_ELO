#include <esp_attr.h>

extern RTC_DATA_ATTR char userName[64];
extern RTC_DATA_ATTR char userPassword[64];
extern RTC_DATA_ATTR bool ap_setup;
extern RTC_DATA_ATTR bool ap_complete;

void serializeInfo(char *buf);
void AP_setup();
void AP_loop();
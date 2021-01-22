#ifndef chipSetup_h
#define chipSetup_h

#define TRIGGER_PIN 0
#define RESET_TIMEOUT 5000  // Time in ms required to press flash button (GPIO0)

#define HOSTNAME_DEFAULT "emeter"
#define HOSTNAME_MAXLENGTH 32

#define HTTP_PORT 80

void tick();

void saveConfigCallback();

void wifiSetup();

void resetChipOnTrigger();

#endif
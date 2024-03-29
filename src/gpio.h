#include <PubSubClient.h>
#include <WiFi.h>//####ESP32

#define NUMGPIO 3

struct gpioSettingsStruct {
  unsigned int gpioPin[NUMGPIO] = {1, 3, 16};
  unsigned int gpioMode[NUMGPIO] = {INPUT_PULLUP, INPUT_PULLUP, INPUT_PULLUP};
};

void setupGPIO(gpioSettingsStruct gpioSettings);

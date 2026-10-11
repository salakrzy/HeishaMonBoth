#if defined(ESP8266)
#define NUMGPIO 3
#include <ESP8266WiFi.h>
#include <ESP8266WiFiGratuitous.h>
#elif defined(ESP32)
#define NUMGPIO 22
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Update.h>
#define relayOnePin 18
#define relayTwoPin 19
#endif

extern const char* mqtt_topic_gpio;

struct gpioSettingsStruct {
#if defined(ESP8266)
  unsigned int gpioPin[NUMGPIO] = {1, 3, 16};
  unsigned int gpioMode[NUMGPIO] = {INPUT_PULLUP, INPUT_PULLUP, INPUT_PULLUP};
#elif defined(ESP32)
  unsigned int gpioPin[NUMGPIO] = {4,5,15, 18, 19,21,22,23 ,32,36,39,
    4,5,15, 18, 19,21,22,23,32,36,39};
  unsigned int gpioMode[NUMGPIO] = {INPUT_PULLUP,INPUT_PULLUP, INPUT_PULLUP,OUTPUT, OUTPUT, INPUT_PULLUP,INPUT_PULLUP, INPUT_PULLUP, INPUT_PULLUP,INPUT, INPUT,
    INPUT_PULLUP, OUTPUT, INPUT_PULLUP, OUTPUT,OUTPUT,INPUT_PULLUP, INPUT_PULLUP,OUTPUT, INPUT_PULLUP, INPUT, INPUT};
#endif
};

void setupGPIO(gpioSettingsStruct gpioSettings);
void mqttGPIOCallback(char* topic, char* value);



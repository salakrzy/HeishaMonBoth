#if defined(ESP8266)
#define NUMGPIO 3
#include <ESP8266WiFi.h>
#include <ESP8266WiFiGratuitous.h>
#elif defined(ESP32)
#define NUMGPIO 7
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
  unsigned int gpioPin[NUMGPIO] = {5,18,19,21,22,23,32};
  unsigned int gpioMode[NUMGPIO] = {OUTPUT,  OUTPUT,OUTPUT,INPUT_PULLUP, INPUT_PULLUP, INPUT_PULLUP, INPUT_PULLUP, };
#endif
};

void setupGPIO(gpioSettingsStruct gpioSettings);
void mqttGPIOCallback(char* topic, char* value);

/*
ESP32 GPIO def https://lastminuteengineers.com/esp32-wroom-32-pinout-reference/?utm_content=cmp-true  
#define ENABLEPIN 2 //RS485 not used so we redirect  GPIO from 5 to 2  for compatibility
#define ENABLEOTPIN 4 // OpenTherm enable/disable
#define BOOTPIN 0 // Boot jumper

#define ETH_TYPE        ETH_PHY_W5500
#define ETH_ADDR        1

// ETH pins
#define ETH_CS          25
#define ETH_IRQ         33
#define ETH_RST         35

// SPI pins
#define ETH_SPI_SCK     14
#define ETH_SPI_MISO    34
#define ETH_SPI_MOSI    12

GPIO2 pin1 ESP_GPIO_12,  ESP_pin14  must be LOW during boot
GPIO2 pin2 ESP_GPIO_14,  ESP_pin13
GPIO2 pin3 ESP_GPIO_25,  ESP_pin10
GPIO2 pin4 ESP_GPIO_33,  ESP_pin9
GPIO2 pin5 ESP_GPIO_34,  ESP_pin6   must be Input
GPIO2 pin6 ESP_GPIO_35,  ESP_pin7   must be Input
GPIO2 pin7 GND,          ESP_pin1,15,38
GPIO2 pin8 +5V,         

GPIO1 pin1 ESP_GPIO_21   ESP_pin33 INPUT Schmidt trigger
GPIO1 pin2 ESP_GPIO_19   ESP_pin31
GPIO1 pin3 ESP_GPIO_18,  ESP_pin30

1Wire      ESP_GPIO32    ESP_pin8 INPUT

Counters pin1 GND
Counters pin2 ESP_GPIO_23 ESP_pin37 INPUT Schmidt trigger
Counters pin3 ESP_GPIO_22 ESP_pin36 INPUT Schmidt trigger

Boot pin1 ESP_GPIO_05 ESP_pin29 must be HIGH during boot 
Boot pin2 ESP_GPIO_00 ESP_pin5 INPUT must be HIGH during boot and LOW for programming
Boot pin3  GND 

SERIAL PORTS DESCRIPTION
Panasonic       ESP_GPIO_27 Rx   ESP_GPIO_26 Tx,  (hardware serial)
CZTAW1          ESP_GPIO_16 Rx   ESP_GPIO_17 Tx,  (hardware serial)
loggingSerial    ESP_GPIO_03 Rx   ESP_GPIO_1 Tx,   (hardware serial)
ModbusSerial    ESP_GPIO_21 Rx   ESP_GPIO_19 Tx,  (software serial)
*/
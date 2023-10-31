#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <DNSServer.h>
#include <Ticker.h>
#include <TelnetStream.h>
#include <TimeLib.h>
#include <sntp.h>

///#define TIME_ZONE TZ_Europe_Warsaw
#define TZ_Europe_Warsaw	"CET-1CEST,M3.5.0,M10.5.0/3"

#define MAXDATASIZE 256
#define QUERYSIZE 110

#define UPDATEALLTIME 300000 // time to resend all to mqtt
#define MQTT_RETAIN_VALUES 1

// config your timing
#define COMMANDTIMER 50 // Command / timer to send commands from buffer to HP
#define QUERYTIMER 10000   // Query / timer to initiate a query
#define BUFFERTIMEOUT 500 // Serial Buffer Filltime / timer to fill the UART buffer with all 203 bytes from HP board
#define SERIALTIMEOUT 600 // Serial Timout / timer to wait on serial to read all 203 bytes from HP
#define RESTARTTIMEOUT 60000 // restart ESP32 if nothing received by Serial1 Rx

void send_pana_command(void);
void send_pana_mainquery(void);
void read_pana_data(void);
void timeout_serial(void);
void write_mqtt_log(char *);
void write_telnet_log(char *);
bool register_new_command(byte);
void restartESP32(void);

#define LWIP_INTERNAL

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <littleFS.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "lwip/apps/sntp.h"
#include "src/common/timerqueue.h"
#include "src/common/stricmp.h"
#include "src/common/log.h"
#include "src/rules/rules.h"
#include "webfunctions.h"
#include "decode.h"
#include "commands.h"
#include "rules.h"

DNSServer dnsServer;

//to read bus voltage in stats
// ADC_MODE(ADC_VCC);  not yet supported for ESP32

// WiFi LOG Level Use from 0 to 4. Higher number, more debugging messages and memory usage.
#define _WIFIMGR_LOGLEVEL_    4
#define USE_LITTLEFS    true 
#define ESP_DRD_USE_LITTLEFS    true
#define ESP_DRD_USE_SPIFFS      false
#define ESP_DRD_USE_EEPROM      false

#define DEBUG_TEST true
#define DOUBLERESETDETECTOR_DEBUG       true  
#include <ESP_DoubleResetDetector.h>
// maximum number of seconds between resets that
// counts as a double reset
#define DRD_TIMEOUT 2

// address to the block in the RTC user memory
// change it if it collides with another usageb
// of the address block
#define DRD_ADDRESS 0x00

const byte DNS_PORT = 53;

#define SERIALTIMEOUT 2000 // wait until all 203 bytes are read, must not be too long to avoid blocking the code

settingsStruct heishamonSettings;

bool sending = false; // mutex for sending data
bool mqttcallbackinprogress = false; // mutex for processing mqtt callback

#define MQTTRECONNECTTIMER 30000 //it takes 30 secs for each mqtt server reconnect attempt
unsigned long lastMqttReconnectAttempt = 0;

#define WIFIRETRYTIMER 15000 // switch between hotspot and configured SSID each 10 secs if SSID is lost
unsigned long lastWifiRetryTimer = 0;

unsigned long lastRunTime = 0;
unsigned long lastOptionalPCBRunTime = 0;

unsigned long sendCommandReadTime = 0; //set to millis value during send, allow to wait millis for answer
unsigned long goodreads = 0;
unsigned long totalreads = 0;
unsigned long cztaw_totalreads = 0;
unsigned long badcrcread = 0;
unsigned long badheaderread = 0;
unsigned long tooshortread = 0;
unsigned long toolongread = 0;
unsigned long timeoutread = 0;
float readpercentage = 0;
static int uploadpercentage = 0;

// instead of passing array pointers between functions we just define this in the global scope
#define MAXDATASIZE 255
#define CZTAW 1 // Pump answer is forwarded to CZTAW
#define CZTAW_NOT 0
int forward_cztaw=CZTAW_NOT;
char data[MAXDATASIZE] = { '\0' };
byte data_length = 0;
char serial2_data[MAXDATASIZE]={ '\0' };// Serial2 connected to CZ_TAW
byte serial2_length = 0;
char initialResponse[51]={'\0'};

// store actual data
String openTherm[2];
char actData[DATASIZE] = { '\0' };
#define OPTDATASIZE 20
char actOptData[OPTDATASIZE]  = { '\0' };
String RESTmsg = "";

// log message to sprintf to
char log_msg[256];

// mqtt topic to sprintf and then publish to
char mqtt_topic[256];

static int mqttReconnects = 0;

// can't have too much in buffer due to memory shortage
#define MAXCOMMANDSINBUFFER 10

// buffer for commands to send
struct cmdbuffer_t {
  uint8_t length;
  uint8_t source;  // Request send from  source ( HeishMon  or CZTAW)
  byte data[128];
} cmdbuffer[MAXCOMMANDSINBUFFER];

static uint8_t cmdstart = 0;
static uint8_t cmdend = 0;
static uint8_t cmdnrel = 0;

//doule reset detection
DoubleResetDetector* drd;

// mqtt
WiFiClient mqtt_wifi_client;
PubSubClient mqtt_client(mqtt_wifi_client);

bool firstConnectSinceBoot = true; //if this is true there is no first connection made yet

struct timerqueue_t **timerqueue = NULL;
int timerqueue_size = 0;

/*
    check_wifi will process wifi reconnecting managing
*/

void setupOTA() {
//   ArduinoOTA.setPort(8266);				Port defaults to 8266
  ArduinoOTA.setPort(3232);              // Port defaults for esp32

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(heishamonSettings.wifi_hostname);

  // Set authentication
  ArduinoOTA.setPassword(heishamonSettings.ota_password);

  ArduinoOTA.onStart([]() {
  });
  ArduinoOTA.onEnd([]() {
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {

  });
  ArduinoOTA.onError([](ota_error_t error) {

  });
  ArduinoOTA.begin();
}

void check_wifi()
{
#define D_CH_WIFI true
  if ((WiFi.status() != WL_CONNECTED) && (WiFi.localIP()))  {
    // special case where it seems that we are not connect but we do have working IP (causing the -1% wifi signal), do a reset.
    log_message((char *)"Weird case, WiFi seems disconnected but is not. Resetting WiFi!");
    setupWifi(&heishamonSettings);
  } else if ((WiFi.status() != WL_CONNECTED) || (!WiFi.localIP()))  {
    /*
        if we are not connected to an AP
        we must be in softAP so respond to DNS
    */
    dnsServer.processNextRequest();

    /* we need to stop reconnecting to a configured wifi network if there is a hotspot user connected
        also, do not disconnect if wifi network scan is active
    */
    if ((heishamonSettings.wifi_ssid[0] != '\0') && (WiFi.status() != WL_DISCONNECTED) && (WiFi.scanComplete() != -1) && (WiFi.softAPgetStationNum() > 0))  {
      log_message("WiFi lost, but softAP station connecting, so stop trying to connect to configured ssid...");
      if (true==WiFi.disconnect(true)){
        log_e("Wifi diconected TRUE, WiFi Status %i= ",WiFi.status());    
      }else{
        log_e("Wifi diconected FALSE");
      }
    }
    /*  only start this routine if timeout on
        reconnecting to AP and SSID is set
    */
    if ((heishamonSettings.wifi_ssid[0] != '\0') && ((unsigned long)(millis() - lastWifiRetryTimer) > WIFIRETRYTIMER ) ) {
      lastWifiRetryTimer = millis();
      if ((WiFi.softAPSSID() == "") or (WiFi.softAPSSID() !="HeishaMon-Setup")) {  // ESP32 set name soft AP ESP-XXXXXX not NULL
        log_message("WiFi lost, starting setup hotspot...");
        WiFi.softAP((char*)"HeishaMon-Setup");
        Serial.println(WiFi.softAPSSID());
      }
      if ((WiFi.status() != WL_DISCONNECTED)  && (WiFi.softAPgetStationNum() == 0 )) {
        log_message("Retrying configured WiFi, ...");
        if (heishamonSettings.wifi_password[0] == '\0') {
          WiFi.begin(heishamonSettings.wifi_ssid);
        } else {
          WiFi.begin(heishamonSettings.wifi_ssid, heishamonSettings.wifi_password);
        }
      } else {
        log_message("Reconnecting to WiFi failed. Waiting a few seconds before trying again.");
        WiFi.disconnect(true);
      }
    }
  } 
  else { //WiFi Status=WL_CONNECTED  and IP>0  check if active AP and disable if yes
  if((WiFi.getMode() & WIFI_MODE_AP) != 0) {
    // ESP8266   if (WiFi.softAPSSID() != "") {
    log_message("WiFi (re)connected, shutting down hotspot...");
    WiFi.softAPdisconnect(true);
  // ESP8266      MDNS.notifyAPChange();
  }

    if (firstConnectSinceBoot) { // this should start only when softap is down or else it will not work properly so run after the routine to disable softap
      firstConnectSinceBoot = false;
      lastMqttReconnectAttempt = 0; //initiate mqtt connection asap
      setupOTA();
      MDNS.begin(heishamonSettings.wifi_hostname);
      MDNS.addService("http", "tcp", 80);
      // ESP8266 experimental::ESP8266WiFiGratuitous::stationKeepAliveSetIntervalMs(5000); //necessary for some users with bad wifi routers

      if (heishamonSettings.wifi_ssid[0] == '\0') {
        log_message("WiFi connected without SSID and password in settings. Must come from persistent memory. Storing in settings.");
        WiFi.SSID().toCharArray(heishamonSettings.wifi_ssid, 40);
        WiFi.psk().toCharArray(heishamonSettings.wifi_password, 40);
        DynamicJsonDocument jsonDoc(1024);
        settingsToJson(jsonDoc, &heishamonSettings); //stores current settings in a json document
        saveJsonToConfig(jsonDoc); //save to config file
      }

      ntpReload(&heishamonSettings);
    }

    /*
       always update if wifi is working so next time on ssid failure
       it only starts the routine above after this timeout
    */
    lastWifiRetryTimer = millis();

    // Allow MDNS processing
  // ESP8266    MDNS.update();
  }
}

void mqtt_reconnect()
{
  unsigned long now = millis();
  if ((lastMqttReconnectAttempt == 0) || ((unsigned long)(now - lastMqttReconnectAttempt) > MQTTRECONNECTTIMER)) { //only try reconnect each MQTTRECONNECTTIMER seconds or on boot when lastMqttReconnectAttempt is still 0
    lastMqttReconnectAttempt = now;
    log_message("Reconnecting to mqtt server ...");
    char topic[256];
    sprintf(topic, "%s/%s", heishamonSettings.mqtt_topic_base, mqtt_willtopic);
    if (mqtt_client.connect(heishamonSettings.wifi_hostname, heishamonSettings.mqtt_username, heishamonSettings.mqtt_password, topic, 1, true, "Offline"))
    {
      mqttReconnects++;

      mqtt_client.subscribe("panasonic_heat_pump/opentherm/#");
      sprintf(topic, "%s/%s/#", heishamonSettings.mqtt_topic_base, mqtt_topic_commands);
      mqtt_client.subscribe(topic);
      sprintf(topic, "%s/%s", heishamonSettings.mqtt_topic_base, mqtt_send_raw_value_topic);
      mqtt_client.subscribe(topic);
      sprintf(topic, "%s/%s", heishamonSettings.mqtt_topic_base, mqtt_willtopic);
      mqtt_client.publish(topic, "Online");
      sprintf(topic, "%s/%s", heishamonSettings.mqtt_topic_base, mqtt_iptopic);
      mqtt_client.publish(topic, WiFi.localIP().toString().c_str(), true);

      if (heishamonSettings.use_s0) { // connect to s0 topic to retrieve older watttotal from mqtt
        sprintf_P(mqtt_topic, PSTR("%s/%s/WatthourTotal/1"), heishamonSettings.mqtt_topic_base, mqtt_topic_s0);
        mqtt_client.subscribe(mqtt_topic);
        sprintf_P(mqtt_topic, PSTR("%s/%s/WatthourTotal/2"), heishamonSettings.mqtt_topic_base, mqtt_topic_s0);
        mqtt_client.subscribe(mqtt_topic);
      }
      if (mqttReconnects == 1) { //only resend all data on first connect to mqtt so a data bomb like and bad mqtt server will not cause a reconnect bomb everytime
        if (heishamonSettings.use_1wire) resetlastalldatatime_dallas(); //resend all 1wire values to mqtt
        resetlastalldatatime(); //resend all heatpump values to mqtt
      }
    }
  }
}

void log_message(const __FlashStringHelper *msg) {
  PGM_P p = (PGM_P)msg;
  int len = strlen_P((const char *)p);
  char *str = (char *)MALLOC(len + 1);
  if (str == NULL) {
    OUT_OF_MEMORY
  }
  strcpy_P(str, p);

  log_message(str);

  FREE(str);
}

void log_message(char* string)
{
  time_t rawtime;
  rawtime = time(NULL);
  struct tm *timeinfo = localtime(&rawtime);
  char timestring[32];
  strftime(timestring, 32, "%c", timeinfo);
  size_t len = strlen(string) + strlen(timestring) + 20; //+20 long enough to contain millis()
  char* log_line = (char *) malloc(len);
  snprintf(log_line, len, "%s (%lu): %s", timestring, millis(), string);

  if (heishamonSettings.logSerial) {
    Serial.println(log_line);
  }
  if (heishamonSettings.logMqtt && mqtt_client.connected())
  {
    char log_topic[256];
    sprintf(log_topic, "%s/%s", heishamonSettings.mqtt_topic_base, mqtt_logtopic);

    if (!mqtt_client.publish(log_topic, log_line)) {
      if (heishamonSettings.logSerial) {
        Serial.print(millis());
        Serial.print(": ");
        Serial.println("MQTT publish log message failed!");
      }
      mqtt_client.disconnect();
    }
  }
  websocket_write_all(log_line, strlen(log_line));
  free(log_line);
}

void logHex(char *hex, byte hex_len) {
#define LOGHEXBYTESPERLINE 32  // please be aware of max mqtt message size
  for (int i = 0; i < hex_len; i += LOGHEXBYTESPERLINE) {
    char buffer [(LOGHEXBYTESPERLINE * 3) + 1];
    buffer[LOGHEXBYTESPERLINE * 3] = '\0';
    for (int j = 0; ((j < LOGHEXBYTESPERLINE) && ((i + j) < hex_len)); j++) {
      sprintf(&buffer[3 * j], "%02X ", hex[i + j]);
    }
    sprintf_P(log_msg, PSTR("data: %s"), buffer ); log_message(log_msg);
  }
}

byte calcChecksum(byte* command, int length) {
  byte chk = 0;
  for ( int i = 0; i < length; i++)  {
    chk += command[i];
  }
  chk = (chk ^ 0xFF) + 01;
  return chk;
}

bool isValidReceiveChecksum(char* serialx_data, byte serialx_length) {
  byte chk = 0;
  for ( int i = 0; i < serialx_length; i++)  {
    chk += serialx_data[i];
  }
  return (chk == 0); //all received bytes + checksum should result in 0
}

void pushCommandBuffer(byte* command, int length, int source) {
  if (cmdnrel + 1 > MAXCOMMANDSINBUFFER) {
    log_message("Too much commands already in buffer. Ignoring this commands.\n");
    return;
  }
  cmdbuffer[cmdend].length = length;
  cmdbuffer[cmdend].source = source;
  memcpy(&cmdbuffer[cmdend].data, command, length);
  cmdend = (cmdend + 1) % (MAXCOMMANDSINBUFFER);
  cmdnrel++;
}



bool send_command(byte* command, int length, int source) {

  if ( heishamonSettings.listenonly and source!=CZTAW) {
    log_message("Not sending this command. Heishamon in listen only mode!");
    return false;
  }
  if ( sending ) {
    log_message("Already sending data. Buffering this send request");
    pushCommandBuffer(command, length, source);
    return false;
  }
  else{
   forward_cztaw=source;
  }
  sending = true; //simple semaphore to only allow one send command at a time, semaphore ends when answered data is received

  byte chk = calcChecksum(command, length);
  int bytesSent = Serial1.write(command, length); //first send command
  bytesSent += Serial1.write(chk); //then calculcated checksum byte afterwards
  if(DEBUG_TEST) sprintf_P(log_msg, PSTR("sent bytes: %d including checksum value: %d source; %i\n"), bytesSent, int(chk), forward_cztaw);
  log_message(log_msg);

  if (heishamonSettings.logHexdump) logHex((char*)command, length);
  sendCommandReadTime = millis(); //set sendCommandReadTime when to timeout the answer of this command
  return true;
}

void send_initial_query() {
  send_command(initialQuery, INITIALQUERYSIZE,CZTAW);
  log_message("Requesting initial start query");
   if (heishamonSettings.logHexdump) logHex((char*) initialQuery, INITIALQUERYSIZE);
  delay(300);
}
/*****************************************************************************/
/* Read raw from Serial2                                                      */
/*****************************************************************************/
void  readCzTaw()
{
  int cztaw_len = 0;
	while ((Serial2.available()) and (cztaw_len <MAXDATASIZE)) {
    serial2_data[serial2_length+cztaw_len] = Serial2.read();
    cztaw_len ++;
    if ((serial2_data[0] != 0x71) and  (serial2_data[0] != 0x31) and  (serial2_data[0] != 0xF1)) {
      log_message("CZTAW Received bad header. Ignoring this data!");
      if (heishamonSettings.logHexdump) logHex(serial2_data, cztaw_len);
      serial2_length = 0;
      return; //return so this while loop does not loop forever if there happens to be a continous invalid data stream     
    }
	}

  if ((cztaw_len > 0) && (data_length == 0 )) cztaw_totalreads++; //this is the start of a new read
  serial2_length += cztaw_len;

  if (serial2_length > 1 ) { //should have received length part of header now
    if ((serial2_length > ( serial2_data[1] + 3)) || (serial2_length >= MAXDATASIZE)) {
      sprintf_P(log_msg, PSTR("CZTAW Received %i bytes serial2_data %i\n"), serial2_length, serial2_data[1]);
      log_message(log_msg);
      log_message(F("CZTAW Received more data than header suggests! Ignoring this as this is bad data."));
      if (heishamonSettings.logHexdump) logHex(serial2_data, serial2_length);
      serial2_length = 0;
      return;
    }

    if (serial2_length == (serial2_data[1] + 3)) { //we received all data (serial2_data[1] is header length field)
      sprintf_P(log_msg, PSTR("CZTAW Received %i bytes serial2_data %i"), serial2_length, serial2_data[1]); log_message(log_msg);
      if (! isValidReceiveChecksum(serial2_data,serial2_length) ) {
        log_message(F("CZTAW Checksum received false!"));
        serial2_length = 0; //for next attempt
        return;
      }
      log_message(F("CZTAW Checksum and header received ok!"));
      if ((serial2_data[0]==0x71 or serial2_data[0]==0xF1) and serial2_length == (PANASONICQUERYSIZE+1)) { //decode the normal data
        log_message(F("CZTAW request to Heat Pump"));
        if (heishamonSettings.logHexdump) logHex(serial2_data, serial2_length);
        send_command((byte*) serial2_data,serial2_length-1,CZTAW);
        serial2_length = 0;
        return;
      }
      else if (serial2_data[0]==0x71 and serial2_length == (OPTIONALPCBQUERYSIZE+1)) { //optional pcb acknowledge answer
        log_message(F("CZTAW request to optional PCB . Decoding this in OPT topics."));
        if (heishamonSettings.logHexdump) logHex(serial2_data, serial2_length);
        send_command((byte*) serial2_data,serial2_length-1,CZTAW);
        serial2_length = 0;
        return;
      }
      else if (serial2_data[0]==0x31 and serial2_length == (INITIALQUERYSIZE+1)) { //init heat pump acknowledge answer
        log_message(F("CZTAW INIT request ."));
        if (heishamonSettings.logHexdump) logHex(serial2_data, serial2_length);
        send_initial_query();
        serial2_length = 0;
        return;
      }
      else {
        log_message(F("CZTAW Received a shorter datagram. Can't decode this yet."));
        return;
      }
    }
  }
}

bool readSerial1()
{
  int len = 0;
  while ((Serial1.available()) && (len <= MAXDATASIZE)) {
    data[data_length + len] = Serial1.read(); //read available data and place it after the last received data
    len++;
    if ((data[0] != 0x71) & (data[0] != 0x31)) { //wrong header received!
      log_message("Received bad header. Ignoring this data!");
      if (heishamonSettings.logHexdump) logHex(data, len);
      badheaderread++;
      forward_cztaw=CZTAW_NOT;
      data_length = 0;
    return false; //return so this while loop does not loop forever if there happens to be a continous invalid data stream     
    }
  }

  if ((len > 0) && (data_length == 0 )) totalreads++; //this is the start of a new read
  data_length += len;

  if (data_length > 1) { //should have received length part of header now

    if ((data_length > (data[1] + 3)) || (data_length >= MAXDATASIZE) ) {
      log_message(F("Received more data than header suggests! Ignoring this as this is bad data."));
      if (heishamonSettings.logHexdump) logHex(data, data_length);
      forward_cztaw=CZTAW_NOT;
      data_length = 0;
      toolongread++;
      return false;
    }

    if (data_length == (data[1] + 3)) { //we received all data (data[1] is header length field)
      sprintf_P(log_msg, PSTR("Received %d bytes data"), data_length); log_message(log_msg);
      sending = false; //we received an answer after our last command so from now on we can start a new send request again
      if (heishamonSettings.logHexdump) logHex(data, data_length);
      if (! isValidReceiveChecksum(data,data_length) ) {
        log_message(F("Checksum received false!"));
        forward_cztaw=CZTAW_NOT;
        data_length = 0; //for next attempt
        badcrcread++;
        return false;
      }
      log_message(F("Checksum and header received ok!"));
      if (data_length == DATASIZE) { //decode the normal data
        decode_heatpump_data(data, actData, mqtt_client, log_message, heishamonSettings.mqtt_topic_base, heishamonSettings.updateAllTime);
        memcpy(actData, data, DATASIZE);
        char mqtt_topic[256];
        sprintf(mqtt_topic, "%s/raw/data", heishamonSettings.mqtt_topic_base);
        mqtt_client.publish(mqtt_topic, (const uint8_t *)actData, DATASIZE, false); //do not retain this raw data
        if(forward_cztaw) {
          int bytesSent = Serial2.write(data, DATASIZE); //forward init answer to CZTAW
          forward_cztaw=CZTAW_NOT;
        }
        data_length = 0;
        goodreads++;
        return true;
      }
      else if (data_length == OPTDATASIZE ) { //optional pcb acknowledge answer
        log_message(F("Received optional PCB ack answer. Decoding this in OPT topics."));
        decode_optional_heatpump_data(data, actOptData, mqtt_client, log_message, heishamonSettings.mqtt_topic_base, heishamonSettings.updateAllTime);
        memcpy(actOptData, data, OPTDATASIZE);
        if(forward_cztaw) {
          int bytesSent = Serial2.write(data, OPTDATASIZE); //forward init answer to CZTAW
          forward_cztaw=CZTAW_NOT;
        }        
        data_length = 0;
        goodreads++;
        return true;
      }
      else if (data_length ==  INITIALQUEERYANSWERSIZE) { //init heat pump acknowledge answer
        log_message("Received optional INIT ack answer. .");
        memcpy(initialResponse, data,  INITIALQUEERYANSWERSIZE);
        if (heishamonSettings.logHexdump) logHex(data,  INITIALQUEERYANSWERSIZE);
        if(forward_cztaw) {
          int bytesSent = Serial2.write(data, INITIALQUEERYANSWERSIZE); //forward init answer to CZTAW
          forward_cztaw=CZTAW_NOT;
        }  
        data_length = 0;
        goodreads++;
        return true;
      }
      else {
        log_message(F("Received a shorter datagram. Can't decode this yet."));
        forward_cztaw=CZTAW_NOT;
//        data_length = 0;
        return false;
      }
    }
  }
  return false;
}

void popCommandBuffer() {
  // to make sure we can pop a command from the buffer
  if ((!sending) && cmdnrel > 0) {
    forward_cztaw=cmdbuffer[cmdstart].source;
    send_command(cmdbuffer[cmdstart].data, cmdbuffer[cmdstart].length, cmdbuffer[cmdstart].source);
    cmdstart = (cmdstart + 1) % (MAXCOMMANDSINBUFFER);
    cmdnrel--;
  }
}



// Callback function that is called when a message has been pushed to one of your topics.
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  if (mqttcallbackinprogress) {
    log_message("Already processing another mqtt callback. Ignoring this one");
  }
  else {
    mqttcallbackinprogress = true; //simple semaphore to make sure we don't have two callbacks at the same time
    char msg[length + 1];
    for (unsigned int i = 0; i < length; i++) {
      msg[i] = (char)payload[i];
    }
    msg[length] = '\0';
    char* topic_command = topic + strlen(heishamonSettings.mqtt_topic_base) + 1; //strip base plus seperator from topic
    if (strcmp(topic_command, mqtt_send_raw_value_topic) == 0)
    { // send a raw hex string
      byte *rawcommand;
      rawcommand = (byte *) malloc(length);
      memcpy(rawcommand, msg, length);

      sprintf_P(log_msg, PSTR("sending raw value"));
      log_message(log_msg);
      send_command(rawcommand, length,CZTAW_NOT);
      free(rawcommand);
    } else if (strncmp(topic_command, mqtt_topic_s0, 2) == 0)  // this is a s0 topic, check for watthour topic and restore it
    {
      char* topic_s0_watthour_port = topic_command + 17; //strip the first 17 "s0/WatthourTotal/" from the topic to get the s0 port
      int s0Port = String(topic_s0_watthour_port).toInt();
      float watthour = String(msg).toFloat();
      restore_s0_Watthour(s0Port, watthour);
      //unsubscribe after restoring the watthour values
      char mqtt_topic[256];
      sprintf(mqtt_topic, "%s", topic);
      if (mqtt_client.unsubscribe(mqtt_topic)) {
        log_message("Unsubscribed from S0 watthour restore topic");
      }
    } else if (strncmp(topic_command, mqtt_topic_commands, 8) == 0)  // check for optional pcb commands
    {
      char* topic_sendcommand = topic_command + 9; //strip the first 9 "commands/" from the topic to get what we need
      send_heatpump_command(topic_sendcommand, msg, send_command, log_message, heishamonSettings.optionalPCB);
    } else if (stricmp((char const *)topic, "panasonic_heat_pump/opentherm/Temperature") == 0) {
      char cpy[length + 1];
      memset(&cpy, 0, length + 1);
      strncpy(cpy, (char *)payload, length);
      openTherm[0] = cpy;
      rules_event_cb("temperature");
    } else if (stricmp((char const *)topic, "panasonic_heat_pump/opentherm/Setpoint") == 0) {
      char cpy[length + 1];
      memset(&cpy, 0, length + 1);
      strncpy(cpy, (char *)payload, length);
      openTherm[1] = cpy;

      rules_event_cb("setpoint");
    }
    mqttcallbackinprogress = false;
  }
}


int8_t webserver_cb(struct webserver_t *client, void *dat) {
  switch (client->step) {
    case WEBSERVER_CLIENT_REQUEST_METHOD: {
        if (strcmp_P((char *)dat, PSTR("POST")) == 0) {
          client->route = 110;
        }
        return 0;
      } break;
    case WEBSERVER_CLIENT_REQUEST_URI: {
        if (strcmp_P((char *)dat, PSTR("/")) == 0) {
          client->route = 1;
        } else if (strcmp_P((char *)dat, PSTR("/tablerefresh")) == 0) {
          client->route = 10;
        } else if (strcmp_P((char *)dat, PSTR("/json")) == 0) {
          client->route = 20;
        } else if (strcmp_P((char *)dat, PSTR("/reboot")) == 0) {
          client->route = 30;
        } else if (strcmp_P((char *)dat, PSTR("/debug")) == 0) {
          client->route = 40;
          log_message("Debug URL requested");
        } else if (strcmp_P((char *)dat, PSTR("/wifiscan")) == 0) {
          client->route = 50;
        } else if (strcmp_P((char *)dat, PSTR("/togglelog")) == 0) {
          client->route = 1;
          log_message("Toggled mqtt log flag");
          heishamonSettings.logMqtt ^= true;
        } else if (strcmp_P((char *)dat, PSTR("/togglehexdump")) == 0) {
          client->route = 1;
          log_message("Toggled hexdump log flag");
          heishamonSettings.logHexdump ^= true;
        } else if (strcmp_P((char *)dat, PSTR("/hotspot-detect.html")) == 0 ||
                   strcmp_P((char *)dat, PSTR("/fwlink")) == 0 ||
                   strcmp_P((char *)dat, PSTR("/generate_204")) == 0 ||
                   strcmp_P((char *)dat, PSTR("/gen_204")) == 0 ||
                   strcmp_P((char *)dat, PSTR("/popup")) == 0) {
          client->route = 80;
        } else if (strcmp_P((char *)dat, PSTR("/factoryreset")) == 0) {
          client->route = 90;
        } else if (strcmp_P((char *)dat, PSTR("/command")) == 0) {
          if ((client->userdata = malloc(1)) == NULL) {
            Serial.printf(PSTR("Out of memory %s:#%d\n"), __FUNCTION__, __LINE__);
            ESP.restart();
            exit(-1);
          }
          ((char *)client->userdata)[0] = 0;
          client->route = 100;
        } else if (client->route == 110) {
          // Only accept settings POST requests
          if (strcmp_P((char *)dat, PSTR("/savesettings")) == 0) {
            client->route = 110;
          } else if (strcmp_P((char *)dat, PSTR("/saverules")) == 0) {
            client->route = 170;

            if (LittleFS.begin()) {
              LittleFS.remove("/rules.new");
              client->userdata = new File(LittleFS.open("/rules.new", "a+"));
            }
          } else if (strcmp_P((char *)dat, PSTR("/firmware")) == 0) {
            if (!Update.isRunning()) {
// ESP8266              Update.runAsync(true);
              if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
                Update.printError(Serial);
                return -1;
              } else {
                client->route = 150;
              }
            } else {
              Serial.println(PSTR("New firmware update client, while previous isn't finished yet! Assume broken connection, abort!"));
              Update.end();
              return -1;
            }
          } else {
            return -1;
          }
        } else if (strcmp_P((char *)dat, PSTR("/settings")) == 0) {
          client->route = 120;
        } else if (strcmp_P((char *)dat, PSTR("/getsettings")) == 0) {
          client->route = 130;
        } else if (strcmp_P((char *)dat, PSTR("/firmware")) == 0) {
          client->route = 140;
        } else if (strcmp_P((char *)dat, PSTR("/rules")) == 0) {
          client->route = 160;
        } else {
          client->route = 0;
        }

        return 0;
      } break;
    case WEBSERVER_CLIENT_ARGS: {
        struct arguments_t *args = (struct arguments_t *)dat;
        switch (client->route) {
          case 10: {
              if (strcmp_P((char *)args->name, PSTR("1wire")) == 0) {
                client->route = 11;
              } else if (strcmp_P((char *)args->name, PSTR("s0")) == 0) {
                client->route = 12;
              }
            } break;
          case 100: {
              unsigned char cmd[256] = { 0 };
              char cpy[args->len + 1];
              char log_msg[256] = { 0 };
              unsigned int len = 0;

              memset(&cpy, 0, args->len + 1);
              snprintf((char *)&cpy, args->len + 1, "%.*s", args->len, args->value);

              for (uint8_t x = 0; x < sizeof(commands) / sizeof(commands[0]); x++) {
                cmdStruct tmp;
                memcpy_P(&tmp, &commands[x], sizeof(tmp));
                if (strcmp((char *)args->name, tmp.name) == 0) {
                  len = tmp.func(cpy, cmd, log_msg);
                  if ((client->userdata = realloc(client->userdata, strlen((char *)client->userdata) + strlen(log_msg) + 2)) == NULL) {
                    Serial.printf(PSTR("Out of memory %s:#%d\n"), __FUNCTION__, __LINE__);
                    ESP.restart();
                    exit(-1);
                  }
                  strcat((char *)client->userdata, log_msg);
                  strcat((char *)client->userdata, "\n");
                  log_message(log_msg);
                  send_command(cmd, len,CZTAW_NOT);
                }
              }

              memset(&cmd, 256, 0);
              memset(&log_msg, 256, 0);

              if (heishamonSettings.optionalPCB) {
                //optional commands
                for (uint8_t x = 0; x < sizeof(optionalCommands) / sizeof(optionalCommands[0]); x++) {
                  optCmdStruct tmp;
                  memcpy_P(&tmp, &optionalCommands[x], sizeof(tmp));
                  if (strcmp((char *)args->name, tmp.name) == 0) {
                    len = tmp.func(cpy, log_msg);
                    if ((client->userdata = realloc(client->userdata, strlen((char *)client->userdata) + strlen(log_msg) + 2)) == NULL) {
                      Serial.printf(PSTR("Out of memory %s:#%d\n"), __FUNCTION__, __LINE__);
                      ESP.restart();
                      exit(-1);
                    }
                    strcat((char *)client->userdata, log_msg);
                    strcat((char *)client->userdata, "\n");
                    log_message(log_msg);
                  }
                }
              }

              if (stricmp((char const *)args->name, "temperature") == 0) {
                char cpy[args->len + 1];
                strcpy(cpy, (char *)args->value);
                openTherm[0] = cpy;
                rules_event_cb("temperature");
              } else if (stricmp((char const *)args->name, "setpoint") == 0) {
                char cpy[args->len + 1];
                strcpy(cpy, (char *)args->value);
                openTherm[1] = cpy;
                rules_event_cb("setpoint");
              }
            } break;
          case 110: {
              return cacheSettings(client, args);
            } break;
          case 150: {
              if (Update.isRunning() && (!Update.hasError())) {
                if ((strcmp((char *)args->name, "md5") == 0) && (args->len > 0)) {
                  char md5[args->len + 1];
                  memset(&md5, 0, args->len + 1);
                  snprintf((char *)&md5, args->len + 1, "%.*s", args->len, args->value);
                  sprintf_P(log_msg, PSTR("Firmware MD5 expected: %s"), md5);
                  log_message(log_msg);
                  if (!Update.setMD5(md5)) {
                    log_message("Failed to set expected update file MD5!");
                    Update.end(false);
                  }
                } else if (strcmp((char *)args->name, "firmware") == 0) {
                  if (Update.write((uint8_t *)args->value, args->len) != args->len) {
                    Update.printError(Serial);
                    Update.end(false);
                  } else {
                    if (uploadpercentage != (unsigned int)(((float)client->readlen / (float)client->totallen) * 20)) {
                      uploadpercentage = (unsigned int)(((float)client->readlen / (float)client->totallen) * 20);
                      sprintf_P(log_msg, PSTR("Uploading new firmware: %d%%"), uploadpercentage * 5);
                      log_message(log_msg);
                    }
                  }
                }
              } else {
                log_message((char*)"New firmware POST data but update not running anymore!");
              }
            } break;
          case 170: {
              File *f = (File *)client->userdata;
              if (!f || !*f) {
                client->route = 160;
              } else {
                f->write(args->value, args->len);
              }
            } break;
        }
      } break;
    case WEBSERVER_CLIENT_HEADER: {
        struct arguments_t *args = (struct arguments_t *)dat;
        return 0;
      } break;
    case WEBSERVER_CLIENT_WRITE: {
        switch (client->route) {
          case 0: {
              if (client->content == 0) {
                webserver_send(client, 404, (char *)"text/plain", 13);
                webserver_send_content_P(client, PSTR("404 Not found"), 13);
              }
              return 0;
            } break;
          case 1: {
              return handleRoot(client, readpercentage, mqttReconnects, &heishamonSettings);
            } break;
          case 10:
          case 11:
          case 12: {
              return handleTableRefresh(client, actData);
            } break;
          case 20: {
              return handleJsonOutput(client, actData);
            } break;
          case 30: {
              return handleReboot(client);
            } break;
          case 40: {
              return handleDebug(client, (char *)data, 203);
            } break;
          case 50: {
              return handleWifiScan(client);
            } break;
          case 80: {
              return handleSettings(client);
            } break;
          case 90: {
              return handleFactoryReset(client);
            } break;
          case 100: {
              if (client->content == 0) {
                webserver_send(client, 200, (char *)"text/plain", 0);
                char *RESTmsg = (char *)client->userdata;
                webserver_send_content(client, (char *)RESTmsg, strlen(RESTmsg));
                free(RESTmsg);
                client->userdata = NULL;
              }
              return 0;
            } break;
          case 110: {
              int ret = saveSettings(client, &heishamonSettings);
              if (heishamonSettings.listenonly) {
                //make sure we disable TX to heatpump-RX using the mosfet so this line is floating and will not disturb cz-taw1
                digitalWrite(5, LOW);
              } else {
                digitalWrite(5, HIGH);
              }
              switch (client->route) {
                case 111: {
                    return settingsNewPassword(client, &heishamonSettings);
                  } break;
                case 112: {
                    return settingsReconnectWifi(client, &heishamonSettings);
                  } break;
                case 113: {
                    webserver_send(client, 301, (char *)"text/plain", 0);
                  } break;
              }
              return 0;
            } break;
          case 111: {
              return settingsNewPassword(client, &heishamonSettings);
            } break;
          case 112: {
              return settingsReconnectWifi(client, &heishamonSettings);
            } break;
          case 120: {
              return handleSettings(client);
            } break;
          case 130: {
              return getSettings(client, &heishamonSettings);
            } break;
          case 140: {
              return showFirmware(client);
            } break;
          case 150: {
              log_message((char*)"In /firmware client write part");
              if (Update.isRunning()) {
                if (Update.end(true)) {
                  log_message((char*)"Firmware update success");
                  timerqueue_insert(2, 0, -2); // Start reboot sequence
                  return showFirmwareSuccess(client);
                } else {
                  Update.printError(Serial);
                  return showFirmwareFail(client);
                }
              }
              return 0;
            } break;
          case 160: {
              return showRules(client);
            } break;
          case 170: {
              File *f = (File *)client->userdata;
              if (f) {
                if (*f) {
                  f->close();
                }
                delete f;
              }
              client->userdata = NULL;
              timerqueue_insert(0, 1, -4);
              webserver_send(client, 301, (char *)"text/plain", 0);
            } break;
          default: {
              webserver_send(client, 301, (char *)"text/plain", 0);
            } break;
        }
        return -1;
      } break;
    case WEBSERVER_CLIENT_CREATE_HEADER: {
        struct header_t *header = (struct header_t *)dat;
        switch (client->route) {
          case 113: {
              header->ptr += sprintf_P((char *)header->buffer, PSTR("Location: /settings"));
              return -1;
            } break;
          case 60:
          case 70: {
              header->ptr += sprintf_P((char *)header->buffer, PSTR("Location: /"));
              return -1;
            } break;
          case 170: {
              header->ptr += sprintf_P((char *)header->buffer, PSTR("Location: /rules"));
              return -1;
            } break;
          default: {
              if (client->route != 0) {
                header->ptr += sprintf_P((char *)header->buffer, PSTR("Access-Control-Allow-Origin: *"));
              }
            } break;
        }
        return 0;
      } break;
    case WEBSERVER_CLIENT_CLOSE: {
        switch (client->route) {
          case 100: {
              if (client->userdata != NULL) {
                free(client->userdata);
              }
            } break;
          case 110: {
              struct websettings_t *tmp = NULL;
              while (client->userdata) {
                tmp = (struct websettings_t *)client->userdata;
                client->userdata = ((struct websettings_t *)(client->userdata))->next;
                free(tmp);
              }
            } break;
          case 160:
          case 170: {
              if (client->userdata != NULL) {
                File *f = (File *)client->userdata;
                if (f) {
                  if (*f) {
                    f->close();
                  }
                  delete f;
                }
              }
            } break;
        }
        client->userdata = NULL;
      } break;
    default: {
        return 0;
      } break;
  }

  return 0;
}

void setupHttp() {
  webserver_start(80, &webserver_cb, 0);
}

  void doubleResetDetect() {
  if (drd->detectDoubleReset()) {
    Serial.println("Double reset detected, clearing config."); //save to print on std serial because serial switch didn't happen yet
    if(LittleFS.begin()) log_message("detectDoubleReset LITTLEFS opened OK");
    LittleFS.format();
    LittleFS.begin(true);
    delay(100); // must be
    WiFi.persistent(true);
    WiFi.disconnect();
    WiFi.persistent(false);
    Serial.println("Config cleared. Please reset to configure this device...");
    //initiate debug led indication for factory reset
   while (true) {
      digitalWrite(LED_BUILTIN, HIGH); // ESP8266 digitalWrite(2, HIGH);
      delay(200);
      digitalWrite(LED_BUILTIN, LOW); // ESP8266 digitalWrite(2, LOW);
      delay(200);
    }

  }
}

void setupSerial() {

  //esp32 has 3 Serial hardware port , HeishaMon_Both use Serial for monitor, Serial to HeatPump cn-cnt , Serial2 to cz-taw1
  // you can change Tx and Rx to any GPIO Serialx.begin(9600, SERIAL_8E1, Rx,Tx);
  Serial.begin(115200,SERIAL_8N1,3,1); //initial Serial for monitoring

  Serial1.begin(9600, SERIAL_8E1,27,26);    //Serial goes to cn-cnt in Panasonic Heat Pump, 
  Serial1.flush();
  Serial2.begin(9600, SERIAL_8E1,16,17);    // Serial2 goes to cz-taw1, 
  Serial2.flush();
}


void setupMqtt() {
  mqtt_client.setBufferSize(1024);
  mqtt_client.setSocketTimeout(10); mqtt_client.setKeepAlive(5); //fast timeout, any slower will block the main loop too long
  mqtt_client.setServer(heishamonSettings.mqtt_server, atoi(heishamonSettings.mqtt_port));
  mqtt_client.setCallback(mqtt_callback);
}
void send_optionalpcb_query() {
  log_message("Sending optional PCB data");
  send_command(optionalPCBQuery, OPTIONALPCBQUERYSIZE,CZTAW_NOT);
}


void setupConditionals() {
  //load optional PCB data from flash
  if (heishamonSettings.optionalPCB) {
    if (loadOptionalPCB(optionalPCBQuery, OPTIONALPCBQUERYSIZE)) {
      log_message("Succesfully loaded optional PCB data from saved flash!");
    }
    else {
      log_message("Failed to load optional PCB data from flash!");
    }
    delay(1500); //need 1.5 sec delay before sending first datagram
    send_optionalpcb_query(); //send one datagram already at start
    lastOptionalPCBRunTime = millis();
  }

  //these two after optional pcb because it needs to send a datagram fast after boot
  if (heishamonSettings.use_1wire) initDallasSensors(log_message, heishamonSettings.updataAllDallasTime, heishamonSettings.waitDallasTime, heishamonSettings.dallasResolution);
  if (heishamonSettings.use_s0) initS0Sensors(heishamonSettings.s0Settings);


}

void timer_cb(int nr) {
  if (nr > 0) {
    rules_timer_cb(nr);
  } else {
    switch (nr) {
      case -1: {
          LittleFS.begin();
          LittleFS.format();
          WiFi.disconnect(true);
          timerqueue_insert(1, 0, -2);
        } break;
      case -2: {
          ESP.restart();
        } break;
      case -3: {
          setupWifi(&heishamonSettings);
        } break;
      case -4: {
          if (rules_parse("/rules.new") == -1) {
            logprintln("new ruleset failed to parse, using previous ruleset");
            rules_parse("/rules.txt");
          } else {
            logprintln("new ruleset successfully parsed");
            if (LittleFS.begin()) {
              LittleFS.rename("/rules.new", "/rules.txt");
            }
          }
          rules_boot();
        } break;
    }
  }

}

void send_panasonic_query() {
if(DEBUG_TEST) log_message("Requesting new panasonic data");
  send_command(panasonicQuery, PANASONICQUERYSIZE,CZTAW_NOT);
}

void read_panasonic_data() {
  if (sending && ((unsigned long)(millis() - sendCommandReadTime) > SERIALTIMEOUT)) {
    log_message("Previous read data attempt failed due to timeout!");
    if(DEBUG_TEST) sprintf_P (log_msg, PSTR("Received %d bytes data"), data_length);
    log_message(log_msg);
    if (heishamonSettings.logHexdump) logHex(data, data_length);
    if (data_length == 0) {
      timeoutread++;
      if (timeoutread>200){   // restart when lost communication with Heat Pump
        Serial.printf("Timeout more tan  %i , ESP will be restarted ",timeoutread);
        ESP.restart();
      }
      totalreads++; //at at timeout we didn't receive anything but did expect it so need to increase this for the stats
    } else {
      tooshortread++;
    }
    data_length = 0; //clear any data in array
    forward_cztaw=CZTAW_NOT;
    sending = false; //receiving the answer from the send command timed out, so we are allowed to send a new command
  }
  if ( (heishamonSettings.listenonly || sending) && (Serial1.available() > 0)) readSerial1();
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void setup() {
  //first get total memory before we do anything
  getFreeMemory();

  //set boottime
  char *up = getUptime();
  free(up);
  pinMode(LED_BUILTIN, OUTPUT);
  setupSerial();
  Serial.println("\n--- HEISHAMON ---");
  Serial.println("starting...........................");
  //double reset detect from start
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  //ESP8266 doubleResetDetect();

  //initiate a wifi scan at boot to prefill the wifi scan json list
  byte numSsid = WiFi.scanNetworks();
  getWifiScanResults(numSsid);

  loadSettings(&heishamonSettings);
  drd->loop();
  setupWifi(&heishamonSettings);

  setupMqtt();
  setupHttp();

  /*ESP8266 sntp_setoperatingmode(SNTP_OPMODE_POLL); 
  ESP32 assert failed: sntp_setoperatingmode IDF/components/lwip/lwip/src/apps/sntp/sntp.c:724 (Operating mode must not be set while SNTP client is running)
  SNTP operating modes: default is to poll using unicast. The mode has to be set before calling sntp_init(). */
  sntp_init();
  // ESP8266  switchSerial(); //switch serial to gpio13/gpio15
  WiFi.printDiag(Serial);
  Serial.print("MAC Address =");
  Serial.println(WiFi.macAddress());
  Serial.print("IP Address =");
  Serial.println(WiFi.localIP());
  Serial.println("________________________________________________________________");
  setupConditionals(); //setup for routines based on settings
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  esp_reset_reason_t reset_reason = esp_reset_reason(); //  https://www.robmiles.com/journal/2021/1/10/esp-reset-message-strings
  Serial.printf(PSTR("Reset reason: %d\n"), reset_reason);
  listDir(LittleFS, "/", 1); 
  if (reset_reason > 0 && reset_reason < 4) {
    if (LittleFS.begin()) {
      LittleFS.rename("/rules.txt", "/rules.old");
    }
    rules_setup();
    if (LittleFS.begin()) {
      LittleFS.rename("/rules.old", "/rules.txt");
    }
  } else {
    rules_setup();
  }
}

void loop() {
#define DEBUG_LOOP 1
  drd->loop();

  webserver_loop();

  // check wifi
  check_wifi();
  // Handle OTA first.
  ArduinoOTA.handle();

  mqtt_client.loop();
  read_panasonic_data();
  if (!sending and initialResponse[0]!=0x31)  send_initial_query();  // init command must be sent in listenonly mode also
  readCzTaw();  
  if ((!sending) && (cmdnrel > 0)) { //check if there is a send command in the buffer
    log_message("Sending command from buffer");
    popCommandBuffer();
  }

  if (heishamonSettings.use_1wire) dallasLoop(mqtt_client, log_message, heishamonSettings.mqtt_topic_base);

  if (heishamonSettings.use_s0) s0Loop(mqtt_client, log_message, heishamonSettings.mqtt_topic_base, heishamonSettings.s0Settings);

  if ((!sending) && (!heishamonSettings.listenonly) && (heishamonSettings.optionalPCB) && ((unsigned long)(millis() - lastOptionalPCBRunTime) > OPTIONALPCBQUERYTIME) ) {
    lastOptionalPCBRunTime = millis();
    send_optionalpcb_query();
  }

  // run the data query only each WAITTIME
  if ((unsigned long)(millis() - lastRunTime) > (1000 * heishamonSettings.waitTime)) {
    lastRunTime = millis();
    //check mqtt
    if ( (WiFi.isConnected()) && (!mqtt_client.connected()) )
    {
      log_message("Lost MQTT connection!");
      mqtt_reconnect();
    }

    //log stats
    if (totalreads > 0 ) readpercentage = (((float)goodreads / (float)totalreads) * 100);
    String message;
    message.reserve(384);
    message += F("Heishamon stats: Uptime: ");
    char *up = getUptime();
    message += up;
    free(up);
    message += F(" ## Free memory: ");
    message += getFreeMemory();
    message += F("% ## Heap fragmentation: ");
    // esp32 missing message += ESP.getHeapFragmentation();
    message += F("% ## Max free block: ");
    //  esp32 missing message += ESP.getMaxFreeBlockSize();
    message += F(" bytes ## Free heap: ");
    message += ESP.getFreeHeap();
    message += F(" bytes ## Wifi: ");
    message += getWifiQuality();
    message += F("% (RSSI: ");
    message += WiFi.RSSI();
    message += F(") ## Mqtt reconnects: ");
    message += mqttReconnects;
    message += F(" ## Correct data: ");
    message += readpercentage;
    message += F("%");
    log_message((char*)message.c_str());

    String stats;
    stats.reserve(384);
    stats += F("{\"uptime\":");
    stats += String(millis());
    stats += F(",\"voltage\":");
    //   esp32 missing stats += ESP.getVcc() / 1024.0;
    stats += F(",\"free memory\":");
    stats += getFreeMemory();
    stats += F(",\"free heap\":");
    stats += ESP.getFreeHeap();
    stats += F(",\"wifi\":");
    stats += getWifiQuality();
    stats += F(",\"mqtt reconnects\":");
    stats += mqttReconnects;
    stats += F(",\"total reads\":");
    stats += totalreads;
    stats += F(",\"good reads\":");
    stats += goodreads;
    stats += F(",\"bad crc reads\":");
    stats += badcrcread;
    stats += F(",\"bad header reads\":");
    stats += badheaderread;
    stats += F(",\"too short reads\":");
    stats += tooshortread;
    stats += F(",\"too long reads\":");
    stats += toolongread;
    stats += F(",\"timeout reads\":");
    stats += timeoutread;
    stats += F("}");
    sprintf_P(mqtt_topic, PSTR("%s/stats"), heishamonSettings.mqtt_topic_base);
    mqtt_client.publish(mqtt_topic, stats.c_str(), MQTT_RETAIN_VALUES);

    //get new data
    if (!heishamonSettings.listenonly) send_panasonic_query();

    //Make sure the LWT is set to Online, even if the broker have marked it dead.
    sprintf_P(mqtt_topic, PSTR("%s/%s"), heishamonSettings.mqtt_topic_base, mqtt_willtopic);
    mqtt_client.publish(mqtt_topic, "Online");

    if (WiFi.isConnected()) {
// ESP8266     MDNS.announce();
    }
  }

  timerqueue_update();
}

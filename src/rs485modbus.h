#ifndef _RS485MODBUS_H_
#define _RS485MODBUS_H_

#include <littleFS.h>
#include <FS.h>
#include <string>
#include <Arduino.h>
#include <ModbusClientRTU.h>	// header for the Modbus Client RTU style from eModbus Library
#include <ModbusServerRTU.h>	// header for the Modbus Server RTU style from eModbus Library
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 
#include "htmlcode.h"
#include "src/common/progmem.h"

#define loggingSerial Serial 
#define LOGRX 3
#define LOGTX 1
#define MODBUS_REDE_PIN GPIO_NUM_21  // RE/DE  GPIO18 is used for RS485 direction control for halfduplex devices
#define MODBUS_RX1_PIN GPIO_NUM_18  	
#define MODBUS_TX1_PIN GPIO_NUM_19
#define MAX_MODBUS_DEVICES 4
#define MODBUS_READ_TIMER 5000		// Modbus read period
#define MAX_MODBUS_VALUEARRAY 16  // max number of values in one modbus device defined in modbusXX.json  definition file stored on LittleFS

struct item {
  int address;
  int divider;
  char name[15];
  char unit[4];
};

struct t_modbusDev {
  char  mqttTopic[10];
  uint16_t addrstart;
  uint16_t numb_registers;
  int datasize1;
  int devAddress;
  item Values[MAX_MODBUS_VALUEARRAY];
};

void modbusSendMSG( int actModbDevact);
void actSendMQTT( item &modbusItem, int actModbDevact);
void InitModbusDev ();
void handleError(Error error, uint32_t token);
void setupModbus();
void readModbus();
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void modbusSendMQTT(int actModbDev);
void handleData1(ModbusMessage response, uint32_t token);
void readFile(fs::FS &fs, String path);
void handleRoot() ;
void handleNotFound();
void log_message(char* string);
void log_message(const __FlashStringHelper *msg);
int  showCountDef(struct webserver_t *client);
int showModbus(struct webserver_t *client);
int showMdbusSuccess(struct webserver_t *client);
int showModbusFail(struct webserver_t *client);
String printDirectory(File dir, int numTabs);
void testWiFi(String tekst, int linia);
#endif


static const char showModbusPage[] PROGMEM =
  "<script>"
  "  function reloadModbus() {"
  "    setTimeout(function() {window.location.href = \"/modbus\";}, 4000);"
  "  }"

  "  function _(el) {  "
  "    return document.getElementById(el);  "
  "  }  "
  "  "
  "  function uploadFile1() {  "
  "    _(\"updatebuttonmod\").disabled = true;"
  "    _(\"status\").innerText = \"\";"
  "    var file = _(\"modbusdef\").files[0];  "
  "    var formdata = new FormData();  "
  "    formdata.append(\"modbusdef\", file);  "
  "    var filename = _(\"modbusdef\").files[0].name;  "
  "    var faz=\"/\"+filename;"
  "    formdata.append(\"modbdefi\", faz);  "

  "    var request = new XMLHttpRequest();  "
  "    request.upload.addEventListener(\"progress\", progressHandler, false);"																			  
  "    request.onreadystatechange = function(response) {"
  "      if (request.readyState === 4) {"
   "        _(\"status\").innerText = request.responseText;"
  "        if (request.responseText.includes(\"success\")) { "
  "          reloadModbus();"
  "        } else {"
  "          _(\"updatebuttonmod\").disabled = false;"
  "        };"
  "      };"
  "    };"
  "    request.open(\"POST\", \"/modbus\");"
  "    request.send(formdata);"
  "  }  "
  "  "
  "  function progressHandler(event) {  "
  "    var percent = (event.loaded / event.total) * 100;  "
  "    _(\"progressBarmod1\").value = Math.round(percent);  "
  "  }  "
    "</script>"
  "<div class=\"w3-sidebar w3-bar-block w3-card w3-animate-left\" style=\"display:none\" id=\"leftMenu\">"
  "<a href=\"/\" class=\"w3-bar-item w3-button\">Home</a>"
  "<a href=\"/reboot\" class=\"w3-bar-item w3-button\">Reboot</a>"
  "<a href=\"/settings\" class=\"w3-bar-item w3-button\">Settings</a>"
  "<a href=\"/togglelog\" class=\"w3-bar-item w3-button\">Toggle mqtt log</a>"
  "<a href=\"/togglehexdump\" class=\"w3-bar-item w3-button\">Toggle hexdump log</a>"
  "</div>"
  "<div class=\"w3-container w3-center\">"
  "   <form method=\"POST\" action=\"/modbusdef\" enctype=\"multipart/form-data\">"
  "       <h2>Add MODBUS Definitions:</h2>"
  "       <input type=\"file\" accept=\".json\"  id=\"modbusdef\" name=\"modbdef\"><br><br>"
  "   </form>"
  "   <button id=\"updatebuttonmod\" onclick=\"uploadFile1()\">Upload File to LittleFS </button><br>  <progress id=\"progressBarmod1\" value=\"0\" max=\"100\" style=\"width:300px;\"></progress><p id=\"status\"></p>"
  "</div>";
  
static const char ModbusSuccessResponse[] PROGMEM =
  "Upload Modbusdef success! You should use Reboot option to use new counter\n You can use MAX_MODBUS_DEVICES=4 counters definitions";

static const char ModbusFailResponse[] PROGMEM =
  "Modbus Upload failed! Please try again...";

static const char showCountDefPage1[] PROGMEM =
  "<div class=\"w3-sidebar w3-bar-block w3-card w3-animate-left\" style=\"display:none\" id=\"leftMenu\">"
  "<a href=\"/\" class=\"w3-bar-item w3-button\">Home</a>"
  "<a href=\"/reboot\" class=\"w3-bar-item w3-button\">Reboot</a>"
  "<a href=\"/firmware\" class=\"w3-bar-item w3-button\">Firmware</a>"
  "<a href=\"/settings\" class=\"w3-bar-item w3-button\">Settings</a>"
   "<a href=\"/settings\" class=\"w3-bar-item w3-button\">Modbus</a>"
  "<a href=\"/togglelog\" class=\"w3-bar-item w3-button\">Toggle mqtt log</a>"
  "<a href=\"/togglehexdump\" class=\"w3-bar-item w3-button\">Toggle hexdump log</a>"
  "</div>"
  "<div class=\"w3-container w3-center\">"
  "  <h2>File View only</h2>"
  "  <form accept-charset=\"UTF-8\" action=\"/modbus\" enctype=\"multipart/form-data\" method=\"POST\">"
  "    <textarea name=\"counterdef\" cols=\"60\" rows=\"12\">";

static const char showCountDefPage2[] PROGMEM =
  "</textarea><br />"
  "    <input class=\"w3-green w3-button\" type=\"submit\"id=\"close_view\"  value=\"Close\">"
  "  </form>"
  "</div>";
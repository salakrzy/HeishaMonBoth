
#include <littleFS.h>
#include <FS.h>
#include <string>
#include "rs485modbus.h"
#include "Logging.h"			// header from eModbus Library
#include <WiFiClient.h>
#include <ESP32WebServer.h>
#include <WiFi.h>
#include "src\common\webserver.h"
#include "webfunctions.h"

#define DEBUG 0

extern bool data_ready;
extern PubSubClient mqtt_client;
extern settingsStruct heishamonSettings;

extern Stream *SerialMonitor; //redirect Serial to loggingSerial
ModbusClientRTU ModbGate(-1); 	//ModbusClientRTU ModbGate(MODBUS_REDE1_PIN);  if you want to use half duplex conwerter TTL to RS485
t_modbusDev modbusDev[MAX_MODBUS_DEVICES];  // declare dynamic Array for modbusDevices
File root1;

bool data_ready = false;
float powerRead[120];
int numModbDev=0;
int actModbDev;   // start ask from first modbus device
int errors = 1;  // count errors number 
uint32_t modbus_request_time ;	
uint8_t * buf;

void changeSerialToLogging(){ // ***********************************************************************************************
  SerialMonitor=&Serial;
}

void setupModbus(){  // ***********************************************************************************************
  if (heishamonSettings.modbusOn){
    ModbusMessage(256);
    ModbGate.onDataHandler(&handleData1); 	// Set up ModbusRTU client - provide onData handler function	
    ModbGate.onErrorHandler(&handleError);	// - provide onError handler function
    ModbGate.setTimeout(1000);			// Set message timeout value in milliseconds, how long we wait for device answer
    ModbGate.begin(Serial);	//  ModbGate.begin(Serial,1);	Start ModbusRTU background task on CPU number 1 
    InitModbusDev (); //  InitModbusDev ( MODBUS_REDE_PIN) for converter required  direction control for halfduplex devices
    SerialMonitor->printf("Setup Modbus detected  %i files with counters definitions only 4 first will be used\n",numModbDev);
    actModbDev=0;
  } else{
    ModbGate.end(); 
  }
    int speed=Serial.baudRate();
    String komunik="UART0 monitor setting baud rate set to ";
    komunik=komunik+speed;
    log_message((char*)komunik.c_str());
    SerialMonitor->printf("\n%c %i Setup MODBUS port speed to %i\n", __FUNCTION__ ,__LINE__,Serial.baudRate());
}

void readFile(fs::FS &fs, String  path){ // **********************************************************************
  SerialMonitor->printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if(!file || file.isDirectory()){
    SerialMonitor->println("- failed to open file for reading");
    return;
  }
  SerialMonitor->println("- read from file:");
  while(file.available()){
    SerialMonitor->write(file.read());
  }
  SerialMonitor->write("\n");
  file.close();
}

//WEBSERWER
String printDirectory(File dir, int numTabs) { // **********************************************************************
  String  response = "<form accept-charset=\'UTF-8\' action='/modbus' method='POST'>";
          response += "<input class='w3-green w3-button' type='submit' name='modbusact' value='View'>";
          response += "<input class='w3-red w3-button' type='submit' name='modbusact' value='Delete'></br>";
  dir.rewindDirectory();
  while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {  // no more files
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       SerialMonitor->print('\t');   // we'll have a nice indentation
     }
     // Recurse for directories, otherwise print the file size
     if (entry.isDirectory()) {
       printDirectory(entry, numTabs+1);
     } else {
      response += String("<input type='radio' name='modbuslist' value='")+'/'+String(entry.name())+"'>" ;
      response += "<a>" + String(entry.name()) + "</a></br>" ;
     }
     entry.close();
   }
  response += "</form>";
  return String("<div class=\"w3-container w3-center\"><h2>Existing files on HeishaMonBoth LittleFS:</h2>") + response ;
}


void listDir(fs::FS &fs, const char * dirname, uint8_t levels){/**************************************************** */
  SerialMonitor->printf("Listing directory: %s\r\n", dirname);
  File root = fs.open(dirname);
  if(!root){
      SerialMonitor->println("- failed to open directory");
      return;
  }
  if(!root.isDirectory()){
      SerialMonitor->println(" - not a directory");
      return;
  }
  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
        SerialMonitor->print("  DIR : ");
        SerialMonitor->println(file.name());
        if(levels){
            listDir(fs, file.path(), levels -1);
        }
    } else {
        SerialMonitor->print("  FILE: ");
        SerialMonitor->print(file.name());
        SerialMonitor->print("\tSIZE: ");
        SerialMonitor->println(file.size());
    }
    file = root.openNextFile();
  }
}


int showModbus(struct webserver_t *client) {  // **********************************************************************
  if (client->content == 0) {
    webserver_send(client, 200, (char *)"text/html", 0);
    webserver_send_content_P(client, webHeader, strlen_P(webHeader));
    webserver_send_content_P(client, webCSS, strlen_P(webCSS));
    webserver_send_content_P(client, webBodyStart, strlen_P(webBodyStart));
  } else  if (client->content == 1) {
    webserver_send_content_P(client, showModbusPage, strlen_P(showModbusPage));
    File root1 = LittleFS.open("/");
    char *flashfiles =(char *)printDirectory(root1, 0).c_str();
    webserver_send_content(client,  flashfiles , strlen(flashfiles ));  
    webserver_send_content_P(client, menuJS, strlen_P(menuJS));
    webserver_send_content_P(client, webFooter, strlen_P(webFooter));
  }
  return 0;
}

int showMdbusSuccess(struct webserver_t *client) {/**************************************************************** */
  if (client->content == 0) {
    webserver_send(client, 200, (char *)"text/html", strlen_P(ModbusSuccessResponse));
    webserver_send_content_P(client, ModbusSuccessResponse, strlen_P(ModbusSuccessResponse));
  }
  return 0;
}

int showMdbusFail(struct webserver_t *client) {/******************************************************************** */
  if (client->content == 0) {
    webserver_send(client, 200, (char *)"text/html", strlen_P(ModbusSuccessResponse));
    webserver_send_content_P(client, ModbusFailResponse, strlen_P(ModbusSuccessResponse));
  }
  return 0;
}


void handleError(Error error, uint32_t token) {  //***********************************************************************************************
  // ModbusError wraps the error code and provides a readable error message for it
ModbusError me(error);
LOG_E(" number=%i  token=%d Error Code= %02X - %s\n", errors, token, (int)me, (const char *)me);
String errmessage;
errmessage+= F("#################### HModbus Handle Error: nr=");
errmessage+=errors++;
errmessage+=F(" token=");
errmessage+=token;
errmessage+= F("  Error Code=");
errmessage+=(int ) me;
errmessage+= F("- ");
errmessage+=(const char *)me;
log_message((char*)errmessage.c_str());
mqtt_client.publish("modbuserror",(char*)errmessage.c_str(),1);
digitalWrite(LED_BUILTIN, LOW);
}

void InitModbusDev (){  //***********************************************************************************************
numModbDev=0;
JsonDocument jsonDoc;
if (LittleFS.begin()) {
  SerialMonitor->print("LittleFS volumen opened\n");
File root = LittleFS.open("/");
File file = root.openNextFile();
actModbDev=0;
  while (file){
    String modbusDef = file.name();
    modbusDef="/"+modbusDef;
    int size = file.size();
    SerialMonitor->printf("File name= /%s   size= %i\n",modbusDef,size);
    if(modbusDef.startsWith("/modbus")){
      file=LittleFS.open(modbusDef,"r");
      std::unique_ptr<char[]> buf(new char[size]);
      file.readBytes(buf.get(), size);
      DeserializationError error = deserializeJson(jsonDoc, buf.get());
      if (error) {
        SerialMonitor->printf("deserializeJson() failed with code %s\n", error.code());
      }
     else {
      if( actModbDev<MAX_MODBUS_DEVICES){
        modbusDev[actModbDev].addrstart=jsonDoc["addrstart"];
        modbusDev[actModbDev].numb_registers=jsonDoc["numb_registers"];
        strncpy(modbusDev[actModbDev].mqttTopic, jsonDoc["mqttTopic"], sizeof(modbusDev[actModbDev].mqttTopic));
        modbusDev[actModbDev].datasize1=jsonDoc["datasize1"];
        modbusDev[actModbDev].devAddress=jsonDoc["devAddress"];
        char Valuen[8];  
        for (int ii=0 ; (ii<MAX_MODBUS_VALUEARRAY); ii++){
          memset(Valuen, 0, 8);
          sprintf(Valuen,"Value%i",ii);
          modbusDev[actModbDev].Values[ii].address=jsonDoc[Valuen]["address"];
          modbusDev[actModbDev].Values[ii].divider=jsonDoc[Valuen]["divider"];
          strncpy(modbusDev[actModbDev].Values[ii].unit,jsonDoc[Valuen]["unit"],sizeof(modbusDev[actModbDev].Values[ii].unit)) ;    
          strncpy(modbusDev[actModbDev].Values[ii].name,jsonDoc[Valuen]["name"],sizeof(modbusDev[actModbDev].Values[ii].name)) ;
        }
        actModbDev++;
      } else {
        log_message(_F("To much Modbus definition files "));
      }
    }
  }
  file = root.openNextFile();
  delete [] buf;
}  //while
file.close();
numModbDev=actModbDev;
actModbDev=0;
  }else LOG_E(" Can't open LittleFS volumen");
}

void actSendMQTT( item &modbusItem,int actModbDevact){  //***********************************************************************************************
  char topic[256];
  sprintf(topic, "%s/%s:%s", modbusDev[actModbDevact].mqttTopic ,modbusItem.name,modbusItem.unit);
  String reg_data = "";
  reg_data += powerRead[modbusItem.address];    
  mqtt_client.publish(topic,reg_data.c_str(),1);
  sprintf(topic,"%s = %s  Counter =%i", topic, reg_data, actModbDev);

}

void modbusSendMSG( int actModbDevact){ 
  String modmessage;
  modmessage += modbusDev[actModbDevact].mqttTopic;
  modmessage += " ";
  for (int i=0 ;i<MAX_MODBUS_VALUEARRAY;i++){
    modmessage += modbusDev[actModbDevact].Values[i].name;
    modmessage += "= ";
    modmessage +=powerRead[modbusDev[actModbDevact].Values[i].address];
    modmessage +=modbusDev[actModbDevact].Values[i].unit;
    modmessage += " ; ";
    if (modmessage.length( )>220){
      log_message((char*)modmessage.c_str());
      modmessage= modbusDev[actModbDevact].mqttTopic;
      modmessage += " ";
    }
  }
      if (modmessage.length( )>sizeof(modbusDev[actModbDevact].mqttTopic)){
      log_message((char*)modmessage.c_str());
    }

}

void  readModbus(){  //***********************************************************************************************
if(heishamonSettings.modbusOn and (numModbDev>0) ){
/*    if (Serial.baudRate()>9700){   
//     SerialMonitor->updateBaudRate(9600);
//     SerialMonitor->begin(9600, SERIAL_8N1, LOGRX,LOGTX);
    int speed=Serial.baudRate();
    String komunik="UART0 monitor setting baud rate set to ";
    komunik=komunik+speed;
      log_message((char*)komunik.c_str());
      SerialMonitor->printf("%s  %i nMODBUS speed=%i\n",__FUNCTION__,__LINE__,Serial.baudRate());
    }
    */
    static uint32_t next_request = millis();
    if ((millis() - next_request > MODBUS_READ_TIMER) and (data_ready == false)) {  // and !data_redy dodac warunek że jeżeli jeszcze nie przekazano danych to nie pytac o nowe dane
      if(actModbDev>=numModbDev ) actModbDev=0;  // all devices asked and start again ask for first device
      modbus_request_time=millis();
      Error err = ModbGate.addRequest(modbus_request_time,modbusDev[actModbDev].devAddress, READ_HOLD_REGISTER, modbusDev[actModbDev].addrstart,modbusDev[actModbDev].numb_registers);
      if (err!=SUCCESS) {
        ModbusError e(err);
        LOG_E("loop() Error creating request: %02X - %s\n", (int)e, (const char *)e);
      }
      next_request = millis();
    }
  }else if (!heishamonSettings.modbusOn and Serial.baudRate()< 110000){
//  SerialMonitor->updateBaudRate(115200);
//    SerialMonitor->begin(115200, SERIAL_8N1, LOGRX,LOGTX);
    int speed=Serial.baudRate();
    String komunik="UART0 monitor setting baud rate set to ";
    komunik=komunik+speed;
    log_message((char*)komunik.c_str());
    SerialMonitor->printf("\n %s  %i nMODBUS speed=%i\n",__FUNCTION__,__LINE__,Serial.baudRate());
  }
}

void modbusSendMQTT(  int actModbDev){  //***********************************************************************************************
  int actModbDevact=actModbDev;
  for (int i=0 ;i<MAX_MODBUS_VALUEARRAY;i++){
    actSendMQTT( modbusDev[actModbDevact].Values[i],actModbDevact);
  }
  data_ready = false;	
}

void handleData1(ModbusMessage response, uint32_t token) {  //***********************************************************************************************
   if (modbus_request_time=token){
    uint8_t words;      			// number of responsed registers
    response.get(2, words);   // third byte says how many registers id declared in response
      if (words==2*modbusDev[actModbDev].numb_registers){
        uint16_t offs = 3;   // First data starts from 4th byte, after server ID byte, function code byte and data length byte
        for (uint8_t i = 0; i < words; i++) {
          offs = response.get(offs, powerRead[i]);
          delay(20);
        }
       if(DEBUG) SerialMonitor->printf("numbers answer bytes declared in answer =  %i   number received bytes =  %i\n",words,2*modbusDev[actModbDev].numb_registers);
        data_ready = true;
        modbusSendMQTT(actModbDev);
      }
      else {
        SerialMonitor->printf("read bad nembers registers expected= %i   read= %i\n",words,2*modbusDev[actModbDev].numb_registers);
      }
      modbusSendMSG( actModbDev);
  }
  else{log_message(_F("read other token than expected"));
  }
  if(DEBUG){ for (int j = 0; j < 40; j++) SerialMonitor->printf("rej=%i; real= %8.4f    ", j,powerRead[j]);
    SerialMonitor->printf("\n");
  }
  actModbDev++;
}

int showCountDef(struct webserver_t *client) {
  uint16_t len = 0, len1 = 0;
  if (client->content == 0) {
    webserver_send(client, 200, (char *)"text/html", 0);
    webserver_send_content_P(client, webHeader, strlen_P(webHeader));
    webserver_send_content_P(client, webCSS, strlen_P(webCSS));
    webserver_send_content_P(client, webBodyStart, strlen_P(webBodyStart));
    webserver_send_content_P(client, showCountDefPage1, strlen_P(showCountDefPage1));
  } else if (client->userdata != NULL) {
    #define BUFFER_SIZE 128
    File *f = (File *)client->userdata;
    char content[BUFFER_SIZE];
    memset(content, 0, BUFFER_SIZE);
    if (f && *f) {
      len = f->size();
    }
    if (len > 0) {
      f->seek((client->content - 1)*BUFFER_SIZE, SeekSet);
      if (client->content * BUFFER_SIZE <= len) {
        f->readBytes(content, BUFFER_SIZE);
        len1 = BUFFER_SIZE;
      } else if ((client->content * BUFFER_SIZE) >= len && (client->content * BUFFER_SIZE) <= len + BUFFER_SIZE) {
        f->readBytes(content, len - ((client->content - 1)*BUFFER_SIZE));
        len1 = len - ((client->content - 1) * BUFFER_SIZE);
      } else {
        len1 = 0;
      }
      if (len1 > 0) {
        webserver_send_content(client, content, len1);
        if (len1 < BUFFER_SIZE || client->content * BUFFER_SIZE == len) {
          if (f) {
            if (*f) {
              f->close();
            } 
            delete f;
          }
          client->userdata = NULL;
          webserver_send_content_P(client, showCountDefPage2, strlen_P(showCountDefPage2));
          webserver_send_content_P(client, menuJS, strlen_P(menuJS));
          webserver_send_content_P(client, webFooter, strlen_P(webFooter));
        }
      } else if (client->content == 1) {
        if (f) {
          if (*f) {
            f->close();
          }
          delete f;
        }
        client->userdata = NULL;
        webserver_send_content_P(client, showCountDefPage2, strlen_P(showCountDefPage2));
        webserver_send_content_P(client, menuJS, strlen_P(menuJS));
        webserver_send_content_P(client, webFooter, strlen_P(webFooter));
      }
    } else if (client->content == 1) {
      if (f) {
        if (*f) {
          f->close();
        }
        delete f;
      }
      client->userdata = NULL;
      webserver_send_content_P(client, showCountDefPage2, strlen_P(showCountDefPage2));
      webserver_send_content_P(client, menuJS, strlen_P(menuJS));
      webserver_send_content_P(client, webFooter, strlen_P(webFooter));
    }
  } else if (client->content == 1) {
    webserver_send_content_P(client, "Acces forbiden", 14);
    webserver_send_content_P(client, showCountDefPage2, strlen_P(showCountDefPage2));
    webserver_send_content_P(client, menuJS, strlen_P(menuJS));
    webserver_send_content_P(client, webFooter, strlen_P(webFooter));
  }
  return 0;
}
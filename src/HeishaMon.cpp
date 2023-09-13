#include "HeishaMon.h"
#include "webfunctions.h"
#include "decode.h"
#include "commands.h"

Ticker Send_Pana_Command_Timer(send_pana_command, COMMANDTIMER, 1);  // one time
Ticker Send_Pana_Mainquery_Timer(send_pana_mainquery, QUERYTIMER, 1); // one time
Ticker Read_Pana_Data_Timer(read_pana_data, BUFFERTIMEOUT, 1); // one time
Ticker Timeout_Serial_Timer(timeout_serial, SERIALTIMEOUT, 1); // one time
Ticker Timeout_Restart_Timer(restartESP32, RESTARTTIMEOUT, 1); // one time

bool serialquerysent = false; // mutex for Serial1 sending
bool cz_taw_query = false;

// Default settings if config does not exists
const char *update_path = "/firmware";
const char *update_username = "admin";
char wifi_hostname[40] = "HeishaMon";
char ota_password[40] = "heisha";
char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_username[40];
char mqtt_password[40];

//log and debug
bool outputMqttLog = false;  // toggle to write logmessages to mqtt (true) or telnetstream (false)
bool outputTelnetLog = true;  // enable/disable telnet DEBUG
bool outputHexLog = true;     // enable/disable HexLog

// global scope
char serial1_data[MAXDATASIZE]; // Serial1 goes to Heat Pump
byte serial1_length = 0;
char serial2_data[MAXDATASIZE];// Serial2 goes to CZ_TAW
byte serial2_length = 0;


// store actual value in an String array
String actual_data[NUMBEROFTOPICS];

// log message
char log_msg[MAXDATASIZE];
unsigned long lastReconnectAttempt = 0;

WebServer httpServer(80);
HTTPUpdateServer httpUpdater;
WiFiClient mqtt_wifi_client;
PubSubClient mqtt_client(mqtt_wifi_client);


byte initialQuery[] =  {0x31, 0x05, 0x10, 0x01, 0x00, 0x00, 0x00,0xB9};
byte mainQuery[]    =  {0x71, 0x6c, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte mainQuery_length = mainQuery[1];   
byte cleanCommand[] =  {0xF1, 0x6c, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte cztaw_Buffer[QUERYSIZE];
byte mqtt_Buffer[QUERYSIZE];
byte initialResponse[51];
//example = {0x31, 0x30, 0x01, 0x10, 0x1E, 0x6C, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C};


/*****************************************************************************/
/* OTA                                                                       */
/*****************************************************************************/
void setupOTA()
{
  ArduinoOTA.setPort(3232);              // Port defaults for esp32
  ArduinoOTA.setHostname(wifi_hostname); // Hostname defaults to esp32-[ChipID]
  ArduinoOTA.setPassword(ota_password);  // Set authentication
  ArduinoOTA.onStart([]() {
  });
  ArduinoOTA.onEnd([]() {
//  Serial.println("\nEnd");
//  ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  });
  ArduinoOTA.onError([](ota_error_t error) {
  });
  ArduinoOTA.begin();
}

/*****************************************************************************/
/* Write to mqtt log                                                         */
/*****************************************************************************/
void write_mqtt_log(char *string)
{
  if (outputMqttLog)
  {
    mqtt_client.publish(Topics::LOG.c_str(), string);
  }
  else
  {
    TelnetStream.printf("[%02d-%02d-%02d %02d:%02d:%02d] %s\n", year(), month(), day(), hour(), minute(), second(), string);
  }
}

/*****************************************************************************/
/* DEBUG to Telnet log                                                       */
/*****************************************************************************/
void write_telnet_log(char *string)
{
  if (outputTelnetLog)
  {
    TelnetStream.printf("[%02d-%02d-%02d %02d:%02d:%02d] \e[31m<DBG>\e[39m %s\n", year(), month(), day(), hour(), minute(), second(), string);
  }
}

/*****************************************************************************/
/* DEBUG hex log  (Igor Ybema)                                               */
/*****************************************************************************/
void write_hex_log(char *hex, byte hex_len) 
{
  int bytesperline = 32; 
  for (int i = 0; i < hex_len; i += bytesperline) {
    char buffer [(bytesperline * 3) + 1];
    buffer[bytesperline * 3] = '\0';
    for (int j = 0; ((j < bytesperline) && ((i + j) < hex_len)); j++) {
      sprintf(&buffer[3 * j], "%02X ", hex[i + j]);
    }
    sprintf(log_msg, "data: %s", buffer );
    write_telnet_log(log_msg);
  }
}

/*****************************************************************************/
/* FreeMemory (Igor Ybema)                                                   */
/*****************************************************************************/
int getFreeMemory() 
{
  //store total memory at boot time
  static uint32_t total_memory = 0;
  if ( 0 == total_memory ) total_memory = ESP.getFreeHeap();
  uint32_t free_memory   = ESP.getFreeHeap();
  return (100 * free_memory / total_memory ) ; // as a %
}

/*****************************************************************************/
/* WiFi Quality (Igor Ybema)                                                  */
/*****************************************************************************/
int getWifiQuality() 
{
  if (WiFi.status() != WL_CONNECTED)
    return -1;
  int dBm = WiFi.RSSI();
  if (dBm <= -100)
    return 0;
  if (dBm >= -50)
    return 100;
  return 2 * (dBm + 100);
}

/*****************************************************************************/
/* HTTP                                                                      */
/*****************************************************************************/
void setupHttp()
{
  httpUpdater.setup(&httpServer, update_path, update_username, ota_password);
  httpServer.on("/", []() {
    handleRoot(&httpServer);
  });
  httpServer.on("/tablerefresh", []() {
    handleTableRefresh(&httpServer, actual_data);
  });
  httpServer.on("/reboot", []() {
    handleReboot(&httpServer);
  });
  httpServer.on("/settings", []() {
    handleSettings(&httpServer, wifi_hostname, ota_password, mqtt_server, mqtt_port, mqtt_username, mqtt_password);
  });
  httpServer.on("/togglelog", []() {
    write_mqtt_log((char *)"Toggled mqtt log flag");
    outputMqttLog ^= true;
    handleRoot(&httpServer);
  });
  httpServer.on("/toggledebug", []() {
    write_mqtt_log((char *)"Toggled debug flag");
    outputTelnetLog ^= true;
    handleRoot(&httpServer);
  });
  httpServer.begin();
}

/*****************************************************************************/
/* Serials setup                                                             */
/*****************************************************************************/
void setupSerial()
{
	//esp32 has 3 Serial hardware port , HeishaMon_Both use Serial for monitor, Serial1 to cn-cnt in HeatPump, Serial2 to cz-taw1
  // you can change Tx and Rx to any GPIO Serialx.begin(9600, SERIAL_8E1, Rx,Tx);
  Serial.begin(115200,SERIAL_8N1,3,1); //initial Serial for monitoring

  Serial1.begin(9600, SERIAL_8E1,27,26);    //Serial1 goes to cn-cnt in Panasonic Heat Pump, 
  Serial1.flush();
  Serial2.begin(9600, SERIAL_8E1,17,16);    // Serial2 goes to cz-taw1, 
  Serial2.flush();
}

/*****************************************************************************/
/* MQTT Client reconnect                                                     */
/*****************************************************************************/
bool mqtt_reconnect()
{
  write_telnet_log((char *)"Mqtt reconnect");
  if (mqtt_client.connect(wifi_hostname, mqtt_username, mqtt_password, Topics::WILL.c_str(), 1, true, "Offline"))
  {
    mqtt_client.publish(Topics::WILL.c_str(), "Online");

    mqtt_client.subscribe(Topics::SET1.c_str());
    mqtt_client.subscribe(Topics::SET2.c_str());
    mqtt_client.subscribe(Topics::SET3.c_str());
    mqtt_client.subscribe(Topics::SET4.c_str());
    mqtt_client.subscribe(Topics::SET5.c_str());
    mqtt_client.subscribe(Topics::SET6.c_str());
    mqtt_client.subscribe(Topics::SET7.c_str());
    mqtt_client.subscribe(Topics::SET8.c_str());
    mqtt_client.subscribe(Topics::SET9.c_str());
    mqtt_client.subscribe(Topics::SET10.c_str());
    mqtt_client.subscribe(Topics::SET11.c_str());
    mqtt_client.subscribe(Topics::SET12.c_str());
    mqtt_client.subscribe(Topics::SET13.c_str());
    mqtt_client.subscribe(Topics::SET14.c_str());
    mqtt_client.subscribe(Topics::SET15.c_str());
    mqtt_client.subscribe(Topics::SET16.c_str());
    mqtt_client.subscribe(Topics::SET17.c_str());
    mqtt_client.subscribe(Topics::SET18.c_str());
    mqtt_client.subscribe(Topics::SET19.c_str());
    mqtt_client.subscribe(Topics::SET20.c_str());
    mqtt_client.subscribe(Topics::SET21.c_str());
    mqtt_client.subscribe(Topics::SET22.c_str());
    mqtt_client.subscribe(Topics::SET23.c_str());
    mqtt_client.subscribe(Topics::SET24.c_str());
    mqtt_client.subscribe(Topics::SET25.c_str());
    mqtt_client.subscribe(Topics::SET26.c_str());
  }
  return mqtt_client.connected();
}

/*****************************************************************************/
/* MQTT Callback                                                             */
/*****************************************************************************/
void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  Send_Pana_Mainquery_Timer.start();
  write_telnet_log((char *)"Callback from mqtt");
  char msg[length + 1];
  for (unsigned int i = 0; i < length; i++)
  {
    msg[i] = (char)payload[i];
  }
  msg[length] = '\0';
  build_heatpump_command(topic, msg);
}

/*****************************************************************************/
/* MQTT Setup                                                                */
/*****************************************************************************/
void setupMqtt()
{
  mqtt_client.setServer(mqtt_server, atoi(mqtt_port));
  mqtt_client.setCallback(mqtt_callback);
  mqtt_reconnect();
}

/*****************************************************************************/
/* Validate checksum                                                         */
/*****************************************************************************/
bool validate_checksum(char* serialx_data, byte serialx_length)
{
  byte chk = 0;
  for (int i = 0; i < serialx_length; i++) chk += serialx_data[i];
  return (chk == 0); //all received bytes + checksum should result in 0
}

/*****************************************************************************/
/* Calculate checksum                                                        */
/*****************************************************************************/
byte calculate_checksum(byte* command) 
{
  byte chk = 0;
  for ( int i = 0; i < QUERYSIZE; i++) chk += command[i];
  chk = (chk ^ 0xFF) + 01;
  return chk;
}

/*****************************************************************************/
/* Read raw from Serial1                                                      */
/*****************************************************************************/
bool readHeatPump()  
{
  write_telnet_log((char *)"read Heat Pump from Serial1");
  while((Serial1.available() > 0)) {
    serial1_data[serial1_length] = Serial1.read();
    serial1_length += 1;
    //only enable next lines to DEBUG
    //sprintf(log_msg, "DEBUG Receive byte Serial1 : %d %#x", serial1_length,serial1_data[serial1_length-1]);
    //write_telnet_log(log_msg);
  }
  if (outputHexLog) write_hex_log((char*)serial1_data, serial1_length);
  if (outputTelnetLog) sprintf(log_msg, "DEBUG Received from Serial1 : %d", serial1_length); write_telnet_log(log_msg);
  if (serial1_length > (serial1_data[1] + 3)) {
    write_telnet_log((char *)"Heat Pump answer is longer than header suggests");
    if (outputHexLog) write_hex_log((char*)serial1_data, serial1_length);
    serial1_length = 0;
    return false;
  }
  else if (serial1_length == (serial1_data[1] + 3)) {
    if (!validate_checksum(serial1_data,serial1_length)) {
      write_telnet_log((char *)"Heat Pump answer Checksum error");
      if (outputHexLog) write_hex_log((char*)serial1_data, serial1_length);
      serial1_length = 0;
      return false;
    }
    else
    {
      if(cz_taw_query) {
        Serial2.write((char*)serial1_data, serial1_length);
        cz_taw_query=false;
      }
      write_telnet_log((char *)"Heat Pump answer Valid data");
      if (outputHexLog) write_hex_log((char*)serial1_data, serial1_length);
      serial1_length = 0;
      return true;
  }}
  else if(serial1_length>0) {
    sprintf(log_msg, "Partial Heat Pump  data length %d, please fix Read_Pana_Data_Timer", serial1_length); 
    write_telnet_log(log_msg);
  }
  return false;
}

/*****************************************************************************/
/* Register new command
/* Wait COMMANDTIMER for multible commands from mqtt
/*****************************************************************************/
bool register_new_command(byte sender)
{
    Send_Pana_Command_Timer.start(); // wait countdown for send another command
    write_telnet_log((char *)"Register new command/query");
    switch(sender){
        case 'M':
        write_telnet_log((char *)"Main Query has request");
        break;
        case 'Q':
        write_telnet_log((char *)"MQTT has request");
        break;
        case 'C':
        write_telnet_log((char *)"cztaw_query has request");
        break;
    }
    return true;
}

/*****************************************************************************/
/* Send commands from buffer to pana  (called from loop)
/* send the set command global mainCommand[]
/* chk calculation must be on each time we send
/*****************************************************************************/
void send_pana_command()
{
  if (!(Timeout_Serial_Timer.state()==RUNNING or (Serial1.available()) > 0)){
  write_telnet_log((char *)"Start send_pana_command()");
    if (!cztaw_Buffer[1]== 0) {
      Serial1.write(cztaw_Buffer, cztaw_Buffer[1]+2);
      Serial1.write(calculate_checksum(cztaw_Buffer));
      serialquerysent = true;    
      cz_taw_query=true;
      write_telnet_log((char *)"send CZTAW buffer");  
      if (outputHexLog) write_hex_log((char*)cztaw_Buffer,  cztaw_Buffer[1]+2);
      cztaw_Buffer[1]=0x0;
    }
    else if (!mainQuery[1]== 0) {
      Serial1.write(mainQuery, mainQuery[1]+2);
      Serial1.write(calculate_checksum(mainQuery));
      serialquerysent = true;    
      mainQuery[1]=0x0;
      write_telnet_log((char *)"send mainQuerry");
      if (outputHexLog) write_hex_log((char*)mainQuery, QUERYSIZE);
    }
    else if (!mqtt_Buffer[1]== 0) {
      Serial1.write(mqtt_Buffer, mqtt_Buffer[1]+2);
      Serial1.write(calculate_checksum(mqtt_Buffer));
      memcpy(mqtt_Buffer, cleanCommand, QUERYSIZE);
      serialquerysent = true;
      mqtt_Buffer[1]=0x0;
      write_telnet_log((char *)"send mqtt buffer");
      if (outputHexLog)  write_hex_log((char*)mqtt_Buffer, QUERYSIZE);
    }
    else write_telnet_log((char *)"Nothing to send");
    Read_Pana_Data_Timer.start();
    Timeout_Serial_Timer.start();
  }
  else Send_Pana_Command_Timer.start(); 
}


/*****************************************************************************/
/* Send query to buffer  (called from loop)                                    
/* only to trigger the next query if we have no new set command on buffer 
/*****************************************************************************/
void send_pana_mainquery()
{
  mainQuery[1]=mainQuery_length;
  if (register_new_command('M')== true) write_telnet_log((char *)"Main query succes");
  else write_telnet_log((char *)"bussy interface"); 
}

/*****************************************************************************/
/* Read from pana and decode (call from loop)                           */
/*****************************************************************************/
void read_pana_data()
{
    write_telnet_log((char *)"Start read_pana_data()");
  if (serialquerysent == true) //only read if we have sent a command so we expect an answer
  {
    if (readHeatPump() == true) {
      write_telnet_log((char *)"Decode topics ---------- Start ------------------");
      publish_heatpump_data(serial1_data, actual_data, mqtt_client); 
      write_telnet_log((char *)"Decode topics ---------- End --------------------\n");
      Send_Pana_Mainquery_Timer.start();
      Timeout_Restart_Timer.start();
    }
  }
    else write_telnet_log((char *)"serialquerysent FALSE");
}

/*****************************************************************************/
/* handle Serial1 timeout                                                     */
/*****************************************************************************/
void timeout_serial() {
  write_telnet_log((char *)"Start  timeout_serial())");
  if (serialquerysent == true) {
    serialquerysent = false; //we are allowed to send a new command
    write_telnet_log((char *)"Serial1 interface read timeout");
    Send_Pana_Mainquery_Timer.start();
  }
}

/*****************************************************************************/
/* handle telnet stream                                                      */
/*****************************************************************************/
void handle_telnetstream()
{
  if (TelnetStream.available() > 0) {
    switch (TelnetStream.read()) {
    case 'R':
      TelnetStream.stop();
      delay(100);
      ESP.restart();
      break;
    case 'C':
      TelnetStream.println("bye bye");
      TelnetStream.flush();
      TelnetStream.stop();
      break;
    case 'L':
      TelnetStream.println("Toggled mqtt log flag");
      outputMqttLog ^= true;
      break;    
    case 'D':
      TelnetStream.println("Toggled debug flag");
      outputTelnetLog ^= true;
      break;
    case 'H':
      TelnetStream.println("Toggled hexlog flag");
      outputHexLog ^= true;
      break;
    case 'M':
      TelnetStream.printf("[%02d-%02d-%02d %02d:%02d:%02d] <INF> Memory: %d\n", year(), month(), day(), hour(), minute(), second(), getFreeMemory());
      break;
    case 'W':
      TelnetStream.printf("[%02d-%02d-%02d %02d:%02d:%02d] <INF> WiFi: %d\n", year(), month(), day(), hour(), minute(), second(), getWifiQuality());
      break;
    case 'I':
      TelnetStream.printf("[%02d-%02d-%02d %02d:%02d:%02d] <INF> localIP: %s\n", year(), month(), day(), hour(), minute(), second(), WiFi.localIP().toString().c_str());
      break;
    }
  }
}
/*****************************************************************************/
/* Read raw from Serial2                                                      */
/*****************************************************************************/
void readCzTaw()
{
	while (Serial2.available() > 0) {
    serial2_data[serial2_length] = Serial2.read();
    // only enable next line to DEBUG
    //  sprintf(log_msg, "DEBUG Receive Serial2 byte : %d  %#x", serial2_length, serial2_data[serial2_length]); 
    //  write_telnet_log(log_msg);
    serial2_length += 1;
	}
	if (serial2_length == (serial2_data[1] + 3)) {	
    if (!validate_checksum(serial2_data,serial2_length)) { 
      write_telnet_log((char *)"Query from Cz_Taw Checksum error");
      byte chk = 0;
      for ( int i = 0; i < serial2_length-1; i++)  chk += serial2_data[i];
      chk = (chk ^ 0xFF) + 01;
      sprintf(log_msg, "checksum should be =  %#x ",chk); 
      write_telnet_log(log_msg);
      serial2_length = 0;
      return ;
		}
    else
    {
      write_telnet_log((char *)"Query from Cz_Taw Valid data");
      if (outputHexLog)  write_hex_log((char*)serial2_data, serial2_length);
      if (serial2_data[0]==0x31 )
      {
        Serial2.write(initialResponse, 51);
        write_telnet_log((char *)"Serial2 send InitialResponse to CZ-TAW");
        if (outputHexLog)  write_hex_log((char*)initialResponse, 51);
      }
      else
      {
        memcpy(cztaw_Buffer,serial2_data, serial2_length-1); //copy query from cz_taw without CRC sum
        register_new_command( 'C');
        }
      serial2_length = 0;
      Send_Pana_Mainquery_Timer.start();
      return ;
	  }
  }
	if (serial2_length > (serial2_data[1] + 3)) {	
    write_telnet_log((char *)"Query from Cz_Taw longer than header declaration");
    Serial2.flush();
    serial2_length = 0;
    return ;
    }
	if(serial2_length>0) 
  {
// sprintf(log_msg, "Query from Cz_Taw  partial data length %d, please fix Read_Pana_Data_Timer", serial2_length); 
// write_telnet_log(log_msg);
  }
	return ;
}

/*****************************************************************************/
/* Setup Time                                                                */
/*****************************************************************************/
void setupTime() {
  configTime(0,0, "pool.ntp.org");
  setenv("TZ", TZ_Europe_Warsaw, 1);  
  tzset();
  delay(250);
  time_t now = time(nullptr);
  while (now < SECS_YR_2000) {
    delay(100);
    now = time(nullptr);
  }
  setTime(now); // FIX CEST dont work
}

void restartESP32() {
  write_telnet_log((char *)"Restart ESP32 because nothing received from Heat Pump");
  ESP.restart();  /*ESP restart function*/
}

/*****************************************************************************/
/* main                                                                      */
/*****************************************************************************/
void setup()
{
  getFreeMemory();
  memcpy(mqtt_Buffer, cleanCommand, QUERYSIZE);
  setupSerial();
  setupWifi(wifi_hostname, ota_password, mqtt_server, mqtt_port, mqtt_username, mqtt_password);
   if (!MDNS.begin(wifi_hostname)) {
    while (1) {
      delay(1000);
    }
  }
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("telnet", "tcp", 23);
  setupOTA();
  setupMqtt();
  setupHttp();
  setupTime();
  TelnetStream.begin();
  write_telnet_log((char *)"Connect Serial1 to to heatpump. Look for debug on mqtt log topic.");
  Timeout_Restart_Timer.start();
  
  while (true) {
    if (Serial1.available() > 0) {initialResponse[serial1_length] = Serial1.read();
      // only enable next line to DEBUG
      //sprintf(log_msg, "DEBUG Receive Serial1 byte : %d  %#x", serial1_length, initialResponse[serial1_length]); write_telnet_log(log_msg);
      //  Serial.printf ( "%#x",initialResponse[serial1_length]);
        serial1_length += 1;
    } 
		else if ((initialResponse[0]==0x31) and  validate_checksum(serial1_data,serial1_length))  {
      Serial.println("Heatpump init answeare is OK");
      break;
    }
    else {
      Serial1.flush();
      serial1_length = 0;
      Serial1.write(initialQuery, 8);
      Serial.println("Init request has been sent to Heatpump, now we are waiting for reply");
      delay(500);
    }
    Timeout_Restart_Timer.update();
	}
  
  serial1_length=0;
  Send_Pana_Mainquery_Timer.start(); // start only the query timer
  lastReconnectAttempt = 0;
}

void loop()
{
  ArduinoOTA.handle();
  httpServer.handleClient();
  handle_telnetstream();  

  if (!mqtt_client.connected())
  {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000)
    {
      lastReconnectAttempt = now;
      if (mqtt_reconnect())
      {
        lastReconnectAttempt = 0;
      }
    }
  }
  else
  {
    mqtt_client.loop(); // Trigger the mqtt_callback and send the set command to the buffer 
  }
  readCzTaw();
  Send_Pana_Command_Timer.update(); // trigger send_pana_command()   - send command or query from buffer
  Read_Pana_Data_Timer.update(); // trigger read_pana_data() - read from Serial1, decode bytes and publish to mqtt
  Timeout_Serial_Timer.update();     // trigger timeout_serial() - stop read from Serial1 after timeout
  Send_Pana_Mainquery_Timer.update();   // trigger send_pana_mainquery() - send query to buffer if no command in buffer
  Timeout_Restart_Timer.update();

}

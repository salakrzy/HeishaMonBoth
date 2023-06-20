#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP_WiFiManager.h> 
#include <ArduinoJson.h> 

#include "webfunctions.h"
#include "decode.h"
#include "version.h"

//flag for saving
bool shouldSaveConfig = false;

static const char refreshMeta[] PROGMEM = "<meta http-equiv='refresh' content='5; url=/' />";

static const char webHeader[] PROGMEM = "<!DOCTYPE html><html><title>Heisha monitor</title><head><meta name='viewport' content='width=device-width, initial-scale=1'><link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'><link rel='stylesheet' href='https://www.w3schools.com/lib/w3-theme-blue.css'>";

static const char ajaxscript[] PROGMEM = "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js'></script>";

static const char webBodyStart[] PROGMEM = "<button class='w3-button w3-blue w3-medium w3-left' onclick='openLeftMenu()'>&#9776;</button><header class='w3-container w3-card-4 w3-theme'><h4>Heisha Stufe 1</h4></header>";

static const char webFooter[] PROGMEM = "</body></html>";

static const char menuJS[] PROGMEM = "<script> function openLeftMenu() {var x = document.getElementById('leftMenu');if (x.style.display === 'none') {x.style.display = 'block';} else {x.style.display = 'none';}}</script>";

static const char refreshJS[] PROGMEM = "<script> $(document).ready(function(){refreshTable();}); function refreshTable(){$('#heishavalues').load('/tablerefresh', function(){setTimeout(refreshTable, 30000);});}</script></head><body>";

static const char sidebar[] PROGMEM = "<a href='/' class='w3-bar-item w3-button w3-small'>Home</a><a href='/reboot' class='w3-bar-item w3-button w3-small'>Reboot</a><a href='/firmware' class='w3-bar-item w3-button w3-small'>Firmware</a><a href='/settings' class='w3-bar-item w3-button w3-small'>Settings</a><a href='/togglelog' class='w3-bar-item w3-button w3-small'>Toggle mqtt log</a><a href='/toggledebug' class='w3-bar-item w3-button w3-small'>Toggle debug log</a>";

//callback notifying us of the need to save config
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setupWifi(char *wifi_hostname, char *ota_password, char *mqtt_server, char *mqtt_port, char *mqtt_username, char *mqtt_password)
{
  //Local intialization. Once its business is done, there is no need to keep it around
  ESP_WiFiManager wifiManager;
 /// wifiManager.setDebugOutput(); //this is debugging on serial port, because serial swap is done after full startup this is ok

  Serial.println("mounting LittleFS...");

  if (LittleFS.begin())
  {
    Serial.println("Mount file system");
    if (LittleFS.exists("/config.json"))
    {
      //file exists, reading and loading
      Serial.println("Read config file");
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("Open config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument jsonDoc(1024);
        DeserializationError error = deserializeJson(jsonDoc, buf.get());
        serializeJson(jsonDoc, Serial);
        if (!error)
        {
          Serial.println("\nparsed json");
          //read updated parameters, make sure no overflow
          strncpy(wifi_hostname, jsonDoc["wifi_hostname"], 39);
          wifi_hostname[39] = '\0';
          strncpy(ota_password, jsonDoc["ota_password"], 39);
          ota_password[39] = '\0';
          strncpy(mqtt_server, jsonDoc["mqtt_server"], 39);
          mqtt_server[39] = '\0';
          strncpy(mqtt_port, jsonDoc["mqtt_port"], 5);
          mqtt_port[5] = '\0';
          strncpy(mqtt_username, jsonDoc["mqtt_username"], 39);
          mqtt_username[39] = '\0';
          strncpy(mqtt_password, jsonDoc["mqtt_password"], 39);
          mqtt_password[39] = '\0';
        }
        else
        {
          Serial.println("Failed to load config, forcing config reset.");
          wifiManager.resetSettings();
        }
        configFile.close();
      }
    }
    else
    {
      Serial.println("No config. Forcing reset to default.");
      wifiManager.resetSettings();
    }
  }
  else
  {
    Serial.println("Failed to mount FS");
    if (LittleFS.begin(true)) Serial.println("empty FS created , please reboot");   /// change
  }

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  ESP_WMParameter custom_text1("<p>Hostname and OTA password</p>");
  ESP_WMParameter custom_wifi_hostname("wifi_hostname", "wifi hostname", wifi_hostname, 39);
  ESP_WMParameter custom_ota_password("ota_password", "ota password", ota_password, 39);
  ESP_WMParameter custom_text2("<p>MQTT settings</p>");
  ESP_WMParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 39);
  ESP_WMParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  ESP_WMParameter custom_mqtt_username("username", "mqtt username", mqtt_username, 39);
  ESP_WMParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 39);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_text1);
  wifiManager.addParameter(&custom_wifi_hostname);
  wifiManager.addParameter(&custom_ota_password);
  wifiManager.addParameter(&custom_text2);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);

  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setConnectTimeout(10);
  if (!wifiManager.autoConnect("HeishaMon-Setup"))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("Wifi connected...yeey :)");

  //read updated parameters, make sure no overflow
  strncpy(wifi_hostname, custom_wifi_hostname.getValue(), 39);
  wifi_hostname[39] = '\0';
  strncpy(ota_password, custom_ota_password.getValue(), 39);
  ota_password[39] = '\0';
  strncpy(mqtt_server, custom_mqtt_server.getValue(), 39);
  mqtt_server[39] = '\0';
  strncpy(mqtt_port, custom_mqtt_port.getValue(), 5);
  mqtt_port[5] = '\0';
  strncpy(mqtt_username, custom_mqtt_username.getValue(), 39);
  mqtt_username[39] = '\0';
  strncpy(mqtt_password, custom_mqtt_password.getValue(), 39);
  mqtt_password[39] = '\0';

  //Set hostname on wifi rather than ESP_xxxxx
  WiFi.hostname(wifi_hostname);

  //save the custom parameters to FS
  if (shouldSaveConfig)
  {
    Serial.println("Save config");
    DynamicJsonDocument jsonDoc(1024);
    jsonDoc["wifi_hostname"] = wifi_hostname;
    jsonDoc["ota_password"] = ota_password;
    jsonDoc["mqtt_server"] = mqtt_server;
    jsonDoc["mqtt_port"] = mqtt_port;
    jsonDoc["mqtt_username"] = mqtt_username;
    jsonDoc["mqtt_password"] = mqtt_password;

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile)
    {
      Serial.println("Failed to open config file for writing");
    }

    serializeJson(jsonDoc, Serial);
    serializeJson(jsonDoc, configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
}

void handleRoot(WebServer *httpServer)
{
  httpServer->setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer->send(200, "text/html");
  httpServer->sendContent_P(webHeader);
  httpServer->sendContent_P(ajaxscript);
  httpServer->sendContent_P(webBodyStart);
  httpServer->sendContent_P(menuJS);
  httpServer->sendContent_P(refreshJS);

  String httptext = "<div class='w3-sidebar w3-bar-block w3-card w3-animate-left' style='display:none' id='leftMenu'>";
  httpServer->sendContent(httptext);
  httpServer->sendContent_P(sidebar);
  httptext = "<hr><div class='w3-text-grey w3-small'>Version: ";
  httptext = httptext + heishamon_version + "<br><a href = 'https://github.com/gregor-samosir/HeishaMonKaskade '>Heishamon</a></div><hr></div>";
  httptext = httptext + "<br><div class='w3-container'><table class = 'w3-table-all w3-card-4 w3-small'><thead><tr class = 'w3-blue'><th>Topic</th><th>Name</th><th>Value</th><th>Description</th></tr></thead><tbody id =\"heishavalues\"><tr><td>... Loading...</td><td>.</td><td>.</td><td>.</td></tr></tbody></table></div>";
  httpServer->sendContent(httptext);
  httpServer->sendContent_P(webFooter);
  httpServer->sendContent("");
  httpServer->client().stop();
}

void handleTableRefresh(WebServer *httpServer, String actual_data[])
{
  String tabletext;
  String topicdesc;

  for (unsigned int top_num = 0; top_num < NUMBEROFTOPICS; top_num++)
  {
    if (strcmp(topicDescription[top_num][0], "value") == 0)
    {
      topicdesc = topicDescription[top_num][1];
    }
    else
    {
      int value = actual_data[top_num].toInt();
      topicdesc = topicDescription[top_num][value];
    }
    if (actual_data[top_num] != "unused")
    {
      tabletext = tabletext + "<tr><td>TOP" + top_num + "</td><td>" + topicNames[top_num] + "</td><td>" + actual_data[top_num] + "</td><td>" + topicdesc + "</td></tr>\n";
    }
  }
  httpServer->setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer->send(200, "text/html");
  httpServer->sendContent(tabletext);
  httpServer->sendContent("");
  httpServer->client().stop();
}

void handleReboot(WebServer *httpServer)
{
  httpServer->setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer->send(200, "text/html");
  httpServer->sendContent_P(webHeader);
  httpServer->sendContent_P(refreshMeta);
  httpServer->sendContent_P(webBodyStart);

  String httptext = "<div class='w3-container w3-center'>";
  httptext = httptext + "<p>Rebooting</p>";
  httptext = httptext + "</div>";
  httpServer->sendContent(httptext);
  httpServer->sendContent_P(menuJS);
  httpServer->sendContent_P(webFooter);
  httpServer->sendContent("");
  httpServer->client().stop();
  delay(1000);
  ESP.restart();
}

void handleSettings(WebServer *httpServer, char *wifi_hostname, char *ota_password, char *mqtt_server, char *mqtt_port, char *mqtt_username, char *mqtt_password)
{
  httpServer->setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer->send(200, "text/html");
  httpServer->sendContent_P(webHeader);
  httpServer->sendContent_P(webBodyStart);

  String httptext = "<div class='w3-sidebar w3-bar-block w3-card w3-animate-left' style='display:none' id='leftMenu'>";
  httpServer->sendContent(httptext);
  httpServer->sendContent_P(sidebar);
  httptext = "</div>";
  httpServer->sendContent(httptext);

  //check if POST was made with save settings, if yes then save and reboot
  if (httpServer->args())
  {
    DynamicJsonDocument jsonDoc(512);
    jsonDoc["wifi_hostname"] = wifi_hostname;
    jsonDoc["ota_password"] = ota_password;
    jsonDoc["mqtt_server"] = mqtt_server;
    jsonDoc["mqtt_port"] = mqtt_port;
    jsonDoc["mqtt_username"] = mqtt_username;
    jsonDoc["mqtt_password"] = mqtt_password;

    if (httpServer->hasArg("wifi_hostname"))
    {
      jsonDoc["wifi_hostname"] = httpServer->arg("wifi_hostname");
    }
    if (httpServer->hasArg("new_ota_password") && (httpServer->arg("new_ota_password") != NULL) && (httpServer->arg("current_ota_password") != NULL))
    {
      if (httpServer->hasArg("current_ota_password") && (strcmp(ota_password, httpServer->arg("current_ota_password").c_str()) == 0))
      {
        jsonDoc["ota_password"] = httpServer->arg("new_ota_password");
      }
      else
      {
        httptext = "<div class='w3-container w3-center'>";
        httptext = httptext + "<h3>------- wrong current password -------</h3>";
        httptext = httptext + "<h3>-- do factory reset if password lost --</h3>";
        httptext = httptext + "</div>";
        httpServer->sendContent(httptext);
        httpServer->sendContent_P(refreshMeta);
        httpServer->sendContent_P(webFooter);
        httpServer->sendContent("");
        httpServer->client().stop();
        return;
      }
    }
    if (httpServer->hasArg("mqtt_server"))
    {
      jsonDoc["mqtt_server"] = httpServer->arg("mqtt_server");
    }
    if (httpServer->hasArg("mqtt_port"))
    {
      jsonDoc["mqtt_port"] = httpServer->arg("mqtt_port");
    }
    if (httpServer->hasArg("mqtt_username"))
    {
      jsonDoc["mqtt_username"] = httpServer->arg("mqtt_username");
    }
    if (httpServer->hasArg("mqtt_password"))
    {
      jsonDoc["mqtt_password"] = httpServer->arg("mqtt_password");
    }

    if (LittleFS.begin(true))
    {
      File configFile = LittleFS.open("/config.json", "w");
      if (configFile)
      {
        serializeJson(jsonDoc, configFile);
        configFile.close();
        delay(1000);

        httptext = "<div class='w3-container w3-center'>";
        httptext = httptext + "<h3>--- saved ---</h3>";
        httptext = httptext + "<h3>-- rebooting --</h3>";
        httptext = httptext + "</div>";
        httpServer->sendContent(httptext);
        httpServer->sendContent_P(refreshMeta);
        httpServer->sendContent_P(webFooter);
        httpServer->sendContent("");
        httpServer->client().stop();
        delay(1000);
        ESP.restart();
      }
    }
  }

  httptext = "<div class='w3-container w3-center'>";
  httptext = httptext + "<h2>Settings</h2>";
  httptext = httptext + "<form action='/settings' method='POST'>";
  httptext = httptext + "Hostname:<br>";
  httptext = httptext + "<input type='text' name='wifi_hostname' value='" + wifi_hostname + "'>";
  httptext = httptext + "<br><br>";
  httptext = httptext + "Current update password:<br>";
  httptext = httptext + "<input type='password' name='current_ota_password' value=''>";
  httptext = httptext + "<br><br>";
  httptext = httptext + "New update password:<br>";
  httptext = httptext + "<input type='password' name='new_ota_password' value=''>";
  httptext = httptext + "<br><br>";
  httptext = httptext + "Mqtt server:<br>";
  httptext = httptext + "<input type='text' name='mqtt_server' value='" + mqtt_server + "'>";
  httptext = httptext + "<br><br>";
  httptext = httptext + "Mqtt port:<br>";
  httptext = httptext + "<input type='number' name='mqtt_port' value='" + mqtt_port + "'>";
  httptext = httptext + "<br><br>";
  httptext = httptext + "Mqtt username:<br>";
  httptext = httptext + "<input type='text' name='mqtt_username' value='" + mqtt_username + "'>";
  httptext = httptext + "<br><br>";
  httptext = httptext + "Mqtt password:<br>";
  httptext = httptext + "<input type='password' name='mqtt_password' value='" + mqtt_password + "'>";
  httptext = httptext + "<br><br>";
  httptext = httptext + "<input class='w3-green w3-button' type='submit' value='Save and reboot'>";
  httptext = httptext + "</form>";
  httptext = httptext + "</div>";
  httpServer->sendContent(httptext);

  httpServer->sendContent_P(menuJS);
  httpServer->sendContent_P(webFooter);
  httpServer->sendContent("");
  httpServer->client().stop();
}

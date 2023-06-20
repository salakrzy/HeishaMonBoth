#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

void setupWifi(char *wifi_hostname, char *ota_password, char *mqtt_server, char *mqtt_port, char *mqtt_username, char *mqtt_password);
void handleRoot(WebServer *httpServer);
void handleTableRefresh(WebServer *httpServer, String actual_data[]);
void handleReboot(WebServer *httpServer);
void handleSettings(WebServer *httpServer, char *wifi_hostname, char *ota_password, char *mqtt_server, char *mqtt_port, char *mqtt_username, char *mqtt_password);

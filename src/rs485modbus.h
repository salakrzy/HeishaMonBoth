#ifndef _RS485MODBUS_H_
#define _RS485MODBUS_H_

#include <littleFS.h>
#include <FS.h>
#include <string>
//#include <Arduino.h>
//#include "version.h"
#include <ModbusClientRTU.h>	// header for the Modbus Client RTU style from eModbus Library
#include <ModbusServerRTU.h>	// header for the Modbus Server RTU style from eModbus Library
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 
#include "htmlcode.h"
#include "src/common/progmem.h"

extern Stream *SerialModbMonitor;

//DEBUGME #define LOGRX 3
//DEBUGME#define LOGTX 1
// use interface not using RE/DE PIN #define MODBUS_REDE_PIN GPIO_NUM_18  // RE/DE  GPIO18 is used for RS485 direction control for halfduplex devices
#define MAX_MODBUS_DEVICES 4
#define MODBUS_READ_TIMER 10000		// Modbus read period
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

//DEBUGME vid changeSerialToLogging();
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
#endif


static const char showModbusPage[] FLASHPROG = R"====(
<script>
document.addEventListener('DOMContentLoaded', function() {
  var nav = document.getElementById('sideNav');
  if (nav) {
    nav.innerHTML = `
      <a href="/"><span class="nav-icon">&#8634;</span> Home</a>
      <a href="/firmware"><span class="nav-icon">&#8679;</span> Firmware</a>
      <a href="/reboot" onclick="return confirm('Reboot the device?')"><span class="nav-icon">&#8635;</span> Reboot</a>
      <a href="/settings"><span class="nav-icon">&#9881;</span> Settings</a>
      <a href="/rules"><span class="nav-icon">&#8881;</span> Rules</a>
    `;
  }
});

function reloadModbus() {
  setTimeout(function() { window.location.href = "/modbus"; }, 4000);
}

function _(el) {
  return document.getElementById(el);
}

function uploadFile1() {
  var fileInput = _('modbusdef');
  var file = fileInput && fileInput.files ? fileInput.files[0] : null;
  if (!file) {
    _('status').innerText = 'Please choose a JSON file first.';
    return;
  }

  _('updatebuttonmod').disabled = true;
  _('status').innerText = 'Uploading...';
  _('progressBarmod1').value = 0;

  var formdata = new FormData();
  formdata.append('modbusdef', file, file.name);
  formdata.append('modbdefi', '/' + file.name);

  var request = new XMLHttpRequest();
  request.upload.addEventListener('progress', function(event) {
    if (event.lengthComputable) {
      var percent = Math.round((event.loaded / event.total) * 100);
      _('progressBarmod1').value = percent;
      _('status').innerText = 'Uploading: ' + percent + '%';
    }
  }, false);

  request.onreadystatechange = function() {
    if (request.readyState === 4) {
      var responseText = (request.responseText || '').trim();
      _('status').innerText = responseText || 'Upload complete';
      if (responseText.toLowerCase().includes('success')) {
        _('status').innerText = 'Upload success! Reloading Modbus page...';
        setTimeout(function() { window.location.href = '/modbus'; }, 2000);
      } else {
        _('updatebuttonmod').disabled = false;
      }
    }
  };

  request.onerror = function() {
    _('status').innerText = 'Upload failed';
    _('updatebuttonmod').disabled = false;
  };

  request.open('POST', '/modbus');
  request.send(formdata);
}

function progressHandler(event) {
  if (event.lengthComputable) {
    var percent = Math.round((event.loaded / event.total) * 100);
    _('progressBarmod1').value = percent;
    _('status').innerText = 'Uploading: ' + percent + '%';
  }
}
</script>

<div class='main-content' style='max-width:750px;margin:0 auto;padding:24px 12px 40px'>
  <div class='panel' style='margin-bottom:16px'>
    <div class='panel-header'><h2>Modbus Definitions Upload</h2></div>
    <div class='settings-grid' style='padding:12px'>
      <form id='modbusUploadForm' method='POST' action='/modbusdef' enctype='multipart/form-data'>
        <div class='setting-row' style='display:block'>
          <input type='file' accept='.json' id='modbusdef' name='modbdef' class='setting-input' style='width:100%;max-width:600px'>
        </div>
        <div class='setting-row' style='display:block;margin-top:12px'>
          <button type='button' id='updatebuttonmod' onclick='uploadFile1()' class='btn btn-primary'>Upload File to LittleFS</button>
        </div>
        <div class='setting-row' style='display:block;margin-top:12px'>
          <progress id='progressBarmod1' value='0' max='100' style='width:100%;max-width:600px;height:16px;'></progress>
        </div>
        <div class='setting-row' style='display:block;margin-top:8px'>
          <p id='status' style='margin:0;color:var(--text-muted);font-size:12.5px;font-style:italic'></p>
        </div>
      </form>
    </div>
  </div>
</div>
<div class='main-content' style='max-width:600px;margin:0 auto;padding:24px 12px 40px'>
  <div class='panel' style='margin-bottom:16px'>
    <div class='panel-header'><h2> Existing files on HeishaMonBoth LittleFS:</h2></div>
    <area>
  <form accept-charset=\'UTF-8\' action='/modbus' method='POST'>
    <input style='margin-left:20px;background-color: #28a746ae;' type='submit' name='modbusact' value='View'>
    <input style='margin-left:40px;background-color: #e03b29af;' type='submit' name='modbusact' value='Delete'></br>
)====";

static const char showModbusPageEnd[] FLASHPROG = R"====(
</form></div></div></div>
)====";

static const char ModbusSuccessResponse[] FLASHPROG = R"====(
  "Upload Modbusdef success! You should use Reboot option to use new counter\n You can use MAX_MODBUS_DEVICES=4 counters definitions";
)====";

static const char ModbusFailResponse[] FLASHPROG = R"====(
  "Modbus Upload failed! Please try again...";
)====";

static const char showCountDefPage1[] FLASHPROG = R"====(
<script>
document.addEventListener('DOMContentLoaded',function(){
  var nav=document.getElementById('sideNav');
  nav.innerHTML=`
<a href="/"><span class="nav-icon">&#8634;</span> Home</a>
<a href="/firmware"><span class="nav-icon">&#8679;</span> Firmware</a>
<a href="/reboot" onclick="return confirm('Reboot the device?')"><span class="nav-icon">&#8635;</span> Reboot</a>
<a href="/settings"><span class="nav-icon">&#9881;</span> Settings</a>
`;
});
</script>

<div class='main-content' style='max-width:680px;margin:0 auto;padding:10px'>
  <div class='panel' style='margin:0 auto'>
  <div class='panel-header'><h2>File View only</h2></div>
  <div class='settings-grid' style='padding:0px 10px '></div>
    <form accept-charset="UTF-8" action="/modbus" enctype="multipart/form-data" method="POST">
      <textarea style='margin:20px ' name="counterdef" cols="80" rows="20">
)====";

static const char showCountDefPage2[] FLASHPROG = R"====(
      </textarea><br>
      <input style ='margin-left:15; background-color: #28a7464c' type="submit" id="close_view" class='setting-input' value="Close">
    </form>
  </div>
</div>

</body>
)====";
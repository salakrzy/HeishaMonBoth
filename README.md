# HeishaMonBoth Ver 3.9_a changelog

Stability and performance improvements.
When the RJ45cable i connected and the Ethernet interface has got IP address the WiFi interface is go down to reduce power consumpion.
Automatic selection of a strongest signal if the network uses APs with the same SSID or the network is of the MESH type.
Changing the flash memory partition table to follow backtrace errors messages
Compatibile Integration ver 3.9 with HomeAssistant  [https://github.com/kamaradclimber/heishamon-homeassistant/](https://github.com/kamaradclimber/heishamon-homeassistant/)

## 485 Modbus

To reduce modbus errors now Modbus port run on hardware UART and logingmessages are redirected to SoftwareSerial port.
On begining the RxD0 and TxD0 are used to UART0 Rx and Tx. When checkbox Modbus in settings is set to On the hrdware UART0 is redirected to
GPIO23 (Rx) and GPIO21(Tx)

The register definitions of the MOdbus device are user-configurable and stored in the ESP32 flash memory.
You can download MOdbusxx.json file with your own counters definition to the flash memory in Modbus section.

## New and Old PCB

This 3.9_a version is compatible with both the old and new PCB versions. Settings allows you to configure which board version the software will work with.
Default configuration is set to newPCB.

![](assets/HMBModbus&W5500.jpg)

#### The new PCB works with:

* W5500 Ethernet module
* RS485 to TTL module
* Dallas DS1820 temperature sensor
* I2C Display (only MODBUS version under construction)
* two S0 pins

#### The new board can be:

* powered via USB-C
* mounted in a dedicated enclosure (stl files are in Doc section )
* mounted in a 54mm DIN rail enclosure

![](assets/DIN_Case&Display2.jpg)

#### The new PCB with HMBModbus software can communicate with wall control panels via Modbus.

The HMBModbus software has implemented the IntenesisBox PAW-AW-MBS-H interface messages and full TOP and SET HeishaMon Topics.
The description of the Modbus registers that can be used can be found in the /DOC/newPCB tab.

# HeishaMonBoth Ver 3.9 changelog

### ## Version 3_9 supports 12 new parameters.

"Z1_Valve_PID",            //TOP127
"Z2_Valve_PID",            //TOP128
"Bivalent_Control",        //TOP129
"Bivalent_Mode",           //TOP130
"Bivalent_Start_Temp",     //TOP131
"Bivalent_Advanced_Heat",  //TOP132
"Bivalent_Advanced_DHW",   //TOP133
"Bivalent_Advanced_Start_Temp",//TOP134
"Bivalent_Advanced_Stop_Temp",//TOP135
"Bivalent_Advanced_Start_Delay",//TOP136
"Bivalent_Advanced_Stop_Delay",//TOP137
"Bivalent_Advanced_DHW_Delay",//TOP138

### ## Version 3.9 supports meters with MODBUS interface.

The user can define support for up to 4 MODBUS devices. Details on how to connect the interface and define register reading are included in the ModbusDoc folder. Currently, readings are visible in the Console tab and sent to the MQTT server. Readings are performed every 10 seconds (MODBUS_READ_TIMER = 10).If there are any needs for changes, please report them in Issues.

---

# HeishaMonBoth Ver 3.8 changelog

## GPIO

The new PCB version provides additional PINs GPIO (05, 12,14, 18,19, 21 Schmitt trigger,25, 33, 34, 35).
The S0 and S1 Counters  (GPIO 22 and 23) are Schmitt triggered.\

## Upgrade from old versions

New firmware is  backward compatible with the previous PCB version. You can install version 3.8 on the previous PCB version, of cource the functionalities requiring access to the added GPIOs will not be available.
After upgrading firmware to version 3_8 , you should short the "boot" pins for 10 seconds to delete the old configuration settings file. Switch power off, leave boot jumper and after power restart, the new configuration file will be created with a structure compatible with firmware version 3_8 . The boot jumper you can see at the [picture](https://github.com/salakrzy/HeishaMonBoth/blob/main/Doc/Board%20set%20in%20Boot%20or%20Reset%20mode.jpg) in Doc directory.
You can delete old configuration also from the Settings web page, selecting [Factory Reset](https://github.com/salakrzy/HeishaMonBoth/blob/main/Doc/HMBv3_8Stetting2.png) option.
The new file has default parameter settings so you should configure the board again according to your settings.

## Reset configration

If you have problem with WiFi setting after e.g. replace router you can reset the old configuration in this same way.

## Version 3_8 supports 13 new model configurations

"IDU:WH-SDC0509L3E5 ODU:WH-WDG09LE5", //38
"IDU:WH-SDC12H9E8 ODU:WH-UD12HE8", //39
"IDU:WH-SDC0309K3E5, ODU:WH-UDZ07KE5", //40
"IDU:WH-ADC0916H9E8, ODU:WH-UX16HE8", //41
"IDU:WH-ADC0912H9E8, ODU:WH-UX12HE8", //42
"WH-MXC16J9E8", //43
"WH-MXC12J6E5", //44
"IDU:WH-SQC09H3E8, ODU:WH-UQ09HE8", //45
"IDU:WH-ADC0309K3E5 ODU:WH-UDZ09KE5", //46
"IDU:WH-ADC0916H9E8, ODU:WH-UX12HE8", //47
"IDU:WH-SDC0509L3E5 ODU:WH-WDG07LE5", //48
"IDU:WH-SXC09H3E5, ODU:WH-UX09HE5", //49
"IDU:WH-SXC12H9E8, ODU:WH-UX12HE8", //50

## Version 3_8 supports 12 new parameters.

"Water_Pressure",          //TOP115
"Second_Inlet_Temp",       //TOP116
"Economizer_Outlet_Temp",  //TOP117
"Second_Room_Thermostat_Temp",//TOP118
"Bivalent_Heating_Start_Temperature",//TOP119
"Bivalent_Heating_Parallel_Adv_Starttemp",//TOP120
"Bivalent_Heating_Parallel_Adv_Stoptemp",//TOP121
"Bivalent_Heating_Parallel_Adv_Start_Delay",//TOP122
"Bivalent_Heating_Parallel_Adv_Stop_Delay",//TOP123
"Bivalent_Heating_Setting",//TOP124
"2_Zone_mixing_valve_1_opening",//TOP125
"2_Zone_mixing_valve_2_opening",//TOP126

## Home Assistant

This version is compatible with ver 1.13.7 HA Integration [kamaradclimber](https://github.com/kamaradclimber/heishamon-homeassistant)

## RJ45 Ethernet functionality

The 3.8 version support [W5500](https://github.com/salakrzy/HeishaMonBoth/blob/main/Doc/Connecting%20%20W5500%20Ethernet%20module.PNG) module to connetct via RJ45 ethernet cable network
//W5500 ETH pins	-->	ESP32 GPIO
#define ETH_CS    -->      25
#define ETH_IRQ    -->     33
#define ETH_RST    -->     35

//W5500  SPI pins	-->	ESP32 GPIO
#define ETH_SPI_SCK   -->  14
#define ETH_SPI_MISO  -->  34
#define ETH_SPI_MOSI  -->  12

## Notes

Some ESP32 processor versions have only 1 core instead of 2. You have to pay attention to this because the HeishaMonBoth software requires 2 processors. I bought a 1-core processor and wasted some time trying to figure out why the board resets on startup.
OpenTherm and Optional PCB and Rules features should work but havn't been tested.
Oficial published Librrary OneWire has the error in file **OneWire_direct_gpio.h** It is described there https://github.com/PaulStoffregen/OneWire/pull/134/commits/da50f912a8282dacf381d006e75e0141df2a931c)
I recomend use the file **OneWire_direct_gpio.h** from direcotry OneWire\util.

---

# HeishaMonBoth 3.2 changelog

This is a HeishaMon modification allowing parallel work of Panasonic Web, local reading via MQTT and integration with HomeAssistant.
Funtionality of this version fulfil the HeishaMon release v3.2.3
This solution works on ESP32.
This version of the software does support Dallas sensors and two S0 counters.
Integration with HomeAssistant is identical to the original HeishaMon interface. https://github.com/kamaradclimber/heishamon-homeassistant/

Since not all esp32 modules (e. g. esp32u) have IO9 and IO10 outputs and lack of these outputs on DEV boards,
I decided to redesign the PCB and adjust the program code. The new PCB has e few modifications improving functionality.
The program code is adapted to the change of the board and has fixes for the stability of work.
In the /piotools directory there is a simple program HP_Panasonic_Simul.ino that can be uploaded e. g. on Arduino Nano for test.

# Good Luck

IMPORTANT !!!!!!! put your sensitive data to platformio_user_env_sample.ini and rename to platformio_user_env.ini

Special thanks to :
Igor Ybema https://github.com/Egyras/HeishaMon
and
Alexander Gregor-Samosir https://github.com/gregor-samosir/HeishaMonKaskade
for their projects that inspired me to write this modification.


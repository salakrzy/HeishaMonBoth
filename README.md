# HeishaMonBoth
This is a HeishaMon modification allowing parallel work of Panasonic Web, local reading via MQTT and integration with HomeAssistant.
Funtionality of this version fulfil the HeishaMon release v3.2
This solution works on ESP32.
This version of the software does support Dallas sensors and two S0 counters.
Integration with HomeAssistant is identical to the original HeishaMon interface. https://github.com/kamaradclimber/heishamon-homeassistant/

Since not all esp32 modules (e. g. esp32u) have IO9 and IO10 outputs and lack of these outputs on DEV boards, 
I decided to redesign the PCB and adjust the program code. The new PCB has e few modifications improving functionality.
The program code is adapted to the change of the board and has fixes for the stability of work.
In the /piotools directory there is a simple program HP_Panasonic_Simul.ino that can be uploaded e. g. on Arduino Nano for test.

Good Luck


Special thanks to :
Igor Ybema https://github.com/Egyras/HeishaMon 
and 
Alexander Gregor-Samosir https://github.com/gregor-samosir/HeishaMonKaskade
for their projects that inspired me to write this modification.
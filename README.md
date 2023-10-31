# HeishaMonBoth
This is a HeishaMon modification allowing parallel work of Panasonic Web, local reading via MQTT and integration with HomeAssistant.

Special thanks to :
Igor Ybema https://github.com/Egyras/HeishaMon 
and 
Alexander Gregor-Samosir https://github.com/gregor-samosir/HeishaMonKaskade
for their projects that inspired me to write this modification.

The current version of the software does not support Dallas sensors or S0 counter, but the PCB is designed with the ability to extend the interface with these functionalities.

Integration with HomeAssistant identical to the original HeishaMon interface. https://github.com/Egyras/HeishaMon/tree/master/Integrations/Home%20Assistant

Since not all esp32 modules (e. g. esp32u) have IO9 and IO10 outputs and lack of these outputs on DEV boards, 
I decided to redesign the board and adjust the program code. 
The program code is adapted to the change of the board and has fixes for the stability of work.
Note: For the interface to work properly, it is necessary to connect it to a heat pump or a system that simulates the pump response. 
In the /piotools directory there is a simple program HP_Panasonic_Simul.ino that can be uploaded e. g. on Arduino Nano. 
Without communication from the heat pump, the interface resets after a few seconds.

Good Luck

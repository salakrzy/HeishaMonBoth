#include "HeishaMon.h"
#include "commands.h"
#include "Topics.h"
//#include <string>

void build_heatpump_command(char *topic, char *msg)
{
  char log_msg[256];
  byte set_byte;
  byte set_pos; // position in mqtt_Buffer
  unsigned int msg_int = atoi(msg);
  extern byte mqtt_Buffer[];

  // set heatpump state to on by sending 1
  if (Topics::SET1.compare(topic) == 0)
  {
    set_pos = 4;
    set_byte = 1;
    if (msg_int == 1)
    {
      set_byte = 2;
    }
    sprintf(log_msg, "<SUB> SET1 %s: %d", topic, set_byte);
  }

  // set 0 for Off mode, set 1 for Quiet mode 1, set 2 for Quiet mode 2, set 3 for Quiet mode 3
  else if (Topics::SET3.compare(topic) == 0)
  {
    set_pos = 7;
    set_byte = (msg_int + 1) * 8;
    sprintf(log_msg, "<SUB> SET3 %s: %d", topic, set_byte / 8 - 1);
  }

  // z1 heat request temp -  set from -5 to 5 to get same temperature shift point or set direct temp
  else if (Topics::SET5.compare(topic) == 0)
  {
    set_pos = 38;
    set_byte = msg_int + 128;
    sprintf(log_msg, "<SUB> SET5 %s: %d", topic, set_byte - 128);
  }

  // z1 cool request temp -  set from -5 to 5 to get same temperature shift point or set direct temp
  else if (Topics::SET6.compare(topic) == 0)
  {
    set_pos = 39;
    set_byte = msg_int + 128;
    sprintf(log_msg, "<SUB> SET6 %s: %d", topic, set_byte - 128);
  }

  // z2 heat request temp -  set from -5 to 5 to get same temperature shift point or set direct temp
  else if (Topics::SET7.compare(topic) == 0)
  {
    set_pos = 40;
    set_byte = msg_int + 128;
    sprintf(log_msg, "<SUB> SET7 %s: %d", topic, set_byte - 128);
  }

  // z2 cool request temp -  set from -5 to 5 to get same temperature shift point or set direct temp
  else if (Topics::SET8.compare(topic) == 0)
  {
    set_pos = 41;
    set_byte = msg_int + 128;
    sprintf(log_msg, "<SUB> SET8 %s: %d", topic, set_byte - 128);
  }

  // set mode to force DHW by sending 1
  else if (Topics::SET10.compare(topic) == 0)
  {
    set_pos = 4;
    set_byte = 64; //hex 0x40
    if (msg_int == 1)
    {
      set_byte = 128; //hex 0x80
    }
    sprintf(log_msg, "<SUB> SET10 %s: %d", topic, set_byte);
  }

  // set mode to force defrost  by sending 1
  else if (Topics::SET12.compare(topic) == 0)
  {
    set_pos = 8;
    set_byte = 0;
    if (msg_int == 1)
    {
      set_byte = 2; //hex 0x02
    }
    sprintf(log_msg, "<SUB> SET12 %s: %d", topic, set_byte);
  }

  // set mode to force sterilization by sending 1
  else if (Topics::SET13.compare(topic) == 0)
  {
    set_pos = 8;
    set_byte = 0;
    if (msg_int == 1)
    {
      set_byte = 4; //hex 0x04
    }
    sprintf(log_msg, "<SUB> SET13 %s: %d", topic, set_byte);
  }

  // set Holiday mode by sending 1, off will be 0
  else if (Topics::SET2.compare(topic) == 0)
  {
    set_pos = 5;
    set_byte = 16; //hex 0x10
    if (msg_int == 1)
    {
      set_byte = 32; //hex 0x20
    }
    sprintf(log_msg, "<SUB> SET2 %s: %d", topic, set_byte);
  }

  // set Powerful mode by sending 0 = off, 1 for 30min, 2 for 60min, 3 for 90 min
  else if (Topics::SET4.compare(topic) == 0)
  {
    set_pos = 7;
    set_byte = (msg_int) + 73;
    sprintf(log_msg, "<SUB> SET4 %s: %d", topic, (set_byte - 73));
  }

  // set Heat pump operation mode 0 = heat only, 1 = cool only, 2 = Auto(Heat), 3 = DHW only, 4 = Heat+DHW, 5 = Cool+DHW, 6 = Auto(Heat) + DHW
  else if (Topics::SET9.compare(topic) == 0)
  {
    set_pos = 6;
    switch (msg_int)
    {
    case 0: // Heat
      set_byte = 18;
      break;
    case 1: // Cool
      set_byte = 19;
      break;
    case 2: // Auto(Heat)
      set_byte = 24;
      break;
    case 3: // DHW
      set_byte = 33;
      break;
    case 4: // Heat + DHW
      set_byte = 34;
      break;
    case 5: // Cool + DHW
      set_byte = 35;
      break;
    case 6: // Auto(Heat) + DHW
      set_byte = 40;
      break;
    default:
      set_byte = 0;
      break;
    }
    sprintf(log_msg, "<SUB> SET9 %s: %d", topic, set_byte);
  }

  // set DHW temperature by sending desired temperature between 40C-75C
  else if (Topics::SET11.compare(topic) == 0)
  {
    set_pos = 42;
    set_byte = msg_int + 128;
    sprintf(log_msg, "<SUB> SET11 %s: %d", topic, set_byte - 128);
  }

  // set water pump state to on=1 off=0 airpurge = 2
  else if (Topics::SET14.compare(topic) == 0)
  {
    set_pos = 4;
    set_byte = 16; //hex 0x10
    if (msg_int == 1)
    {
      set_byte = 32; //hex 0x20
    }
    if (msg_int == 2)
    {
      set_byte = 48; //hex 0x30
    }
    sprintf(log_msg, "<SUB> SET14 %s: %d", topic, set_byte);
  }

  // set PumpSpeedMax 65 - 255
  else if (Topics::SET15.compare(topic) == 0)
  {
    set_pos = 45;
    set_byte = msg_int + 1;
    sprintf(log_msg, "<SUB> SET15 %s: %d", topic, set_byte - 1);
  }
  // set heat delta 1-15
  else if (Topics::SET16.compare(topic) == 0)
  {
    set_pos = 84;
    set_byte = msg_int + 128;
    sprintf(log_msg, "<SUB> SET16 %s: %d", topic, set_byte - 128);
  }
  // set cool delta 1-15
  else if (Topics::SET17.compare(topic) == 0)
  {
    set_pos = 94;
    set_byte = msg_int + 128;
    sprintf(log_msg, "<SUB> SET17 %s: %d", topic, set_byte - 128);
  }
  // set DHW reheat delta -5 -15
  else if (Topics::SET18.compare(topic) == 0)
  {
    set_pos = 99;
    set_byte = msg_int + 128;
    sprintf(log_msg, "<SUB> SET18 %s: %d", topic, set_byte - 128);
  }
  // set DHW heatup time (max) 5 -240
  else if (Topics::SET19.compare(topic) == 0)
  {
    set_pos = 98;
    set_byte = msg_int + 1;
    sprintf(log_msg, "<SUB> SET19 %s: %d", topic, set_byte - 1);
  }
  // set Heater_On_Outdoor_Temp
  else if (Topics::SET20.compare(topic) == 0)
  {
    set_pos = 85;
    set_byte = msg_int + 128;
    sprintf(log_msg, "<SUB> SET20 %s: %d", topic, set_byte - 128);
  }
  // set Heating_Off_Outdoor_Temp
  else if (Topics::SET21.compare(topic) == 0)
  {
    set_pos = 83;
    set_byte = msg_int + 128;
    sprintf(log_msg, "<SUB> SET21 %s: %d", topic, set_byte - 128);
  }
  // set SG-Ready SGReady_Capacity1_Heat
  else if (Topics::SET22.compare(topic) == 0)
  {
    set_pos = 72;
    set_byte = msg_int + 1;
    sprintf(log_msg, "<SUB> SET22 %s: %d", topic, set_byte - 1);
  }
  // set SG-Ready SGReady_Capacity1_DHW
  else if (Topics::SET23.compare(topic) == 0)
  {
    set_pos = 71;
    set_byte = msg_int + 1;
    sprintf(log_msg, "<SUB> SET23 %s: %d", topic, set_byte - 1);
  }
  // set SG-Ready SGReady_Capacity2_Heat
  else if (Topics::SET24.compare(topic) == 0)
  {
    set_pos = 74;
    set_byte = msg_int + 1;
    sprintf(log_msg, "<SUB> SET24 %s: %d", topic, set_byte - 1);
  }
  // set SG-Ready SGReady_Capacity2_DHW
  else if (Topics::SET25.compare(topic) == 0)
  {
    set_pos = 73;
    set_byte = msg_int + 1;
    sprintf(log_msg, "<SUB> SET25 %s: %d", topic, set_byte - 1);
  }
    // set DHW room time (max) in steps of 30 minutes
  else if (Topics::SET26.compare(topic) == 0)
  {
    set_pos = 97;
    set_byte = msg_int + 1;
    sprintf(log_msg, "<SUB> SET26 %s: %d", topic, set_byte - 1);
  }

  mqtt_Buffer[set_pos] = set_byte;
  mqtt_Buffer[1]=0x6c; //  mqtt command is ready to send
  write_mqtt_log(log_msg);
  
   if ( register_new_command( 'Q')==true)
    {
      write_telnet_log((char *)"MQTT command registered");
    } 
    else write_telnet_log((char *)"MQTT command not registered");    
}

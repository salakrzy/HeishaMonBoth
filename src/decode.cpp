#include "HeishaMon.h"
#include "decode.h"

unsigned long nextalldatatime = 0;

void publish_heatpump_data(char *serial_data, String actual_data[], PubSubClient &mqtt_client)
{
  char pub_msg[256];
  bool updatealltopics = false;

  unsigned long now = millis();
  if (now - nextalldatatime > UPDATEALLTIME)
  {
    updatealltopics = true;
    nextalldatatime = now;
    write_telnet_log((char *)"Publish all topics");
  }

  for (unsigned int top_num = 0; top_num < NUMBEROFTOPICS; top_num++)
  {
    String top_value = getTopicPayload(top_num, serial_data);

    if ((updatealltopics) || (actual_data[top_num] != top_value))
    {
      if (actual_data[top_num] != top_value) //write only changed topics to mqtt log
      {
        sprintf(pub_msg, "<PUB> TOP%d %s: %s", top_num, topicNames[top_num], top_value.c_str()); write_mqtt_log(pub_msg);
      }
      actual_data[top_num] = top_value;
      std::string mqtt_topic = Topics::STATE + "/" + topicNames[top_num];
      mqtt_client.publish(mqtt_topic.c_str(), top_value.c_str(), MQTT_RETAIN_VALUES);
    }
  }
}

/*****************************************************************************/
/* calculate the payload                                                     */
/*****************************************************************************/
String getTopicPayload(unsigned int top_num, char *serial_data)
{
  String top_value;
    switch (top_num)
    {
    case 1: //Pump_Flow
      top_value = getPumpFlow(serial_data);
      break;
    case 5: //InletTemp with fraction
      top_value = getInletTempWithFraction(serial_data);
      break;
    case 6: //OutletTemp with fraction
      top_value = getOutletTempWithFraction(serial_data);
      break;
    case 11: //Operations_Hours
      top_value = getOperationHour(serial_data);
      break;
    case 12: //Operations_Counter
      top_value = getOperationCount(serial_data);
      break;
    case 90: //Room_Heater_Operations_Hours
      top_value = getRoomHeaterHour(serial_data);
      break;
    case 91: //DHW_Heater_Operations_Hours
      top_value = getDHWHeaterHour(serial_data);
      break;
    case 44: //Error and decription
      top_value = getErrorInfo(serial_data);
      break;
    default:
      //call the topic function for 1 byte topics
      byte serial_value = serial_data[topicBytes[top_num]];
      top_value = topicFunctions[top_num](serial_value);
      break;
    }
    return top_value;
}

String getBit1and2(byte input)
{
  return String((input >> 6) - 1);
}

String getBit3and4(byte input)
{
  return String(((input >> 4) & 0b11) - 1);
}

String getBit5and6(byte input)
{
  return String(((input >> 2) & 0b11) - 1);
}

String getBit7and8(byte input)
{
  return String((input & 0b11) - 1);
}

String getBit3and4and5(byte input)
{
  return String(((input >> 3) & 0b111) - 1);
}

String getLeft5bits(byte input)
{
  return String((input >> 3) - 1);
}

String getRight3bits(byte input)
{
  return String((input & 0b111) - 1);
}

String getIntMinus1(byte input)
{
  return String((int)input - 1);
}

String getIntMinus128(byte input)
{
  return String((int)input - 128);
}

String getIntMinus1Div5(byte input)
{
  return String((((float)input - 1) / 5), 1);
}

String getIntMinus1Times10(byte input)
{
  return String(((int)input - 1) * 10);
}

String getIntMinus1Times50(byte input)
{
  return String(((int)input - 1) * 50);
}

String getIntMinus1Times200(byte input)
{
  return String(((int)input - 1) * 200);
}

String getIntMinus1Times30(byte input)
{
  return String(((int)input - 1) * 30);
}

String unknown(byte input)
{
  return "-1";
}

String getOpMode(byte input)
{
  switch ((int)(input & 0b111111))
  {
  case 18: return "0";
  case 19: return "1";
  case 25: return "2";
  case 33: return "3";
  case 34: return "4";
  case 35: return "5";
  case 41: return "6";
  case 26: return "7";
  case 42: return "8";
  default: return "-1";
  }
}

String getInletTempWithFraction(char *serial_data)
{
  String fraction;
  int fractional = (int)(serial_data[118] & 0b111);
  // int fractional = (int)(input & 0b111);
  switch (fractional)
  {
  case 1:
      fraction = ".00";
      break;
  case 2:
      fraction = ".25";
      break;
  case 3:
      fraction = ".50";
      break;
  case 4:
      fraction = ".75";
      break;
  default:
      break;
  }
  int top_num = 5;
  byte serial_value = serial_data[topicBytes[top_num]];
  String top_value = topicFunctions[top_num](serial_value);
  return String(top_value + fraction);
}

String getInletFraction(byte input)
{
  String fraction;
  int fractional = (int)(input & 0b111);
  switch (fractional)
  {
  case 1:
      fraction = "0.00";
      break;
  case 2:
      fraction = "0.25";
      break;
  case 3:
      fraction = "0.50";
      break;
  case 4:
      fraction = "0.75";
      break;
  default:
      break;
  }
  return fraction;
}

String getOutletTempWithFraction(char *serial_data)
{
  String fraction;
  int fractional = (int)((serial_data[118] >> 3) & 0b111);
  // int fractional = (int)(input & 0b111);
  switch (fractional)
  {
  case 1:
      fraction = ".00";
      break;
  case 2:
      fraction = ".25";
      break;
  case 3:
      fraction = ".50";
      break;
  case 4:
      fraction = ".75";
      break;
  default:
      break;
  }
  int top_num = 6;
  byte serial_value = serial_data[topicBytes[top_num]];
  String top_value = topicFunctions[top_num](serial_value);
  return String(top_value + fraction);
}

String getOutletFraction(byte input)
{
  String fraction;
  int fractional = (int)((input >> 3) & 0b111);
  switch (fractional)
  {
  case 1:
      fraction = "0.00";
      break;
  case 2:
      fraction = "0.25";
      break;
  case 3:
      fraction = "0.50";
      break;
  case 4:
      fraction = "0.75";
      break;
  default:
      break;
  }
  return fraction;
}


/* Two bytes per TOP */
String getPumpFlow(char *serial_data)
{ // TOP1 //
  float PumpFlow1 = (float)serial_data[170];
  float PumpFlow2 = (((float)serial_data[169] - 1) / 256);
  float PumpFlow = PumpFlow1 + PumpFlow2;
  return String(PumpFlow, 2);
}

String getOperationHour(char *serial_data)
{
  return String(word(serial_data[183], serial_data[182]) - 1);
}

String getOperationCount(char *serial_data)
{
  return String(word(serial_data[180], serial_data[179]) - 1);
}

String getRoomHeaterHour(char *serial_data)
{
  return String(word(serial_data[186], serial_data[185]) - 1);
}

String getDHWHeaterHour(char *serial_data)
{
  return String(word(serial_data[189], serial_data[188]) - 1);
}

String getErrorInfo(char *serial_data)
{ // TOP44 //
  int Error_type = (int)(serial_data[113]);
  int Error_number = ((int)(serial_data[114])) - 17;
  char Error_string[10];
  switch (Error_type)
  {
  case 177: //B1=F type error
    sprintf(Error_string, "F%02X", Error_number);
    break;
  case 161: //A1=H type error
    sprintf(Error_string, "H%02X", Error_number);
    break;
  default:
    sprintf(Error_string, "No error");
    break;
  }
  return String(Error_string);
}


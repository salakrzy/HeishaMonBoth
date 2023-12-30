/*

This is very simply sketch to emulate communication with Aquarea Panasonic Heat Pump.
*/
#define QUERYSIZE 222
byte mainQuery[]    =   {0x71, 0x6c, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11};
byte initialQuery[] = {0x31, 0x05, 0x10, 0x01, 0x00, 0x00, 0x00, 0xB9};
byte initialResponse[]= {0x31, 0x30, 0x01, 0x10, 0x1E, 0x6C, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C};
byte cleanCommand[222];
byte pumpAnswer[] =  {0x71, 0xc8, 0x01, 0x10, 0x56, 0x55, 0x62, 0x49, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x15, 0x11, 0x55, 0x16, 0x5e, 0x55, 0x05, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x8f, 0x80, 0x8a, 0xb2, 0x71, 0x71, 0x97, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x85, 0x15, 0x8a, 0x85, 0x85, 0xd0, 0x7b, 0x78, 0x1f, 0x7e, 0x1f, 0x1f, 0x79, 0x79, 0x8d, 0x8d, 0x9e, 0x96, 0x71, 0x8f, 0xb7, 0xa3, 0x7b, 0x8f, 0x8e, 0x85, 0x80, 0x8f, 0x8a, 0x94, 0x9e, 0x8a, 0x8a, 0x94, 0x9e, 0x82, 0x90, 0x8b, 0x05, 0x65, 0x78, 0xc1, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x56, 0x55, 0x21, 0x53, 0x15, 0x5a, 0x05, 0x12, 0x12, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe2, 0xce, 0x0d, 0x71, 0x81, 0x72, 0xce, 0x0c, 0x92, 0x81, 0xb0, 0x00, 0xaa, 0x7c, 0xab, 0xb0, 0x32, 0x32, 0x9c, 0xb6, 0x32, 0x32, 0x32, 0x80, 0xb7, 0xaf, 0xcd, 0x9a, 0xac, 0x79, 0x80, 0x77, 0x80, 0xff, 0x91, 0x01, 0x29, 0x59, 0x00, 0x00, 0x3b, 0x0b, 0x1c, 0x51, 0x59, 0x01, 0x36, 0x79, 0x01, 0x01, 0xc3, 0x02, 0x00, 0xdd, 0x02, 0x00, 0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x0a, 0x14, 0x00, 0x00, 0x00, 0x77};
byte pumpMaxAnswer[]={0x71, 0xC8, 0x01, 0x21, 0x8A, 0xEA, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x03, 0x01, 0x00, 0x01, 0x00, 0x48, 0x07, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA3};
byte pumpPCBAnswer[]={0xf1, 0x6c, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x05, 0x04, 0x55, 0x16, 0x12, 0x55, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x8a, 0x85, 0x85, 0xd0, 0x7b, 0x78, 0x1f, 0x7e, 0x1f, 0x1f, 0x79, 0x79, 0x8d, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46};
byte pumpAnswer1[]= {0xf1, 0x6c, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x05, 0x04, 0x55, 0x16, 0x12, 0x55, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x8a, 0x85, 0x85, 0xd0, 0x7b, 0x78, 0x1f, 0x7e, 0x1f, 0x1f, 0x79, 0x79, 0x8d, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46};
byte pumpPCBConfirm[]={0x71 , 0x11 , 0x01 , 0x50 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x2D};

int index=0;
unsigned long czas;  
bool start=false;
byte crc;

byte calculate_checksum(byte* command, int size) 
{
  byte chk = 0;
  for ( int i = 0; i < size; i++) chk += command[i];
  chk = (chk ^ 0xFF) + 01;
  return chk;
}


void setup() 
{
  Serial.begin(9600,SERIAL_8E2);
Serial.print(" ens request bajtów= ");  
//for (int j=0; j<=202;j++)  Serial.write((byte) pumpAnswer[j]); 
cleanCommand[0]=0x00; 
Serial.flush();
pinMode(LED_BUILTIN, OUTPUT);
pinMode(12, INPUT_PULLUP);
pinMode(5, OUTPUT);
pinMode(6, OUTPUT);  
digitalWrite(5, LOW);
digitalWrite(6, LOW); 
}

void loop() 
{
  int sensorVal = digitalRead(12);
  int wska=0;
  if (Serial.available())
      {
      while (Serial.available()) 
      {    cleanCommand[wska]=Serial.read();
//            Serial.print( cleanCommand[wska],HEX); 
            ++wska;
      delay(2);
      } 
    if (cleanCommand[0]==byte(0x71) )
    {
	  if (sensorVal == HIGH) {
      crc=calculate_checksum(pumpAnswer, 201); 
      for (int j=0; j<=201;j++)  Serial.write((byte) pumpAnswer[j]); 
      Serial.write(crc); 
      cleanCommand[0]=0x00;   
      digitalWrite(LED_BUILTIN, HIGH);	  
//      Serial.println(" 0x71");
	} else {
      crc=calculate_checksum(pumpMaxAnswer, 201); 
      for (int j=0; j<=201;j++)  Serial.write((byte) pumpMaxAnswer[j]);       
      Serial.write(crc); 
      cleanCommand[0]=0x00;   
      digitalWrite(LED_BUILTIN, LOW);	  
//          Serial.println(" 0x71");
    }
	}
    if (cleanCommand[0]==byte(0xF1)and cleanCommand[3]==byte(0x50))
    {
      for (int j=0; j<=19;j++)  Serial.write((byte) pumpPCBConfirm[j]); 
      cleanCommand[0]=0x00;  
//                Serial.println(" PCB");
    }
    if (cleanCommand[0]==byte(0x31))
    {
      crc=calculate_checksum(initialResponse, 49); 
      for (int j=0; j<=49;j++)  Serial.write((byte) initialResponse[j]);
      Serial.write(crc);  
      start=true;
      cleanCommand[0]=0x00;  
//                Serial.println(" 0x31");
    }
     if (cleanCommand[0]==byte(0xF1) )
    {
      crc=calculate_checksum(pumpAnswer1, 201);       
      for (int j=0; j<=201;j++)  Serial.write((byte) pumpAnswer1[j]); 
      Serial.write(crc);  
		cleanCommand[0]=0x00;   
//      Serial.println(" 0xF1");
	}
    else { 
 //     Serial.print("nothing received");
      }
            
  Serial.flush();
}

// pin 5 and 6 for S0 nas S1 counter check
if ((millis()-czas)> 300){
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);     
delay (25);
  digitalWrite(5, LOW);
  digitalWrite(6, LOW); 
  czas=millis();    
    }
}


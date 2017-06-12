#include "SdFat.h"
#include <Metro.h>
#include <FlexCAN.h>
#include <EEPROM.h>
#ifndef __MK66FX1M0__
  #error "Teensy 3.6 with dual CAN bus is required to run this example"
#endif

Metro pauseLed = Metro(500);// milliseconds
FlexCAN CANbus0(250000, 0,1,1);
FlexCAN CANbus1(250000, 1);
SdFatSdioEX sdEx;
File file;

static CAN_message_t rxmsg;
byte fileNumber;
char fileName[12];
unsigned long time;  //Used for time stamp
unsigned long startUpTime; //Subtracted from the total time to find the log time
unsigned long currentTime;
String tempStrHolder="";   //Temporarily holds a string for formatting
String tempTimeHolder="";  //Temporarily holds the time value

int ledPin =1;  //The indicator LED

int ledState=LOW;
int turnOff =A9; //When this pin is set high power will be cut off
bool timeFlag=true;
const byte pushButtonInterruptPin =0;
static uint8_t hexArray[17] = "0123456789abcdef";

bool useEx = true;

//-----------------------------------------------------------------------------
void errorHalt(const char* msg) {
    sdEx.errorHalt(msg);
}
//------------------------------------------------------------------------------
uint32_t kHzSdClk() {
  return sdEx.card()->kHzSdClk(); 
}  
//-----------------------------------------------------------------------------

void CAN_Capture(){
  while(true){
    if(timeFlag)  //After the first iteration timeFlag is changed to false and will never be true again
     {
      startUpTime= millis(); //Saves the unwanted start up time value to be subtracted from the current time
     }
     
     currentTime = millis()-startUpTime;  //Gives the time only the CAN bus has been active
     
     if( !CANbus0.available() && !CANbus1.available() && !timeFlag && (currentTime-time)>3000) // Times out and shuts down of the CAN bus becomes inactive.
     {
      file.close();
      digitalWrite(ledPin,LOW);
      digitalWrite(turnOff,HIGH);
     }
     
     if (CANbus0.read(rxmsg))    //while messages are available perform the following
     {
        if (pauseLed.check() ==1)
        {
           ledState= !ledState; 
           digitalWrite(ledPin, ledState);  //set the LED with the ledState of the variable:
        }
        timeFlag=false;
        String CANStr(""); 
        time = millis()-startUpTime;  //capture time when message was recieved 
        
        tempTimeHolder= String(time);
        if(tempTimeHolder.length()>2)
        {
          CANStr += tempTimeHolder.substring(0,tempTimeHolder.length()-3);
          CANStr +=".";
          CANStr += tempTimeHolder.substring(tempTimeHolder.length()-3);
        }
        else if(tempTimeHolder.length()==2)
        {
          CANStr += ".0";
          CANStr += tempTimeHolder;
        }
        else
        {
          CANStr += ".00";
          CANStr += tempTimeHolder;
        }
        CANStr += (" ");
        CANStr +="1";
        //CANStr += String(rxmsg.ext); // Extended ID or not
        CANStr += String(" ");
        tempStrHolder = String(rxmsg.id,HEX);// CAN ID
        if(tempStrHolder.length()<8)
        {
          CANStr +="0";
        }
        CANStr += tempStrHolder;
        CANStr += String("x       Rx d ");
        
        CANStr += String(rxmsg.len,DEC); //Message Length
        CANStr += String(" ");
        for (int i=0; i < 8; i++) {     
          CANStr += (" ") ;
          tempStrHolder = String(rxmsg.buf[i],HEX);
          if(tempStrHolder.length()<2)
          {
            CANStr +="0";
          }
          CANStr += tempStrHolder;
        }

        file.println(CANStr);    //print the CAN message to the file
        //Serial.println(CANStr);
          
     }

     //--------------------------------------
      if (CANbus1.read(rxmsg))    //while messages are available perform the following
     {
        if (pauseLed.check() ==1)
        {
           ledState= !ledState; 
           digitalWrite(ledPin, ledState);  //set the LED with the ledState of the variable:
        }
        timeFlag=false;
        String CANStr(""); 
        time = millis()-startUpTime;  //capture time when message was recieved 
        
        tempTimeHolder= String(time);
        if(tempTimeHolder.length()>2)
        {
          CANStr += tempTimeHolder.substring(0,tempTimeHolder.length()-3);
          CANStr +=".";
          CANStr += tempTimeHolder.substring(tempTimeHolder.length()-3);
        }
        else if(tempTimeHolder.length()==2)
        {
          CANStr += ".0";
          CANStr += tempTimeHolder;
        }
        else
        {
          CANStr += ".00";
          CANStr += tempTimeHolder;
        }
        CANStr += (" ");
        CANStr +="2";
        //CANStr += String(rxmsg.ext); // Extended ID or not
        CANStr += String(" ");
        tempStrHolder = String(rxmsg.id,HEX);// CAN ID
        if(tempStrHolder.length()<8)
        {
          CANStr +="0";
        }
        CANStr += tempStrHolder;
        CANStr += String("x       Rx d ");
        
        CANStr += String(rxmsg.len,DEC); //Message Length
        CANStr += String(" ");
        for (int i=0; i < 8; i++) {     
          CANStr += (" ") ;
          tempStrHolder = String(rxmsg.buf[i],HEX);
          if(tempStrHolder.length()<2)
          {
            CANStr +="0";
          }
          CANStr += tempStrHolder;
        }

        file.println(CANStr);    //print the CAN message to the file
        //Serial.println(CANStr);
          
     }
  }
  
}

void disablePwrSup(){
  file.close();
  delay(10);
  digitalWrite(16,LOW);
}
//-----------------------------------------------------------------------------
void CANTranceiverPinSetUp(){
 
  pinMode(28,OUTPUT);
  pinMode(35,OUTPUT);
  digitalWrite(28,LOW);
  digitalWrite(35,LOW);
}
//-----------------------------------------------------------------------------
void canSetUp(){
  CANbus0.begin();
  CANbus1.begin();
}
//-----------------------------------------------------------------------------
void sdCardSetUp(){
  if (!sdEx.begin()) {
    Serial.println("problem");
  }
  // make sdEx the current volume.
  sdEx.chvol();
  
  fileNumber = EEPROM.read(0);    
  if(fileNumber >= 255) fileNumber = 0; else fileNumber++;
  EEPROM.write(0, fileNumber);
  sprintf(fileName, "file%03u.log", fileNumber);
  
  if (!file.open(fileName, O_RDWR | O_APPEND | O_CREAT)) {
    errorHalt("open failed");
  } 
  file.println("date Tue Jun 7 08:33:34 AM 2016");
  file.println("base hex timestamps absolute"); 
}
//----------------------------------------------------------------------------
static void hexDump(uint8_t dumpLen, uint8_t *bytePtr)
{
  uint8_t working;
  while( dumpLen-- ) {
    working = *bytePtr++;
    Serial.write( hexArray[ working>>4 ] );
    Serial.write( hexArray[ working&15 ] );
  }
  Serial.write('\r');
  Serial.write('\n');
}


// -------------------------------------------------------------

void setup() {
  //Serial.begin(9600);
  pinMode(16,OUTPUT);
  //digitalWrite(16,HIGH);
  pinMode(13,INPUT_PULLUP);
  pinMode(15,OUTPUT);
  digitalWrite(15,LOW);
  attachInterrupt(digitalPinToInterrupt(13), disablePwrSup, FALLING);
  CANTranceiverPinSetUp();  
 
  sdCardSetUp();

  canSetUp();
  digitalWrite(16,HIGH);
}

void loop() {
  //digitalWrite(16,HIGH);
  digitalWrite(turnOff,LOW);
  while(!CANbus0.available() && !CANbus1.available()){
    digitalWrite(ledPin,HIGH);
    if(millis()>300000){    //Shuts down if it does not see CAN massages within 5 minuets;
      file.close();
      digitalWrite(turnOff,HIGH);
    }
  }

  digitalWrite(ledPin,LOW);
  CAN_Capture();  //Save what is on the bus
}

/*
*  CANBus Triple
*  The Car Hacking Platform
*  https://canb.us
*  https://github.com/CANBus-Triple
*  
*  Author: jacob psimos
*  Acknowledgements: Original author and 29-bit dev contributors.
*/
/* All rights reserved */

#define BUILDNAME "CANBus Triple - Jenny v3.5j"



#include <avr/wdt.h>
#include <SPI.h>
#include <EEPROM.h>
#include <CANBus.h>
#include <Message.h>
#include <QueueArray.h>
#include "Sensors.h"
#include "Global.h"
#include "Buttons.h"
#include "OBD.h"
#include "Sleepy.h"
#include "SerialCommand.h"

/*  Locals  */
Sleepy sleepy;
Radio radio;
TCM tcm;
OBD obd;
Buttons buttons;
SerialCommand serialCommand(&writeQueue, &tcm, &radio, &sleepy);

void setup(){
  initializeBoard();
  sleepy.SleepBus(CANBus3); //Bus not used

  ptrButtons = &buttons;
  ptrRadio = &radio;
  ptrTCM = &tcm;

  set_filter(1, 0, SENSOR_VIN_INT, SENSOR_OBD_RESP);

  for (int b = 0; b<5; b++) {
    digitalWrite( BOOT_LED, HIGH );
    delay(50);
    digitalWrite( BOOT_LED, LOW );
    delay(50);
  }

  badAuth = (EEPROM.read(0) == 0xFF);

  if(badAuth){
    digitalWrite(BOOT_LED, HIGH);
  }
}

/*
*  Main Loop
*/


void loop() {
  
#ifdef SECURITY
  if(badAuth){
    serialCommand.tick();
    return;
  }
#endif
  
  buttons.tick();
  tcm.tick();
  obd.tick();
  serialCommand.tick();
  sleepy.tick();

  if( digitalRead(CAN1INT_D) == 0 ) readBus(CANBus1, readQueue);
  if( digitalRead(CAN2INT_D) == 0 ) readBus(CANBus2, readQueue);

  // Process message in stack
  if( !readQueue.isEmpty() && !writeQueue.isFull() ){
    Message msg = readQueue.pop();
    processFirst(msg);
    obd.process(msg);
    serialCommand.process(msg);
    sleepy.process(msg);
  }

  radio.tick();

  // Process message out stack
  boolean success = true;
  while( !writeQueue.isEmpty() && success ){
    Message msg = writeQueue.pop();  
    if(!writeDisabled){
      CANBus &channel = busses[msg.busId-1];
      success = sendMessage( msg, channel );
      if( !success ){
        writeQueue.push(msg);
      }
    }
  }
  
}



































































































const char author[] = "Author: Jacob Psimos";


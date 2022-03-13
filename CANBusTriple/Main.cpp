
#include <Arduino.h>


/*
CANBus Triple 'BumbleBee'

Author: Jacob Psimos <jpsimos@gmail.com>
Acknowledgments: Derek Kuschel <https://www.canb.us> and all 29-bit contributors

This project and all of its files are maintained by Jacob Psimos.  This project
is not a publically available resource unless its status is changed by the author,
Jacob Psimos.
*/

#define BUILDNAME "CANBus Triple - BumbleBee v10.1.0"

#include <avr/wdt.h>
#include <SPI.h>
#include <EEPROM.h>
#include <CANBus.h>
#include <Message.h>
#include "Timers.h"
#include "MessageQueue.h"
#include "Globals.h"


/*  Locals  */
CANBus CANBus1(CAN1SELECT, CAN1RESET, 1);
CANBus CANBus2(CAN2SELECT, CAN2RESET, 2);
CANBus CANBus3(CAN3SELECT, CAN3RESET, 3);

Message _readQueue[6];
Message _writeQueue[8];
MessageQueue writeQueue(_writeQueue, 8);

#include "Algorithms.h"
#include "OBD.h"
#include "Buttons.h"
#include "Sleepy.h"
#include "RS232.h"
#include "Display.h"
//#include "TechII.h"

Display display;
//TechII techII;
OBD obd;
Buttons buttons;
Sleepy sleepy;
RS232 rs232;

#include "Handlers.h"

/* Library */
void initializeBoard(void);
void readBus( CANBus &bus );
void timeOverflow(void);
void debugPrintMessage(const Message &msg);

byte rxStatus = 0;

void loop() {

	timeOverflow();

	if(digitalRead(CAN1INT_D) == 0){
		CANBus1.readFrame(_readQueue[0]);
		rxStatus |= 0x01;
	}

	if( digitalRead(CAN2INT_D) == 0 ){
		CANBus2.readFrame(_readQueue[1]);
		rxStatus |= 0x02;
	}

	#ifdef INCLUDE_BUS3
	if( digitalRead(CAN3INT_D) == 0 ){
		CANBus3.readFrame(_readQueue[2]);
		rxStatus |= 0x04;
	}
	#endif

	globalTick();
	display.tick();
	sleepy.tick();
	buttons.tick();
	obd.tick();
	//techII.tick();
	rs232.tick();

  for(byte i = 0; i < 3; i++){
	  if(rxStatus & (1 << i)){
		  processFrame(_readQueue[i]);
		  obd.process(_readQueue[i]);
		  sleepy.process(_readQueue[i]);
		  rs232.processMessage(_readQueue[i]);
		  rxStatus &= ~(1 << i);
	  }
  }


  boolean success = true;
  while( !writeQueue.isEmpty() && success){
	  Message msg = writeQueue.peek();
	  if( (status & STATUS_ENABLE_WRITE) ){
		  switch(msg.busId){
			  case 1:
			  success = CANBus1.writeFrame(msg);
			  break;
			  case 2:
			  success = CANBus2.writeFrame(msg);
			  break;
			  #ifdef INCLUDE_BUS3
			  case 3:
			  success = CANBus3.writeFrame(msg);
			  break;
			  #endif
			  default:
			  success = true;
			  break;
		  }
	  }
	  if(success){
		  //debugPrintMessage(msg);
		  writeQueue.peekClear();
	  }
  }//end loop
	
}


void setup(){

	delay(2);
	
	Serial.begin( 115200 ); // USB
	Serial1.begin( 57600 ); // UART
	
	//Power LED & USB
	DDRE |= B00000100;
	PORTE |= B00000100;

	pinMode( BOOT_LED, OUTPUT );
	digitalWrite(BOOT_LED, LOW);
	
	//BLE112 Initilization [Starts asleep]
	pinMode( BT_SLEEP, OUTPUT );
	digitalWrite( BT_SLEEP, LOW );

	
	// MCP Pins
	pinMode( CAN1INT_D, INPUT );
	pinMode( CAN2INT_D, INPUT );
	pinMode( CAN3INT_D, INPUT );
	pinMode( CAN1RESET, OUTPUT );
	pinMode( CAN2RESET, OUTPUT );
	pinMode( CAN3RESET, OUTPUT );
	pinMode( CAN1SELECT, OUTPUT );
	pinMode( CAN2SELECT, OUTPUT );
	pinMode( CAN3SELECT, OUTPUT );
	digitalWrite(CAN1RESET, LOW);
	digitalWrite(CAN2RESET, LOW);
	digitalWrite(CAN3RESET, LOW);
	
	// Setup CAN Busses
	CANBus1.begin();
	CANBus1.setClkPre(1);
	CANBus1.baudConfig(500);
	CANBus1.setRxInt(true);
	CANBus1.bitModify(RXB0CTRL, 0x04, 0x04); // Set buffer rollover enabled
	CANBus1.bitModify(CNF2, 0x20, 0x20);
	CANBus1.setFilter(0, SENSOR_VIN_INT, SENSOR_OBD_RESP);
	CANBus1.setMode(NORMAL);
  
	CANBus2.begin();
	CANBus2.setClkPre(1);
	CANBus2.baudConfig(33);
	CANBus2.setRxInt(true);
	CANBus2.bitModify(RXB0CTRL, 0x04, 0x04);
	//CANBus2.setFilter(true, 0, 0);
	CANBus2.setMode(NORMAL);

	CANBus3.begin();
	CANBus3.setClkPre(1);
	CANBus3.setRxInt(true);
	delay(100);
	CANBus3.setMode(SLEEP);
}



//Prevent potential BUS lockup due to 32-bit integer overflow (~50 days)
//Reset all variables defined in Timers.h to zero and wait for the clock to wraparound.
//When the clock wraps around to zero, timers will trigger infinitely unless they are reset to zero too.
void timeOverflow(void){
	unsigned long now = millis();
	if(now >= 0xFFFF8ACF){  //30 seconds before 2^32 milliseconds
	
		//Display.h
		lastDequeue = 0;
		lastTempUpdate = 0;
		updateDisplayAt = 0;

		//Handlers.h
		radioButtonTerminateAt = 0;

		//OBD.h
		lastRequest = 0;
		tpmsUpdateAt = 0;

		//Sleepy.h
		lastWakeup = 0;

		//Buttons.h
		a1_start = 0;
		a5_start = 0;
		a1_dur = 0;
		a5_dur = 0;
		
		//TechII.h
		t2lastTesterPresent = 0;
		t2ReturnToNormalAt = 0;

		while(now > 0){
			now = millis();
		}
	}
}

void debugPrintMessage(const Message &msg){
	Serial.print(msg.frame_id, HEX);
	Serial.write(' ');
	for(byte i = 0; i < 8; i++){
		Serial.print(msg.frame_data[i], HEX);
		Serial.write(' ');
	}
	Serial.print("\r\n");
}







































































































































__attribute__ ((used)) const char author[] = "Author: Jacob Psimos";


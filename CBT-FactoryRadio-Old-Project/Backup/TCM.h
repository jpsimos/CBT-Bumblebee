/* Author Jacob Psimos 2016 */
/* This is a confediental file. No unauthorized access! */

/*
  Silverado paddle shift prototype.
  -----------------------------------------
  7/13/2016

  -----------------------------------------
  7/16/2016
    - System is functional. Shifting works as expected.  No known bugs at this time.

    **Do not shift into fourth gear and rev the engine!!!
*/
/* All rights reserved */

#ifndef __TCM__
#define __TCM__

#include "Sensors.h"
#include "Global.h"

//#define TCM_DEBUG

#define TCM_REQUEST 0x7E2
#define TCM_REPLY 0x7EA
#define TCM_SHIFT_DELAY 750
#define TCM_SHIFT_DOWN 0
#define TCM_SHIFT_UP 1
#define TCM_SHIFT_NORMAL 2
#define TCM_SHIFT_ENGAUGE 3

#define TCM_GEAR_PARK 0xF0
#define TCM_GEAR_REVERSE 0xE0
#define TCM_GEAR_NEUTRAL 0xD0
#define TCM_GEAR_1 0x10
#define TCM_GEAR_2 0x20
#define TCM_GEAR_3 0x30
#define TCM_GEAR_4 0x40

class TCM{
  public:
    TCM();
    void Shift(const byte);
    void tick();
    void tcm_handle_doublePress();
    void tcm_handle_a1Press();
    void tcm_handle_a5Press();
	  bool enabled;
	private:
    void startSession();
    void testerPresent();
    void sendMessage(byte*, const byte, const byte);
		bool started;
    bool engauged;
		unsigned long lastAck;
    unsigned long lastShift;
    unsigned long lastButtonPress;
};

TCM::TCM(){
	enabled = false;
	started = false;
  engauged = false;
	lastAck = 0;
  lastShift = 0;
  lastButtonPress = 0;
}

void TCM::tcm_handle_doublePress(){

  now = millis();
  
  if(lastButtonPress + 500 <= now){
    if(this->enabled){
      this->enabled = false;
      #ifdef TCM_DEBUG
        Serial.write("[TCM] Disabled\r\n");
      #endif
    }else{
      this->enabled = true;
      #ifdef TCM_DEBUG
        Serial.write("[TCM] Enabled\r\n");
      #endif
    }
    lastButtonPress = now;
  }else if(lastButtonPress > now){
    lastButtonPress = 0;  //wrap around
  }
}


//A1 = SHIFT DOWN [RIGHT BUTTON]
void TCM::tcm_handle_a1Press(){
  now = millis();
  Shift(TCM_SHIFT_DOWN);
}


//A5 = SHIFT UP [LEFT BUTTON]
void TCM::tcm_handle_a5Press(){
  now = millis();
  Shift(TCM_SHIFT_UP);
}

void TCM::tick(){
  
  now = millis();

  if(this->enabled){
    
    if(!this->started){
      this->engauged = false;
      this->started = true;
      startSession();
      ptrRadio->SetTempDisplayText("MANUAL MODE", 11, RADIO_DISP_XM_CAT, 2048);
    }else if(!this->engauged){
      this->engauged = true;
      manualGear = true;
      Shift(TCM_SHIFT_ENGAUGE);
    }
    
    if(lastAck + 2500 <= now){
      testerPresent();
      lastAck = now;
    }else if(lastAck > now){
      lastAck = 0;    //Wrap around
    }
    
  }else{  //Enabled = false
    
    if(this->started){
      this->started = false;
      this->engauged = false;
      manualGear = false;
      Shift(TCM_SHIFT_NORMAL);  //Return control of transmission to ECU
      ptrRadio->SetTempDisplayText("AUTOMATIC", 9, RADIO_DISP_XM_CAT, 2048);
    }
    
  }
  
}

void TCM::startSession(){
  byte frame[3] = { 0x02, 0x10, 0x03 };
  sendMessage(frame, 3, 8);
}

void TCM::Shift(const byte dir){

  if(!this->enabled) { return; }

  now = millis();

  if(lastShift + TCM_SHIFT_DELAY > now){
    return;
  }else if(lastShift > now){
    lastShift = 0;  //wrap around
  }
  
  byte payload[8] = { 0x07, 0xAE, 0x30, 0, 0, 0, 0, 0 };
  byte shift = 0xFF;

  if(dir == TCM_SHIFT_NORMAL){
    //Return control of transmission to the ECU
    shift = 0;
  }else if(dir == TCM_SHIFT_ENGAUGE){

    //Retain current gear!
    if(currentGear == TCM_GEAR_1){
      shift = 0x90;
    }else if(currentGear == TCM_GEAR_2){
      shift = 0xA0;
    }else if(currentGear == TCM_GEAR_3){
      shift = 0xB0;
    }else if(currentGear == TCM_GEAR_4){
      shift = 0xC0;
    }
    
  }else{
  
    switch(currentGear){
      case TCM_GEAR_1:
        shift = dir == TCM_SHIFT_UP ? 0xA0 : 0x90;
      break;
      case TCM_GEAR_2:
        shift = dir == TCM_SHIFT_UP ? 0xB0 : 0x90;
      break;
      case TCM_GEAR_3:
        if(dir == TCM_SHIFT_DOWN && currentSpeed < 45){
          shift = 0xA0;
        }else if(dir == TCM_SHIFT_UP){
          shift = 0xC0;
        }
        //shift = dir == TCM_SHIFT_UP ? 0xC0 : 0xA0;
      break;
      case TCM_GEAR_4:
        shift = dir == TCM_SHIFT_UP ? 0xC0 : 0xB0;
      break;
      default:
        shift = 0xFF;
      break;
    }
    
  }

  if(shift != 0xFF){
    payload[3] = shift;
    this->sendMessage(payload, 8, 8);
    if(dir != TCM_SHIFT_ENGAUGE){
      lastShift = now;
    }
  }
}

void TCM::sendMessage(byte *data, const byte buflen, const byte msglen){
  if(buflen > 8 || msglen > 8) { return; }
  
  Message msg;
  msg.busId = 1;
  msg.ide = 0;
  msg.frame_id = TCM_REQUEST;
  for(byte i = 0; i < buflen; i++){
    msg.frame_data[i] = data[i];
  }
  msg.length = msglen;
  msg.dispatch = true;

  writeQueue.push(msg);
}


void TCM::testerPresent(){
  byte frame[2] = { 0x01, 0x3E };
  this->sendMessage(frame, 2, 8);
}

#endif



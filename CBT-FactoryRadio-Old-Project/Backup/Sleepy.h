/* Author Jacob Psimos 2016 */
/* This is a confediental file. No unauthorized access! */
/* All rights reserved */

#ifndef __SLEEPY__
#define __SLEEPY__

#include <avr/power.h>
#include <avr/sleep.h>

#define SLEEP_TIMEOUT 5555

class Sleepy{
public:
  Sleepy();
  void tick();
  void SleepBus(CANBus&);
  void WakeBus(CANBus&);
  void SleepAVR();
  void process(const Message&);

private:
  static void handleInterrupt();
  unsigned long lastTime;
  byte canctrlRecall[3];
};

Sleepy::Sleepy(){
  lastTime = 0;
  canctrlRecall[0] = 0;
  canctrlRecall[1] = 0;
  canctrlRecall[2] = 0;
}


void Sleepy::tick(){
  if(auth1 && auth2){
    now = millis();
    if(lastTime + SLEEP_TIMEOUT <= now){
      SleepAVR();
    }
  }
}

void Sleepy::process(const Message &msg){
   if(msg.frame_id == SENSOR_VIN_INT){
      now = millis();
      lastTime = now;
   }
}

void Sleepy::SleepAVR(){

  for( byte i=0; i<4; i++ ){
      digitalWrite(BOOT_LED, HIGH); delay(100);
      digitalWrite(BOOT_LED, LOW);  delay(100);
  }
  
  PORTE &= B00000000;

  ptrTCM->enabled = false;
  ptrRadio->Clear();
  ptrRadio->update = false;
  writeQueue.clear();
  readQueue.clear();
  
  SleepBus(busses[1]);

  set_filter(1, 0, SENSOR_VIN_INT, SENSOR_VIN_INT);
  
  delay(2);
    
  attachInterrupt(digitalPinToInterrupt(CAN1INT_D), reinterpret_cast<void (*)()>(&handleInterrupt), LOW);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();
  sleep_enable();
  sei();
  power_adc_disable();
  sleep_cpu();

  //Wakeup here

  delay(1);
  sleep_disable();
  power_adc_enable();
  delay(4);

  WakeBus(busses[1]);

  set_filter(1, 0, SENSOR_VIN_INT, SENSOR_OBD_RESP);

  PORTE |= B00000100;
  digitalWrite(BOOT_LED, HIGH);
  delay(1333);
  digitalWrite(BOOT_LED, LOW);

  now = millis();
  lastTime = now;

  ptrButtons->WakeUp();
  ptrRadio->update = true;
}



void Sleepy::handleInterrupt(){
  detachInterrupt(digitalPinToInterrupt(CAN1INT_D));
}

void Sleepy::WakeBus(CANBus &bus){
  if(bus.busId <= 3 && bus.busId > 0){
    bus.bitModify(CANINTF, WAKIF, WAKIF);
    bus.bitModify(CANINTE, WAKIE, WAKIE);
    
    delay(3);
    
    bus.bitModify(CANINTE, 0, WAKIE);
    bus.bitModify(CANINTF, 0, WAKIF);
    bus.bitModify(CANINTF, 0, 0xFF);
    bus.bitModify(BFPCTRL, 0, B1BFS);
    
    delay(1);
    
    bus.bitModify(CANCTRL, canctrlRecall[bus.busId - 1], 0xE0);
  }
}

void Sleepy::SleepBus(CANBus& bus){
  if(bus.busId <= 3 && bus.busId > 0){
    
    canctrlRecall[bus.busId - 1] = bus.readRegister(CANCTRL);
    
    bus.bitModify(BFPCTRL, B1BFE, B1BFE);
    bus.bitModify(BFPCTRL, 0, B1BFM);
    bus.bitModify(BFPCTRL, B1BFS, B1BFS);
    delay(1);
    bus.setMode(SLEEP);
    bus.bitModify(CANINTF, 0, 0x03);  //Clear RX bufferzzz
  }
}

#endif







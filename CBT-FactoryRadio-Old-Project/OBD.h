#ifndef __OBD__
#define __OBD__

/* Author Jacob Psimos 8/25/2016 */
/* All rights reserved */

#define OBD_REQ_INTERVAL_FAST 700
#define OBD_REQ_INTERVAL_MEDIUM 2000
#define OBD_REQ_INTERVAL_SLOW 10000

#define OBD_PID_MAF 0x10
#define OBD_PID_BAROMETER 0x33
#define OBD_PID_INTAKE_AIR_TEMP 0x0F

class OBD{
  public:
    OBD();
    void tick();
    void process(const Message &msg);
    void request(byte);
    boolean modeChanged;
  private:
    boolean rateLimit;
    byte requested_pid;
    byte mpgIndex;
    unsigned long lastRequest;
    unsigned int interval;
};

OBD::OBD(){
  rateLimit = false;
  lastRequest = 0;
  mpgIndex = 0;
  requested_pid = 0;
  interval = OBD_REQ_INTERVAL_MEDIUM;
  modeChanged = false;
}

void OBD::tick(){

  rateLimit = lastRequest + this->interval <= now;

  switch(ptrRadio->mode){
    case RADIO_DISP_MODE_MPG:
      if(engineRunning){
        request(OBD_PID_MAF);
      }else{
        MPG = 0.0;
      }
    break;
    case RADIO_DISP_MODE_INTAKE:
      request(OBD_PID_INTAKE_AIR_TEMP);
    break;
  }
  
}

void OBD::process(const Message &msg){
  if(msg.frame_id == SENSOR_OBD_RESP){
    switch(msg.frame_data[3]){
      case OBD_PID_MAF:
        MAF = (((unsigned int)msg.frame_data[4] << 8) + (unsigned int)msg.frame_data[5]) / 100;
        MPG = ((float)currentSpeed / 60.0) / (((float)MAF / 14.7 / 6.17 / 454.0) * 60.0);
        if(MPG > 99.9) { MPG = 99.9; }

        if(mpgIndex >= 48){ mpgIndex = 0; }
        averageMPG[mpgIndex++] = (byte)MPG;
        
        ptrRadio->update = true;
      break;
      case OBD_PID_INTAKE_AIR_TEMP:
        intakeAirTemp = ((((float)msg.frame_data[4]) - 40.0) * 1.8) + 32.0;
        ptrRadio->update = true;
      break;
    }
  }
}

void OBD::request(byte pid){
  if(!rateLimit && !modeChanged) { return; }

  Message msg;
  msg.frame_id = SENSOR_OBD_REQ;
  msg.busId = 1;
  msg.dispatch = true;
  msg.length = 8;
  msg.frame_data[0] = 0x03;
  msg.frame_data[1] = 0x22;   //Enhanced service
  msg.frame_data[3] = pid;   //MAF PID

  requested_pid = pid;
  lastRequest = now;
  modeChanged = false;

  if(!writeQueue.isFull()){
    writeQueue.push(msg);
  }
}


#endif

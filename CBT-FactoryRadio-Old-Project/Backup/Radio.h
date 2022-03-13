/* Author Jacob Psimos 2016 */
/* This is a confediental file. No unauthorized access! */
/* All rights reserved */

/*
 * Take over a radio station, anyone?
 */

#ifndef __RADIO__
#define __RADIO__

#define RADIO_QUEUE_DISPATCH_DELAY 64
#define RADIO_DISP_SET_DELAY 100

#define RADIO_DISP_XM_STATION 0x04
#define RADIO_DISP_XM_SONG 0x01
#define RADIO_DISP_XM_ARTIST 0x02
#define RADIO_DISP_XM_CAT 0x05

#define RADIO_DISP_MODE_GEARS 1
#define RADIO_DISP_MODE_SPEED 2
#define RADIO_DISP_MODE_THROTTLE 3
#define RADIO_DISP_MODE_FUEL_USED 4
#define RADIO_DISP_MODE_FUEL_REMAIN 5
#define RADIO_DISP_MODE_MPG 6
#define RADIO_DISP_MODE_HORSEPOWER 7
#define RADIO_DISP_MODE_INTAKE 8
#define RADIO_DISP_MODE_OIL_TEMP 9
#define RADIO_DISP_MODE_GPS_1 10       // Displays heading in degrees
#define RADIO_DISP_MODE_GPS_2 11       // Displays elevation in feet
#define RADIO_DISP_MODE_BAROMETER 12
#define RADIO_DISP_MODE_DISCO 13
#define RADIO_DISP_MAX 13              // Keep updated to maximum numeric RADIO_DISP_MODE value

#define GEAR_PARK_STR "PARK"
#define GEAR_NEUTRAL_STR "NEUTRAL"
#define GEAR_REVERSE_STR "REVERSE"
#define GEAR_1_STR "FIRST"
#define GEAR_2_STR "SECOND"
#define GEAR_3_STR "THIRD"
#define GEAR_4_STR "FOURTH"

class Radio{
  public:
    Radio();
    void tick();
    void SwitchMode(boolean);
    void Update();
    void HandleSatUpdate();
    void Clear();
    void SetDisplayText(const char*, const byte, const byte);
    void SetTempDisplayText(const char*, const byte, const byte, const unsigned int);
    byte mode;
    boolean update;
    boolean enabled;
  private:
    QueueArray<Message> rqueue;
    unsigned long lastRadioTx;
    unsigned long lastTempTx;
    unsigned int tempDuration;
    byte satUpdateIterations;
    char *gearStr;
    boolean showingTemp;
    boolean satUpdate;
    boolean rateLimit;
    boolean dispatchRateLimit;
    void getGearString();
};

Radio::Radio(){
  rateLimit = false;
  dispatchRateLimit = false;
  satUpdate = false;
  lastRadioTx = 0;
  lastTempTx = 0;
  tempDuration = 0;
  satUpdateIterations = 0;
  showingTemp = false;
  enabled = true;
  update = true;
  gearStr = GEAR_PARK_STR;
  mode = RADIO_DISP_MODE_GEARS;
}

void Radio::HandleSatUpdate(){
  rqueue.clear();
  satUpdate = true;
  satUpdateIterations = 18;
}

void Radio::tick(){
  if(!enabled) { return; }

  now = millis();
  
  rateLimit = lastRadioTx + RADIO_DISP_SET_DELAY <= now;
  dispatchRateLimit = lastRadioTx + RADIO_QUEUE_DISPATCH_DELAY <= now;

  if(satUpdate){
     if(rateLimit && satUpdateIterations > 0){
      byte radioBuffer[12];
      memset((char*)radioBuffer, ' ', 11);
      radioBuffer[11] = 0x0A;
      SetDisplayText((char*)radioBuffer, 11, RADIO_DISP_XM_CAT);
      satUpdateIterations--;
     }

     if(satUpdateIterations == 0){
      satUpdate = false;
      showingTemp = false;
     }
     
  }else if(showingTemp){
    if(lastTempTx + tempDuration <= now){
      showingTemp = false;
      update = true;
    }
  }
  
  if(!rqueue.isEmpty() && dispatchRateLimit){
    Message msg = rqueue.pop();
    writeQueue.push(msg);
    lastRadioTx = now;
  }else if(rqueue.isEmpty() && dispatchRateLimit){
    Update();
  }
}

void Radio::SwitchMode(boolean up){
  if(!enabled) { return; }
  
  if(up){
    this->mode = (this->mode == RADIO_DISP_MAX) ? RADIO_DISP_MODE_GEARS : (this->mode + 1);
  }else{
    this->mode = (this->mode == RADIO_DISP_MODE_GEARS) ? RADIO_DISP_MAX : (this->mode - 1);
  }

  if(this->mode > RADIO_DISP_MAX){ this->mode = RADIO_DISP_MAX; }
  
  showingTemp = false;
  this->update = true;
}

void Radio::getGearString(){
  switch(currentGear){
    case 0xF0:
      gearStr = GEAR_PARK_STR;
    break;
    case 0xE0:
      gearStr = GEAR_REVERSE_STR;
    break;
    case 0xD0:
      gearStr = GEAR_NEUTRAL_STR;
    break;
    case 0x10:
      gearStr = GEAR_1_STR;
    break;
    case 0x20:
      gearStr = GEAR_2_STR;
    break;
    case 0x30:
      gearStr = GEAR_3_STR;
    break;
    case 0x40:
      gearStr = GEAR_4_STR;
    break;
    default:
      gearStr = GEAR_PARK_STR;
    break;
  }
}

void Radio::Update(){
  if(!update || !rateLimit || showingTemp || ptrButtons->waiting || !enabled) { return; }
  
  byte radioBuffer[24];
  memset((char*)radioBuffer, 0, 24);
  
  byte consumed = 0;
  switch(this->mode){
    case RADIO_DISP_MODE_SPEED:
      consumed = snprintf((char*)radioBuffer, 19, "%u mph", currentSpeed);
    break;
    case RADIO_DISP_MODE_GEARS:
      getGearString();
      consumed = snprintf((char*)radioBuffer, 19, 
        (manualGear ? "Gear: [M] %s" : "Gear: %s"), gearStr);
    break;
    case RADIO_DISP_MODE_THROTTLE:
      //dtostrf(float, StringLengthIncDecimalPoint, numVarsAfterDecimal, charbuf);
      if(brakes){
        memcpy((char*)radioBuffer, F("Throttle: -1"), 12);
        consumed = 12; 
      }else{
        memcpy((char*)radioBuffer, F("Throttle: "), 10);
        if(throttle >= 0xFE){
          radioBuffer[10] = '1';
          radioBuffer[11] = '0';
          radioBuffer[12] = '0';
          radioBuffer[13] = ' ';
          radioBuffer[14] = '%';
          consumed = 15;
        }else{
          dtostrf(throttleF, 4, 1, (char*)(radioBuffer + 10));
          radioBuffer[14] = ' ';
          radioBuffer[15] = '%';
          consumed = 16;
        }
      }
    break;
    case RADIO_DISP_MODE_FUEL_REMAIN:
      memcpy((char*)radioBuffer, F("Fuel = "), 7);
      dtostrf(fuelF, 4, 1, (char*)(radioBuffer + 7));
      memcpy((char*)(radioBuffer + 11), F("  gal"), 5);
      consumed = 16;
    break;
    case RADIO_DISP_MODE_FUEL_USED:
      memcpy((char*)radioBuffer, F("Fuel ^ "), 7);
      dtostrf(26.0 - fuelF, 4, 1, (char*)(radioBuffer + 7));
      memcpy((char*)(radioBuffer + 11), F("  gal"), 5);
      consumed = 16;
    break;
    case RADIO_DISP_MODE_MPG:
      memcpy((char*)radioBuffer, F("MPG: "), 5);
      dtostrf(MPG, 4, 1, ((char*)radioBuffer) + 5);
      consumed = 9;
    break;
    case RADIO_DISP_MODE_HORSEPOWER:
      consumed = snprintf((char*)radioBuffer, 19, "HP: %u | %u", horsepower, demandedHorsepower);
    break;
    case RADIO_DISP_MODE_INTAKE:
      memcpy((char*)radioBuffer, F("Intake: "), 8);
      dtostrf(intakeAirTemp, 5, 1, ((char*)radioBuffer) + 8);
      radioBuffer[13] = ' ';
      radioBuffer[14] = 'F';
      consumed = 15;
    break;
    case RADIO_DISP_MODE_OIL_TEMP:
      memcpy((char*)radioBuffer, F("Oil Temp: "), 10);
      dtostrf(oilTemp, 5, 1, ((char*)radioBuffer) + 10);
      radioBuffer[15] = ' ';
      radioBuffer[16] = 'F';
      consumed = 17;
    break;
    case RADIO_DISP_MODE_GPS_1:
      consumed = snprintf((char*)radioBuffer, 19, "<-| %3u deg |->", heading);
    break;
    case RADIO_DISP_MODE_GPS_2:
      consumed = snprintf((char*)radioBuffer, 19, "Elev: %u ft", elevation);
    break;
    case RADIO_DISP_MODE_BAROMETER:
      consumed = snprintf((char*)radioBuffer, 19, "baro: %u kPa", barometricPressure);
    break;
    case RADIO_DISP_MODE_DISCO:
      strncpy((char*)radioBuffer, "Disco", 19);
      consumed = 5;
    break;
    default:
      radioBuffer[0] = ' ';
      consumed = 1;
    break;
  }
  SetDisplayText((char*)radioBuffer, consumed, RADIO_DISP_XM_CAT);
  update = false;
}

void Radio::Clear(){
  rqueue.clear();
}

void Radio::SetDisplayText(const char *text, const byte length, const byte field){
  if(length > 17 || length < 5 || !enabled) { return; }

  //thau shall check array bounds
  char *overflow = strchr(text, '\0');
  if(overflow != NULL){
    if(((unsigned long)overflow - (unsigned long)text) > length){
      return;
    }
  }else { return; }
  
  Message message;
  byte copyIndex = 0;
  byte numMessages = ((length - 5) / 6) + 2;
  byte sequence = 0x63;
  byte isequence = numMessages > 2 ? 0x03 : 0x02;

  message.busId = 2;
  message.ide = 0x08;
  message.frame_id = SENSOR_RADIO_TEXT_REQ;
  message.length = 8;
  message.dispatch = true;

  message.frame_data[0] = sequence;
  message.frame_data[1] = isequence;
  message.frame_data[2] = field;
  memcpy(message.frame_data + 3, text, 5);
  copyIndex = 5;
  rqueue.push(message);

  sequence = ~(~sequence ^ 1);

  for(byte messageIndex = 1; messageIndex < numMessages; messageIndex++){
    memset(message.frame_data, 0, 8);
    
    isequence = numMessages > 2 ? ~(~isequence ^ 1) : isequence;
    message.frame_data[0] = sequence;
    message.frame_data[1] = isequence;
    
    if((length - copyIndex) >= 6){
      memcpy(message.frame_data + 2, text + copyIndex, 6);
      copyIndex += 6;
    }else{
      memcpy(message.frame_data + 2, text + copyIndex, length - copyIndex);
      message.frame_data[length - copyIndex + 2] = 0x0A;
    }
    rqueue.push(message);
  }
}

void Radio::SetTempDisplayText(const char *text, const byte len, const byte field, const unsigned int duration){
  now = millis();

  if(len > 17 || len == 0 || (lastTempTx + RADIO_DISP_SET_DELAY > now) || showingTemp || satUpdate || !enabled){ return; }
  
  
  showingTemp = true;
  lastTempTx = now;
  tempDuration = duration;
  update = false;
  SetDisplayText(text, len, field);
}

#endif








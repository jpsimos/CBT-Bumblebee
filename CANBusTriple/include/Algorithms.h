/*
	CANBus Triple Jenny
	Author Jacob Psimos
	
	Algorithms.h

  Last updated: Thu 5/29/17
*/

#ifndef __ALGOS__
#define __ALGOS__

#include <Math.h>

#include "Sensors.h"

#define STATUS_ENABLE_WRITE 0x01						//Enable writing messages to the bus
#define STATUS_ENGINE_RUNNING	0x02
#define STATUS_AUTH 0x04
#define STATUS_ENABLE_OBD 0x20
#define STATUS_CRUISE_ON 0x40
#define STATUS_BRAKES 0x80
#define STATUS_ENABLE_SLEEP 0x200
#define STATUS_WHEEL_BUTTON 0x400						//Steering wheel button request is awaiting termination
#define STATUS_ENABLE_DISPLAY 0x800
#define STATUS_ALERT_SPEED 0x1000
#define STATUS_BT_CONNECTED 0x2000

#define TRIGGER_SPEED 0x01								//When the vehicle speed changes
#define TRIGGER_PEDAL 0x02								//When the gas pedal position changes
#define TRIGGER_HORSEPOWER 0x04							//When the horsepower value changes
#define TRIGGER_FUEL 0x08								//When the fuel level changes
#define TRIGGER_SHIFT 0x10								//When the vehicle shifts gears
#define TRIGGER_HVAC 0x20								//When the HVAC fan speed changes
#define TRIGGER_TPMS 0x40								//Start a timer than sends TPMS wait command after some time
#define TRIGGER_GPS 0x80								//When GPS location changes
#define TRIGGER_GEOGRAPHIC 0x100						//When GPS elevation and heading changes
#define TRIGGER_MANIFOLD 0x200							//When the manifold pressure changes
#define TRIGGER_INTAKE 0x400							//When the intake air temperature changes
#define TRIGGER_MPG 0x800								//When the MPG value changes
#define TRIGGER_DIMMER 0x1000							//When the dimmer value changes
#define TRIGGER_DATE 0x2000						//When GPS time has been updated
#define TRIGGER_TIMEZONE 0x4000						//When the timezone changes
#define TRIGGER_THROTTLE 0x8000						//When the throttle changes
#define TRIGGER_CRUISE 0x10000							//When cruise control is enabled or disabled
#define TRIGGER_DISP_UPDATE 0x20000				//Tell the display to update
#define TRIGGER_DISP_UPDATE_DELAYED 0x40000		//Update the display after some time
#define TRIGGER_XM_CHANNEL 0x80000				//When stuff on the radio changes
#define TRIGGER_ALERT_SPEED 0x100000			//Slow down!
#define TRIGGER_TIME    0x200000				//When the GPS time changes (synced with satellites in space....)

/* Methods */
void processVIN(Message &msg, const byte *method);
void processLighting(Message &msg);
void processSpeed(Message &msg);
void processFuel(Message &msg);
void processTransmission(Message &msg);
void processHVAC(Message &msg);
void processBrakesAndCruise(Message &msg);
void processRadio(Message &msg);
void processGPS(Message &msg);
void processGeographic(Message &msg);
void processDimmer(Message &msg);
void processTime(Message &msg);
void processTextReq(Message &msg);
void processKey(Message &msg);

const byte vin_1[8] = { 0x47, 0x43, 0x45, 0x4B, 0x31, 0x34, 0x4A, 0x58 };
const byte vin_2[8] = { 0x37, 0x5A, 0x35, 0x38, 0x37, 0x31, 0x33, 0x30 };

unsigned int status = STATUS_ENABLE_WRITE;
unsigned long triggers = 0;
byte speedMPH = 0;
float wheelRPM = 0.0;
unsigned int engineRPM = 0;
byte throttle = 0;
float throttleF = 0.0;
float horsePower = 0.0;
byte pedal = 0;
float pedalF = 0.0;
byte fuel = 0;
float fuelRemaining = 0.0;
unsigned int MAF = 0;
float totalMAF = 0.0; //lbs
float MPG = 0.0;
float rangeToEmpty = 0.0;
byte manifoldPressure = 0;
float intakeAirTemperature = 0.0;
byte currentGear = 0;
byte hvacStatus = 0;
byte doorStatus = 0;
byte dimmer = 0;
//byte lighting = 0;
unsigned int heading = 0;
unsigned int elevation = 0;
float latitude = 0.0;
float longitude = 0.0;
//float distanceFromPoint = 0.0;
//float markedLatitude = 0.0;
//float markedLongitude = 0.0;
byte ignition = 0;
byte xmChannel = 0;
byte year = 0;
byte month = 0;
byte day = 0;
byte hour = 0;
byte minute = 0;
byte second = 0;
byte timeZone = 0;
byte ampm = 0;

byte lighting_status[6];

void processFrame(Message &msg){

  switch(msg.frame_id){
    
    case SENSOR_VIN_1:
      processVIN(msg, vin_1);
    break;
    
    case SENSOR_VIN_2:
      processVIN(msg, vin_2);
    break;

    case SENSOR_LIGHTING:
      processLighting(msg);
    break;

    case SENSOR_SPEED:
      processSpeed(msg);
    break;

    case SENSOR_FUEL:
      processFuel(msg);
    break;

    case SENSOR_TRANSMISSION:
      processTransmission(msg);
    break;

    case SENSOR_BRAKES_AND_CRUISE:
      processBrakesAndCruise(msg);
    break;

    case SENSOR_HVAC_FAN:
      processHVAC(msg);
    break;

    case SENSOR_RADIO_REQ:
	  processRadio(msg);
    break;

    case SENSOR_DIMMER:
      processDimmer(msg);
    break;

    case SENSOR_GPS:
      processGPS(msg);
    break;

    case SENSOR_GEOGRAPHIC:
      processGeographic(msg);
    break;

    case SENSOR_DATE_TIME:
      processTime(msg);
    break;

    case SENSOR_DRIVER_DOOR:
      doorStatus = msg.frame_data[0] != 0 ? (doorStatus | 0x80) : (doorStatus & ~0x08);
    break;

    case SENSOR_PASSENGER_DOOR:
      doorStatus = msg.frame_data[0] != 0 ? (doorStatus | 0x01) : ~(doorStatus & ~0x01);
    break;

	case SENSOR_TIRE_PRESSURE:
		triggers |= TRIGGER_TPMS;
	break;

	case SENSOR_KEY:
		processKey(msg);
	break;
  }
}

void processKey(Message &msg){
	if(!msg.frame_data[0]){ ignition = 0; return; }
	ignition = msg.frame_data[1];
}

void processTime(Message &msg){

	if(msg.frame_data[2] != day){
		triggers |= TRIGGER_DATE;
	}

	year = msg.frame_data[0];
	month = msg.frame_data[1] > 12 ? 12 : msg.frame_data[1];
	day = msg.frame_data[2];
	hour = (msg.frame_data[3] / 2);
	minute = (msg.frame_data[4] / 2);
	second = (msg.frame_data[5] / 2);
	ampm = 0;

	//Convert GMT time to local time

	byte tzOffset = trunc(longitude / -15.0);	//too bad this doesn't handle community agreed on time zone borders :(

	if(hour <= tzOffset){
		hour = 12 - (tzOffset - hour);
		day -= 1;
	}else{
		hour -= tzOffset;
	}

	if(month >= 3 && month < 11){
		hour += 1;
	}

	if(hour > 12){
		ampm = 1;
		hour -= 12;
	}

	hour += hour == 0 ? 1 : 0;
	day += day == 0 ? 1 : 0;

	if(timeZone != tzOffset){
		triggers |= TRIGGER_TIMEZONE;
		timeZone = tzOffset;
	}

	triggers |= TRIGGER_TIME;
}

void processRadio(Message &msg){
	if(xmChannel != msg.frame_data[3]){
		xmChannel = msg.frame_data[3];
		triggers |= TRIGGER_XM_CHANNEL;
	}
}

void processDimmer(Message &msg){
  byte dimmerPercent = (byte)((unsigned int)((unsigned int)msg.frame_data[0] * 100) / 255);
  //if(dimmer < dimmerPercent + 2 || dimmer > dimmerPercent - 2){
  dimmer = (byte)dimmerPercent;
  /*
  if(dimmer != dimmerPercent){
    dimmer = (byte)dimmerPercent;
    triggers |= TRIGGER_DIMMER;
  }
  */
}

void processGeographic(Message &msg){
  unsigned int tempHeading = ((unsigned int)msg.frame_data[0] << 8) + (unsigned int)msg.frame_data[1];
  unsigned long tempElevation = ((unsigned long)msg.frame_data[5] << 16) + ((unsigned long)msg.frame_data[6] << 8) + ((unsigned long)msg.frame_data[7]);
  tempElevation = (tempElevation - (unsigned long)100000) / (unsigned long)30;

  if(tempHeading != heading || (unsigned int)tempElevation != elevation){
    heading = tempHeading;
    elevation = (unsigned int)tempElevation;
    triggers |= TRIGGER_GEOGRAPHIC;
  }
}

void processGPS(Message &msg){
  float ilat = ((long)msg.frame_data[0] << 24) + ((long)msg.frame_data[1] << 16) + ((long)msg.frame_data[2] << 8) + (long)msg.frame_data[3];
  float ilog = ((long)msg.frame_data[4] << 24) + ((long)msg.frame_data[5] << 16) + ((long)msg.frame_data[6] << 8) + (long)msg.frame_data[7];
  latitude = ilat / 3600000.0;
  longitude = (ilog / 3600000.0) - 596.52325;

  //distanceFromPoint = distanceMeters(latitude, longitude, markedLatitude, markedLongitude);
  //distanceFromPoint = (distanceFromPoint / 1000.0) * 0.621371;
  
  triggers |= TRIGGER_GPS;
}

void processHVAC(Message &msg){
  if(hvacStatus != msg.frame_data[3]){
    hvacStatus = msg.frame_data[3]; // hvacStatus & 0x20 means recirculation mode
    triggers |= TRIGGER_HVAC;
  }
}

void processBrakesAndCruise(Message &msg){
  if( (msg.frame_data[3] & 0x80) ){
    status |= STATUS_BRAKES;
    triggers |= TRIGGER_PEDAL;
  }

  if( (msg.frame_data[0] & 0x40) && (msg.frame_data[1] & 0x04) ){	//TODO: check these, seems cruise is still on after its not but the trigger still fires
	if(!(status & STATUS_CRUISE_ON)){
		triggers |= TRIGGER_CRUISE;
	}
    status |= STATUS_CRUISE_ON;
  }else{
	if(status & STATUS_CRUISE_ON){
		triggers |= TRIGGER_CRUISE;
	}
	status &= ~STATUS_CRUISE_ON;
  }
}

void processTransmission(Message &msg){
  if(currentGear != msg.frame_data[0]){
	triggers |= (currentGear == 0xF0) ? (TRIGGER_SHIFT | TRIGGER_TPMS) : TRIGGER_SHIFT;	//Turn off the TPMS light if shifting OUT of park.
    currentGear = msg.frame_data[0];
  }
}

void processFuel(Message &msg){
  if(msg.frame_data[1] != fuel){
    fuel = msg.frame_data[1];
    fuelRemaining = ((float)msg.frame_data[1] / 255.0) * 26.0;
    rangeToEmpty = 15.0 * fuelRemaining;
    triggers |= TRIGGER_FUEL;
  }
}

void processSpeed(Message &msg){
  if(msg.frame_data[5] > 0 || msg.frame_data[4] > 0){
    status |= STATUS_ENGINE_RUNNING | STATUS_ENABLE_OBD;
  }else{
    status &= ~(STATUS_ENGINE_RUNNING | STATUS_ENABLE_OBD);
  }

  unsigned int temp = 0;

  engineRPM = ((((unsigned int)msg.frame_data[4]) << 8) + ((unsigned int)msg.frame_data[5])) / 4;

  temp = ((((unsigned int)msg.frame_data[2]) << 8) + ((unsigned int)msg.frame_data[3])) / 100;
  if((byte)temp != speedMPH){
    speedMPH = (byte)temp;
    triggers |= TRIGGER_SPEED;
  }

  wheelRPM = (((float)speedMPH / 60.0F) * 63360.0F) / (32.0F * (355.0F / 113.0F));

  if(speedMPH >= 73){
	if(!(status & STATUS_ALERT_SPEED)){
		triggers |= TRIGGER_ALERT_SPEED;
		status |= STATUS_ALERT_SPEED;
	}
  }else if(speedMPH < 68){
	status &= ~STATUS_ALERT_SPEED;
  }

  temp = (status & STATUS_ENGINE_RUNNING) ? msg.frame_data[6] : 0;
  if(throttle != (byte)temp){
	throttle = temp;
	throttleF = ((float)throttle / 255.0) * 100.0;
	//horsePower = ((float)throttle / 255.0) * 350.0;
	triggers |= TRIGGER_THROTTLE | TRIGGER_HORSEPOWER;
  }

  if(pedal != msg.frame_data[7]){
    pedal = msg.frame_data[7];
    pedalF = ((float)msg.frame_data[7] / 255.0) * 100.0;
    triggers |= TRIGGER_PEDAL;
  }

  //A more accurate way to measure HP for a 5.3L V8 engine with 338 maximum torque
  horsePower = (338.0F * (float)engineRPM) / 5252.0F;
}

void processLighting(Message &msg){
  memcpy(lighting_status, msg.frame_data, 6);
  //highBeamStatusON = (msg.frame_data[0] & 0x08);
}

void processVIN(Message &msg, const byte *method){
  if( (status & STATUS_AUTH) ){ return; }

  static byte lockStatus = 0;
  if(!memcmp(method, msg.frame_data, 8)){
    lockStatus |= method[0];
  }else{
    lockStatus &= ~method[0];
  }

  if(lockStatus == (vin_1[0] | vin_2[0])){
	lockStatus = 0;
    status |= STATUS_AUTH | STATUS_ENABLE_SLEEP  | STATUS_ENABLE_DISPLAY | STATUS_ENABLE_OBD ;
	blinkLED(3, 100);
  }
}

#endif
















































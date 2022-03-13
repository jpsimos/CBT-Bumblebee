#ifndef __HANDLERS__
#define __HANDLERS__
/* Author Jacob Psimos 2017 */

#include <Math.h>

//unsigned long radioButtonTerminateAt = 0;

void globalTick(void){
	unsigned long now = millis();

	if(status & STATUS_WHEEL_BUTTON){
		if(radioButtonTerminateAt < now){
			steeringWheelButton(0, 0);
		}
	}

}

void blinkLED(byte times, const unsigned int &duration){
	while(times-- > 0){
		digitalWrite(BOOT_LED, HIGH);
		delay(duration);
		digitalWrite(BOOT_LED, LOW);
		delay(duration);
	}
}

void chimeRequest(const byte properties, const byte numChimes, const byte chimeDelay){
  Message tempMsg;
  tempMsg.busId = 2;
  tempMsg.ide = 0x08;
  tempMsg.frame_id = SENSOR_CHIMER_IPC;
  tempMsg.length = 5; //Or 6 if the last byte of the ID is 0x40 apparently
  tempMsg.frame_data[0] = properties;
  tempMsg.frame_data[1] = chimeDelay;
  tempMsg.frame_data[2] = numChimes;
  tempMsg.frame_data[3] = 0x40;
  tempMsg.frame_data[4] = 0xFF;

  writeQueue.push(tempMsg);
}

void onSleep(){
  writeQueue.clear();
  status &= ~STATUS_ENABLE_OBD;
  triggers = 0; //clear triggers
}

void getDisplayMode(byte &upper, byte &lower){
	upper = display.upperDisplayMode;
	lower = display.lowerDisplayMode;
}

void buttonHandler(const byte btnStatus){
  if( (btnStatus & DBL_HANDLE) ){
    triggers |= TRIGGER_TPMS;
  }else if( (btnStatus & A1_HANDLE) && !(btnStatus & A5_HANDLE) ){ /* RED BUTTON */
	display.upperDisplayMode = display.upperDisplayMode + 1 > DISPLAY_NUM_MODES ? 1 : display.upperDisplayMode + 1;
	triggers |= TRIGGER_DISP_UPDATE;
	if(btnStatus & BTN_HELD){
		steeringWheelButton(BUTTON_VOL_DOWN, 3000);
	}
  }else if( (btnStatus & A5_HANDLE) && !(btnStatus & A1_HANDLE) ){	/* BLK BUTTON */
	display.lowerDisplayMode = display.lowerDisplayMode + 1 > DISPLAY_NUM_MODES ? 1 : display.lowerDisplayMode + 1;
	triggers |= TRIGGER_DISP_UPDATE;
	if(btnStatus & BTN_HELD){
		steeringWheelButton(BUTTON_VOL_UP, 3000);
	}
  }
}


void steeringWheelButton(const byte &button, const unsigned int &holdDuration){
	Message msg;
	msg.frame_id = SENSOR_WHEEL_BUTTONS;
	msg.busId = 2;
	msg.ide = 0x08;
	msg.length = 4;
	msg.frame_data[0] = button;

	if(writeQueue.push(msg)){
		if(button != 0){
			status |= STATUS_WHEEL_BUTTON;
			radioButtonTerminateAt = millis() + holdDuration;
		}else{
			status &= ~STATUS_WHEEL_BUTTON;
		}
	}
}

float distanceMeters(const float &lat1, const float &log1, const float &lat2, const float &log2){
  //My implementation of the haversine formula - jp
  const float radians = (355.0F / 113.0F) / 180.0F;
  const float radius = 6371e3;
  const float o1 = lat1 * radians;
  const float o2 = lat2 * radians;
  const float o = (lat2 - lat1) * radians;
  const float delta = (log2 - log1) * radians;

  float a = sin(o / 2.0F) * sin(o / 2.0F) + cos(o1) * cos(o2) * sin(delta / 2.0F) * sin(delta / 2.0F);
  float c = 2.0F * atan2(sqrt(a), sqrt(1.0F - a));
  float distance = radius * c;

  return distance;
}

void setPressures(byte leftFront, byte rightFront, byte leftRear, byte rightRear){
	Message msg;
	msg.frame_id = SENSOR_TIRE_PRESSURE;
	msg.busId = 2;
	msg.ide = 0x08;
	msg.length = 8;
	msg.frame_data[0] = 0x24;
	msg.frame_data[1] = 0x24;
	msg.frame_data[2] = (byte)round((float)leftFront / 0.572727F);
	msg.frame_data[3] = (byte)round((float)leftRear / 0.572727F);
	msg.frame_data[4] = (byte)round((float)rightFront / 0.572727F);
	msg.frame_data[5] = (byte)round((float)rightRear / 0.572727F);
	msg.frame_data[7] = 0xFF;
	writeQueue.push(msg);
}


#endif

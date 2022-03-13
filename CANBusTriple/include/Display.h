/*
 * Display.h
 *
 * Created: 6/20/2017 1:34:19 AM
 *  Author: jacob psimos
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_

#define DISP_DEQUEUE_RATE 48
#define DISP_SETTEXT_RATE 200

#define DISP_DEQUEUE_READY 0x02			//Are we ready to dequeue the next message yet?
#define DISP_SETTEXT_READY 0x04		//Are we ready to update the display yet?
#define DISP_SHOWING_TEMP 0x08			//Is there a temporary update currently being displayed?
#define DISP_TEMP_NOCLEAR 0x10			//Do not allow temporary displays to be overwritten before they expire


class Display{
	public:
		Display(void);
		void tick(void);
		void updateStatusBits(void);
		bool updateText(const byte *text, const byte &length);
		bool updateText(const byte *text, const byte &length, const unsigned int &duration);
		void setClusterParameters(const bool &enableMsg);

		byte upperDisplayMode;
		byte lowerDisplayMode;
	private:
		Message _queue[6];
		MessageQueue queue = MessageQueue(_queue, 6);
		byte displayStatus;
		//unsigned long lastDequeue;
		//unsigned long lastTempUpdate;
		unsigned int tempUpdateLength;
		//unsigned long updateDisplayAt;
		char displayBuffer[37];
		byte length;
		
		void writeToBuffer(char *buffer, const byte &line, unsigned long &triggerID);
		const bool handleTemporaryTriggers(void);
		const char *getGearString(void);
		void autoUpdateDisplay(void);
};

Display::Display(void){
	upperDisplayMode = 1;
	lowerDisplayMode = 4;
	displayStatus = 0;
	length = 0;
	lastDequeue = 0;
	lastTempUpdate = 0;
	tempUpdateLength = 0;
}


void Display::updateStatusBits(void){
	unsigned long now = millis();

	if(lastDequeue + DISP_DEQUEUE_RATE < now){
		displayStatus |= DISP_DEQUEUE_READY;
	}else{
		displayStatus &= ~DISP_DEQUEUE_READY;
	}

	if(lastDequeue + DISP_SETTEXT_RATE < now){
		displayStatus |= DISP_SETTEXT_READY;
	}else{
		displayStatus &= ~DISP_SETTEXT_READY;
	}

	//Prevents boarding readouts from expiring
	if(lastDequeue + 4096 < now){
		triggers |= TRIGGER_DISP_UPDATE;
	}

	if(displayStatus & DISP_SHOWING_TEMP){
		if(lastTempUpdate + tempUpdateLength < now){
			displayStatus &= ~(DISP_SHOWING_TEMP | DISP_TEMP_NOCLEAR);
			triggers |= TRIGGER_DISP_UPDATE;
		}
	}

	if(triggers & TRIGGER_DISP_UPDATE_DELAYED){
		//static unsigned long updateDisplayAt = 0;
		if(!updateDisplayAt){
			updateDisplayAt = now + 3000;
		}else if(updateDisplayAt <= now){
			triggers &= ~TRIGGER_DISP_UPDATE_DELAYED;
			triggers |= TRIGGER_DISP_UPDATE;
			updateDisplayAt = 0;
		}
	}
}

void Display::tick(void){
	if(!(status & STATUS_ENABLE_DISPLAY)){ 
		return; 
	}

	updateStatusBits();

	/*
		A trigger display update occurs when the following conditions are met:
			1.	Display queue empty (previous display request completed)
			2.	DISP_SETTEXT_READY bit is set
				a. lastDequeue + DISP_SETTEXT_RATE is less than the current time
				* reminder * the lastDequeue is the previous display queue message pop
	*/

	if(!queue.isEmpty() && (displayStatus & DISP_DEQUEUE_READY)){
		Message nextMessage = queue.peek();
		if(writeQueue.push(nextMessage)){
			queue.pop();
			lastDequeue = millis();
		}
	}else if(queue.isEmpty() && (displayStatus & DISP_SETTEXT_READY)){
		autoUpdateDisplay();
	}
}

/* Update the display text */
bool Display::updateText(const byte *text, const byte &length){
	if(text == (const byte*)NULL || length >= 36){ return false; }

	byte numMessages = length <= 5 ? 1 : (length / 6) + 1;

	if(numMessages > 6 || !queue.accomodate(numMessages)){
		return false;
	}

	setClusterParameters(true);
	lastDequeue = millis() + 33;

	byte copyIndex = 0;
	byte copyNum = 0;

	Message msg;
	msg.busId = 2;
	msg.ide = 0x08;
	msg.frame_id = SENSOR_DRIVER_TEXT_REQ;
	msg.length = 8;

	msg.frame_data[0] = 0x27;
	msg.frame_data[1] = numMessages;

	//Algorithm to encode TEXT to the dash display.

	for(byte i = 1; i <= numMessages; i++){
		memset(msg.frame_data + 2, '\0', 6);
	
		if(copyIndex + 6 > length){
			copyNum = length - copyIndex;
		}else{
			copyNum = 6;
		}
	
		memcpy(msg.frame_data + 2, text + copyIndex, (copyNum > 6 ? 6 : copyNum));
		copyIndex += copyNum;
	
		if(numMessages > 1){
			for(byte t = 0; t < 8; t++){
				if(msg.frame_data[t] == 0x00){
					msg.frame_data[t] = 0x04;
				}
			}
		}

		queue.push(msg);
	
		msg.frame_data[0] = 0x26;
		msg.frame_data[1] = numMessages - (numMessages - i) + 1;
	}
	return true;
}

/* Update the display that will expire after duration milliseconds */
/* AKA temporary display that expires */
/* EG dimmer status, A/C fan speed, etc */
bool Display::updateText(const byte *text, const byte &length, const unsigned int &duration){
	if(!(displayStatus & DISP_TEMP_NOCLEAR) && updateText(text, length)){
		lastTempUpdate = millis();
		tempUpdateLength = duration;
		displayStatus |= DISP_SHOWING_TEMP;
		return true;
	}
	return false;
}

void Display::setClusterParameters(const bool &enableMsg){
	Message msg;
	msg.frame_id = SENSOR_DRIVER_TEXT_PARAM;
	msg.length = 6;
	msg.busId = 2;
	msg.ide = 0x08;
	msg.frame_data[1] = 0x02;
	msg.frame_data[2] = enableMsg ? (byte)0x62 : (byte)0x60;
	if(enableMsg){
		msg.frame_data[3] = 0x02;
	}
	msg.frame_data[5] = 0x15;

	writeQueue.push(msg);
}

void Display::writeToBuffer(char *buffer, const byte &line, unsigned long &triggerID){
	if(buffer == (char*)NULL){ return; }
		
#pragma region Dates

	PROGMEM const char *jan = PSTR("Jan %u, 20%u");
	PROGMEM const char *feb = PSTR("Feb %u, 20%u");
	PROGMEM const char *mar = PSTR("Mar %u, 20%u");
	PROGMEM const char *apr = PSTR("Apr %u, 20%u");
	PROGMEM const char *may = PSTR("May %u, 20%u");
	PROGMEM const char *jun = PSTR("Jun %u, 20%u");
	PROGMEM const char *jul = PSTR("Jul %u, 20%u");
	PROGMEM const char *aug = PSTR("Aug %u, 20%u");
	PROGMEM const char *sept = PSTR("Sept %u, 20%u");
	PROGMEM const char *oct = PSTR("Oct %u, 20%u");
	PROGMEM const char *nov = PSTR("Nov %u, 20%u");
	PROGMEM const char *dec = PSTR("Dec %u, 20%u");

	PROGMEM const char *months[] = {
		jan,feb,mar,apr,may,jun,jul,aug,sept,oct,nov,dec
	};
#pragma endregion Dates

	PROGMEM const char *manifoldFormat = PSTR("Vacuum: %3u kPa");
	PROGMEM const char *intakeFormat = PSTR("Intake: %s\xBA\x46");
	PROGMEM const char *speedFormat = PSTR("MPH: %u");
	PROGMEM const char *wheelrpmFormat = PSTR("Wheel RPM: %s");
	PROGMEM const char *horsepowerFormat = PSTR("HP: %s");
	PROGMEM const char *pedalFormat = PSTR("Pedal: %s%%");
	PROGMEM const char *throttleFormat = PSTR("Throttle: %s%%");
	PROGMEM const char *fuelFormat = PSTR("Fuel: %s gal");
	PROGMEM const char *mpgFormat = PSTR("Inst MPG: %s");
	PROGMEM const char *airFormat = PSTR("Air: %s lbs");
	PROGMEM const char *rangeFormat = PSTR("Range: %s mi");
	PROGMEM const char *gpsFormat = PSTR("%3u ft %3u\xBA");
	PROGMEM const char *timeFormat = PSTR("%02u:%02u:%02u %s");
	PROGMEM const char *pedal2Base = PSTR("\x9F\x9E\x9D\x9C\x9B\x99\x99\x99\x99\x99\x99\x99\x99\x99\x99\x99\x99");
	
	byte index = 0;

	memset(buffer, '\0', 18);

	switch(line){
		case DISPLAY_MODE_TRANSMISSION:
			snprintf_P(buffer, 18,  getGearString(), "Gear: ");
			triggerID = TRIGGER_SHIFT;
		break;
		case DISPLAY_MODE_MANIFOLD:
			snprintf_P(buffer, 18, manifoldFormat, manifoldPressure);
			triggerID = TRIGGER_MANIFOLD;
		break;
		case DISPLAY_MODE_INTAKE:
			dtostrf(intakeAirTemperature, -5, 1, displayBuffer);
			snprintf_P(buffer, 18, intakeFormat, displayBuffer);
			triggerID = TRIGGER_INTAKE;
		break;
		case DISPLAY_MODE_SPEED:
			snprintf_P(buffer, 18, speedFormat, speedMPH);
			triggerID = TRIGGER_SPEED;
		break;
		case DISPLAY_MODE_WHEELRPM:
			dtostrf(wheelRPM, -1, 1, displayBuffer);
			snprintf_P(buffer, 18, wheelrpmFormat, displayBuffer);
			triggerID = TRIGGER_SPEED;
		break;
		case DISPLAY_MODE_HORSEPOWER:
			dtostrf(horsePower, -5, 1, displayBuffer);
			snprintf_P(buffer, 18, horsepowerFormat, displayBuffer);
			triggerID = TRIGGER_HORSEPOWER;
		break;
		case DISPLAY_MODE_PEDAL:
			dtostrf(pedalF, -5, 2, displayBuffer);
			snprintf_P(buffer, 18, pedalFormat, displayBuffer);
			triggerID = TRIGGER_PEDAL;
		break;
		case DISPLAY_MODE_THROTTLE:
			dtostrf(throttleF, -5, 2, displayBuffer);
			snprintf_P(buffer, 18, throttleFormat, displayBuffer);
			triggerID = TRIGGER_THROTTLE;
		break;
		case DISPLAY_MODE_FUEL:
			dtostrf(fuelRemaining, -5, 2, displayBuffer);
			snprintf_P(buffer, 18, fuelFormat, displayBuffer);
			triggerID = TRIGGER_FUEL;
		break;
		case DISPLAY_MODE_AIR:
			dtostrf(totalMAF, -5, 2, displayBuffer);
			snprintf_P(buffer, 18, airFormat, displayBuffer);
			triggerID = TRIGGER_TIME;	//used this to avoid creating a new trigger just for totalMAF
		break;
		case DISPLAY_MODE_MPG:
			dtostrf(MPG, -5, 2, displayBuffer);
			snprintf_P(buffer, 18, mpgFormat, displayBuffer);
			triggerID = TRIGGER_MPG;
		break;
		case DISPLAY_MODE_RTE:
			dtostrf(rangeToEmpty, -5, 1, displayBuffer);
			snprintf_P(buffer, 18, rangeFormat, displayBuffer);
			triggerID = TRIGGER_FUEL;
		break;
		case DISPLAY_MODE_GPS:
			snprintf_P(buffer, 18, gpsFormat, elevation, heading);
			triggerID = TRIGGER_GEOGRAPHIC;
		break;
		case DISPLAY_MODE_DATE:
			snprintf_P(buffer, 18, timeFormat, hour, minute, second, ampm ? "PM" : "AM");
			triggerID = TRIGGER_TIME;
		break;
		case DISPLAY_MODE_TIME:
			snprintf_P(buffer, 18, months[month - 1], day, year);
			triggerID = TRIGGER_DATE;
		break;
		case DISPLAY_MODE_PEDAL2:
			index = (pedal * 20) / 255;
			index = index > 17 ? 17 : (byte)index;
			memcpy_P(buffer, pedal2Base, index);
			triggerID = TRIGGER_PEDAL;
		break;
		case DISPLAY_MODE_THROTTLE2:
			index = (throttle * 20) / 255;
			index = index > 17 ? 17 : (byte)index;
			memcpy_P(buffer, pedal2Base, index);
			triggerID = TRIGGER_THROTTLE;
		break;
		default:
			triggerID = 0;
		break;
	}
}

const bool Display::handleTemporaryTriggers(void){
	PROGMEM const char *dimmerFormat = PSTR("Dimmer: %3u%%");
	PROGMEM const char *hvacFormat = PSTR("Fan: %u");
	PROGMEM const char *xmFormat = PSTR("XM Channel: %u");
	PROGMEM const char *speedAlertFormat = PSTR("    Warning      \n  Speed >= 70");
	//PROGMEM const char *welcomeJacob = PSTR("\x99\x99\x99Welcome Back\x99\x99\n\x99\x99\x99\x99\x99\x99Jacob\x99\x99\x99\x99\x99\x99");

	unsigned long now = millis();

	/*if(triggers & TRIGGER_WELCOME){
		if(updateText((const byte*)displayBuffer, (byte)length, 6000)){
			length = snprintf_P(displayBuffer, 36, welcomeJacob);
			triggers &= ~TRIGGER_WELCOME;
			return true;
		}
	}*/

	if(triggers & TRIGGER_DIMMER){
		length = snprintf_P(displayBuffer, 17, dimmerFormat, dimmer);
		if(updateText((const byte*)displayBuffer, (byte)length, 2000)){
			triggers &= ~TRIGGER_DIMMER;
			return true;
		}
	}

	if(triggers & TRIGGER_HVAC){
		length = snprintf_P(displayBuffer, 17, hvacFormat, hvacStatus);
		if(updateText((const byte*)displayBuffer, (byte)length, 2000)){
			triggers &= ~TRIGGER_HVAC;
			return true;
		}
	}

	if(triggers & TRIGGER_XM_CHANNEL){
		length = snprintf_P(displayBuffer, 17, xmFormat, xmChannel);
		if(updateText((const byte*)displayBuffer, (byte)length, 2000)){
			triggers &= ~TRIGGER_XM_CHANNEL;
			return true;
		}
	}

	if(triggers & TRIGGER_ALERT_SPEED){
		length = snprintf_P(displayBuffer, 36, speedAlertFormat);
		if(updateText((const byte*)displayBuffer, (byte)length, 4000)){
			//chimeRequest(CHIME_EMIT_LF | CHIME_EMIT_LR | CHIME_BING, 2, 0x70);
			triggers &= ~TRIGGER_ALERT_SPEED;
			return true;
		}
	}
	return false;
}

const char *Display::getGearString(void){
	PROGMEM const char *PARK = PSTR("%sPARK");
	PROGMEM const char *REVERSE = PSTR("%sREVERSE");
	PROGMEM const char *NEUTRAL = PSTR("%sNEUTRAL");
	PROGMEM const char *ONE = PSTR("%sONE");
	PROGMEM const char *TWO = PSTR("%sTWO");
	PROGMEM const char *THREE = PSTR("%sTHREE");
	PROGMEM const char *FOUR = PSTR("%sFOUR");

	switch(currentGear){
		case 0xF0:
			return PARK;
		break;
		case 0xE0:
			return REVERSE;
		break;
		case 0xD0:
			return NEUTRAL;
		break;
		case 0x10:
			return ONE;
		break;
		case 0x20:
			return TWO;
		break;
		case 0x30:
			return THREE;
		break;
		case 0x40:
			return FOUR;
		break;
	}
	return PARK;
}


void Display::autoUpdateDisplay(void){
	
	if(handleTemporaryTriggers()){
		return;
	}

	if(displayStatus & DISP_SHOWING_TEMP){ return; }

	char upperBuffer[18];
	char lowerBuffer[18];
	unsigned long triggerUpper = 0;
	unsigned long triggerLower = 0;
	PROGMEM const char *displayFormat = PSTR("%-17s\n%s");

	writeToBuffer(upperBuffer, upperDisplayMode, triggerUpper);
	writeToBuffer(lowerBuffer, lowerDisplayMode, triggerLower);
	
	length = (byte)snprintf_P(displayBuffer, 36, displayFormat, upperBuffer, lowerBuffer);
	length = length > 36 ? 36 : (byte)length;

	if(triggers & (triggerUpper | triggerLower | TRIGGER_DISP_UPDATE)){
		if(updateText((const byte*)displayBuffer, (byte)length)){
			triggers &= ~(triggerUpper | triggerLower | TRIGGER_DISP_UPDATE);
		}
	}
}




#endif /* DISPLAY_H_ */
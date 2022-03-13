/*
 * TechII.h
 *
 * Created: 7/2/2017 10:30:36 PM
 *  Author: jacob
 */ 


#ifndef TECHII_H_
#define TECHII_H_

#define T2_ENABLE 0x01				// enable | disable diagnostic session
#define T2_SESSION_ACTIVE 0x02		// is there a diagnostic session currently active (tester presents schedule)
#define T2_SESSION_RTN 0x04			// end current session (if any)

/* Flags */
#define LF_SIGNAL 0x01
#define RF_SIGNAL 0x04
#define LR_SIGNAL 0x10
#define RR_SIGNAL 0x40
#define LOW_BEAMS 0x01
#define HIGH_BEAMS 0x04
#define WIPERS_LOW 0x01
#define WIPERS_PUMP 0x04

class TechII{
	public:
		TechII(void);
		void tick(void);
		void enable(const byte &yn);
		void turnSignal(const byte &flags);
		void headLights(const byte &flags);
		void horn(const byte &honk);
		void wipers(const byte &flags);
		void unlockDoors(void);
		void lockDoors(void);
		void setDomeBrightness(const byte &val);

	private:
		bool beginSession(void);
		bool testerPresent(void);
		bool endSession(void);
		
		byte t2Status;
};

TechII::TechII(void){
	t2Status = 0;
}

void TechII::enable(const byte &yn){
	t2Status = (yn ? (t2Status | T2_ENABLE) : (t2Status & ~T2_ENABLE));
	if(yn){
		digitalWrite(BOOT_LED, HIGH);
	}else{
		digitalWrite(BOOT_LED, LOW);
	}
}

void TechII::tick(void){
	if(!(status & STATUS_AUTH) || currentGear != 0xF0){
		t2Status &= ~T2_ENABLE;
		return;
	}

	if((!(t2Status & T2_ENABLE) && (t2Status & T2_SESSION_ACTIVE))){
		if(endSession()){
			t2Status = 0;
		}
		return;
	}

	if((t2Status & T2_SESSION_RTN)){
		if(millis() >= t2ReturnToNormalAt){
			if(endSession()){
				t2Status = 0;
			}
		}
		return;
	}

	if((t2Status & T2_ENABLE) && !(t2Status & T2_SESSION_ACTIVE)){
		if(beginSession()){
			t2Status |= T2_SESSION_ACTIVE;
		}
	}

	if(!(t2Status & T2_ENABLE)){ 
		return; 
	}

	unsigned long now = millis();

	if(t2lastTesterPresent + 1800 < now){
		if(testerPresent()){
			t2lastTesterPresent = now;
		}
	}
}

void TechII::turnSignal(const byte &flags){
	if(!(t2Status & T2_SESSION_ACTIVE)){ return; }

	Message msg;
	msg.frame_id = TECHII_BODY_REQ;
	msg.busId = 1;
	msg.length = 8;
	msg.frame_data[0] = 0x07;
	msg.frame_data[1] = 0xAE;
	msg.frame_data[2] = 0x13;

	if(flags & LF_SIGNAL){
		msg.frame_data[3] = 0x04;
		msg.frame_data[4] = (flags & (LF_SIGNAL << 1)) ? 0x04 : 0x00;
		writeQueue.push(msg);
	}

	if(flags & RF_SIGNAL){
		msg.frame_data[3] = 0x10;
		msg.frame_data[4] = (flags & (RF_SIGNAL << 1)) ? 0x10 : 0x00;
		writeQueue.push(msg);
	}

	if(flags & LR_SIGNAL){
		msg.frame_data[3] = 0x40;
		msg.frame_data[4] = (flags & (LR_SIGNAL << 1)) ? 0x40 : 0x00;
		writeQueue.push(msg);
	}

	if(flags & RR_SIGNAL){
		msg.frame_data[3] = 0x80;
		msg.frame_data[4] = (flags & (RR_SIGNAL << 1)) ? 0x80 : 0x00;
		writeQueue.push(msg);
	}
}

void TechII::headLights(const byte &flags){
	if(!(t2Status & T2_SESSION_ACTIVE)){ return; }

	Message msg;
	msg.frame_id = TECHII_BODY_REQ;
	msg.busId = 1;
	msg.length = 8;
	msg.frame_data[0] = 0x07;
	msg.frame_data[1] = 0xAE;
	msg.frame_data[2] = 0x12;

	if(flags & LOW_BEAMS){
		msg.frame_data[3] = 0x40;
		msg.frame_data[4] = (flags & (LOW_BEAMS << 1)) ? 0x40 : 0x00;
		writeQueue.push(msg);
	}

	if(flags & HIGH_BEAMS){
		msg.frame_data[3] = 0x20;
		msg.frame_data[4] = (flags & (HIGH_BEAMS << 1)) ? 0x20 : 0x00;
		writeQueue.push(msg);
	}
}

void TechII::horn(const byte &honk){
	if(!(t2Status & T2_SESSION_ACTIVE)){ return; }

	Message msg;
	msg.frame_id = TECHII_BODY_REQ;
	msg.length = 8;
	msg.busId = 1;
	msg.frame_data[0] = 0x07;
	msg.frame_data[1] = 0xAE;
	msg.frame_data[2] = 0x1A;
	if(honk){
		msg.frame_data[3] = 0x02;
		msg.frame_data[4] = 0x00;
		msg.frame_data[5] = 0xFF;
	}
	writeQueue.push(msg);
}

void TechII::wipers(const byte &flags){
	if(!(t2Status & T2_SESSION_ACTIVE)){ return; }

	Message msg;
	msg.frame_id = TECHII_BODY_REQ;
	msg.length = 8;
	msg.busId = 1;
	msg.frame_data[0] = 0x07;
	msg.frame_data[1] = 0xAE;
	msg.frame_data[2] = 0x18;
	
	if(flags & WIPERS_LOW){
		msg.frame_data[3] = 0x08;
		msg.frame_data[4] = (flags & (WIPERS_LOW << 1)) ? 0x08 : 0x00;
		writeQueue.push(msg);
	}

	if(flags & WIPERS_PUMP){
		msg.frame_data[3] = 0x02;
		msg.frame_data[4] = (flags & (WIPERS_PUMP << 1)) ? 0x02 : 0x00;
		writeQueue.push(msg);
	}
}

void TechII::unlockDoors(void){
	if(!(status & STATUS_AUTH)){ return; }
	Message msg;
	msg.frame_id = SENSOR_RFA;
	msg.busId = 2;
	msg.ide = 0x08;
	msg.length = 2;
	msg.frame_data[0] = 0x02;
	msg.frame_data[1] = 0x02;
	writeQueue.push(msg);
}

void TechII::lockDoors(void){
	if(!(status & STATUS_AUTH)){ return; }
	Message msg;
	msg.frame_id = SENSOR_RFA;
	msg.busId = 2;
	msg.ide = 0x08;
	msg.length = 2;
	msg.frame_data[0] = 0x02;
	msg.frame_data[1] = 0x01;
	writeQueue.push(msg);
}

void TechII::setDomeBrightness(const byte &val){
	if(!(status & STATUS_AUTH)){ return; }
	Message msg;
	msg.frame_id = TECHII_BODY_REQ;
	msg.busId = 1;
	msg.length = 8;
	msg.frame_data[0] = 0x07;
	msg.frame_data[1] = 0xAE;
	msg.frame_data[2] = 0x06;
	msg.frame_data[3] = 0xFF;
	msg.frame_data[4] = 0xFF;
	msg.frame_data[5] = 0xFF;
	msg.frame_data[6] = val;
	msg.frame_data[7] = 0xFF;
	writeQueue.push(msg);
}

/*
void TechII::unlockDoors(void){	
	Message msg;
	msg.frame_id = TECHII_BODY_REQ;
	msg.busId = 1;
	msg.length = 8;
	msg.frame_data[0] = 0x07;
	msg.frame_data[1] = 0xAE;
	msg.frame_data[2] = 0x10;
	msg.frame_data[3] = 0x04;
	msg.frame_data[4] = 0x04;
	msg.frame_data[5] = 0x02;
	msg.frame_data[6] = 0x02;

	if(writeQueue.push(msg)){
		t2Status |= T2_SESSION_RTN;
		trReturnToNormal = millis() + 100;
	}
}

void TechII::lockDoors(void){
	Message msg;
	msg.frame_id = TECHII_BODY_REQ;
	msg.busId = 1;
	msg.length = 8;
	msg.frame_data[0] = 0x07;
	msg.frame_data[1] = 0xAE;
	msg.frame_data[2] = 0x10;
	msg.frame_data[3] = 0x02;
	msg.frame_data[4] = 0x02;
	msg.frame_data[5] = 0x04;
	msg.frame_data[6] = 0x04;
	
	if(writeQueue.push(msg)){
		t2Status |= T2_SESSION_RTN;
		trReturnToNormal = millis() + 100;
	}
}
*/

bool TechII::beginSession(void){
	Message msg;
	msg.frame_id = TECHII_BODY_REQ;
	msg.busId = 1;
	msg.length = 8;
	msg.frame_data[0] = 0x02;	// ??
	msg.frame_data[1] = 0x10;	// BEGIN DIAGNOSTIC SESSION
	msg.frame_data[2] = 0x03;	// ??
	
	return writeQueue.push(msg);
}

bool TechII::testerPresent(void){
	Message msg;
	msg.frame_id = TECHII_BODY_REQ;
	msg.busId = 1;
	msg.length = 8;
	msg.frame_data[0] = 0x01;	// ??
	msg.frame_data[1] = 0x3E;	//TESTER PRESENT
	
	return writeQueue.push(msg);
}

bool TechII::endSession(void){
	Message msg;
	msg.frame_id = TECHII_BODY_REQ;
	msg.busId = 1;
	msg.length = 8;
	msg.frame_data[0] = 0x01;	// ??
	msg.frame_data[1] = 0x20;	//RETURN TO NORMAL
	
	return writeQueue.push(msg);
}


#endif /* TECHII_H_ */
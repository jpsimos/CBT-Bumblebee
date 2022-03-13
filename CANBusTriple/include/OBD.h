#ifndef __OBD__
#define __OBD__

/* Author Jacob Psimos 8/25/2016 */
/* All rights reserved */

#define OBD_PID_MAF 0x10
#define OBD_PID_BAROMETER 0x33
#define OBD_PID_INTAKE_AIR_TEMP 0x0F
#define OBD_PID_MANIFOLD_PRESSURE 0x0B

class OBD{
	public:
		OBD(void);
		void tick(void);
		void process(const Message &msg);
		void request(const byte &pid);
	private:
		void calculateMPG(void);
		void tpms(void);
		byte requestIndex;
		unsigned int requestDelay;
		//unsigned long lastRequest;
};

OBD::OBD(void){
	requestIndex = 0;
	lastRequest = 0;
	requestDelay = 260;
}

void OBD::tick(void){
	if(!(status & STATUS_AUTH)) { return; }

	const byte pids[4] = {
		OBD_PID_MAF,
		OBD_PID_BAROMETER,
		OBD_PID_INTAKE_AIR_TEMP,
		OBD_PID_MANIFOLD_PRESSURE
	};

	if((status & STATUS_ENABLE_OBD) && (lastRequest + requestDelay < millis())){
		request(pids[requestIndex]);
		requestIndex = (requestIndex + 1 > sizeof(pids)) ? 0 : requestIndex + 1;
	}

	if(triggers & TRIGGER_TPMS){
		tpms();
	}
}

void OBD::process(const Message &msg){
	unsigned int temp = 0;
	float tempf = 0.0;

	if(msg.frame_id == SENSOR_OBD_RESP){
		switch(msg.frame_data[3]){
			case OBD_PID_MAF:
				temp = (unsigned int)((((unsigned int)msg.frame_data[4] << 8) + (unsigned int)msg.frame_data[5]) / (unsigned int)100);
				if(temp != MAF){
					triggers |= TRIGGER_MPG;
				}

				MAF = temp;
				tempf = (float)MAF * 0.002205;
				totalMAF += tempf + ((tempf * ((float)millis() - (float)lastRequest)) / 1000.0);

				calculateMPG();
			break;
			case OBD_PID_INTAKE_AIR_TEMP:
				tempf = ((((float)msg.frame_data[4]) - 40.0) * 1.8) + 32.0;
				if(abs(tempf - intakeAirTemperature) > 0.1){
					intakeAirTemperature = tempf;
					triggers |= TRIGGER_INTAKE;
				}
			break;
			case OBD_PID_MANIFOLD_PRESSURE:
				if(msg.frame_data[4] != manifoldPressure){
					manifoldPressure = msg.frame_data[4];
					triggers |= TRIGGER_MANIFOLD;
				}
			break;
		}
	}
}

void OBD::calculateMPG(void){
	float gallonsPerHour = ((float)MAF / 14.7 / 6.17 / 453.592) * 60.0;
	if((status & STATUS_ENGINE_RUNNING) && gallonsPerHour > 0.0){
		MPG = ((float)speedMPH / 60.0) / gallonsPerHour;
	}else{
		MPG = 0.0;
	}
}

void OBD::request(const byte &pid){
	Message msg;
	msg.frame_id = SENSOR_OBD_REQ;
	msg.busId = 1;
	msg.length = 8;
	msg.frame_data[0] = 0x03;
	msg.frame_data[1] = 0x22;
	msg.frame_data[3] = pid;

	if(writeQueue.push(msg)){
		lastRequest = millis();
	}
}

void OBD::tpms(){
	//static unsigned long ttl = 0;
	unsigned long now = millis();

	if(!tpmsUpdateAt){
		tpmsUpdateAt = now + 2048;
	}else if(tpmsUpdateAt < now){
		Message msg;
		msg.ide = 0x08;
		msg.busId = 2;
			
		msg.frame_id = SENSOR_TPMS_DSP;
		msg.frame_data[0] = 0x7C;	//0x7E triggers the warning light.
		msg.length = 2;
		
		if(writeQueue.push(msg)){
			tpmsUpdateAt = 0;
			triggers &= ~TRIGGER_TPMS;
			setPressures(46, 46, 46, 46);
		}
	}

}


#endif


/*
 * Timers.h
 *
 * Created: 6/28/2017 6:44:21 AM
 *  Author: jacob
 */ 


#ifndef TIMERS_H_
#define TIMERS_H_


	//Display.h
	unsigned long lastDequeue = 0;
	unsigned long lastTempUpdate = 0;
	unsigned long updateDisplayAt = 0;
	
	//Handlers.h
	unsigned long radioButtonTerminateAt = 0;

	//OBD.h
	unsigned long lastRequest = 0;
	unsigned long tpmsUpdateAt = 0;

	//Sleepy.h
	unsigned long lastWakeup = 0;

	//Buttons.h
	unsigned long a1_start = 0;
	unsigned long a5_start = 0;
	unsigned long a1_dur = 0;
	unsigned long a5_dur = 0;
	
	//TechII.h
	unsigned long t2lastTesterPresent = 0;
	unsigned long t2ReturnToNormalAt = 0;

#endif /* TIMERS_H_ */
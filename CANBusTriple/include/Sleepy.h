/* Author Jacob Psimos 2016 */
/* This is a confediental file. No unauthorized access! */
/* All rights reserved */

#ifndef __SLEEPY__
#define __SLEEPY__

#include <avr/power.h>
#include <avr/sleep.h>

#define SLEEP_TIMER 10000

class Sleepy{
	public:
	Sleepy(void);
	void tick(void);
	void process(const Message&);
	private:
	void SleepAVR(void);
	static void handleInterrupt(void);
	unsigned long lastWakeup;     //last wakeup message received
};

Sleepy::Sleepy(void){
	lastWakeup = 0;
}

void Sleepy::tick(void){
	if(!(status & STATUS_ENABLE_SLEEP)){ return; }
	unsigned long now = millis();
	if(lastWakeup + SLEEP_TIMER < now){
		onSleep();
		SleepAVR();
	}
}

void Sleepy::process(const Message &msg){
	if(msg.frame_id != SENSOR_VIN_INT){ return; }
	
	lastWakeup = millis();
}


void Sleepy::SleepAVR(void){

	if(!(status & STATUS_ENABLE_SLEEP) || !(status & STATUS_AUTH)){
		return;
	}

	status &= ~(STATUS_ENABLE_SLEEP | STATUS_AUTH);
	lastWakeup = millis() + 10000;

	for(byte i = 0; i < 5; i++){
		digitalWrite(BOOT_LED, HIGH);
		delay(200);
		digitalWrite(BOOT_LED, LOW);
		delay(200);
	}

	//Sleep CAN busses & set wakeup filter
	CANBus1.prepareWakeupInterrupt();
	CANBus1.setFilter(0, SENSOR_VIN_INT, SENSOR_VIN_INT);
	CANBus2.powerDown();
	#ifdef INCLUDE_BUS3
	CANBus3.powerDown();
	#endif
	
	//CANBus1.clearRxInt();
	delay(10);
	
	PORTE &= B11111011;
	
	attachInterrupt(digitalPinToInterrupt(CAN1INT_D), reinterpret_cast<void (*)()>(&handleInterrupt), LOW);

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	cli();
	sleep_enable();
	sei();
	power_adc_disable();
	sleep_cpu();

	//Wakeup here

	delay(4);
	sleep_disable();
	power_adc_enable();
	delay(4);
	
	//Wakeup CAN busses & restore filter
	CANBus1.powerUp();
	CANBus1.setFilter(0, SENSOR_VIN_INT, SENSOR_OBD_RESP);
	CANBus2.powerUp();
	#ifdef INCLUDE_BUS3
	CANBus3.powerUp();
	#endif

	PORTE |= B00000100;
}

void Sleepy::handleInterrupt(){
	detachInterrupt(digitalPinToInterrupt(CAN1INT_D));
}


#endif







#ifndef __GLOBALS__
#define __GLOBALS__

/* Buttons.h */
#define A1_DOWN 0x01
#define A5_DOWN 0x02
#define DBL_DOWN 0x04
#define A1_HANDLE 0x08
#define A5_HANDLE 0x10
#define DBL_HANDLE 0x20
#define BTN_HELD 0x40

/* Chime Properties */
#define CHIME_DING_FADE 0x07
#define CHIME_LOW_BEEP 0x04
#define CHIME_TICK_TOCK 0x03
#define CHIME_BEEP 0x05
#define CHIME_BING 0x06
#define CHIME_EMIT_LF 0x80
#define CHIME_EMIT_RF 0x40
#define CHIME_EMIT_LR 0x20
#define CHIME_EMIT_RR 0x10

/* Steering wheel buttons (not cruise control) */
#define BUTTON_MUTE 0x01
#define BUTTON_VOL_DOWN 0x02
#define BUTTON_VOL_UP 0x03
#define BUTTON_RIGHT 0x04
#define BUTTON_LEFT 0x05
#define BUTTON_SOURCE 0x06
#define BUTTON_UP 0x07
#define BUTTON_UNKNOWN 0x08
#define BUTTON_DOWN 0x09
#define BUTTON_INFO 0x0B


#define DISPLAY_MODE_TRANSMISSION 1
#define DISPLAY_MODE_MANIFOLD 2
#define DISPLAY_MODE_INTAKE 3
#define DISPLAY_MODE_SPEED 4
#define DISPLAY_MODE_WHEELRPM 5
#define DISPLAY_MODE_HORSEPOWER 6
#define DISPLAY_MODE_PEDAL 7
#define DISPLAY_MODE_PEDAL2 8
#define DISPLAY_MODE_THROTTLE 9
#define DISPLAY_MODE_THROTTLE2 10
#define DISPLAY_MODE_FUEL 11
#define DISPLAY_MODE_MPG 12
#define DISPLAY_MODE_AIR 13
#define DISPLAY_MODE_RTE 14
#define DISPLAY_MODE_GPS 15
#define DISPLAY_MODE_DATE 16
#define DISPLAY_MODE_TIME 17
#define DISPLAY_NUM_MODES 17	//KEEP updated

void globalTick(void);
void blinkLED(byte times, const unsigned int &duration);
void onSleep(void);
void getDisplayMode(byte &upper, byte &lower);
void buttonHandler(const byte);
void chimeRequest(const byte properties, const byte numChimes, const byte chimeDelay);
void steeringWheelButton(const byte &button, const unsigned int &duration);
float distanceMeters(const float &lat1, const float &log1, const float &lat2, const float &log2);
void setPressures(byte leftFront, byte rightFront, byte leftRear, byte rightRear);

#endif

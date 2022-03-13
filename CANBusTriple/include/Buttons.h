#ifndef __BUTTONS__
#define __BUTTONS__

/*
 * Button handler for CanBusTriple
 * Author Jacob Psimos 2017
 * Version 2
 * 
 * Detects button presses and press duration, allowing for toggles,
 * momentary contacts, and functional meanings from press durations.
 */

/* A5 = BLACK BUTTON */
/* A1 = RED BUTTON */

#define BUTTON_DURATION_MIN 150
#define BUTTON_DURATION_MAX 5000
#define BUTTON_DURATION_HELD 1000

class Buttons{
  public:
  
    Buttons(void);
    void tick(void);

  private:
    byte btnStatus;
    //unsigned long a1_start;	//RED
    //unsigned long a5_start; //BLK
    //unsigned long a1_dur;
    //unsigned long a5_dur;
};

Buttons::Buttons(void){
  btnStatus = 0;

  pinMode(A0, OUTPUT);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A4, OUTPUT);
  pinMode(A5, INPUT_PULLUP);
  digitalWrite(A0, LOW);
  digitalWrite(A4, LOW);
}

void Buttons::tick(void){
  if(!(status & STATUS_AUTH)){
	btnStatus = 0;
	return;
  }

  byte _a1 = (byte)digitalRead(A1);
  byte _a5 = (byte)digitalRead(A5);
  unsigned long now = millis();

  if(now <= 2){
	btnStatus = 0;
  }

  /* Detect button DOWN */

  if( _a1 == HIGH && !(btnStatus & A1_DOWN) ){
    btnStatus |= A1_DOWN;
    a1_start = now;
  }

  if( _a5 == HIGH && !(btnStatus & A5_DOWN) ){
    btnStatus |= A5_DOWN;
    a5_start = now;
  }

  if( _a1 == HIGH && _a5 == HIGH && !(btnStatus & DBL_DOWN) ){
    btnStatus |= DBL_DOWN;
  }

  /* Detect button RELEASE */
  
  if( _a1 == LOW && (btnStatus & A1_DOWN) ){
    btnStatus &= ~A1_DOWN;
    a1_dur = now - a1_start;
    if(a1_dur >= BUTTON_DURATION_MIN && a1_dur <= BUTTON_DURATION_MAX){
      btnStatus |= A1_HANDLE;
	  if(a1_dur >= BUTTON_DURATION_HELD){
		btnStatus |= BTN_HELD;
	  }
    }else{
      btnStatus &= ~A1_HANDLE;
    }
  }

  if( _a5 == LOW && (btnStatus & A5_DOWN) ){
    btnStatus &= ~A5_DOWN;
    a5_dur = now - a5_start;
    if(a5_dur >= BUTTON_DURATION_MIN && a5_dur <= BUTTON_DURATION_MAX){
      btnStatus |= A5_HANDLE;
	  if(a5_dur >= BUTTON_DURATION_HELD){
		  btnStatus |= BTN_HELD;
	  }
    }else{
      btnStatus &= ~A5_HANDLE;
    }
  }

  if( _a1 == LOW && _a5 == LOW && (btnStatus & DBL_DOWN) ){
    btnStatus &= ~DBL_DOWN;
    if( (btnStatus & A1_HANDLE) && (btnStatus & A5_HANDLE) ){
      btnStatus |= DBL_HANDLE;
    }else{
      btnStatus &= ~DBL_HANDLE;
    }
  }

  /* Process Handlers */
  if( !(btnStatus & (A1_DOWN | A5_DOWN | DBL_DOWN)) && (btnStatus >= A1_HANDLE) ){
     buttonHandler(btnStatus);
     btnStatus &= ~(A1_HANDLE | A5_HANDLE | DBL_HANDLE | BTN_HELD);
  }
}



#endif

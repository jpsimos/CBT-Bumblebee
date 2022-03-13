/* Author Jacob Psimos 2016 */
/* This is a confediental file. No unauthorized access! */

/*
    BUTTON to PIN map
    A5 = RIGHT BUTTON
    A1 = LEFT BUTTON
 */

#ifndef __BUTTONS__
#define __BUTTONS__

#define BUTTON_DURATION 150
#define BUTTON_DURATION_MAX 2000

//#define BDEBUG

class Buttons{
  public:
    Buttons();
    void tick();
    bool disabled;
    bool waiting;
    void WakeUp();
  private:
    void handle();
    unsigned long a1_start;
    unsigned long a5_start;
    unsigned int a1_dur;
    unsigned int a5_dur;
    bool a1_down;
    bool a5_down;
    bool dbl_down;
    bool a1_handle;
    bool a5_handle;
    bool dbl_handle;
};

Buttons::Buttons(){
  a5_start = 0;
  a1_start = 0;
  a5_dur = 0;
  a1_dur = 0;
  a1_down = false;
  a5_down = false;
  a1_handle = false;
  a5_handle = false;
  dbl_handle = false;
  dbl_down = false;
  this->disabled = false;
  waiting = false;

  pinMode(A0, OUTPUT);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A4, OUTPUT);
  pinMode(A5, INPUT_PULLUP);
  digitalWrite(A0, LOW);
  digitalWrite(A4, LOW);
}

void Buttons::WakeUp(){
  this->waiting = false;
  this->a1_down = false;
  this->a5_down = false;
  this->a1_handle = false;
  this->a5_handle = false;
  this->dbl_handle = false;
  this->dbl_down = false;
  this->disabled = false;
}


void Buttons::handle(){
  
  if(dbl_handle){
    #ifdef BDEBUG
      Serial.print("[A1+A5] ");
      Serial.print(a1_dur, DEC);
      Serial.print(" ms\r\n");
    #else
      handle_doublePress();
    #endif
  }else if(a1_handle && !a5_handle){
    #ifdef BDEBUG
      Serial.print("[A1] ");
      Serial.print(a1_dur, DEC);
      Serial.print(" ms\r\n");
    #else
      handle_A1Press();
    #endif
  }else if(a5_handle && !a1_handle){
    #ifdef BDEBUG
      Serial.print("[A5] ");
      Serial.print(a5_dur, DEC);
      Serial.print(" ms\r\n");
    #else
      handle_A5Press();
    #endif
  }

  dbl_handle = false;
  a1_handle = false;
  a5_handle = false;
}

void Buttons::tick(){
  
  if(this->disabled){ return; }
  
  now = millis();
  
  byte _a1 = (byte)digitalRead(A1);     //LEFT BUTTON
  byte _a5 = (byte)digitalRead(A5);     //RIGHT BUTTON

  if(_a1 == HIGH && !a1_down){
    a1_down = true;
    a1_start = now;
    waiting = true;
  }

  if(_a5 == HIGH && !a5_down){
    a5_down = true;
    a5_start = now;
    waiting = true;
  }

  if(_a1 == HIGH && _a5 == HIGH && !dbl_down){
    dbl_down = true;
  }

  if(_a1 == LOW && a1_down){
    a1_down = false;
    a1_dur = (unsigned int)(now - a1_start);
    a1_handle = a1_dur >= BUTTON_DURATION && a1_dur <= BUTTON_DURATION_MAX;
  }

  if(_a5 == LOW && a5_down){
    a5_down = false;
    a5_dur = (unsigned int)(now - a5_start);
    a5_handle = a5_dur >= BUTTON_DURATION && a5_dur <= BUTTON_DURATION_MAX;
  }

  if(_a1 == LOW && _a5 == LOW && dbl_down){
    dbl_down = false;
    dbl_handle = a1_handle && a5_handle;
  }

  if((a1_handle || a5_handle || dbl_handle) && (!a1_down && !a5_down && !dbl_down)){
    waiting = false;
    handle();
  }
}


#endif












/* Author Jacob Psimos 2016 */
/* This is a confediental file. No unauthorized access! */

#ifndef __BCM__
#define __BCM__

#define BCM_REQUEST 0x241
#define BCM_REPLY 0x641

#define BCM_LEFT_FRONT_SIGNAL 1
#define BCM_LEFT_REAR_SIGNAL 2
#define BCM_RIGHT_FRONT_SIGNAL 3
#define BCM_RIGHT_REAR_SIGNAL 4

#define BCM_UNLOCK_DOORS false
#define BCM_LOCK_DOORS true

//#define BCM_DEBUG

class BCM{
  public:
    BCM();
	  void process(const Message &);
    void tick();
    void Wipers(bool);
    void TurnSignal(byte, bool);
    void HighBeams(bool);
    //void DoorLocks(bool);
	  bool enabled;
    bool discoEnabled;
	private:
    void testerPresent();
    void startSession();
    void sendMessage(byte*, const byte, const byte);
		bool started;
    byte discoIndex;
    boolean quitDisco;
    unsigned long lastDisco;
		unsigned long lastAck;
};

BCM::BCM(){
	enabled = false;
	started = false;
  discoEnabled = false;
  discoIndex = BCM_LEFT_FRONT_SIGNAL;
  quitDisco = false;
  lastDisco = 0;
	lastAck = millis();
}

void BCM::tick(){

  if(!auth1 || !auth2) { return; }

  if(this->enabled){
    if(!started){
      startSession();
    }
    
    now = millis();
    
    if(lastAck + 2500 <= now){
      testerPresent();
      lastAck = now;
    }else if(lastAck > now){
      lastAck = 0;
    }

    if(discoEnabled && lastDisco + 150 <= now){
      lastDisco = now;
      discoIndex = (discoIndex == BCM_LEFT_FRONT_SIGNAL) ? BCM_RIGHT_FRONT_SIGNAL : BCM_LEFT_FRONT_SIGNAL;
      TurnSignal(discoIndex, true);
      quitDisco = true;
    }else if(!discoEnabled && quitDisco){
      quitDisco = false;
      TurnSignal( discoIndex , false );
    }
    
  }else{
    if(this->started) { 
      this->started = false; 
      this->discoEnabled = false;
    }
  }
  
}

//(byte index 1) [0x01 = lock door] [0x02 = unlock driver door] [0x03 = unlock all doors]
/*
void BCM::DoorLocks(bool lock){
  Message msg;
  msg.frame_id = SENSOR_RFA;
  msg.length = 2;
  msg.dispatch = true;
  msg.busId = 2;
  msg.ide = 0x08;
  msg.frame_data[0] = 0x02;
  msg.frame_data[1] = lock ? 0x01 : 0x02;

  if(!writeQueue.isFull()){
    writeQueue.push(msg);
  }
}
*/

void BCM::TurnSignal(byte sig, bool on){
  if(!this->enabled || !started) { return; }

  byte frame[5] = { 0x07, 0xAE, 0x13 };
  
  switch(sig){
    case BCM_LEFT_FRONT_SIGNAL:
      frame[3] = 0x04;
      frame[4] = (on ? (byte)0x04 : 0); 
    break;
    case BCM_LEFT_REAR_SIGNAL:
      frame[3] = 0x40;
      frame[4] = (on ? (byte)0x40 : 0);
    break;
    case BCM_RIGHT_FRONT_SIGNAL:
      frame[3] = 0x10;
      frame[4] = (on ? (byte)0x10 : 0);
    break;
    case BCM_RIGHT_REAR_SIGNAL:
      frame[3] = 0x80;
      frame[4] = (on ? (byte)0x80 : 0);
    break;
    default:
      frame[3] = 0xFF;
    break;
  }

  if(frame[3] != 0xFF){
      sendMessage(frame, 5, 8);
  }
}

void BCM::HighBeams(bool on){
  if(this->enabled && started){
    byte frame[5] = { 0x07, 0xAE, 0x12, 0x20 };
    frame[4] = (on ? (byte)0x20 :  0);
    sendMessage(frame, 5, 8);
  }
}

void BCM::startSession(){
  byte frame[3] = { 0x02, 0x10, 0x03 };
  sendMessage(frame, 3, 8);
  
  this->started = true;
  digitalWrite(BOOT_LED, HIGH);
}

void BCM::sendMessage(byte *data, const byte buflen, const byte msglen){
  if(buflen > 8 || msglen > 8 || !auth1 || !auth2) { return; }
  
  Message msg;
  msg.busId = 1;
  msg.ide = 0;
  msg.frame_id = BCM_REQUEST;
  for(byte i = 0; i < buflen; i++){
    msg.frame_data[i] = data[i];
  }
  msg.length = msglen;
  msg.dispatch = true;

  if(!writeQueue.isFull()){
    writeQueue.push(msg);
  }
}

void BCM::testerPresent(){
  byte frame[2] = { 0x01, 0x3E };
  sendMessage(frame, 2, 8);
}

#endif






/*
 * Can Bus Triple - SerialCommand rewrite by Jacob Psimos
 */

#include "Global.h"

#define COMMAND_BUSLOG 0x01
#define COMMAND_FILTER 0x08
#define COMMAND_BUSLOG_MODE 0x09
#define COMMAND_DASHLOG 0x02
#define COMMAND_BLUETOOTH 0x03
#define COMMAND_SEND 0x04
#define COMMAND_HALT 0x05
#define COMMAND_DEBUG 0x06
#define COMMAND_RADIO 0x11

#define BUSLOG_MODE_DEFAULT 0x00
#define BUSLOG_MODE_ASCII 0x01

/* RADIO */
#define COMMAND_RADIO_SET_TEXT 0x01

/* BLE112 */
#define COMMAND_BLUETOOTH_PASSTHRU 1
#define COMMAND_BLUETOOTH_CONNECTED 2
#define COMMAND_BLUETOOTH_DFU 4
#define COMMAND_BLUETOOTH_RESET 5
#define COMMAND_BLUETOOTH_TOGGLE 6

class SerialCommand
{
  public:
    //Constructors
    SerialCommand( QueueArray<Message> *q, TCM*, Radio*, Sleepy*);
    
    //Global Variables
    Stream* activeSerial;

    //Methods
    void tick();
    void commandHandler(byte*, int);
    void printMessageToSerial(const Message& );
    void resetToBootloader();
    void process( const Message& );
    
  private:

    //Private Global Variables
    QueueArray<Message>* mainQueue;
    boolean passthroughMode;
    byte busLogEnabled[3];
    byte busLogMode;
    boolean dashLogEnabled;
    TCM *tcm;
    Radio *radio;
    Sleepy *sleepy;
    
    //Private Methods
    void busLogCommand(byte *, int );
    void filterCommand(byte *, int );
    void dashLogCommand(byte *, int );
    void radioCommand(byte *, int );
    void bluetoothCommand(byte *, int );
    void processCommand(byte);
    int  getCommandBody(byte *, int );
    void clearBuffer();
    void getAndSend(byte *, int );
};

SerialCommand::SerialCommand( QueueArray<Message> *q, TCM *tcm, Radio *radio, Sleepy *sleepy){
  mainQueue = q;
  passthroughMode = false;
  activeSerial = &Serial;
  dashLogEnabled = false;
  busLogEnabled[0] = 0;
  busLogEnabled[1] = 0;
  busLogEnabled[2] = 0;
  busLogMode = BUSLOG_MODE_DEFAULT;
  this->tcm = tcm;
  this->radio = radio;
  this->sleepy = sleepy;
}

void SerialCommand::tick(){
  if( passthroughMode == true ){
    while(Serial.available()) Serial1.write(Serial.read());
    while(Serial1.available()) Serial.write(Serial1.read());
    return;
  }

  if( Serial1.available() > 0 ){
    activeSerial = &Serial1;
    processCommand( Serial1.read() );
  }

  if( Serial.available() > 0 ){
    activeSerial = &Serial;
    processCommand( Serial.read() );
  }
}

void SerialCommand::process(const Message &msg ){
  printMessageToSerial(msg);
}

void SerialCommand::processCommand(byte command){
  
  delay(32);

  byte commandBody[32] = {0x0};
  int bytesRead = getCommandBody(commandBody, 32);
  
  switch( command ){
    case COMMAND_BUSLOG:
      busLogCommand(commandBody, bytesRead);
    break;
    case COMMAND_FILTER:
      filterCommand(commandBody, bytesRead);
    break;
    case COMMAND_BUSLOG_MODE:
      busLogMode = commandBody[0] ? BUSLOG_MODE_ASCII : BUSLOG_MODE_DEFAULT;
    break;
    case COMMAND_DASHLOG:
      dashLogCommand(commandBody, bytesRead);
    break;
    case COMMAND_BLUETOOTH:
      bluetoothCommand(commandBody, bytesRead);
    break;
    case COMMAND_SEND:
      if(activeSerial == &Serial){
        getAndSend(commandBody, bytesRead);
      }
    break;
    case COMMAND_HALT:
      busLogEnabled[0] = 0;
      busLogEnabled[1] = 0;
      busLogEnabled[2] = 0;
      dashLogEnabled = false;
    break;
    case COMMAND_RADIO:
      radioCommand(commandBody, bytesRead);
    break;
    case COMMAND_DEBUG:
      Serial.write("Stolen: ");
      Serial.write(badAuth ? "Yes\r\n" : "No\r\n");
      Serial.write("Auth: ");
      Serial.write((auth1 && auth2) ? "Yes\r\n" : "No\r\n");
      Serial.write("A1: ");
      Serial.write(digitalRead(A1) == HIGH ? "HIGH\r\n" : "LOW\r\n");
      Serial.write("A5: ");
      Serial.write(digitalRead(A5) == HIGH ? "HIGH\r\n" : "LOW\r\n");
      Serial.write("Engine: ");
      Serial.write(engineRunning ? "Running\r\n" : "Not running\r\n");
    break;
  }
  
  clearBuffer();
}

void SerialCommand::radioCommand(byte *commandBody, int buflen){
  switch(commandBody[0]){
    case COMMAND_RADIO_SET_TEXT:
      radio->SetDisplayText((const char*)(commandBody + 2), buflen - 2, commandBody[1]);
    break;
  }
}

void SerialCommand::printMessageToSerial( const Message &msg ){
    if(!busLogEnabled[msg.busId - 1]){ return; }

    if(dashLogEnabled){
      if(
          msg.frame_id != SENSOR_FUEL && /* fuel */
          msg.frame_id != SENSOR_SPEED && /* speed info */
          msg.frame_id != SENSOR_GPS_1 && /* gps */
          msg.frame_id != SENSOR_GPS_2 && /* gps */
          msg.frame_id != SENSOR_LIGHTING && /* lighting */
          msg.frame_id != SENSOR_BRAKES && /* brakes */
          msg.frame_id != SENSOR_TRANSMISSION    /* gears */
        ){ return; }
    }

    if(busLogMode == BUSLOG_MODE_DEFAULT){
      byte messageBuffer[19];
      memset((char*)messageBuffer, 0, 19);
  
      //Protocol header 0-2
      messageBuffer[0] = 0x03;
      messageBuffer[1] = msg.busId;
      messageBuffer[2] = msg.ide;
      
      //PID 3-6
      messageBuffer[3] = (msg.frame_id >> 24);
      messageBuffer[4] = (msg.frame_id >> 16);
      messageBuffer[5] = (msg.frame_id >> 8);
      messageBuffer[6] = (msg.frame_id);
      
      //Data 7-14
      messageBuffer[7] = msg.frame_data[0];
      messageBuffer[8] = msg.frame_data[1];
      messageBuffer[9] = msg.frame_data[2];
      messageBuffer[10] = msg.frame_data[3];
      messageBuffer[11] = msg.frame_data[4];
      messageBuffer[12] = msg.frame_data[5];
      messageBuffer[13] = msg.frame_data[6];
      messageBuffer[14] = msg.frame_data[7];
  
      //Protocol footer 15-18
      messageBuffer[15] = msg.length;
      messageBuffer[16] = msg.busStatus;
      messageBuffer[17] = 13;
      messageBuffer[18] = 10;
  
      Serial.write(messageBuffer, 19);
    }else{
      /*
      Serial.print("[");
      Serial.print(msg.frame_id, HEX);
      Serial.print("] BUS: ");
      Serial.print(msg.busId, HEX);
      Serial.print(" IDE: ");
      Serial.print(msg.ide, HEX);
      Serial.print(" { ");
      for(byte b = 0; b < 8; b++){
        if(msg.frame_data[b] < 0x10){ Serial.print('0'); }
        Serial.print(msg.frame_data[b], HEX);
        if(b < 6){
          Serial.print(" , ");
        }else{
          Serial.print("} ");
        }
      }
      Serial.print("[");
      Serial.print(msg.length, DEC);
      Serial.print("]\r\n");
      */
    }
}

void SerialCommand::busLogCommand(byte *commandBody, int buflen){
  if(buflen != 3){ return; }
  busLogEnabled[0] = commandBody[0];
  busLogEnabled[1] = commandBody[1];
  busLogEnabled[2] = commandBody[2];
}

void SerialCommand::dashLogCommand(byte *commandBody, int buflen){
  if(buflen != 1){ return; }

  if(commandBody[0]){
    dashLogEnabled = true;
    busLogEnabled[0] = 0;
    busLogEnabled[1] = 1;
    busLogEnabled[2] = 0;
  }else{
    dashLogEnabled = false;
    busLogEnabled[0] = 0;
    busLogEnabled[1] = 0;
    busLogEnabled[2] = 0;
  }
}

void SerialCommand::filterCommand(byte *cmd, int buflen){
  //[0 BUS] [1 IDE] [2-5 F1] [6-9 F2]

  unsigned long f1 = 0;
  unsigned long f2 = 0;

  if(buflen == 6){
    f1 = ((unsigned long)cmd[2] << 24) + ((unsigned long)cmd[3] << 16) + ((unsigned long)cmd[4] << 8) + ((unsigned long)cmd[5]);
    f2 = f1;
  }else if(buflen == 10){
    f1 = ((unsigned long)cmd[2] << 24) + ((unsigned long)cmd[3] << 16) + ((unsigned long)cmd[4] << 8) + ((unsigned long)cmd[5]);
    f2 = ((unsigned long)cmd[6] << 24) + ((unsigned long)cmd[7] << 16) + ((unsigned long)cmd[8] << 8) + ((unsigned long)cmd[9]);
  }else{
    return;
  }

  if(cmd[0] != 0x01 && cmd[0] != 0x02 && cmd[0] != 0x03){ return; }

  set_filter(cmd[0], cmd[1], f1, f2);
}

void SerialCommand::bluetoothCommand(byte *commandBody, int buflen){
  if(buflen != 1 && buflen != 2 && buflen != 3) { return; }

  byte btBuffer[8] = {0};
  byte b;
  
  switch(commandBody[0]){
    case COMMAND_BLUETOOTH_PASSTHRU:
      if(commandBody[1] & 1){
        digitalWrite(BOOT_LED, HIGH);
        passthroughMode = true;
      }
    break;
    case COMMAND_BLUETOOTH_DFU:
      if(activeSerial == &Serial){
        btBuffer[0] = 0xff;
        btBuffer[1] = 0xdd;
        Serial1.write(btBuffer, 8);
      }
    break;
    case COMMAND_BLUETOOTH_RESET:
      btBuffer[0] = 0xff;
      btBuffer[1] = 0xcc;
      Serial1.write(btBuffer, 8);
    break;
    case COMMAND_BLUETOOTH_TOGGLE:
      if(digitalRead(BT_SLEEP) == HIGH){
        digitalWrite(BT_SLEEP, LOW);
        delay(1);
        digitalWrite(BT_SLEEP, HIGH);
        delay(2);
        digitalWrite(BT_SLEEP, LOW);
        Serial.print("[BT] SLEEP = LOW\r\n");
      }else{
        digitalWrite(BT_SLEEP, HIGH);
        Serial.print("[BT] SLEEP = HIGH\r\n");
      }
    break;
  }
  
}

void SerialCommand::getAndSend(byte *cmd, int bytesRead){

  if(bytesRead != 15 || activeSerial == &Serial1){ return; }

  Message msg;    
  msg.busId = cmd[0];
  msg.ide = cmd[1];   
  msg.frame_id += ((unsigned long)cmd[2] << 24);
  msg.frame_id += ((unsigned long)cmd[3] << 16);
  msg.frame_id += ((unsigned long)cmd[4] << 8);
  msg.frame_id += ((unsigned long)cmd[5]);
  msg.frame_data[0] = cmd[6];
  msg.frame_data[1] = cmd[7];
  msg.frame_data[2] = cmd[8];
  msg.frame_data[3] = cmd[9];
  msg.frame_data[4] = cmd[10];
  msg.frame_data[5] = cmd[11];
  msg.frame_data[6] = cmd[12];
  msg.frame_data[7] = cmd[13];
  msg.length = cmd[14];

  msg.dispatch = true;
  mainQueue->push( msg );
}


int SerialCommand::getCommandBody( byte* cmd, int length ){
  unsigned int i = 0;

  // Loop until requested amount of bytes are sent. Needed for BT latency
  while( i < length && activeSerial->available() ){
    cmd[i] = activeSerial->read();
    i++;
  }

  return i;
}

void SerialCommand::clearBuffer(){
  while(activeSerial->available()){
    activeSerial->read();
  }
}


void SerialCommand::resetToBootloader()
{
  cli();
  UDCON = 1;
  USBCON = (1<<FRZCLK);  // disable USB
  UCSR1B = 0;
  delay(5);
  EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0; ADCSRA = 0;
  TIMSK0 = 0; TIMSK1 = 0; TIMSK3 = 0;
  asm volatile("jmp 0x7000");
}






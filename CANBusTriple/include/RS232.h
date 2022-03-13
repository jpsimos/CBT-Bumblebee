#ifndef __RS232__
#define __RS232__

/* Author Jacob Psimos 2016 */
/* RS232 handling for CanBusTriple - Jacob Psimos Fork for GMLAN */

/* INCOMMING COMMAND PREFIXES */
#define COMMAND_BUSLOG 0x01         // 0x01 [BUS1] [BUS2] [BUS3] 
#define COMMAND_INJECT 0x02         // 0x02 [busID] [idX] [len] [ID, ID, ID, ID] [payload]
#define COMMAND_FILTER 0x03         // 0x03 [1:BUS_ID] [2:idXtended] [3 4 5 6] [7 8 9 10]
/* SERIAL PORT STATUS BITS */ 
#define SP_LOG_BUS1 0x02
#define SP_LOG_BUS2 0x04
#define SP_LOG_BUS3 0x08
#define SP_BT_PASSTHRU 0x10

/* AVR->BLE112 */
#define COMMAND_BT_PASSTHRU 0xB0
#define COMMAND_BT_WRITE 0xB1

/* BLE112->AVR */
#define BLE112_CONNECTED 0xB1
#define BLE112_DISCONNECTED 0xB2

#define RS232_RECEIVE_SIZE 24   //Must not be greater an 255
#define RS232_FRAME_TERMINATE "\r\n\0"

class RS232{
  public:
    RS232();
    void tick();
    void processMessage(const Message &);
  private:
    Stream *serialPort;
    byte serialStatus;
	bool simulate;
    byte receiveData(byte *);
    void processUSBData(const byte *, const byte);
    void processBTData(const byte *, const byte);
    void executeLogCommand(const byte *, const byte);
    void executeFilterCommand(const byte *, const byte);
    void executeInjectCommand(const byte *, const byte);
};

RS232::RS232(){
  serialPort = NULL;
  serialStatus = 0;
}

byte RS232::receiveData(byte *buffer){
  if(serialPort == (Stream*)NULL) { return 0; }
  memset(buffer, 0, RS232_RECEIVE_SIZE);
  byte index = 0;
  byte next;
  while(serialPort->available()){
    next = serialPort->read();
    if(index < RS232_RECEIVE_SIZE){
      buffer[index] = next;
      index += 1;
    } 
  }
  return index;
}


void RS232::tick(){
  byte buffer[RS232_RECEIVE_SIZE];
  byte rxLength = 0;

  //passthru if check
  if( (serialStatus & SP_BT_PASSTHRU) ){

    //RX (Serial1): 000209030000 completed flash
    
    while(Serial.available()){  Serial1.write(Serial.read());  }
    while(Serial1.available()){ Serial.write(Serial1.read());  }
    
  }else{

    if(Serial.available()){
      serialPort = &Serial;
      rxLength = receiveData(buffer);
      processUSBData(buffer, rxLength);
    }
  
    if(Serial1.available()){
      delay(8);
      serialPort = &Serial1;
      rxLength = receiveData(buffer);
      processBTData(buffer, rxLength);
    }

  } //end passthru if check

  /*
  if(serialStatus & SP_LOG_BUS3){
	static unsigned long last = 0;
	static byte lasti = 0;
	if(!last){
		last = millis() + 100;
	}else if(last < millis()){
		Message msg;
		msg.frame_id = 0x10203040;
		msg.length = 8;
		msg.busId = 3;
		msg.ide = 0x08;
		msg.frame_data[0] = rand();
		msg.frame_data[2] = rand();
		msg.frame_data[4] = rand();
		msg.frame_data[7] = lasti++;

		processMessage(msg);
		last = 0;
	}
  }
  */
}

/* Array bounds checked in tick() */
void RS232::processUSBData(const byte *buffer, const byte length){
  if(buffer == (const byte*)NULL) { return; }

  byte temp = 0;

  switch(buffer[0]){
    case COMMAND_BUSLOG:
      executeLogCommand(buffer, length);
    break;
    case COMMAND_INJECT:
      executeInjectCommand(buffer, length);
    break;
    case COMMAND_FILTER:
      executeFilterCommand(buffer, length);
    break;
    case COMMAND_BT_PASSTHRU:
      serialStatus |= SP_BT_PASSTHRU;
      digitalWrite(BOOT_LED, HIGH);
    break;
    case COMMAND_BT_WRITE:
		if(length > 1){
			Serial1.write(buffer + 1, length - 1);
		}
    break;
  }
}

void RS232::processBTData(const byte *buffer, const byte length){
  if(buffer == (const byte*)NULL) { return; }
  switch(buffer[0]){
    case COMMAND_BT_PASSTHRU:
      //received from BLE112 after it has been commanded to enter DFU mode
      serialStatus |= SP_BT_PASSTHRU;
      digitalWrite(BOOT_LED, HIGH);
    break;
    case BLE112_CONNECTED:
      status |= STATUS_BT_CONNECTED;
    break;
    case BLE112_DISCONNECTED:
      status &= ~STATUS_BT_CONNECTED;
    break;
  }
}

void RS232::executeInjectCommand(const byte *buffer, const byte length){
  if(buffer == (const byte*)NULL || length < 8) { return; }

  Message msg;
  msg.busId = buffer[1];
  msg.ide = buffer[2];
  msg.length = buffer[3];
  msg.frame_id = (((unsigned long)buffer[4]) << 24) + (((unsigned long)buffer[5]) << 16) + (((unsigned long)buffer[6]) << 8) + (unsigned long)buffer[7];
  memcpy(msg.frame_data, buffer + 8, 8);
  writeQueue.push(msg);
}

void RS232::executeLogCommand(const byte *buffer, const byte length){
  if(buffer == (const byte*)NULL) { return; }
  if(length == 4){
    serialStatus = (buffer[1] ? (serialStatus | SP_LOG_BUS1) : (serialStatus & ~(SP_LOG_BUS1)));
    serialStatus = (buffer[2] ? (serialStatus | SP_LOG_BUS2) : (serialStatus & ~(SP_LOG_BUS2)));
    serialStatus = (buffer[3] ? (serialStatus | SP_LOG_BUS3) : (serialStatus & ~(SP_LOG_BUS3)));
	if(serialStatus & SP_LOG_BUS1){
		CANBus1.setMode(CONFIGURATION);
		delay(1);
		CANBus1.setFilter(false, 0, 0);
		CANBus1.setMode(NORMAL);
	}
  }
}

void RS232::executeFilterCommand(const byte *buffer, const byte length){
  if(buffer == (const byte*)NULL) { return; }
  
  // 0x03 [1:BUS_ID] [2:idXtended] [3 4 5 6] [7 8 9 10]
  unsigned long filter1 = 0;
  unsigned long filter2 = 0;

  filter1 = ((unsigned long)buffer[3]) << 24;
  filter1 += ((unsigned long)buffer[4]) << 16;
  filter1 += ((unsigned long)buffer[5]) << 8;
  filter1 += (unsigned long)buffer[6];

  filter2 = ((unsigned long)buffer[7]) << 24;
  filter2 += ((unsigned long)buffer[8]) << 16;
  filter2 += ((unsigned long)buffer[9]) << 8;
  filter2 += ((unsigned long)buffer[10]);

  switch(buffer[1]){
    case 1:
		CANBus1.setMode(CONFIGURATION);
		delay(1);
		CANBus1.setFilter(buffer[2], filter1, filter2);
		CANBus1.setMode(NORMAL);
    break;
    case 2:
		CANBus2.setMode(CONFIGURATION);
		delay(1);
		CANBus2.setFilter(buffer[2], filter1, filter2);
		CANBus2.setMode(NORMAL);
    break;
    case 3:
		CANBus3.setMode(CONFIGURATION);
		delay(1);
		CANBus3.setFilter(buffer[2], filter1, filter2);
		CANBus1.setMode(NORMAL);
    break;
  }
}

void RS232::processMessage(const Message &msg){

  if( msg.busId > 3 ) { return; }
  if( !(serialStatus & (1 << msg.busId) )) { return; }

  //Protocol header 0-2
  Serial.write(0x02); //ASCII start_of_text
  Serial.write(msg.busId);
  Serial.write(msg.ide);

  //PID 3-6
  Serial.write( (msg.frame_id >> 24) );
  Serial.write( (msg.frame_id >> 16) );
  Serial.write( (msg.frame_id >> 8) );
  Serial.write( (msg.frame_id) );

  //Data 7-14
  Serial.write(msg.frame_data[0]);
  Serial.write(msg.frame_data[1]);
  Serial.write(msg.frame_data[2]);
  Serial.write(msg.frame_data[3]);
  Serial.write(msg.frame_data[4]);
  Serial.write(msg.frame_data[5]);
  Serial.write(msg.frame_data[6]);
  Serial.write(msg.frame_data[7]);

  //Protocol footer 15-19
  Serial.write(msg.length);
  Serial.write(msg.busStatus);
  Serial.write(RS232_FRAME_TERMINATE, 3);
}

/*
void RS232::bootloader(){
    cli();
    UDCON = 1;
    USBCON = (1<<FRZCLK);  // disable USB
    UCSR1B = 0;
    delay(5);
    EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0; ADCSRA = 0;
    TIMSK0 = 0; TIMSK1 = 0; TIMSK3 = 0;
    asm volatile("jmp 0x7000");
}
*/


#endif

/*
 * Canbus Triple - v0.45(j)enny - Global.h
 * Shared set of instruction for compiler
 */
 /* All rights reserved */

#ifndef __GLOBALZ__
#define __GLOBALZ__

//#define INC_DEBUG
#define SECURITY

#define NORTH 1
#define NORTH_EAST 2
#define EAST 3
#define SOUTH_EAST 4
#define SOUTH 5
#define SOUTH_WEST 6
#define WEST 7
#define NORTH_WEST 8
#define MILLIARC_SECONDS 3600000.0
#define MILLIARC_OFFSET_NORTH_AMERICA 596.52325

/*
enum{
  BIT_1 = 0x01, BIT_2 = 0x02, BIT_3 = 0x04, BIT_4 = 0x08,
  BIT_5 = 0x10, BIT_6 = 0x20, BIT_7 = 0x40, BIT_8 = 0x80
};
*/

/* Global VARS */
CANBus CANBus1(CAN1SELECT, CAN1RESET, 1, "GM-HS CAN");
CANBus CANBus2(CAN2SELECT, CAN2RESET, 2, "GM-SW CAN");
CANBus CANBus3(CAN3SELECT, CAN3RESET, 3, "LOOPBACK");
CANBus busses[] = { CANBus1, CANBus2, CANBus3 };
QueueArray<Message> readQueue;
QueueArray<Message> writeQueue;

boolean auth1 = false;
boolean auth2 = false;
boolean badAuth = false;
boolean writeDisabled = false;    //safety is number one concern.
unsigned long now = 0;
boolean leftTurnSignal = false;
boolean rightTurnSignal = false;
boolean engineRunning = false;
unsigned int currentRPM = 0;
unsigned int currentSpeed = 0;    //In MPH
unsigned int horsepower = 0;            //relative to rpm
unsigned int demandedHorsepower = 0;    //relative to throttle position
byte currentGear = 0;
boolean manualGear = false;
boolean brakes = false;
boolean driverSeatbelt = false;     //buckle up :)
byte throttle = 0;                //As hex (0-ff)
float throttleF = 0.0;            //As float-percent
unsigned int MAF = 0;              //As grams / second
float intakeAirTemp = 0.0;           //degrees f
float MPG = 0.0;                  //miles per gallon
byte averageMPG[48];              //average miles per gallon
byte fuel = 0;                    //In hex (0-255)
float fuelF = 0.0;                //In gallons
unsigned int heading = 0;         //In degrees
unsigned int elevation = 0;       //In feet (received as CM)
byte fanSpeed = 0;                //0-6 fan speed
//float latitude = 0.0;             //In degrees
//float longitude = 0.0;            //In degrees


/* Global METHODS */
void handle_doublePress();
void handle_A1Press();
void handle_A5Press();
void initializeBoard();
void readBus(CANBus& ,QueueArray<Message>&);
boolean sendMessage(Message&, CANBus&);
void processFirst(Message &);
void update_gps(const Message &);
void tpms();
void set_filter(const byte, const byte, unsigned long, unsigned long);
int freeRam();

const byte vin_1[8] = { 0x47, 0x43, 0x45, 0x4B, 0x31, 0x34, 0x4A, 0x58 };
const byte vin_2[8] = { 0x37, 0x5A, 0x35, 0x38, 0x37, 0x31, 0x33, 0x30 };

#ifdef INC_DEBUG
void debugPrintMessage(const Message &, boolean);
#endif

#include "Buttons.h"
Buttons *ptrButtons = NULL;

#include "Radio.h"
Radio *ptrRadio = NULL;

#include "TCM.h"
TCM *ptrTCM = NULL;

void initializeBoard(){

  delay(3);
  
  Serial.begin( 115200 ); // USB
  Serial1.begin( 57600, SERIAL_8N2   ); // UART
  
  //Power LED & USB
  DDRE |= B00000100;
  PORTE |= B00000100;
  
  //BLE112 Initilization [Start alive]
  pinMode( BT_SLEEP, OUTPUT );
  digitalWrite( BT_SLEEP, LOW );
  
  pinMode( BOOT_LED, OUTPUT );
  
  // MCP Pins
  pinMode( CAN1INT_D, INPUT );
  pinMode( CAN2INT_D, INPUT );
  pinMode( CAN3INT_D, INPUT );
  pinMode( CAN1RESET, OUTPUT );
  pinMode( CAN2RESET, OUTPUT );
  pinMode( CAN3RESET, OUTPUT );
  pinMode( CAN1SELECT, OUTPUT );
  pinMode( CAN2SELECT, OUTPUT );
  pinMode( CAN3SELECT, OUTPUT );
  digitalWrite(CAN1RESET, LOW);
  digitalWrite(CAN2RESET, LOW);
  digitalWrite(CAN3RESET, LOW);
  
  
  // Setup CAN Busses
  CANBus1.begin();
  CANBus1.setClkPre(1);
  CANBus1.baudConfig(500);
  CANBus1.setRxInt(true);
  CANBus1.bitModify(RXB0CTRL, 0x04, 0x04); // Set buffer rollover enabled
  CANBus1.bitModify(CNF2, 0x20, 0x20); // Enable wake-up filter
  CANBus1.clearFilters();
  CANBus1.setMode(NORMAL);
  
  CANBus2.begin();
  CANBus2.setClkPre(1);
  CANBus2.baudConfig(33);
  CANBus2.setRxInt(true);
  CANBus2.bitModify(RXB0CTRL, 0x04, 0x04);
  CANBus2.bitModify(CNF2, 0x20, 0x20); // Enable wake-up filter
  CANBus2.clearFilters();
  CANBus2.setMode(NORMAL);

  now = millis();

}

void processFirst(Message &msg){

  //thau shall not follow the NULL pointer
  if(ptrRadio == NULL || ptrButtons == NULL || ptrTCM == NULL) { return; }

  byte b;
  unsigned int temp = 0;
  
  switch(msg.frame_id){
    case SENSOR_LIGHTING:
      leftTurnSignal = (msg.frame_data[1] & 0x20);
      rightTurnSignal = (msg.frame_data[1] & 0x08);
    break;

    case SENSOR_SPEED:
      currentRPM = ((((unsigned int)msg.frame_data[4]) << 8) + ((unsigned int)msg.frame_data[5])) / 4;
      engineRunning = (msg.frame_data[4] > 0 || msg.frame_data[5] > 0);

      //Check for speed change
      temp = ((((unsigned int)msg.frame_data[2]) << 8) + ((unsigned int)msg.frame_data[3])) / 100;
      if(temp != currentSpeed){
        currentSpeed = temp;
        if(ptrRadio->mode == RADIO_DISP_MODE_SPEED){
          ptrRadio->update = true;
        }
      }

      //Check for throttle change
      if(throttle != msg.frame_data[7]){
        throttle = msg.frame_data[7];
        //throttleP = (((unsigned int)throttle) * 100) / (unsigned int)0xFF;
        throttleF = ((float)msg.frame_data[7] / 256.0) * 100.0;
        if(ptrRadio->mode == RADIO_DISP_MODE_THROTTLE){
          ptrRadio->update = true;
        }
      }

      //Mode specific updates
      if(ptrRadio->mode == RADIO_DISP_MODE_HORSEPOWER){
        temp = (unsigned int)((unsigned long)((unsigned long)338 * (unsigned long)currentRPM) / (unsigned long)5252);
        demandedHorsepower = (unsigned int)((throttleF / 100.0) * 350.0);
        if(horsepower != temp){
          horsepower = temp;
          ptrRadio->update = true;
        }
      }
    break;

    case SENSOR_FUEL:
      if(msg.frame_data[1] != fuel){
        fuel = msg.frame_data[1];
        fuelF = ((float)((float)msg.frame_data[1] / 255.0)) * 26.0;
        if(ptrRadio->mode == RADIO_DISP_MODE_FUEL_USED || ptrRadio->mode == RADIO_DISP_MODE_FUEL_REMAIN){
          ptrRadio->update = true;
        }
      }
    break;

    case SENSOR_TRANSMISSION:
      if(currentGear != msg.frame_data[0]){
        currentGear = msg.frame_data[0];
        ptrRadio->update = true;
        tpms();
      }
    break;

    case SENSOR_BRAKES:
      b = (msg.frame_data[1] & 0x80);
      if( (b == 0x80) != brakes ){
        brakes = b == 0x80;
        if(ptrRadio->mode == RADIO_DISP_MODE_THROTTLE){
          ptrRadio->update = true;
        }
      }
    break;

    case SENSOR_FANSPEED:
      if(msg.frame_data[3] != fanSpeed){
        fanSpeed = msg.frame_data[3];
        b = snprintf(ptrRadio->tempBuffer, 18, "FAN SPEED: %u", fanSpeed);
        ptrRadio->SetTempDisplayText(ptrRadio->tempBuffer, b, RADIO_DISP_XM_CAT, 1000);
      }
    break;

    case SENSOR_SEATBELT:
      driverSeatbelt = (msg.frame_data[0] & 0x40) == 0x40;
    break;

    case SENSOR_RADIO_AUDIO_STATUS:
      if(msg.frame_data[0] == 0x0C && msg.frame_data[1] == 0x25){
        ptrRadio->enabled = true;
      }else{
        ptrRadio->enabled = false;
      }
    break;

    case SENSOR_GPS_2:
      update_gps(msg);
    break;

    case SENSOR_RADIO_TEXT_REQ:
      if(msg.frame_data[2] == RADIO_DISP_XM_CAT){
        ptrRadio->HandleSatUpdate();
      }
    break;

    case SENSOR_CBT:
      if(msg.frame_data[0] == 1){
        writeDisabled = true;
      }else{
        writeDisabled = false;
      }
    break;
    
  case SENSOR_DIMMER:
    //dimmer = msg.frame_data[1];
    asm("nop");
  break;
    
  #ifdef SECURITY
    case SENSOR_VIN_1:
      if(!auth1 && !badAuth){
        for(b = 0; b < 8; b++){
          if(msg.frame_data[b] != vin_1[b]){
            auth1 = false;
            badAuth = true;
            EEPROM.write(0, 0xFF);
            break;
          }
        }
        auth1 = true;
      }
    break;
    
    case SENSOR_VIN_2:
      if(!auth2 && !badAuth){
        for(b = 0; b < 8; b++){
          if(msg.frame_data[b] != vin_2[b]){
            auth2 = false;
            badAuth = true;
            EEPROM.write(0, 0xFF);
            break;
          }
        }
        auth2 = true;
      }
    break;
  #endif
  
  }
}

void update_gps(const Message &msg){
    /*
    long ilat = ((long)msg.frame_data[0] << 24) + ((long)msg.frame_data[1] << 16) + ((long)msg.frame_data[2] << 8) + (long)msg.frame_data[3];
    long ilog = ((long)msg.frame_data[4] << 24) + ((long)msg.frame_data[5] << 16) + ((long)msg.frame_data[6] << 8) + (long)msg.frame_data[7];
    latitude = (float)ilat / MILLIARC_SECONDS;
    longitude = ((float)ilog / MILLIARC_SECONDS) - MILLIARC_OFFSET_NORTH_AMERICA;
    */

    if(ptrRadio->mode == RADIO_DISP_MODE_GPS_1){
      unsigned int tempHeading = ((unsigned int)msg.frame_data[0] << 8) + (unsigned int)msg.frame_data[1];
      unsigned long tempElevation = ((unsigned long)msg.frame_data[5] << 16) + ((unsigned long)msg.frame_data[6] << 8) + ((unsigned long)msg.frame_data[7]);
      tempElevation = (tempElevation - (unsigned long)100000) / (unsigned long)30;
      
      if(tempHeading != heading || (unsigned int)tempElevation != elevation){
        heading = tempHeading;
        elevation = (unsigned int)tempElevation;
        ptrRadio->update = true;
      }
    }
}

void handle_doublePress(){
  switch(ptrRadio->mode){
    case RADIO_DISP_MODE_GEARS:
      ptrTCM->tcm_handle_doublePress();
    break;
    default:
      ptrRadio->update = true;
      tpms();
    break;
  }
}

void handle_A1Press(){
  switch(ptrRadio->mode){
    case RADIO_DISP_MODE_GEARS:
      if(ptrTCM->enabled){
        ptrTCM->tcm_handle_a1Press();
      }else{
        ptrRadio->SwitchMode(false);
      }
    break;
    default:
      ptrRadio->SwitchMode(false);
    break;
  }
}

void handle_A5Press(){
  switch(ptrRadio->mode){
    case RADIO_DISP_MODE_GEARS:
      if(ptrTCM->enabled){
        ptrTCM->tcm_handle_a5Press();
      }else{
        ptrRadio->SwitchMode(true);
      }
    break;
    default:
      ptrRadio->SwitchMode(true);
    break;
  }
}


void tpms(){
    Message msg;
    msg.ide = 0x08;
    msg.busId = 2;
    msg.dispatch = true;
  
    msg.frame_id = SENSOR_TPMS_DSP;
    msg.frame_data[0] = 0x7C;
    msg.length = 2;

    writeQueue.push(msg);
}

void readBus( CANBus &bus, QueueArray<Message> &_readQueue ){

  // Abort if readQueue is full
  if( _readQueue.isFull() ) return;

  byte rx_status = bus.readStatus();
  //byte rx_id = ((rx_status & 0x1) == 0x1) ? READ_RX_BUF_0_ID : (((rx_status & 0x2) == 0x2) ? READ_RX_BUF_1_ID : 0xFF);
  byte rx_id;
  if( (rx_status & 0x1) == 0x1 ){
    rx_id = READ_RX_BUF_0_ID;
  }else if( (rx_status & 0x2) == 0x2 ){
    rx_id = READ_RX_BUF_1_ID;
  }else{
    return;
  }

  
  Message msg;
  msg.busStatus = rx_status;
  msg.busId = bus.busId;
  bus.readDATA_ff_X(rx_id, &msg.length, msg.frame_data, &msg.frame_id, &msg.ide );
  _readQueue.push(msg);

}

 boolean sendMessage( Message &msg, CANBus &bus ){

  if( !msg.dispatch ) return true;

  byte sendBuffer = 0xFF;
  byte ch = bus.getNextTxBuffer(&sendBuffer);
  if(ch == 0xFF || sendBuffer == 0xFF){ return false; }

  bus.load_ff_X(ch, msg.length, msg.frame_id, msg.frame_data, msg.ide);
  bus.send_X(sendBuffer);
  
  return true;
}

void set_filter(const byte busId, const byte ide, unsigned long f1, unsigned long f2){
  if(busId > 3 || busId < 1) { return; }

  busses[busId - 1].setMode(CONFIGURATION);
  busses[busId - 1].clearFilters();
  busses[busId - 1].bitModify(CANINTF, 0, 0x03);
  if(f1 != 0){
    busses[busId - 1].setFilter(ide, f1, f2);
  }
  busses[busId - 1].setMode(NORMAL);
}

#ifdef INC_DEBUG

void debugPrintMessage(const Message &msg, boolean newline){
  Serial.print(msg.frame_id, HEX);
  Serial.print("  ");
  for(byte b = 0; b < 8; b++){
    Serial.print(msg.frame_data[b], HEX);
    Serial.print(' ');
  }

  if(newline){
    Serial.print("\r\n");
  }
}

#endif

int freeRam (){
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

  /*
  if((heading >= 340 && heading <= 360) || (heading >= 0 && heading <= 20)){
    compass = NORTH;
  }else if(heading >= 21 && heading <= 70){
    compass = NORTH_EAST;
  }else if(heading >= 71 && heading <= 110){
    compass = EAST;
  }else if(heading >= 111 && heading <= 160){
    compass = SOUTH_EAST;
  }else if(heading >= 161 && heading <= 200){
    compass = SOUTH;
  }else if(heading >= 201 && heading <= 250){
    compass = SOUTH_WEST;
  }else if(heading >= 251 && heading <= 290){
    compass = WEST;
  }else if(heading >= 291 && heading <= 339){
    compass = NORTH_WEST;
  }
  */

#endif


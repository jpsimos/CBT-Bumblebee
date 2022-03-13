
#include "Message.h"

Message::Message(){
	busId = 0;
	ide = 0;
	busStatus = 0;
	length = 0;
	frame_id = 0;
	
	frame_data[0] = 0;
	frame_data[1] = 0;
	frame_data[2] = 0;
	frame_data[3] = 0;
	frame_data[4] = 0;
	frame_data[5] = 0;
	frame_data[6] = 0;
	frame_data[7] = 0;
	
    dispatch = false;
	delay = 0;
}

Message::Message(const Message &src){
	this->busId = src.busId;
	this->ide = src.ide;
	this->busStatus = src.busStatus;
	this->length = src.length;
	this->frame_id = src.frame_id;
	
	for(byte i = 0; i < 8; i++){
		this->frame_data[i] = src.frame_data[i];
	}
	
	this->dispatch = src.dispatch;
	this->delay = src.delay;
}

Message::~Message(){	}


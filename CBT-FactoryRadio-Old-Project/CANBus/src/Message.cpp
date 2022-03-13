
#include "Message.h"

Message::Message(){
	busId = 0;
	ide = 0;
	busStatus = 0;
	length = 0;
	frame_id = 0;
    dispatch = false;
	memset(this->frame_data, '\0', 8);
}

Message::Message(const Message &src){
	this->busId = src.busId;
	this->ide = src.ide;
	this->busStatus = src.busStatus;
	this->length = src.length;
	this->frame_id = src.frame_id;
	this->dispatch = src.dispatch;
	memcpy(this->frame_data, src.frame_data, 8);
}

Message::~Message(){	}


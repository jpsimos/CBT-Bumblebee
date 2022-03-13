#ifndef CANMessage_H
#define CANMessage_H

#include <Arduino.h>
#include <CANBus.h>

//6/20/16 - added copy constructor - jpsimos
//10/1/16 - busStatus need not be an integer, changed to byte


class Message {
    public:
        Message();
		Message(const Message&);
        ~Message();

		unsigned long frame_id;
		byte busId;
		byte ide;
		byte busStatus;
        byte length;
        byte frame_data[8];
        bool dispatch;
};

#endif
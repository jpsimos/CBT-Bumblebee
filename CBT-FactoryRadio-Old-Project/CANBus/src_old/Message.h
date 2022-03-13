#ifndef CANMessage_H
#define CANMessage_H

#include <Arduino.h>
#include <CANBus.h>

//6/20/16 - added copy constructor - jpsimos


class Message {
    public:
        Message();
		Message(const Message&);
        ~Message();

		unsigned int busId;
		byte ide;
		unsigned int busStatus;
        byte length;
        unsigned long frame_id;
        byte frame_data[8];
        bool dispatch;
		unsigned int delay;
};

#endif
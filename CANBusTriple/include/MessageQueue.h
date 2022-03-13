#ifndef __MSG__QUEUE
#define __MSG__QUEUE

/*
*  MessageQueue for CanBusTriple
*  Author Jacob Psimos <jpsimos@gmail.com>
*
*  Version 2
*/

#include "Message.h"

class MessageQueue{
	public:
	MessageQueue(Message *, const byte);
	
	const bool isFull();
	const bool isEmpty();
	const bool push(const Message &);
	const bool accomodate(const byte);
	Message pop();
	Message peek();
	void peekClear();
	void clear();
	
	private:
	byte size;
	byte items;
	byte head;
	byte tail;
	Message *contents;
};

MessageQueue::MessageQueue(Message *contents, const byte maximum){
	size = maximum;
	items = 0;
	head = 0;
	tail = 0;
	
	this->contents = contents;
}

const bool MessageQueue::accomodate(const byte n){
	return (items + n) <= size;
}

void MessageQueue::clear(){
	items = 0;
	head = 0;
	tail = 0;
}

const bool MessageQueue::push(const Message &msg){
	if(items < size && contents != (Message*)NULL){
		contents[tail++] = Message(msg);
		if(tail == size){
			tail = 0;
		}
		items++;
		return true;
	}
	return false;
}

Message MessageQueue::pop(){
	Message result;
	
	if(items == 0 || contents == (Message*)NULL){
		return result;
	}
	
	result = contents[head++];
	
	items--;
	if(head == size){
		head = 0;
	}
	
	return result;
}

Message MessageQueue::peek(){
	Message result;

	if(items == 0 || contents == (Message*)NULL){
		return result;
	}

	result = contents[head];
	return result;
}

void MessageQueue::peekClear(){
	if(items > 0){
		head++;
		items--;
		if(head == size){
			head = 0;
		}
	}
}

const bool MessageQueue::isEmpty(){
	return (items == 0);
}

const bool MessageQueue::isFull(){
	return (items == size);
}

#endif
#pragma once
#include "DBevent.h"

#define FIND_MAP_SIZE 16

struct AstarNode
{
	byte index;
	byte from;
	byte value;
	byte pathlen;
};

class PriorityQueue
{
	AstarNode heap[FIND_MAP_SIZE*FIND_MAP_SIZE];
	int count;
public:
	PriorityQueue();
	~PriorityQueue();

	void Enq(byte v, byte l, byte i, byte f_i);
	AstarNode Deq();
	bool IsEmpty() const;
};
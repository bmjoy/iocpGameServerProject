#include "object.h"

EvnetQueue::EvnetQueue()
{
	head.next = nullptr;
	free_list.next = nullptr;
}

EvnetQueue::~EvnetQueue()
{
	EVENT_NODE *prev = &head;
	while (prev->next != nullptr)
	{
		EVENT_NODE *del = prev->next;
		prev->next = del->next;

		delete del;
	}

	prev = &free_list;
	while (prev->next != nullptr)
	{
		EVENT_NODE *del = prev->next;
		prev->next = del->next;
		delete del;
	}
}

void EvnetQueue::enq(unsigned short id, char command, unsigned int milli, unsigned short target)
{
	EVENT_NODE *new_event;
	access_fl.lock();
	if (free_list.next == nullptr) {
		access_fl.unlock();
		new_event = new EVENT_NODE;
	}
	else {
		new_event = free_list.next;
		free_list.next = new_event->next;
		access_fl.unlock();
	}
	new_event->id = id;
	new_event->command = command;
	new_event->when = system_clock::now() + milliseconds(milli);
	new_event->target = target;

	EVENT_NODE *prev = &head;
	access_q.lock();
	while (prev->next != nullptr)
	{
		if (new_event->when < prev->next->when)
			break;
		prev = prev->next;
	}
	new_event->next = prev->next;
	prev->next = new_event;
	access_q.unlock();
}

EVENT_NODE * EvnetQueue::peek()
{
	access_q.lock();
	EVENT_NODE *ret = head.next;
	access_q.unlock();
	return ret;;
}

EVENT_NODE * EvnetQueue::deq()
{
	access_q.lock();
	EVENT_NODE *ret = head.next;
	head.next = ret->next;
	access_q.unlock();
	return ret;
}

void EvnetQueue::ReturnMemory(EVENT_NODE *m)
{
	access_fl.lock();
	m->next = free_list.next;
	free_list.next = m;
	access_fl.unlock();
}

void EvnetQueue::DeleteEvents(unsigned short id)
{
	access_q.lock();
	EVENT_NODE *prev = &head;
	while (prev->next != nullptr)
	{
		EVENT_NODE *del = prev->next;
 		if (del->id == id) {
			prev->next = del->next;
			cout << id << "'s " << int(del->command) << "event delete" << endl;
			ReturnMemory(del);
			//delete del;
		}
		prev = prev->next;
		if (prev == nullptr) break;
	}
	access_q.unlock();
}

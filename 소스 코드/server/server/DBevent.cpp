#include "DBevent.h"

DBEvnetQueue::DBEvnetQueue()
{
	head.next = nullptr;
	free_list.next = nullptr;
}

DBEvnetQueue::~DBEvnetQueue()
{
	DB_EVENT_NODE *prev = &head;
	while (prev->next != nullptr)
	{
		DB_EVENT_NODE *del = prev->next;
		prev->next = del->next;

		delete del;
	}

	prev = &free_list;
	while (prev->next != nullptr)
	{
		DB_EVENT_NODE *del = prev->next;
		prev->next = del->next;
		delete del;
	}
}

void DBEvnetQueue::EnqSelectByNameCMD(char command, unsigned int milli, unsigned short id, WCHAR * name, SOCKET sock)
{
	DB_EVENT_NODE *new_event;
	access_fl.lock();
	if (free_list.next == nullptr) {
		access_fl.unlock();
		new_event = new DB_EVENT_NODE;
	}
	else {
		new_event = free_list.next;
		free_list.next = new_event->next;
		access_fl.unlock();
	}
	memcpy(new_event->t_name, name, (NAME_LEN - 1) * sizeof(WCHAR));
	new_event->t_id = id;
	new_event->t_sock = sock;
	new_event->db_command = command;
	new_event->when = system_clock::now() + milliseconds(milli);

	DB_EVENT_NODE *prev = &head;
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

void DBEvnetQueue::EnqUpdatePlayerStatCmd(char command, unsigned int milli, WCHAR * name
	, unsigned short x, unsigned short y, unsigned short hp, byte level, unsigned long exp
	, unsigned short nFish, unsigned short nStone, unsigned short nPotion, unsigned short nArmor)
{
	DB_EVENT_NODE *new_event;
	access_fl.lock();
	if (free_list.next == nullptr) {
		access_fl.unlock();
		new_event = new DB_EVENT_NODE;
	}
	else {
		new_event = free_list.next;
		free_list.next = new_event->next;
		access_fl.unlock();
	}
	memcpy(new_event->t_name, name, (NAME_LEN - 1) * sizeof(WCHAR));
	new_event->t_exp = exp;
	new_event->t_hp = hp;
	new_event->t_level = level;
	new_event->t_x = x;
	new_event->t_y = y;
	new_event->t_fish = nFish;
	new_event->t_stone = nStone;
	new_event->t_potion = nPotion;
	new_event->t_armor = nArmor;
	new_event->t_sock = INVALID_SOCKET;
	new_event->db_command = command;
	new_event->when = system_clock::now() + milliseconds(milli);

	DB_EVENT_NODE *prev = &head;
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

DB_EVENT_NODE * DBEvnetQueue::peek()
{
	access_q.lock();
	DB_EVENT_NODE *ret = head.next;
	access_q.unlock();
	return ret;;
}

DB_EVENT_NODE * DBEvnetQueue::deq()
{
	access_q.lock();
	DB_EVENT_NODE *ret = head.next;
	head.next = ret->next;
	access_q.unlock();
	return ret;
}

void DBEvnetQueue::ReturnMemory(DB_EVENT_NODE *m)
{
	access_fl.lock();
	m->next = free_list.next;
	free_list.next = m;
	access_fl.unlock();
}


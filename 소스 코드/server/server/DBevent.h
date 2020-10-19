#pragma once
#include "object.h"

struct DB_EVENT_NODE {
	system_clock::time_point when;
	char db_command;

	// DB query에 사용할 data
	WCHAR t_name[NAME_LEN];
	unsigned short t_id, t_hp, t_x, t_y, t_fish, t_stone, t_potion, t_armor;
	byte t_level;
	unsigned long t_exp;

	SOCKET t_sock{ INVALID_SOCKET };

	DB_EVENT_NODE *next;
};


// 시간에 따른 우선순위 큐
class DBEvnetQueue
{
	DB_EVENT_NODE head;
	mutex access_q;

	DB_EVENT_NODE free_list;
	mutex access_fl;

public:
	DBEvnetQueue();

	~DBEvnetQueue();

	void EnqSelectByNameCMD(char command, unsigned int milli, unsigned short id
							, WCHAR *name, SOCKET sock);
	void EnqUpdatePlayerStatCmd(char command, unsigned int milli, WCHAR *name
		, unsigned short x, unsigned short y, unsigned short hp , byte level, unsigned long exp
		, unsigned short nFish, unsigned short nStone, unsigned short nPotion, unsigned short nArmor);


	DB_EVENT_NODE *peek();

	DB_EVENT_NODE *deq();

	void ReturnMemory(DB_EVENT_NODE *);
};
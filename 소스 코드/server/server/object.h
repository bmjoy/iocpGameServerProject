#pragma once
#include "network.h"

#define MONSTER_EXP 100
#define MAX_EXP 1000
#define MOVE_TERM 1000
#define STAT_SAVE_TERM	10000
#define STD_SKILL_COOl 1000
#define CROSS_SKILL_COOL 5000
#define ULTIMATE_SKILL_COOL	30000
#define STD_SKILL_DAMAGE 10
#define CROSS_SKILL_DAMAGE 30
#define ULTIMATE_SKILL_DAMAGE 80
#define MONSTER_L_SKILL_DAMAGE 20
#define MONSTER_S_SKILL_DAMAGE 15
#define REVIVE_TERM 10000
#define MAX_MINING_CNT	20

struct PLAYER {
	unsigned short id;
	unsigned short x, y;
	unsigned short hp;
	byte level;
	unsigned long exp;
	unordered_set<unsigned short> view_list;
	byte kind;
	WCHAR name[NAME_LEN];

	bool std_skill_ready;
	bool cross_skill_ready;
	bool ultimate_skill_ready;

	unsigned short prev_mining_x, prev_mining_y;
	unsigned short mining_cnt;

	unsigned short nFish;
	unsigned short nStone;
	unsigned short nPotion;
	unsigned short nArmor;

	unsigned short durability;

	SOCKET sock;
};

struct NPC {
	unsigned short id;
	unsigned short x, y;
	unsigned short hp;
	byte dir;
	byte kind;
	char state;
	bool set_x;
	unsigned short target;
	byte add_v;
	byte cur_add_v;

	bool is_active;
	lua_State *L{ nullptr };
};

struct EVENT_NODE {
	unsigned short id;
	system_clock::time_point when;
	char command;
	unsigned short target;

	EVENT_NODE *next;
};


// 시간에 따른 우선순위 큐
class EvnetQueue 
{
	EVENT_NODE head;
	mutex access_q;

	EVENT_NODE free_list;
	mutex access_fl;
public:
	EvnetQueue();

	~EvnetQueue();

	void enq(unsigned short id, char command, unsigned int milli, unsigned short target=NULL);

	EVENT_NODE *peek();

	EVENT_NODE *deq();

	void ReturnMemory(EVENT_NODE *);

	void DeleteEvents(unsigned short id);
};
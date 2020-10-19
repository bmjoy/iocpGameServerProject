#pragma once
#include "client.h"

#define NPC_CUBE_RADIUS 38
#define DEFAULT_HP 100

class Character
{
	WCHAR chat[MAX_CHAT_SIZE];
	std::chrono::system_clock::time_point valid_chat_time;

public:
	byte kind;
	unsigned short id;
	unsigned short x, y;
	bool chatting;
	unsigned short hp;


	Character();
	Character(unsigned short id, unsigned short x, unsigned short y, byte kind);
	~Character();

	void SetPos(unsigned short nx, unsigned short ny);

	void Draw(HDC, unsigned short cx, unsigned short cy);
	void SetChat(wchar_t *msg);
	void HPChange(unsigned short hp);
};

struct EFFECT
{
	char type;
	unsigned short x;
	unsigned short y;
};
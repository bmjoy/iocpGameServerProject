#pragma once

#define SERVER_PORT		8888
#define BOARD_WIDTH		300
#define BOARD_HEIGHT	300
#define VIEW_RADIUS		9

#define MAX_PLAYER		1000

#define MAX_BUF_SIZE	512
#define MAX_PACKET_SIZE	255
#define MAX_CHAT_SIZE	50

#define NUM_FISH_TO_POTION 3
#define NUM_STONE_TO_ARMOR 5

#define MAX_DURABILITY 10
#define POTION_HP 50

enum C2S : char {
	LOGIN = 1, LOGOUT, MOVE, ATTACK, CHAT_C2S, BUY_ITEM, USE_ITEM
};

enum S2C : char {
	LOGIN_OK = 1, LOGIN_FAIL, POSITION_INFO, CHAT_S2C, STAT_CHANGE, REMOVE_OBJECT, ADD_OBJECT
	, ADD_EFFECT, ADD_ITEM, DEAL_OK, DEAL_FAIL, ITEM_USED
};

enum KIND : char
{
	PLAYER_KIND, MILD_MONSTER, AGGR_MONSTER, FISH, NEUTRAL_MONSTER
};

enum SKILL : char
{
	PLAYER_STD_SKILL = 3, PLAYER_CROSS_SKILL, PLAYER_ULTIMATE_SKILL, NPC_SKILL, FISHING, MINING
};

enum ITEM : char
{
	FISH_ITEM, STONE, POTION, ARMOR
};

#pragma pack (push, 1)

struct cs_packet_login {
	BYTE size;
	BYTE type;
	WCHAR name[10];
};

struct cs_packet_logout {
	BYTE size;
	BYTE type;
};

struct cs_packet_move {
	BYTE size;
	BYTE type;
	// 0:up, 1:down, 2:left, 3:right
	BYTE dir;
};

struct cs_packet_attack {
	BYTE size;
	BYTE type;
	BYTE skill;
};

struct cs_packet_chat {
	BYTE size;
	BYTE type;
	WCHAR chat[MAX_CHAT_SIZE];
};

struct cs_packet_buy_item {
	BYTE size;
	BYTE type;
	BYTE item;
	BYTE number;
};

struct cs_packet_use_item {
	BYTE size;
	BYTE type;
	BYTE item;
};


struct sc_packet_login_ok {
	BYTE size;
	BYTE type;
	WORD id;
	WORD x, y;
	WORD hp;
	BYTE level;
	DWORD exp;
	WORD fish, stone, potion, armor;
};

struct sc_packet_login_fail {
	BYTE size;
	BYTE type;
};

struct sc_packet_position_info {
	BYTE size;
	BYTE type;
	WORD id;
	WORD x, y;
};

struct sc_packet_chat {
	BYTE size;
	BYTE type;
	WCHAR chat[MAX_CHAT_SIZE];
	WORD id;
};

struct sc_packet_stat_change {
	BYTE size;
	BYTE type;
	WORD hp;
	BYTE level;
	DWORD exp;
	WORD id;
	WORD durability;
};

struct sc_packet_remove_object {
	BYTE size;
	BYTE type;
	WORD id;
};

struct sc_packet_add_object {
	BYTE size;
	BYTE type;
	WORD id;
	BYTE kind;
	WORD x, y;
};

struct sc_packet_hp_change {
	BYTE size;
	BYTE type;
	int hp;
	int id;
};

struct sc_packet_add_effect {
	BYTE size;
	BYTE type;
	BYTE skill;
	WORD x, y;
};

struct sc_packet_add_item {
	BYTE size;
	BYTE type;
	BYTE item;
	BYTE num;
};


struct sc_packet_deal_ok {
	BYTE size;
	BYTE type;
	WORD potion;
	WORD armor;
};

struct sc_packet_deal_fail {
	BYTE size;
	BYTE type;
};

struct sc_packet_item_used {
	BYTE size;
	BYTE type;
	BYTE item;
};


#pragma pack (pop)
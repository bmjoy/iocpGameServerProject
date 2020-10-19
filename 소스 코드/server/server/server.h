#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "lua53.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <windows.h>  
#include <sqlext.h>  
#include <locale.h>
#include <iostream>
#include <unordered_set>
#include <set>
#include <mutex>
#include <chrono>
#include <fstream>

extern "C" {
#include "include\lua.h"
#include "include\lauxlib.h"
#include "include\lualib.h"
}

#include "protocol.h"
#define NUM_NPC			1400
#define NAME_LEN		11

enum COMMAND : char
{
	RECV_CMD, SEND_CMD, MOVE_CMD, RECV_NAME_CMD, STAT_SAVE_CMD, CHECK_PLAYER_CMD, SKILL_ON_CMD
	, REVIVE_CMD
};

enum DB_COMMAND : char
{
	GET_PLAYER_INFO_BY_NAME, UPDATE_PLAYER_POS, UPDATE_DEAL_DATA
};

enum MONSTER_STATE : char
{
	IDLE, AGGRESSIVE
};

const unsigned short default_HP = 100;

using namespace std;
using namespace chrono;

const unsigned int FRAME_SIZE = (VIEW_RADIUS * 2) + 1;
const unsigned int SECTOR_ROW_SIZE = (BOARD_HEIGHT / FRAME_SIZE) 
									+ ((BOARD_HEIGHT % FRAME_SIZE > 0) ? 1 : 0);
const unsigned int SECTOR_COL_SIZE = (BOARD_WIDTH / FRAME_SIZE)
									+ ((BOARD_WIDTH % FRAME_SIZE > 0) ? 1 : 0);

const unsigned int RECV_NAME_PACKET_SIZE = sizeof(cs_packet_login);



// 루아에서 호출하는 함수
int CAPI_send_message(lua_State *L);
int CAPI_get_x(lua_State *L);
int CAPI_get_y(lua_State *L);
int CAPI_move_neutral_npc(lua_State *L);
int CAPI_register_event(lua_State *L);
int CAPI_set_npc_dir(lua_State *L);

/*
void display_lua_error(lua_State *L, int error)
{
// error가 나면 lua stack에 에러 메시지 push
printf("Error : %s\n", lua_tostring(L, -1));
lua_pop(L, 1);
}
*/
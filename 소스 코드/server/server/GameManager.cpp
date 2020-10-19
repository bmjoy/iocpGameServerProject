#include "GameManager.h"
#include <random>

GameManager::GameManager(SendPacketPool &send_packet_pool
	, OVEXMemoryPool &ovex_pool, EvnetQueue &event_q, DBEvnetQueue &db_event_q)
	: send_packet_pool(send_packet_pool), ovex_pool(ovex_pool)
	, event_q(event_q), db_event_q(db_event_q)
{
	for (int i = 0; i < MAX_PLAYER; ++i) {
		ZeroMemory(&(players_recv_info[i].recv_ovex.wsaoverlapped), sizeof(WSAOVERLAPPED));
		players_recv_info[i].recv_ovex.wsabuf.buf = players_recv_info[i].recv_buf;
		players_recv_info[i].recv_ovex.wsabuf.len = MAX_BUF_SIZE;
		players_recv_info[i].recv_ovex.command = COMMAND::RECV_CMD;
		players_recv_info[i].remain_packet_size = 0;
	}

	for (int i = 0; i < MAX_PLAYER; ++i) {
		players[i].id = i;
		players[i].sock = INVALID_SOCKET;
		players[i].kind = KIND::PLAYER_KIND;
		players[i].std_skill_ready = true;
		players[i].cross_skill_ready = true;
		players[i].ultimate_skill_ready = true;
		players[i].prev_mining_x = 0;
		players[i].prev_mining_y = 0;
		players[i].mining_cnt = rand() % (MAX_MINING_CNT - 2) + 2;

		/////////////////////////////////////
		players[i].hp = default_HP;
		players[i].level = 1;
		players[i].exp = 0;
		players[i].durability = 0;
	}

	for (unsigned int i = 0; i < MAX_PLAYER; ++i) {
		empty_idx[i] = (MAX_PLAYER - 1) - i;
	}


	random_device r;
	default_random_engine dre{ r() };
	uniform_int_distribution<> uid_dir(0, 3);
	uniform_int_distribution<> uid_mild_or_aggresive(0, 1);
	uniform_int_distribution<> uid_add_v(FRAME_SIZE /2, FRAME_SIZE);
	ifstream f("map_data.txt", ios::in);
	int row = 0, col = 0;
	int i = 0;
	while (!f.eof()) {
		int a;
		f >> a;
		if (a == 9) {
			npcs[i].id = MAX_PLAYER + i;
			npcs[i].x = col;
			npcs[i].y = row;
			npcs[i].is_active = false;
			npcs[i].dir = uid_dir(dre);
			npcs[i].set_x = true;
			npcs[i].target = MAX_PLAYER;
			npcs[i].add_v = uid_add_v(dre);
			npcs[i].cur_add_v = npcs[i].add_v;

			//////////////////////////////
			npcs[i].state = MONSTER_STATE::IDLE;
			if(uid_mild_or_aggresive(dre) == 0) npcs[i].kind = KIND::AGGR_MONSTER;
			else npcs[i].kind = KIND::MILD_MONSTER;
			npcs[i].hp = default_HP;

			sectors[(npcs[i].y / FRAME_SIZE)*SECTOR_COL_SIZE + (npcs[i].x / FRAME_SIZE)].insert(npcs[i].id);
			a = 1;
			++i;
		}
		else if (a == 10) {
			npcs[i].id = MAX_PLAYER + i;
			npcs[i].x = col;
			npcs[i].y = row;
			npcs[i].is_active = false;
			npcs[i].dir = uid_dir(dre);
			npcs[i].set_x = true;
			npcs[i].target = MAX_PLAYER;
			npcs[i].add_v = uid_add_v(dre);
			npcs[i].cur_add_v = npcs[i].add_v;

			//////////////////////////////
			npcs[i].state = MONSTER_STATE::IDLE;
			npcs[i].kind = KIND::FISH;
			npcs[i].hp = default_HP;


			sectors[(npcs[i].y / FRAME_SIZE)*SECTOR_COL_SIZE + (npcs[i].x / FRAME_SIZE)].insert(npcs[i].id);
			a = 2;
			++i;
		}
		map[row][col] = a;
		++col;
		if (col == BOARD_WIDTH) {
			col = 0;
			++row;
		}
	}
	f.close();
	for(int r = 0; r<BOARD_HEIGHT; ++r)
		for (int c = 0; c < BOARD_WIDTH; ++c) {
			if (map[r][c] == 1 && rand() % 200 == 1 && i < NUM_NPC) {
					npcs[i].id = MAX_PLAYER + i;
					npcs[i].x = c;
					npcs[i].y = r;
					npcs[i].is_active = true;
					npcs[i].dir = uid_dir(dre);
					npcs[i].set_x = true;
					npcs[i].target = MAX_PLAYER;
					npcs[i].add_v = uid_add_v(dre);
					npcs[i].cur_add_v = npcs[i].add_v;

					npcs[i].state = MONSTER_STATE::IDLE;
					npcs[i].kind = KIND::NEUTRAL_MONSTER;
					npcs[i].hp = default_HP;

					// 루아 가상머신 초기화
					npcs[i].L = luaL_newstate();	// 초기화, 가상머신 할당
					luaL_openlibs(npcs[i].L);		// 기본 라이브러리 로딩
					int error = luaL_loadfile(npcs[i].L, "script/monster.lua");	// 루아 프로그램 로딩
					lua_pcall(npcs[i].L, 0, 0, 0); // 프로그램 실행
					lua_getglobal(npcs[i].L, "set_myid");
					lua_pushnumber(npcs[i].L, npcs[i].id);
					lua_pcall(npcs[i].L, 1, 0, 0);
					//lua_pop(npcs[i].L, 1);

					// API 등록
					lua_register(npcs[i].L, "API_send_message", CAPI_send_message);
					lua_register(npcs[i].L, "API_get_x", CAPI_get_x);
					lua_register(npcs[i].L, "API_get_y", CAPI_get_y);
					lua_register(npcs[i].L, "API_move_npc", CAPI_move_neutral_npc);
					lua_register(npcs[i].L, "API_register_event", CAPI_register_event);
					lua_register(npcs[i].L, "API_set_npc_dir", CAPI_set_npc_dir);

					sectors[(npcs[i].y / FRAME_SIZE)*SECTOR_COL_SIZE + (npcs[i].x / FRAME_SIZE)].insert(npcs[i].id);
					++i;
			}
		}
	
}

GameManager::~GameManager()
{
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (players[i].sock != INVALID_SOCKET)
			closesocket(players[i].sock);
	}
	// lua_close(L);
}

void GameManager::SetIOCPHande(HANDLE g_iocp)
{
	hIOCP = g_iocp;
}

unsigned short GameManager::GetEmptyPlayerSize()
{
	player_in_out.lock();
	unsigned short ret = nEmpty;
	player_in_out.unlock();
	return ret;
}

void GameManager::SendPacket(char type, unsigned short to, unsigned short target, wchar_t *msg)
{
	SEND_PACKET_NODE *packet = send_packet_pool.GetSendMemory();

	switch (type)
	{
	case S2C::LOGIN_OK:
	{
		sc_packet_login_ok p;
		p.id = players[to].id;
		p.x = players[to].x;
		p.y = players[to].y;
		p.hp = players[to].hp;
		p.level = players[to].level;
		p.exp = players[to].exp;
		p.fish = players[to].nFish;
		p.stone = players[to].nStone;
		p.potion = players[to].nPotion;
		p.armor = players[to].nArmor;
		p.size = sizeof(sc_packet_login_ok);
		p.type = S2C::LOGIN_OK;
		memcpy(packet->buf, &p, p.size);
		break;
	}
	case S2C::ADD_OBJECT:{
		sc_packet_add_object p;
		if (target < MAX_PLAYER) {
			p.id = players[target].id;
			p.kind = players[target].kind;
			p.x = players[target].x;
			p.y = players[target].y;
		}
		else {
			p.id = npcs[target - MAX_PLAYER].id;
			p.kind = npcs[target - MAX_PLAYER].kind;
			p.x = npcs[target - MAX_PLAYER].x;
			p.y = npcs[target - MAX_PLAYER].y;
		}
		p.size = sizeof(sc_packet_add_object);
		p.type = S2C::ADD_OBJECT;
		memcpy(packet->buf, &p, p.size);
		break;
	}
	case S2C::POSITION_INFO: {
		sc_packet_position_info p;
		if (target < MAX_PLAYER) {
			p.id = players[target].id;
			p.x = players[target].x;
			p.y = players[target].y;
		}
		else
		{
			p.id = npcs[target - MAX_PLAYER].id;
			p.x = npcs[target - MAX_PLAYER].x;
			p.y = npcs[target - MAX_PLAYER].y;
		}
		p.size = sizeof(sc_packet_position_info);
		p.type = S2C::POSITION_INFO;
		memcpy(packet->buf, &p, p.size);
		break;
	}
	case S2C::REMOVE_OBJECT: {
		sc_packet_remove_object p;
		if (target < MAX_PLAYER) p.id = players[target].id;
		else p.id = npcs[target - MAX_PLAYER].id;
		p.size = sizeof(sc_packet_remove_object);
		p.type = S2C::REMOVE_OBJECT;
		memcpy(packet->buf, &p, p.size);
		break;
	}
	case S2C::LOGIN_FAIL:
		break;
	case S2C::CHAT_S2C: {
		sc_packet_chat p;
		if (target < MAX_PLAYER) p.id = players[target].id;
		else p.id = npcs[target - MAX_PLAYER].id;
		p.size = sizeof(sc_packet_chat);
		p.type = S2C::CHAT_S2C;
		wcscpy_s(p.chat, MAX_CHAT_SIZE, msg);
		memcpy(packet->buf, &p, p.size);
		break;
	}
	case S2C::STAT_CHANGE: {
		sc_packet_stat_change p;
		if (target < MAX_PLAYER) {
			p.exp = players[target].exp;
			p.level = players[target].level;
			p.hp = players[target].hp;
			p.id = players[target].id;
			p.durability = players[target].durability;
		}
		else {
			p.exp = 0;
			p.level = 0;
			p.id = npcs[target - MAX_PLAYER].id;
			p.hp = npcs[target - MAX_PLAYER].hp;
		}
		p.size = sizeof(sc_packet_stat_change);
		p.type = S2C::STAT_CHANGE;
		memcpy(packet->buf, &p, p.size);
		break;
	}
	case S2C::DEAL_FAIL: {
		sc_packet_deal_fail p;
		p.size = sizeof(sc_packet_deal_fail);
		p.type = S2C::DEAL_FAIL;
		memcpy(packet->buf, &p, p.size);
		break;
	}
	default:
		cout << "s2c packet type error" << endl;
		return;
	}

	packet->exov.wsabuf.len = (packet->buf)[0];
	ZeroMemory(&(packet->exov.wsaoverlapped), sizeof(WSAOVERLAPPED));
	WSASend(players[to].sock, &(packet->exov.wsabuf), 1, NULL, 0
		, &(packet->exov.wsaoverlapped), NULL);
}

void GameManager::SendEffectPacket(unsigned short to, unsigned short x, unsigned short y, char skill)
{
	SEND_PACKET_NODE *packet = send_packet_pool.GetSendMemory();

	sc_packet_add_effect p;
	p.size = sizeof(sc_packet_add_effect);
	p.type = S2C::ADD_EFFECT;
	p.skill = skill;
	p.x = x;
	p.y = y;

	memcpy(packet->buf, &p, p.size);

	packet->exov.wsabuf.len = (packet->buf)[0];
	ZeroMemory(&(packet->exov.wsaoverlapped), sizeof(WSAOVERLAPPED));
	WSASend(players[to].sock, &(packet->exov.wsabuf), 1, NULL, 0
		, &(packet->exov.wsaoverlapped), NULL);
}

void GameManager::SendItemPacket(char type, unsigned short to, char item, short n)
{
	SEND_PACKET_NODE *packet = send_packet_pool.GetSendMemory();

	switch (type)
	{
	case S2C::ADD_ITEM:
	{
		sc_packet_add_item p;
		p.item = item;
		p.num = n;
		p.size = sizeof(sc_packet_add_item);
		p.type = S2C::ADD_ITEM;
		memcpy(packet->buf, &p, p.size);
		break;
	}
	default:
		cout << "s2c packet type error" << endl;
		return;
	}

	packet->exov.wsabuf.len = (packet->buf)[0];
	ZeroMemory(&(packet->exov.wsaoverlapped), sizeof(WSAOVERLAPPED));
	WSASend(players[to].sock, &(packet->exov.wsabuf), 1, NULL, 0
		, &(packet->exov.wsaoverlapped), NULL);
}

void GameManager::SendDealOKPacket(unsigned short to, unsigned short nPotion, unsigned short nArmor)
{
	SEND_PACKET_NODE *packet = send_packet_pool.GetSendMemory();

	sc_packet_deal_ok p;
	p.size = sizeof(sc_packet_deal_ok);
	p.type = S2C::DEAL_OK;
	p.potion = nPotion;
	p.armor = nArmor;

	memcpy(packet->buf, &p, p.size);

	packet->exov.wsabuf.len = (packet->buf)[0];
	ZeroMemory(&(packet->exov.wsaoverlapped), sizeof(WSAOVERLAPPED));
	WSASend(players[to].sock, &(packet->exov.wsabuf), 1, NULL, 0
		, &(packet->exov.wsaoverlapped), NULL);
}

void GameManager::SendItemUsedPacket(unsigned short to, char item)
{
	SEND_PACKET_NODE *packet = send_packet_pool.GetSendMemory();
	
	sc_packet_item_used p;
	p.item = item;
	p.size = sizeof(sc_packet_item_used);
	p.type = S2C::ITEM_USED;
	memcpy(packet->buf, &p, p.size);

	packet->exov.wsabuf.len = (packet->buf)[0];
	ZeroMemory(&(packet->exov.wsaoverlapped), sizeof(WSAOVERLAPPED));
	WSASend(players[to].sock, &(packet->exov.wsabuf), 1, NULL, 0
		, &(packet->exov.wsaoverlapped), NULL);
}

void GameManager::ProcessPacket(unsigned short idx)
{
	if (players[idx].hp <= 0) return;
	switch (players_recv_info[idx].recved_packet[sizeof(byte)]) 
	{
	case C2S::MOVE:
		MovePlayer(idx);
		break;
	case C2S::LOGOUT:
		break;
	case C2S::ATTACK:
		MakePlayerSkill(idx);
		break;
	case C2S::CHAT_C2S:
		break;
	case C2S::BUY_ITEM:
		DealItem(idx);
		break;
	case C2S::USE_ITEM:
		UseItem(idx);
		break;
	default:
		cout << "process packet error " << endl;
		break;
	}
}

void GameManager::MovePlayer(unsigned short idx)
{
	unsigned short prev_x = players[idx].x; unsigned short prev_y = players[idx].y;
	unsigned short new_x = prev_x; unsigned short new_y = prev_y;

	switch (players_recv_info[idx].recved_packet[sizeof(byte) + sizeof(byte)])
	{
	case 0:	// up
		if (prev_y > 0)new_y = prev_y - 1;
		break;
	case 1:	// down
		if (prev_y < BOARD_HEIGHT - 1) new_y = prev_y + 1;
		break;
	case 2:	// left
		if (prev_x > 0)new_x = prev_x - 1;
		break;
	case 3:	// right
		if (prev_x < BOARD_WIDTH - 1) new_x = prev_x + 1;
		break;
	default:
		break;
	}
	if (prev_x == new_x && prev_y == new_y) return;
	if (map[new_y][new_x] == 2 || map[new_y][new_x] == 4
		|| map[new_y][new_x] == 5 || map[new_y][new_x] == 6)
		return;
	

	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, prev_x, prev_y);
	GetViewSectors(view_sectors, new_x, new_y);
	set<unsigned short> new_view_sectors;
	GetViewSectors(new_view_sectors, new_x, new_y);

	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}

	players[idx].x = new_x; players[idx].y = new_y;

	if (prev_x / FRAME_SIZE != new_x / FRAME_SIZE
		|| prev_y / FRAME_SIZE != new_y / FRAME_SIZE) {
		sectors[(prev_y / FRAME_SIZE)*SECTOR_COL_SIZE + (prev_x / FRAME_SIZE)].erase(idx);
		sectors[(new_y / FRAME_SIZE)*SECTOR_COL_SIZE + (new_x / FRAME_SIZE)].insert(idx);
	}

	unordered_set<unsigned short> new_view_list;
	for (auto s : new_view_sectors) {
		for (auto i : sectors[s]) {
			if (i == idx) continue;
			short tx, ty;
			if (i < MAX_PLAYER) {
				tx = players[i].x;
				ty = players[i].y;
			}
			else
			{
				tx = npcs[i - MAX_PLAYER].x;
				ty = npcs[i - MAX_PLAYER].y;
			}
			if (((new_x - VIEW_RADIUS <= tx && tx <= new_x + VIEW_RADIUS)
				&& (new_y - VIEW_RADIUS <= ty && ty <= new_y + VIEW_RADIUS)))
				new_view_list.insert(i);
		}
	}

	for (auto i : new_view_list) {
		if (players[idx].view_list.find(i) != players[idx].view_list.end()) {
			if (i < MAX_PLAYER) SendPacket(S2C::POSITION_INFO, i, idx);
			// 시야 내에 있는 플레이어가 이동했다는 이벤트 발생
			// 누가 움직였는지 알려줘야 한다
			else
				if(npcs[i - MAX_PLAYER].kind == KIND::NEUTRAL_MONSTER)
					event_q.enq(i, COMMAND::CHECK_PLAYER_CMD, 0, idx);
		}
		else
		{
			players[idx].view_list.insert(i);
			SendPacket(S2C::ADD_OBJECT, idx, i);
			if (i < MAX_PLAYER) {
				players[i].view_list.insert(idx);
				SendPacket(S2C::ADD_OBJECT, i, idx);
			}
			else {
				if (false == npcs[i - MAX_PLAYER].is_active 
					&& npcs[i - MAX_PLAYER].kind != KIND::NEUTRAL_MONSTER) {
					npcs[i - MAX_PLAYER].is_active = true;
					event_q.enq(i, COMMAND::MOVE_CMD, MOVE_TERM);
				}
			}
		}

	}

	for (auto it = players[idx].view_list.begin(); it != players[idx].view_list.end();)
	{
		if (new_view_list.find(*it) == new_view_list.end()) {
			SendPacket(S2C::REMOVE_OBJECT, idx, *it);

			if (*it < MAX_PLAYER) {
				players[*it].view_list.erase(idx);
				SendPacket(S2C::REMOVE_OBJECT, *it, idx);
			}
			it = players[idx].view_list.erase(it);
		}
		else ++it;
	}


	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}

	SendPacket(S2C::POSITION_INFO, idx, idx);

}

void GameManager::MakePlayerSkill(unsigned short idx)
{
	char skill = players_recv_info[idx].recved_packet[sizeof(byte) + sizeof(byte)];
	switch (skill)
	{
	// standard skill
	case 0:
	case 1:
	case 2:
	case 3:
		MakeStandardSkill(idx, skill);
		break;
	case 4:
		MakeCrossSkill(idx);
		break;
	case 5:
		MakeUltimateSkill(idx);
		break;
	case SKILL::FISHING:
		Fishing(idx);
		break;
	case SKILL::MINING:
		Mining(idx);
		break;
	default:
		break;
	}
}

void GameManager::MakeStandardSkill(unsigned short idx, char d)
{
	if (false == players[idx].std_skill_ready)return;
	players[idx].std_skill_ready = false;

	short dir[4][2] = { { 0, -1 },{ 1, 0 },{ 0, 1 },{ -1, 0 } };
	const unsigned short x = players[idx].x;
	const unsigned short y = players[idx].y;

	unsigned short end_x = x + dir[d][0];
	unsigned short end_y = y + dir[d][1];

	bool dif = true;
	if (map[end_y][end_x] == 2 || map[end_y][end_x] == 4
		|| map[end_y][end_x] == 5 || map[end_y][end_x] == 6) {
		dif = false;
	}

	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x, y);
	if (true == dif) GetViewSectors(view_sectors, end_x, end_y);

	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}

	set<unsigned short> viewers;
	set<unsigned short> removed;

	for (auto s : view_sectors) {
		for (auto i : sectors[s]) {
			if (i == idx) continue;
			if (i < MAX_PLAYER) {
				short tx, ty;
				tx = players[i].x;
				ty = players[i].y;
				if (((tx - VIEW_RADIUS <= x && x <= tx + VIEW_RADIUS)
					&& (ty - VIEW_RADIUS <= y && y <= ty + VIEW_RADIUS))
					|| ((tx - VIEW_RADIUS <= end_x && end_x <= tx + VIEW_RADIUS)
						&& (ty - VIEW_RADIUS <= end_y && end_y <= ty + VIEW_RADIUS)))
					viewers.insert(i);

			}
			else
			{
				short tx, ty;
				tx = npcs[i - MAX_PLAYER].x;
				ty = npcs[i - MAX_PLAYER].y;
				if ((tx == x && ty == y) || (tx == end_x && ty == end_y)) {
					short new_hp = npcs[i - MAX_PLAYER].hp - STD_SKILL_DAMAGE;
					if (new_hp < 0) new_hp = 0;
					npcs[i - MAX_PLAYER].hp = new_hp;
					if (KIND::MILD_MONSTER == npcs[i - MAX_PLAYER].kind) {
						npcs[i - MAX_PLAYER].state = MONSTER_STATE::AGGRESSIVE;
						npcs[i - MAX_PLAYER].target = idx;
					}
					SendPacket(S2C::STAT_CHANGE, idx, i);
					if (new_hp <= 0) {
						int mul = 1;
						if (npcs[i - MAX_PLAYER].kind == AGGR_MONSTER) mul = 2;
						players[idx].exp += MONSTER_EXP * mul;
						if (players[idx].exp >= MAX_EXP) {
							players[idx].exp = 0;
							players[idx].level += 1;
						}
						SendPacket(S2C::STAT_CHANGE, idx, idx);
						removed.insert(i);
						players[idx].view_list.erase(i);
						SendPacket(S2C::REMOVE_OBJECT, idx, i);
					}
				}
			}
		}
	}

	for (auto r : removed) {
		unsigned short tx = npcs[r - MAX_PLAYER].x;
		unsigned short ty = npcs[r - MAX_PLAYER].y;
		// sector에서 제거
		sectors[(ty / FRAME_SIZE)*SECTOR_COL_SIZE + (tx / FRAME_SIZE)].erase(r);
		npcs[r - MAX_PLAYER].is_active = false;
		for (auto i : viewers) {
			if (players[i].view_list.find(r) != players[i].view_list.end()) {
				players[i].view_list.erase(r);
				SendPacket(S2C::REMOVE_OBJECT, i, r);
			}
		}
	}
		
	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}

	// effect pakcet 보내기
	SendEffectPacket(idx, x, y, d);
	for (auto i : viewers)
		SendEffectPacket(i, x, y, d);
	

	// remove monster 처리
	for (auto r : removed) {
		// 부활 타이머 등록
		event_q.enq(r, COMMAND::REVIVE_CMD, REVIVE_TERM);
	}	

	// skill timer등록
	event_q.enq(idx, COMMAND::SKILL_ON_CMD, STD_SKILL_COOl, SKILL::PLAYER_STD_SKILL);
}

void GameManager::MakeCrossSkill(unsigned short idx)
{
	if (false == players[idx].cross_skill_ready)return;
	players[idx].cross_skill_ready = false;

	const unsigned short x = players[idx].x;
	const unsigned short y = players[idx].y;
	unsigned short pos[4][2];

	short dir[4][2] = { { 0, -1 },{ 1, 0 },{ 0, 1 },{ -1, 0 } };
	for (int i = 0; i < 4; ++i) {
		if (map[y + dir[i][1]][x + dir[i][0]] == 2 || map[y + dir[i][1]][x + dir[i][0]] == 4
			|| map[y + dir[i][1]][x + dir[i][0]] == 5 || map[y + dir[i][1]][x + dir[i][0]] == 6) {
			pos[i][0] = x; pos[i][1] = y;
		}
		else {
			pos[i][0] = x + dir[i][0];
			pos[i][1] = y + dir[i][1];
		}
	}

	set<unsigned short> view_sectors;
	for (int i = 0; i < 4; ++i)
		GetViewSectors(view_sectors, pos[i][0], pos[i][1]);
	
	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}

	set<unsigned short> viewers;
	set<unsigned short> removed;

	for (auto s : view_sectors) {
		for (auto i : sectors[s]) {
			if (i == idx) continue;
			if (i < MAX_PLAYER) {
				short tx, ty;
				tx = players[i].x;
				ty = players[i].y;
				for (int d = 0; d < 4; ++d) {
					if ((tx - VIEW_RADIUS <= pos[d][0] && pos[d][0] <= tx + VIEW_RADIUS)
						&& (ty - VIEW_RADIUS <= pos[d][1] && pos[d][1] <= ty + VIEW_RADIUS)) {
						viewers.insert(i);
						continue;
					}
				}
			}
			else
			{
				short tx, ty;
				tx = npcs[i - MAX_PLAYER].x;
				ty = npcs[i - MAX_PLAYER].y;
				bool is_same = false;
				if (tx == x && ty == y) is_same = true;
				for (int d = 0; d < 4; ++d) {
					if (is_same || ( tx == pos[d][0] && ty == pos[d][1])) {
						short new_hp = npcs[i - MAX_PLAYER].hp - CROSS_SKILL_DAMAGE;
						if (new_hp < 0) new_hp = 0;
						npcs[i - MAX_PLAYER].hp = new_hp;
						if (KIND::MILD_MONSTER == npcs[i - MAX_PLAYER].kind) {
							npcs[i - MAX_PLAYER].state = MONSTER_STATE::AGGRESSIVE;
							npcs[i - MAX_PLAYER].target = idx;
						}
						SendPacket(S2C::STAT_CHANGE, idx, i);
						if (new_hp <= 0) {
							int mul = 1;
							if (npcs[i - MAX_PLAYER].kind == AGGR_MONSTER) mul = 2;
							players[idx].exp += MONSTER_EXP * mul;
							if (players[idx].exp >= MAX_EXP) {
								players[idx].exp = 0;
								players[idx].level += 1;
							}
							SendPacket(S2C::STAT_CHANGE, idx, idx);
							removed.insert(i);
							players[idx].view_list.erase(i);
							SendPacket(S2C::REMOVE_OBJECT, idx, i);
						}
						continue;
					}
				}
			}
		}
	}

	for (auto r : removed) {
		unsigned short tx = npcs[r - MAX_PLAYER].x;
		unsigned short ty = npcs[r - MAX_PLAYER].y;
		// sector에서 제거
		sectors[(ty / FRAME_SIZE)*SECTOR_COL_SIZE + (tx / FRAME_SIZE)].erase(r);
		npcs[r - MAX_PLAYER].is_active = false;
		for (auto i : viewers) {
			if (players[i].view_list.find(r) != players[i].view_list.end()) {
				players[i].view_list.erase(r);
				SendPacket(S2C::REMOVE_OBJECT, i, r);
			}
		}
	}

	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}

	// effect pakcet 보내기
	SendEffectPacket(idx, x, y, SKILL::PLAYER_CROSS_SKILL);
	for (auto i : viewers)
		SendEffectPacket(i, x, y, SKILL::PLAYER_CROSS_SKILL);


	// remove monster 처리
	for (auto r : removed) {
		// 부활 타이머 등록
		event_q.enq(r, COMMAND::REVIVE_CMD, REVIVE_TERM);
	}

	// skill timer등록
	event_q.enq(idx, COMMAND::SKILL_ON_CMD, CROSS_SKILL_COOL, SKILL::PLAYER_CROSS_SKILL);
}

void GameManager::MakeUltimateSkill(unsigned short idx)
{
	if (false == players[idx].ultimate_skill_ready)return;
	players[idx].ultimate_skill_ready = false;

	const unsigned short x = players[idx].x;
	const unsigned short y = players[idx].y;
	unsigned short pos[12][2];

	short dir[12][2] = { { 0, -2 },{ 1, -1 }, { 2, 0 },{ 1, 1 },{ 0, 2 },{ -1, 1 },{ -2, 0 },{ -1, -1 }
		,{ 0, -1 },{ 1, 0 },{ 0, 1 },{ -1, 0 } };

	for (int i = 0; i < 12; ++i) {
		if (map[y + dir[i][1]][x + dir[i][0]] == 2 || map[y + dir[i][1]][x + dir[i][0]] == 4
			|| map[y + dir[i][1]][x + dir[i][0]] == 5 || map[y + dir[i][1]][x + dir[i][0]] == 6) {
			pos[i][0] = x; pos[i][1] = y;
		}
		else {
			pos[i][0] = x + dir[i][0];
			pos[i][1] = y + dir[i][1];
		}
	}

	set<unsigned short> view_sectors;
	for (int i = 0; i < 12; ++i)
		GetViewSectors(view_sectors, pos[i][0], pos[i][1]);

	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}

	set<unsigned short> viewers;
	set<unsigned short> removed;

	for (auto s : view_sectors) {
		for (auto i : sectors[s]) {
			if (i == idx) continue;
			if (i < MAX_PLAYER) {
				short tx, ty;
				tx = players[i].x;
				ty = players[i].y;
				for (int d = 0; d < 12; ++d) {
					if ((tx - VIEW_RADIUS <= pos[d][0] && pos[d][0] <= tx + VIEW_RADIUS)
						&& (ty - VIEW_RADIUS <= pos[d][1] && pos[d][1] <= ty + VIEW_RADIUS)) {
						viewers.insert(i);
						continue;
					}
				}
			}
			else
			{
				short tx, ty;
				tx = npcs[i - MAX_PLAYER].x;
				ty = npcs[i - MAX_PLAYER].y;
				bool is_same = false;
				if (tx == x && ty == y) is_same = true;
				for (int d = 0; d < 12; ++d) {
					if (is_same || (tx == pos[d][0] && ty == pos[d][1])) {
						short new_hp = npcs[i - MAX_PLAYER].hp - ULTIMATE_SKILL_DAMAGE;
						if (new_hp < 0) new_hp = 0;
						npcs[i - MAX_PLAYER].hp = new_hp;
						if (KIND::MILD_MONSTER == npcs[i - MAX_PLAYER].kind) {
							npcs[i - MAX_PLAYER].state = MONSTER_STATE::AGGRESSIVE;
							npcs[i - MAX_PLAYER].target = idx;
						}
						SendPacket(S2C::STAT_CHANGE, idx, i);
						if (new_hp <= 0) {
							int mul = 1;
							if (npcs[i - MAX_PLAYER].kind == AGGR_MONSTER) mul = 2;
							players[idx].exp += MONSTER_EXP * mul;
							if (players[idx].exp >= MAX_EXP) {
								players[idx].exp = 0;
								players[idx].level += 1;
							}
							SendPacket(S2C::STAT_CHANGE, idx, idx);
							removed.insert(i);
							players[idx].view_list.erase(i);
							SendPacket(S2C::REMOVE_OBJECT, idx, i);
						}
						continue;
					}
				}
			}
		}
	}

	for (auto r : removed) {
		unsigned short tx = npcs[r - MAX_PLAYER].x;
		unsigned short ty = npcs[r - MAX_PLAYER].y;
		// sector에서 제거
		sectors[(ty / FRAME_SIZE)*SECTOR_COL_SIZE + (tx / FRAME_SIZE)].erase(r);
		npcs[r - MAX_PLAYER].is_active = false;
		for (auto i : viewers) {
			if (players[i].view_list.find(r) != players[i].view_list.end()) {
				players[i].view_list.erase(r);
				SendPacket(S2C::REMOVE_OBJECT, i, r);
			}
		}
	}

	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}

	// effect pakcet 보내기
	SendEffectPacket(idx, x, y, SKILL::PLAYER_ULTIMATE_SKILL);
	for (auto i : viewers)
		SendEffectPacket(i, x, y, SKILL::PLAYER_ULTIMATE_SKILL);


	// remove monster 처리
	for (auto r : removed) {
		// 부활 타이머 등록
		event_q.enq(r, COMMAND::REVIVE_CMD, REVIVE_TERM);
	}

	// skill timer등록
	event_q.enq(idx, COMMAND::SKILL_ON_CMD, ULTIMATE_SKILL_COOL, SKILL::PLAYER_ULTIMATE_SKILL);
}

void GameManager::Fishing(unsigned short idx)
{
	short dir[4][2] = { { 0, -1 },{ 1, 0 },{ 0, 1 },{ -1, 0 } };
	const unsigned short x = players[idx].x;
	const unsigned short y = players[idx].y;

	int d = -1;
	for (int i = 0; i < 4; ++i) {
		if (map[y + dir[i][1]][x + dir[i][0]] == 2) {
			d = i;
			break;
		}
	}
	if (d == -1) return;

	unsigned short fish_x = x + dir[d][0] * 3;
	unsigned short fish_y = y + dir[d][1] * 3;

	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, fish_x, fish_y);

	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}

	set<unsigned short> viewers;
	bool is_fishing = false;
	unsigned short fish_ID = 0;

	for (auto s : view_sectors) {
		for (auto i : sectors[s]) {
			if (i == idx) continue;
			if (i < MAX_PLAYER) {
				short tx, ty;
				tx = players[i].x;
				ty = players[i].y;
				if ((tx - VIEW_RADIUS <= fish_x && fish_x <= tx + VIEW_RADIUS)
					&& (ty - VIEW_RADIUS <= fish_y && fish_y <= ty + VIEW_RADIUS))
					viewers.insert(i);
			}
			else
			{
				if (npcs[i - MAX_PLAYER].kind == KIND::FISH && false == is_fishing) {
					unsigned short tx, ty;
					tx = npcs[i - MAX_PLAYER].x;
					ty = npcs[i - MAX_PLAYER].y;
					if (tx == fish_x && ty == fish_y) {
						is_fishing = true;
						fish_ID = i;
						
						players[idx].nFish += 1;
						SendItemPacket(S2C::ADD_ITEM, idx, ITEM::FISH_ITEM, 1);
				
						players[idx].view_list.erase(i);
						SendPacket(S2C::REMOVE_OBJECT, idx, i);
					}
				}
			}
		}
	}
	
	if (is_fishing) {
		unsigned short tx = npcs[fish_ID - MAX_PLAYER].x;
		unsigned short ty = npcs[fish_ID - MAX_PLAYER].y;
		// sector에서 제거
		sectors[(ty / FRAME_SIZE)*SECTOR_COL_SIZE + (tx / FRAME_SIZE)].erase(fish_ID);
		npcs[fish_ID - MAX_PLAYER].is_active = false;

		for (auto i : viewers) {
			if (players[i].view_list.find(fish_ID) != players[i].view_list.end()) {
				players[i].view_list.erase(fish_ID);
				SendPacket(S2C::REMOVE_OBJECT, i, fish_ID);
			}
		}
		event_q.enq(fish_ID, COMMAND::REVIVE_CMD, REVIVE_TERM);
	}

	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}
}

void GameManager::Mining(unsigned short idx)
{
	const unsigned short x = players[idx].x;
	const unsigned short y = players[idx].y;
	if (map[y][x] != 3) return;

	const unsigned short px = players[idx].prev_mining_x;
	const unsigned short py = players[idx].prev_mining_y;

	if (x == px && y == py) {
		if (players[idx].mining_cnt > 0) {
			--players[idx].mining_cnt;
			if (players[idx].mining_cnt == 0) {
				players[idx].nStone += 1;
				SendItemPacket(S2C::ADD_ITEM, idx, ITEM::STONE, 1);
			}
		}
	}
	else
	{
		players[idx].mining_cnt = rand() % (MAX_MINING_CNT - 2) + 2;
		players[idx].prev_mining_x = x;
		players[idx].prev_mining_y = y;
	}
}

void GameManager::DealItem(unsigned short idx)
{
	cs_packet_buy_item *p
		= reinterpret_cast<cs_packet_buy_item *>(players_recv_info[idx].recved_packet);
	switch (p->item)
	{
	case ITEM::POTION: {
		if ( p->number > 0 && 0 <= players[idx].nFish - ((p->number) * NUM_FISH_TO_POTION)) {
			// 거래 성공
			players[idx].nFish -= (p->number) * NUM_FISH_TO_POTION;
			players[idx].nPotion += p->number;
			RecordStat(idx, true);
			return;
		}
		SendPacket(S2C::DEAL_FAIL, idx);
		return;
	}
	case ITEM::ARMOR: {
		if (p->number > 0 && 0 <= players[idx].nStone - ((p->number) * NUM_STONE_TO_ARMOR)) {
			// 거래 성공
			players[idx].nStone -= (p->number) * NUM_STONE_TO_ARMOR;
			players[idx].nArmor += p->number;
			RecordStat(idx, true);
			return;
		}
		SendPacket(S2C::DEAL_FAIL, idx);
		return;
	}
	default:
		cout << "item deal error" << endl;
		break;
	}
}


void GameManager::UseItem(unsigned short idx)
{
	cs_packet_use_item *p = reinterpret_cast<cs_packet_use_item *>(players_recv_info[idx].recved_packet);

	switch (p->item)
	{
	case ITEM::POTION: {
		if (players[idx].nPotion <= 0) return;
		UsePotion(idx);
		SendItemUsedPacket(idx, ITEM::POTION);
		return;
	}
	case ITEM::ARMOR: {
		if (players[idx].nArmor <= 0) return;
		UseArmor(idx);
		SendItemUsedPacket(idx, ITEM::ARMOR);
		return;
	}
	default:
		cout << "item use error" << endl;
		break;
	}
}

void GameManager::UsePotion(unsigned short idx)
{
	const unsigned short x = players[idx].x;
	const unsigned short y = players[idx].y;
	--players[idx].nPotion;

	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x, y);

	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}
	
	players[idx].hp += POTION_HP;
	if (players[idx].hp > default_HP) players[idx].hp = default_HP;
	SendPacket(S2C::STAT_CHANGE, idx, idx);

	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}
}

void GameManager::UseArmor(unsigned short idx)
{
	const unsigned short x = players[idx].x;
	const unsigned short y = players[idx].y;
	--players[idx].nArmor;

	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x, y);

	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}

	players[idx].durability = MAX_DURABILITY;
	SendPacket(S2C::STAT_CHANGE, idx, idx);

	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}
}



void GameManager::RecvPacket(unsigned short idx, unsigned long data_size)
{
	// key 검증 -> 수정 필요 (data race)
	//int idx = LOWORD(key);
	//if (key != players[idx].id) return;

	unsigned int rest_size = data_size;
	unsigned int cur_recv_buf_idx = 0;
	while (rest_size) {
		int remain = players_recv_info[idx].remain_packet_size;
		unsigned int required;
		if (remain == 0) required = players_recv_info[idx].recv_buf[cur_recv_buf_idx];
		else required = players_recv_info[idx].recved_packet[0] - remain;

		if (required > rest_size) {
			memcpy(&(players_recv_info[idx].recved_packet[remain])
				, &(players_recv_info[idx].recv_buf[cur_recv_buf_idx]), rest_size);
			players_recv_info[idx].remain_packet_size += rest_size;
			break;
		}

		memcpy(&(players_recv_info[idx].recved_packet[remain])
			, &(players_recv_info[idx].recv_buf[cur_recv_buf_idx]), required);
		cur_recv_buf_idx += required;
		rest_size -= required;
		players_recv_info[idx].remain_packet_size = 0;
		ProcessPacket(idx);
	}

	// recv 요청
	ZeroMemory(&(players_recv_info[idx].recv_ovex.wsaoverlapped), sizeof(WSAOVERLAPPED));
	unsigned long flag = 0;
	WSARecv(players[idx].sock, &(players_recv_info[idx].recv_ovex.wsabuf), 1, NULL, &flag
		, &(players_recv_info[idx].recv_ovex.wsaoverlapped), NULL);

}

unsigned short GameManager::GetID()
{
	player_in_out.lock();
	unsigned short idx = empty_idx[--nEmpty];
	player_in_out.unlock();
	return idx;
}

void GameManager::ReturnID(unsigned short idx)
{
	player_in_out.lock();
	empty_idx[nEmpty++] = idx;
	player_in_out.unlock();
}

void GameManager::InPlayer(unsigned short idx, SOCKET sock, unsigned short x, unsigned short y
	, unsigned short hp, byte level, unsigned long exp, WCHAR *name
	, unsigned short fish, unsigned short stone, unsigned short potion, unsigned short armor)
{
	// iocp에 새로운 소켓 등록
	//CreateIoCompletionPort(reinterpret_cast<HANDLE>(sock), hIOCP, players[idx].id, 0);
	players[idx].sock = sock;
	players[idx].x = x;
	players[idx].y = y;
	players[idx].hp = default_HP;
	players[idx].level = level;
	players[idx].exp = exp;
	players[idx].nFish = fish;
	players[idx].nStone = stone;
	players[idx].nPotion = potion;
	players[idx].nArmor = armor;
	memcpy(players[idx].name, name, (lstrlen(name)) * sizeof(WCHAR));
	SendPacket(S2C::LOGIN_OK, idx);

	std::cout << idx << ",  client in" << endl;


	int n_sector = (y / FRAME_SIZE)*SECTOR_COL_SIZE + (x / FRAME_SIZE);
	
	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x, y);

	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}
	//			-> 현재 이 sector에 있는 플레이어 움직이거나 나갈 수 x
	for (auto s : view_sectors) {
		for (auto i : sectors[s])
		{
			unsigned short tx, ty;
			if (i < MAX_PLAYER) {
				tx = players[i].x; ty = players[i].y;
			}
			else {
				tx = npcs[i - MAX_PLAYER].x; ty = npcs[i - MAX_PLAYER].y;
			}
			if ((x - VIEW_RADIUS <= tx && tx <= x + VIEW_RADIUS)
				&& (y - VIEW_RADIUS <= ty && ty <= y + VIEW_RADIUS)) {
				players[idx].view_list.insert(i);
				SendPacket(S2C::ADD_OBJECT, idx, i);
				if (i < MAX_PLAYER) {
					players[i].view_list.insert(idx);
					SendPacket(S2C::ADD_OBJECT, i, idx);
				}
				else {
					if (false == npcs[i - MAX_PLAYER].is_active
						&& npcs[i - MAX_PLAYER].kind != KIND::NEUTRAL_MONSTER) {
						npcs[i - MAX_PLAYER].is_active = true;
						event_q.enq(i, COMMAND::MOVE_CMD, MOVE_TERM);
					}
				}
			}
		}
	}

	sectors[n_sector].insert(idx);
	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}


	// request recv
	ZeroMemory(&(players_recv_info[idx].recv_ovex.wsaoverlapped), sizeof(WSAOVERLAPPED));
	unsigned long flag = 0;
	WSARecv(players[idx].sock, &(players_recv_info[idx].recv_ovex.wsabuf), 1, NULL,
		&flag, &(players_recv_info[idx].recv_ovex.wsaoverlapped), NULL);
}

void GameManager::OutPlayer(unsigned short idx)
{
	// recv error 일때만 out 처리 -> recv는 한개 -> move/out 겹치지 x 
	// pos 변하면 바로 반영 되어야 한다 -> lock 걸고 바꾼다 **(atomic memory 써야하는지)

	//int idx = LOWORD(key);
	//if (key != players[idx].id) return;

	RecordStat(idx);
	event_q.DeleteEvents(idx);
	unsigned short x = players[idx].x; unsigned short y = players[idx].y;
	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x, y);
	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}
	// 1. sector에서 제거
	sectors[(y / FRAME_SIZE)*SECTOR_COL_SIZE + (x / FRAME_SIZE)].erase(idx);

	// 2. view list의 객체에 out 보내기
	for (auto i : players[idx].view_list) {
		if (i < MAX_PLAYER) {
			players[i].view_list.erase(idx);
			SendPacket(S2C::REMOVE_OBJECT, i, idx);
		}
	}

	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}


	// 3. 데이터 초기화
	//players[idx].id += (1 << 4);

	ZeroMemory(&(players_recv_info[idx].recv_ovex.wsaoverlapped), sizeof(WSAOVERLAPPED));
	players_recv_info[idx].remain_packet_size = 0;
	players[idx].view_list.clear();
	closesocket(players[idx].sock);
	players[idx].sock = INVALID_SOCKET;
	players[idx].std_skill_ready = true;
	players[idx].cross_skill_ready = true;
	players[idx].ultimate_skill_ready = true;

	// 4. empty idx에 추가
	ReturnID(idx);
	std::cout << idx << ",  client out" << endl;


}

void GameManager::GetViewSectors(set<unsigned short>& view_sector
	, unsigned short x, unsigned short y)
{
	if (x - VIEW_RADIUS >= 0 && y + VIEW_RADIUS < SECTOR_ROW_SIZE*FRAME_SIZE)
		view_sector.insert(((y + VIEW_RADIUS) / FRAME_SIZE)*SECTOR_COL_SIZE + ((x - VIEW_RADIUS) / FRAME_SIZE));
	if (x - VIEW_RADIUS >= 0 && y - VIEW_RADIUS >= 0)
		view_sector.insert(((y - VIEW_RADIUS) / FRAME_SIZE)*SECTOR_COL_SIZE + ((x - VIEW_RADIUS) / FRAME_SIZE));
	if (x + VIEW_RADIUS < SECTOR_COL_SIZE*FRAME_SIZE && y + VIEW_RADIUS < SECTOR_ROW_SIZE*FRAME_SIZE)
		view_sector.insert(((y + VIEW_RADIUS) / FRAME_SIZE)*SECTOR_COL_SIZE + ((x + VIEW_RADIUS) / FRAME_SIZE));
	if (x + VIEW_RADIUS < SECTOR_COL_SIZE*FRAME_SIZE && y - VIEW_RADIUS >= 0)
		view_sector.insert(((y - VIEW_RADIUS) / FRAME_SIZE)*SECTOR_COL_SIZE + ((x + VIEW_RADIUS) / FRAME_SIZE));
}

int GameManager::GetSquareDistance(unsigned short x1, unsigned short y1
	, unsigned short x2, unsigned short y2)
{
	return ((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

void GameManager::MoveNPC(unsigned short id)
{
	if (false == npcs[id - MAX_PLAYER].is_active) return;

	switch (npcs[id - MAX_PLAYER].kind)
	{
	case KIND::MILD_MONSTER:
		MildMonsterMove(id);
		return;
	case KIND::AGGR_MONSTER:
		AgrressiveMonsterMove(id);
		return;
	case KIND::FISH:
		FishMove(id);
		return;
	case KIND::NEUTRAL_MONSTER:
		lua_getglobal(npcs[id - MAX_PLAYER].L, "event_move");
		lua_pcall(npcs[id - MAX_PLAYER].L, 0, 0, 0);
		return;
	default:
		cout << "monster kind error" << endl;
		return;
	}
}

void GameManager::MildMonsterMove(unsigned short id)
{
	unsigned short x = npcs[id - MAX_PLAYER].x, y = npcs[id - MAX_PLAYER].y;
	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x - 1, y);
	GetViewSectors(view_sectors, x + 1, y);
	GetViewSectors(view_sectors, x, y + 1);
	GetViewSectors(view_sectors, x, y - 1);

	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}

	if (npcs[id - MAX_PLAYER].state == MONSTER_STATE::AGGRESSIVE) {
		unsigned short target = npcs[id - MAX_PLAYER].target;
		unsigned short tx = players[target].x, ty = players[target].y;
		if (IsInView(x, y, tx, ty) && players[target].hp != 0) {
			if (AttackPlayer(id, target) == false)
				ChasingMove(id, target);
			for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
				sectors_mutex[*rit].unlock();
			}
			return;
		}
		npcs[id - MAX_PLAYER].state = MONSTER_STATE::IDLE;
	}

	IdleMove(id);

	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}
}

unsigned short GameManager::GetMostClosePlayer(set<unsigned short>& view_sectors
	, unsigned short x, unsigned short y)
{
	int close_distance = 0x7fffffff;
	unsigned short close_player = MAX_PLAYER;
	for (auto s : view_sectors) {
		for (auto i : sectors[s]) {
			if (i >= MAX_PLAYER) continue;
			short tx = players[i].x;
			short ty = players[i].y;
			if ((x - VIEW_RADIUS - 1 <= tx && tx <= x + VIEW_RADIUS + 1)
				&& (y - VIEW_RADIUS - 1 <= ty && ty <= y + VIEW_RADIUS + 1)
				&& (map[ty][tx] == 1 || map[ty][tx] == 3) && (players[i].hp != 0)) {
				int d = GetSquareDistance(x, y, tx, ty);
				if (d < close_distance) {
					close_distance = d;
					close_player = i;
				}
				continue;
			}
		}
	}
	return close_player;
}

void GameManager::AgrressiveMonsterMove(unsigned short id)
{
	unsigned short x = npcs[id - MAX_PLAYER].x, y = npcs[id - MAX_PLAYER].y;
	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x - 1, y);
	GetViewSectors(view_sectors, x + 1, y);
	GetViewSectors(view_sectors, x, y + 1);
	GetViewSectors(view_sectors, x, y - 1);

	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}

	unsigned short close_player = GetMostClosePlayer(view_sectors ,x ,y);
	if (close_player != MAX_PLAYER ) {
		// 공격할 수 있으면
		if (AttackPlayer(id, close_player) == false)
			ChasingMove(id, close_player);
		for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
			sectors_mutex[*rit].unlock();
		}
		return;
	}
	IdleMove(id);

	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}
}

void GameManager::FishMove(unsigned short id)
{
	short dir[4][2] = { { 1, 0 },{ -1, 0 },{ 0, 1 },{ 0, -1 } };
	const unsigned short x = npcs[id - MAX_PLAYER].x;
	const unsigned short y = npcs[id - MAX_PLAYER].y;
	byte dir_idx = npcs[id - MAX_PLAYER].dir;

	short new_x = x + dir[dir_idx][0];
	short new_y = y + dir[dir_idx][1];

	if (map[new_y][new_x] != 2) {
		dir_idx = dir_idx + (1 - 2 * (dir_idx % 2));
		npcs[id - MAX_PLAYER].dir = dir_idx;
		new_x = x + dir[dir_idx][0];
		new_y = y + dir[dir_idx][1];
	}

	int r_dir = rand() % 4;
	if (r_dir != npcs[id - MAX_PLAYER].dir
		&& (map[y + dir[r_dir][1]][x + dir[r_dir][0]] == 2))
	{
		npcs[id - MAX_PLAYER].dir = r_dir;
		new_x = x + dir[r_dir][0];
		new_y = y + dir[r_dir][1];
	}


	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x, y);
	GetViewSectors(view_sectors, new_x, new_y);
	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}
	npcs[id - MAX_PLAYER].x = new_x; npcs[id - MAX_PLAYER].y = new_y;

	int nViewed = 0;
	for (auto s : view_sectors) {
		for (auto i : sectors[s]) {
			if (i >= MAX_PLAYER) continue;
			short tx = players[i].x;
			short ty = players[i].y;
			if (((new_x - VIEW_RADIUS - 1 <= tx && tx <= new_x + VIEW_RADIUS + 1)
				&& (new_y - VIEW_RADIUS - 1 <= ty && ty <= new_y + VIEW_RADIUS + 1))) {
				if (players[i].view_list.find(id)
					!= players[i].view_list.end()) {
					SendPacket(S2C::POSITION_INFO, i, id);
				}
				else
				{
					players[i].view_list.insert(id);
					SendPacket(S2C::ADD_OBJECT, i, id);
				}
				nViewed += 1;
				continue;
			}
			if (players[i].view_list.find(id)
				!= players[i].view_list.end()) {
				players[i].view_list.erase(id);
				SendPacket(S2C::REMOVE_OBJECT, i, id);
			}

		}
	}

	if (x / FRAME_SIZE != new_x / FRAME_SIZE
		|| y / FRAME_SIZE != new_y / FRAME_SIZE) {
		sectors[(y / FRAME_SIZE)*SECTOR_COL_SIZE + (x / FRAME_SIZE)].erase(id);
		sectors[(new_y / FRAME_SIZE)*SECTOR_COL_SIZE + (new_x / FRAME_SIZE)].insert(id);
	}

	if (nViewed == 0) {
		--(npcs[id - MAX_PLAYER].cur_add_v);
		if (0 == npcs[id - MAX_PLAYER].cur_add_v) {
			npcs[id - MAX_PLAYER].cur_add_v = npcs[id - MAX_PLAYER].add_v;
			npcs[id - MAX_PLAYER].is_active = false;
			for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
				sectors_mutex[*rit].unlock();
			}
			return;
		}
	}

	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}
	event_q.enq(id, COMMAND::MOVE_CMD, MOVE_TERM);
}

void GameManager::IdleMove(unsigned short id)
{
	short dir[4][2] = { { 0, -1 },{ 1, 0 },{ 0, 1 },{ -1, 0 } };
	const unsigned short x = npcs[id - MAX_PLAYER].x;
	const unsigned short y = npcs[id - MAX_PLAYER].y;
	byte dir_idx = npcs[id - MAX_PLAYER].dir;

	short new_x = x + dir[dir_idx][0];
	short new_y = y + dir[dir_idx][1];

	if (map[new_y][new_x] != 1 && map[new_y][new_x] != 3) {
		dir_idx = dir_idx + (1 - (2 * (dir_idx / 2))) * 2;
		npcs[id - MAX_PLAYER].dir = dir_idx;
		new_x = x + dir[dir_idx][0];
		new_y = y + dir[dir_idx][1];
	}
	if (rand() % 5 == 0) {
		int r_dir = rand() % 4;
		if (r_dir != npcs[id - MAX_PLAYER].dir
			&& (map[y + dir[r_dir][1]][x + dir[r_dir][0]] == 1
				|| map[y + dir[r_dir][1]][x + dir[r_dir][0]] == 3))
		{
			npcs[id - MAX_PLAYER].dir = r_dir;
			new_x = x + dir[r_dir][0];
			new_y = y + dir[r_dir][1];
		}
	}

	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x, y);
	GetViewSectors(view_sectors, new_x, new_y);

	npcs[id - MAX_PLAYER].x = new_x; npcs[id - MAX_PLAYER].y = new_y;
	int nViewed = 0;
	for (auto s : view_sectors) {
		for (auto i : sectors[s]) {
			if (i >= MAX_PLAYER) continue;
			short tx = players[i].x;
			short ty = players[i].y;
			if (((new_x - VIEW_RADIUS - 1 <= tx && tx <= new_x + VIEW_RADIUS + 1)
				&& (new_y - VIEW_RADIUS - 1 <= ty && ty <= new_y + VIEW_RADIUS + 1))) {
				if (players[i].view_list.find(id)
					!= players[i].view_list.end()) {
					SendPacket(S2C::POSITION_INFO, i, id);
				}
				else
				{
					players[i].view_list.insert(id);
					SendPacket(S2C::ADD_OBJECT, i, id);
				}
				nViewed += 1;
				continue;
			}
			if (players[i].view_list.find(id)
				!= players[i].view_list.end()) {
				players[i].view_list.erase(id);
				SendPacket(S2C::REMOVE_OBJECT, i, id);
			}

		}
	}

	if (x / FRAME_SIZE != new_x / FRAME_SIZE
		|| y / FRAME_SIZE != new_y / FRAME_SIZE) {
		sectors[(y / FRAME_SIZE)*SECTOR_COL_SIZE + (x / FRAME_SIZE)].erase(id);
		sectors[(new_y / FRAME_SIZE)*SECTOR_COL_SIZE + (new_x / FRAME_SIZE)].insert(id);
	}

	if (nViewed == 0) {
		--(npcs[id - MAX_PLAYER].cur_add_v);
		if (0 == npcs[id - MAX_PLAYER].cur_add_v) {
			npcs[id - MAX_PLAYER].cur_add_v = npcs[id - MAX_PLAYER].add_v;
			npcs[id - MAX_PLAYER].is_active = false;
			return;
		}
	}

	// event 등록
	event_q.enq(id, COMMAND::MOVE_CMD, MOVE_TERM);
}

void GameManager::ChasingMove(unsigned short id, unsigned short target)
{
	unsigned short x = npcs[id - MAX_PLAYER].x, y = npcs[id - MAX_PLAYER].y;
	unsigned short tx = players[target].x, ty = players[target].y;
	short dir[4][2] = { { 0, -1 },{ 1, 0 },{ 0, 1 },{ -1, 0 } };

	if (npcs[id - MAX_PLAYER].target != target) {
		npcs[id - MAX_PLAYER].target = target;
		if (x <= tx) npcs[id - MAX_PLAYER].dir = 1;
		else npcs[id - MAX_PLAYER].dir = 3;
	}

	// 방향정하기
	if (true == npcs[id - MAX_PLAYER].set_x) {
		if (x == tx) {
			npcs[id - MAX_PLAYER].set_x = false;
			if (y <= ty) npcs[id - MAX_PLAYER].dir = 2;
			else npcs[id - MAX_PLAYER].dir = 0;
		}
		else {
			byte c_dir = npcs[id - MAX_PLAYER].dir;
			byte s_dir;
			if (x <= tx) s_dir = 1;
			else s_dir = 3;

			if (s_dir == c_dir)
			{
				if (map[y + dir[c_dir][1]][x + dir[c_dir][0]] != 1
					&& map[y + dir[c_dir][1]][x + dir[c_dir][0]] != 3) {
					for (int i = 0; i < 3; ++i) {
						c_dir = (c_dir + 1) % 4;
						if (map[y + dir[c_dir][1]][x + dir[c_dir][0]] == 1
							|| map[y + dir[c_dir][1]][x + dir[c_dir][0]] == 3)
							break;
					}
				}
			}
			else
			{
				if (c_dir == 0) s_dir = 3;
				else s_dir = c_dir - 1;
				
				if (map[y + dir[s_dir][1]][x + dir[s_dir][0]] == 1
					|| map[y + dir[s_dir][1]][x + dir[s_dir][0]] == 3)
					c_dir = s_dir;
				else
				{
					if (map[y + dir[c_dir][1]][x + dir[c_dir][0]] != 1
						&& map[y + dir[c_dir][1]][x + dir[c_dir][0]] != 3) {
						for (int i = 0; i < 3; ++i) {
							c_dir = (c_dir + 1) % 4;
							if (map[y + dir[c_dir][1]][x + dir[c_dir][0]] == 1
								|| map[y + dir[c_dir][1]][x + dir[c_dir][0]] == 3)
								break;
						}
					}
				}
			}
			npcs[id - MAX_PLAYER].dir = c_dir;
		}
	}
	if (false == npcs[id - MAX_PLAYER].set_x) {
		if (ty == y) {
			npcs[id - MAX_PLAYER].set_x = true;
			if (x <= tx) npcs[id - MAX_PLAYER].dir = 1;
			else npcs[id - MAX_PLAYER].dir = 3;
			event_q.enq(id, COMMAND::MOVE_CMD, MOVE_TERM);
			return;
		}
		else {
			byte c_dir = npcs[id - MAX_PLAYER].dir;
			byte s_dir;
			if (y <= ty) s_dir = 2;
			else s_dir = 0;

			if (s_dir == c_dir)
			{
				if (map[y + dir[c_dir][1]][x + dir[c_dir][0]] != 1
					&& map[y + dir[c_dir][1]][x + dir[c_dir][0]] != 3) {
					for (int i = 0; i < 3; ++i) {
						c_dir = (c_dir + 1) % 4;
						if (map[y + dir[c_dir][1]][x + dir[c_dir][0]] == 1
							|| map[y + dir[c_dir][1]][x + dir[c_dir][0]] == 3)
							break;
					}
				}
			}
			else
			{
				if (c_dir == 0) s_dir = 3;
				else s_dir = c_dir - 1;

				if (map[y + dir[s_dir][1]][x + dir[s_dir][0]] == 1
					|| map[y + dir[s_dir][1]][x + dir[s_dir][0]] == 3)
					c_dir = s_dir;
				else
				{
					if (map[y + dir[c_dir][1]][x + dir[c_dir][0]] != 1
						&& map[y + dir[c_dir][1]][x + dir[c_dir][0]] != 3) {
						for (int i = 0; i < 3; ++i) {
							c_dir = (c_dir + 1) % 4;
							if (map[y + dir[c_dir][1]][x + dir[c_dir][0]] == 1
								|| map[y + dir[c_dir][1]][x + dir[c_dir][0]] == 3)
								break;
						}
					}
				}
			}
			npcs[id - MAX_PLAYER].dir = c_dir;
		}
	}
	
	// 이동
	unsigned short new_x = x + dir[npcs[id - MAX_PLAYER].dir][0];
	unsigned short new_y = y + dir[npcs[id - MAX_PLAYER].dir][1];
	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x, y);
	GetViewSectors(view_sectors, new_x, new_y);

	npcs[id - MAX_PLAYER].x = new_x; npcs[id - MAX_PLAYER].y = new_y;
	for (auto s : view_sectors) {
		for (auto i : sectors[s]) {
			if (i >= MAX_PLAYER) continue;
			if (((new_x - VIEW_RADIUS - 1 <= tx && tx <= new_x + VIEW_RADIUS + 1)
				&& (new_y - VIEW_RADIUS - 1 <= ty && ty <= new_y + VIEW_RADIUS + 1))) {
				if (players[i].view_list.find(id)
					!= players[i].view_list.end()) {
					SendPacket(S2C::POSITION_INFO, i, id);
				}
				else
				{
					players[i].view_list.insert(id);
					SendPacket(S2C::ADD_OBJECT, i, id);
				}
				continue;
			}
			if (players[i].view_list.find(id)
				!= players[i].view_list.end()) {
				players[i].view_list.erase(id);
				SendPacket(S2C::REMOVE_OBJECT, i, id);
			}

		}
	}

	if (x / FRAME_SIZE != new_x / FRAME_SIZE
		|| y / FRAME_SIZE != new_y / FRAME_SIZE) {
		sectors[(y / FRAME_SIZE)*SECTOR_COL_SIZE + (x / FRAME_SIZE)].erase(id);
		sectors[(new_y / FRAME_SIZE)*SECTOR_COL_SIZE + (new_x / FRAME_SIZE)].insert(id);
	}

	// event 등록
	event_q.enq(id, COMMAND::MOVE_CMD, MOVE_TERM);
}

char GameManager::OnAstar(unsigned short id, unsigned short target)
{
	unsigned short cx = npcs[id - MAX_PLAYER].x;
	unsigned short cy = npcs[id - MAX_PLAYER].y;
	unsigned short target_x = players[target].x;
	unsigned short target_y = players[target].y;
	
	short dx = cx - (FIND_MAP_SIZE / 2);
	short dy = cy - (FIND_MAP_SIZE / 2);

	byte x = cx - dx;
	byte y = cy - dy;
	byte tx = target_x - dx;
	byte ty = target_y - dy;

	char visited[FIND_MAP_SIZE][2];
	for (int i = 0; i < FIND_MAP_SIZE; ++i) {
		visited[i][0] = 0;
		visited[i][1] = 0;
	}
	visited[y][x / 8] |= (1 << (x % 8));

	PriorityQueue queue;
	if(map[cy][cx + 1] == 1 || map[cy][cx + 1] == 3)
		queue.Enq(1 + GetSquareDistance(x + 1, y, tx, ty), 1
			,y * FIND_MAP_SIZE + (x + 1), y * FIND_MAP_SIZE + (x + 1));
	visited[y][(x + 1) / 8] |= (1 << ((x + 1) % 8));
	if (map[cy][cx - 1] == 1 || map[cy][cx - 1] == 3)
		queue.Enq(1 + GetSquareDistance(x - 1, y, tx, ty), 1
			, y * FIND_MAP_SIZE + (x - 1), y * FIND_MAP_SIZE + (x - 1));
	visited[y][(x - 1) / 8] |= (1 << ((x - 1) % 8));
	if (map[cy + 1][cx] == 1 || map[cy + 1][cx] == 3)
		queue.Enq(1 + GetSquareDistance(x, y + 1, tx, ty), 1
			, (y + 1) * FIND_MAP_SIZE + x, (y + 1) * FIND_MAP_SIZE + x);
	visited[y + 1][x / 8] |= (1 << (x % 8));
	if (map[cy - 1][cx] == 1 || map[cy - 1][cx] == 3)
		queue.Enq(1 + GetSquareDistance(x, y - 1, tx, ty), 1
			, (y - 1) * FIND_MAP_SIZE + x, (y - 1) * FIND_MAP_SIZE + x);
	visited[y - 1][x / 8] |= (1 << (x % 8));

	while (true)
	{
		AstarNode u = queue.Deq();
		if (u.pathlen > FIND_MAP_SIZE * 2)return -1;
		if (u.value == -1) return -1;	// 길이 없는 경우
		int ux = u.index % FIND_MAP_SIZE;
		int uy = u.index / FIND_MAP_SIZE;
		if (ux == tx && uy == ty) {
			unsigned short fx = u.from % FIND_MAP_SIZE;
			unsigned short fy = u.from / FIND_MAP_SIZE;
			if (fy > y)return 1;
			if (fx > x)return 2;
			if (fy < y)return 3;
			return 4;
		}
		const short d[][2] = { 0, -1, 1, 0, 0, 1, -1, 0 };
		for (int i = 0; i < 4; ++i) {
			byte nx = ux + d[i][0];
			byte ny = uy + d[i][1];
			if (nx < 0 || nx >= FIND_MAP_SIZE || ny < 0 || ny >= FIND_MAP_SIZE) continue;
			if (nx + dx < 0 || nx + dx >= BOARD_WIDTH || ny + dy < 0 || ny + dy >= BOARD_HEIGHT) continue;
			if (map[ny + dy][nx + dx] != 1 && map[ny + dy][nx + dx] != 3) continue;
			if (0 != (visited[ny][nx / 8] & (1 << (nx % 8)))) continue;
			visited[ny][nx / 8] |= (1 << (nx % 8));
			queue.Enq(u.pathlen + 1 + GetSquareDistance(nx, ny, tx, ty)
				, u.pathlen + 1, ny * FIND_MAP_SIZE + nx, u.from);
		}
	}
}

bool GameManager::AttackPlayer(unsigned short id, unsigned short target)
{
	// 공격할 수 있으면
	unsigned short x = npcs[id - MAX_PLAYER].x, y = npcs[id - MAX_PLAYER].y;
	unsigned short tx = players[target].x, ty = players[target].y;
	if (KIND::AGGR_MONSTER == npcs[id - MAX_PLAYER].kind) {
		int d = GetSquareDistance(x, y, tx, ty);
		if (d <= 2) {
			MakeNPCSkill(id, target);
			npcs[id - MAX_PLAYER].set_x = true;
			event_q.enq(id, COMMAND::MOVE_CMD, MOVE_TERM);
			return true;
		}
	}
	else {
		if ((x == tx && y == ty)) {
			MakeNPCSkill(id, target);
			npcs[id - MAX_PLAYER].set_x = true;
			//npcs[id - MAX_PLAYER].target = MAX_PLAYER;
			event_q.enq(id, COMMAND::MOVE_CMD, MOVE_TERM);
			return true;
		}
	}
	return false;
}

bool GameManager::IsInView(unsigned short x, unsigned short y, unsigned tx, unsigned short ty)
{
	if ((x - VIEW_RADIUS - 1 <= tx && tx <= x + VIEW_RADIUS + 1)
		&& (y - VIEW_RADIUS - 1 <= ty && ty <= y + VIEW_RADIUS + 1)) {
		return true;
	}
	return false;
}

void GameManager::MakeNPCSkill(unsigned short id, unsigned short target)
{
	unsigned short target_x = players[target].x, target_y = players[target].y;

	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, target_x, target_y);

	set<unsigned short> viewers;

	for (auto s : view_sectors) {
		for (auto i : sectors[s]) {
			if (i < MAX_PLAYER) {
				short tx, ty;
				tx = players[i].x;
				ty = players[i].y;
				if ((tx - VIEW_RADIUS <= target_x && target_x <= tx + VIEW_RADIUS)
						&& (ty - VIEW_RADIUS <= target_y && target_y <= ty + VIEW_RADIUS))
				{
					viewers.insert(i);
				}
			}
		}
	}
	
	short new_hp;
	int devide = 1;
	if (players[target].durability > 0) {
		devide = 3;
		--players[target].durability;
	}
	if(npcs[id - MAX_PLAYER].kind == KIND::AGGR_MONSTER)
		new_hp = players[target].hp - (MONSTER_L_SKILL_DAMAGE / devide);
	else
		new_hp = players[target].hp - (MONSTER_S_SKILL_DAMAGE / devide);

	if (new_hp < 0) new_hp = 0;
	players[target].hp = new_hp;
	SendPacket(S2C::STAT_CHANGE, target, target);

	if (new_hp <= 0) {
		for (auto i : players[target].view_list) {
			if (i < MAX_PLAYER) {
				SendPacket(S2C::STAT_CHANGE, i, target);
			}
		}
		// 부활 타이머 등록
		event_q.enq(target, COMMAND::REVIVE_CMD, REVIVE_TERM);
	}

	// effect pakcet 보내기
	for (auto i : viewers)
		SendEffectPacket(i, target_x, target_y, SKILL::NPC_SKILL);
}

void GameManager::RecordStat(unsigned short id, bool deal_record)
{
	unsigned short x = players[id].x; unsigned short y = players[id].y;
	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x, y);
	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}
	char cmd = DB_COMMAND::UPDATE_PLAYER_POS;
	if (true == deal_record) cmd = DB_COMMAND::UPDATE_DEAL_DATA;
	db_event_q.EnqUpdatePlayerStatCmd(cmd, 0, players[id].name
		, players[id].x, players[id].y, players[id].hp, players[id].level, players[id].exp
		, players[id].nFish, players[id].nStone, players[id].nPotion, players[id].nArmor);
	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}
}

void GameManager::CheckPlayer(unsigned short id, unsigned short target)
{
	lua_getglobal(npcs[id - MAX_PLAYER].L, "event_player_move");
	lua_pushnumber(npcs[id - MAX_PLAYER].L, target);
	lua_pcall(npcs[id - MAX_PLAYER].L, 1, 0, 0);
}

unsigned short GameManager::GetX(unsigned short id)
{
	unsigned short x;
	if (id < MAX_PLAYER) x = players[id].x;
	else x = npcs[id - MAX_PLAYER].x;
	return x;
}

unsigned short GameManager::GetY(unsigned short id)
{
	unsigned short y;
	if (id < MAX_PLAYER) y = players[id].y;
	else y = npcs[id - MAX_PLAYER].y;
	return y;
}

void GameManager::NeutralNPCMove(unsigned short id)
{
	if (npcs[id - MAX_PLAYER].kind != KIND::NEUTRAL_MONSTER) return;

	short dir[4][2] = { { 0, -1 },{ 1, 0 },{ 0, 1 },{ -1, 0 } };
	const unsigned short x = npcs[id - MAX_PLAYER].x;
	const unsigned short y = npcs[id - MAX_PLAYER].y;
	byte dir_idx = npcs[id - MAX_PLAYER].dir;

	short new_x = x + dir[dir_idx][0];
	short new_y = y + dir[dir_idx][1];

	if (map[new_y][new_x] != 1 && map[new_y][new_x] != 3) {
		for (int i = 0; i < 3; ++i) {
			dir_idx = (dir_idx + 1) % 4;
			if ((map[y + dir[dir_idx][1]][x + dir[dir_idx][0]] == 1
				|| map[y + dir[dir_idx][1]][x + dir[dir_idx][0]] == 3))
				break;
		}
		npcs[id - MAX_PLAYER].dir = dir_idx;
		new_x = x + dir[dir_idx][0];
		new_y = y + dir[dir_idx][1];
	}

	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x, y);
	GetViewSectors(view_sectors, new_x, new_y);

	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}

	npcs[id - MAX_PLAYER].x = new_x; npcs[id - MAX_PLAYER].y = new_y;

	for (auto s : view_sectors) {
		for (auto i : sectors[s]) {
			if (i >= MAX_PLAYER) continue;
			short tx = players[i].x;
			short ty = players[i].y;
			if (((new_x - VIEW_RADIUS - 1 <= tx && tx <= new_x + VIEW_RADIUS + 1)
				&& (new_y - VIEW_RADIUS - 1 <= ty && ty <= new_y + VIEW_RADIUS + 1))) {
				if (players[i].view_list.find(id)
					!= players[i].view_list.end()) {
					SendPacket(S2C::POSITION_INFO, i, id);
				}
				else
				{
					players[i].view_list.insert(id);
					SendPacket(S2C::ADD_OBJECT, i, id);
				}
				continue;
			}
			if (players[i].view_list.find(id)
				!= players[i].view_list.end()) {
				players[i].view_list.erase(id);
				SendPacket(S2C::REMOVE_OBJECT, i, id);
			}

		}
	}

	if (x / FRAME_SIZE != new_x / FRAME_SIZE
		|| y / FRAME_SIZE != new_y / FRAME_SIZE) {
		sectors[(y / FRAME_SIZE)*SECTOR_COL_SIZE + (x / FRAME_SIZE)].erase(id);
		sectors[(new_y / FRAME_SIZE)*SECTOR_COL_SIZE + (new_x / FRAME_SIZE)].insert(id);
	}

	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}
	
}

void GameManager::SetNeutralNPCDir(unsigned short id, byte dir)
{
	if (npcs[id - MAX_PLAYER].kind == KIND::NEUTRAL_MONSTER)
		npcs[id - MAX_PLAYER].dir = dir;
}

void GameManager::ActivateSkill(unsigned short id, char type)
{
	if (id < MAX_PLAYER) {
		switch (type)
		{
		case SKILL::PLAYER_STD_SKILL:
			players[id].std_skill_ready = true;
			break;
		case SKILL::PLAYER_CROSS_SKILL:
			players[id].cross_skill_ready = true;
			break;
		case  SKILL::PLAYER_ULTIMATE_SKILL:
			players[id].ultimate_skill_ready = true;
			break;
		default:
			break;
		}
	}
	else
	{

	}
}

void GameManager::ReviveNPC(unsigned short id)
{
	std::cout << id << ",  npc revive" << endl;
	
	// 데이터 초기화
	npcs[id - MAX_PLAYER].is_active = false;
	npcs[id - MAX_PLAYER].set_x = true;
	npcs[id - MAX_PLAYER].target = MAX_PLAYER;
	npcs[id - MAX_PLAYER].state = MONSTER_STATE::IDLE;
	npcs[id - MAX_PLAYER].hp = default_HP;
	

	unsigned short x = npcs[id - MAX_PLAYER].x;
	unsigned short y = npcs[id - MAX_PLAYER].y;
	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, x, y);

	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}

	sectors[(y / FRAME_SIZE)*SECTOR_COL_SIZE + (x / FRAME_SIZE)].insert(npcs[id - MAX_PLAYER].id);

	int nViewed = 0;
	for (auto s : view_sectors) {
		for (auto i : sectors[s])
		{
			if (i >= MAX_PLAYER) continue;

			unsigned short tx, ty;
			tx = players[i].x; ty = players[i].y;
	
			if ((tx - VIEW_RADIUS <= x && x <= tx + VIEW_RADIUS)
				&& (ty - VIEW_RADIUS <= y && y <= ty + VIEW_RADIUS)) {
				players[i].view_list.insert(id);
				SendPacket(S2C::ADD_OBJECT, i, id);
				++nViewed;
			}
		}
	}

	if (0 != nViewed) {
		npcs[id - MAX_PLAYER].is_active = true;
		event_q.enq(id, COMMAND::MOVE_CMD, MOVE_TERM);
	}


	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}
}

void GameManager::RevivePlayer(unsigned short id)
{
	set<unsigned short> view_sectors;
	GetViewSectors(view_sectors, players[id].x, players[id].y);

	for (auto it = view_sectors.begin(); it != view_sectors.end(); ++it) {
		sectors_mutex[*it].lock();
	}

	players[id].hp = default_HP;
	SendPacket(S2C::STAT_CHANGE, id, id);
	for (auto i : players[id].view_list) {
		if (i < MAX_PLAYER) {
			SendPacket(S2C::STAT_CHANGE, i, id);
		}
	}

	for (auto rit = view_sectors.rbegin(); rit != view_sectors.rend(); ++rit) {
		sectors_mutex[*rit].unlock();
	}
}

#include "network.h"
#include "resource.h"


HINSTANCE	hInst;
LPCWSTR lpszClass = L"client";
HDC hMemDC;					// 더블 버퍼링 위한 메모리 디바이스 컨텍스트
//WCHAR lpszIP[40];
WCHAR lpszID[11];
DWORD ip_addr;
HWND gHwnd;
byte map[BOARD_HEIGHT][BOARD_WIDTH];

bool init_instance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DialogProcStore(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

mutex m;
thread *recv_t;
unsigned short myid = MAX_PLAYER;
unsigned short myHP;
byte myLEVEL;
unsigned long myEXP;
unsigned short nFish, nStone, nPotion, nArmor;
unsigned short nDurability = 0;
std::unordered_map<unsigned short, Character *> characters;
std::unordered_map<WPARAM, EFFECT *> effects;

SOCKET hSock = INVALID_SOCKET;

char recv_buf[MAX_BUF_SIZE];
char recv_packet_buf[MAX_PACKET_SIZE];
unsigned short remain_packet_size;

WCHAR msg_buf[MAX_CHAT_SIZE + 1];

void OnCreate(HWND);		// WM_CREATE 핸들러 함수
void OnDestroy();			// WM_DESTORY 핸들러 함수
void OnPaint(HWND);			// WM_TIMER 핸들러 함수
void Draw(HWND);



int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance
	, LPSTR lpszCmdParam, int nCmdShow)
{
	msg_buf[0] = 0;
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = lpszClass;
	wcex.lpszMenuName = NULL;
	wcex.hIconSm = NULL;

	RegisterClassExW(&wcex);

	if (!init_instance(hInstance, nCmdShow))
	{
		return FALSE;
	}
	MSG Message;

	while (GetMessage(&Message, NULL, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return (int)Message.wParam;
}


bool init_instance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	RECT rt = { 0, 0, FRAME_SIZE * CELL_SIZE, FRAME_SIZE * CELL_SIZE };
	AdjustWindowRect(&rt, WS_OVERLAPPEDWINDOW, TRUE);

	HWND hWnd = CreateWindowW(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, rt.right - rt.left, rt.bottom - rt.top, NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}





void OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	BitBlt(hdc, 0, 0, FRAME_SIZE * CELL_SIZE, FRAME_SIZE * CELL_SIZE, hMemDC, 0, 0, SRCCOPY);
	EndPaint(hWnd, &ps);
}

void DrawPlayerStdSkillEffect(HWND hWnd, unsigned short cx, unsigned short cy
	, unsigned short x, unsigned short y, char d) 
{
	short dir[4][2] = { { 0, -1 },{ 1, 0 },{ 0, 1 },{ -1, 0 } };
	unsigned short end_x = x + dir[d][0];
	unsigned short end_y = y + dir[d][1];

	if (map[end_y][end_x] == 2 || map[end_y][end_x] == 4
		|| map[end_y][end_x] == 5 || map[end_y][end_x] == 6) {
		end_x = x; end_y = y;
	}

	HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 252));
	HGDIOBJ hOldBrush = SelectObject(hMemDC, hBrush);
	HGDIOBJ hOldPen = SelectObject(hMemDC, GetStockObject(NULL_PEN));

	short dx = x - cx; short dy = y - cy;

	Rectangle(hMemDC, dx * CELL_SIZE, dy * CELL_SIZE, dx * CELL_SIZE + CELL_SIZE, dy * CELL_SIZE + CELL_SIZE);
	dx = end_x - cx;
	dy = end_y - cy;
	Rectangle(hMemDC, dx * CELL_SIZE, dy * CELL_SIZE, dx * CELL_SIZE + CELL_SIZE, dy * CELL_SIZE + CELL_SIZE);


	SelectObject(hMemDC, hOldBrush);
	DeleteObject(hBrush);
	SelectObject(hMemDC, hOldPen);

}

void DrawPlayerCrossSkillEffect(HWND hWnd, unsigned short cx, unsigned short cy
	, unsigned short x, unsigned short y)
{
	short dir[4][2] = { { 0, -1 },{ 1, 0 },{ 0, 1 },{ -1, 0 } };
	HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 252));
	HGDIOBJ hOldBrush = SelectObject(hMemDC, hBrush);
	HGDIOBJ hOldPen = SelectObject(hMemDC, GetStockObject(NULL_PEN));

	short dx = x - cx; short dy = y - cy;

	Rectangle(hMemDC, dx * CELL_SIZE, dy * CELL_SIZE, dx * CELL_SIZE + CELL_SIZE, dy * CELL_SIZE + CELL_SIZE);

	for (int i = 0; i < 4; ++i) {
		unsigned short nx = x + dir[i][0];
		unsigned short ny = y + dir[i][1];
		if (map[ny][nx] == 1 || map[ny][nx] == 3
			|| map[ny][nx] == 7 || map[ny][nx] == 8) {
			dx = nx - cx;
			dy = ny - cy;
			Rectangle(hMemDC, dx * CELL_SIZE, dy * CELL_SIZE, dx * CELL_SIZE + CELL_SIZE, dy * CELL_SIZE + CELL_SIZE);
		}
	}
	
	SelectObject(hMemDC, hOldBrush);
	DeleteObject(hBrush);
	SelectObject(hMemDC, hOldPen);
}

void DrawPlayerUltimateSkillEffect(HWND hWnd, unsigned short cx, unsigned short cy
	, unsigned short x, unsigned short y)
{
	short dir[12][2] = { { 0, -2 },{ 1, -1 },{ 2, 0 },{ 1, 1 },{ 0, 2 },{ -1, 1 },{ -2, 0 },{ -1, -1 }
	,{ 0, -1 },{ 1, 0 },{ 0, 1 },{ -1, 0 } };
	HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 252));
	HGDIOBJ hOldBrush = SelectObject(hMemDC, hBrush);
	HGDIOBJ hOldPen = SelectObject(hMemDC, GetStockObject(NULL_PEN));

	short dx = x - cx; short dy = y - cy;

	Rectangle(hMemDC, dx * CELL_SIZE, dy * CELL_SIZE, dx * CELL_SIZE + CELL_SIZE, dy * CELL_SIZE + CELL_SIZE);

	for (int i = 0; i < 12; ++i) {
		unsigned short nx = x + dir[i][0];
		unsigned short ny = y + dir[i][1];
		if (map[ny][nx] == 1 || map[ny][nx] == 3
			|| map[ny][nx] == 7 || map[ny][nx] == 8) {
			dx = nx - cx;
			dy = ny - cy;
			Rectangle(hMemDC, dx * CELL_SIZE, dy * CELL_SIZE, dx * CELL_SIZE + CELL_SIZE, dy * CELL_SIZE + CELL_SIZE);
		}
	}

	SelectObject(hMemDC, hOldBrush);
	DeleteObject(hBrush);
	SelectObject(hMemDC, hOldPen);
}

void DrawNPCSkillEffect(HWND hWnd, unsigned short cx, unsigned short cy
	, unsigned short x, unsigned short y)
{
	// kind에 따라 색 다르게

	HBRUSH hBrush = CreateSolidBrush(RGB(255, 134, 86));
	
	HGDIOBJ hOldBrush = SelectObject(hMemDC, hBrush);
	HGDIOBJ hOldPen = SelectObject(hMemDC, GetStockObject(NULL_PEN));

	short dx = x - cx; short dy = y - cy;

	Rectangle(hMemDC, dx * CELL_SIZE, dy * CELL_SIZE, dx * CELL_SIZE + CELL_SIZE, dy * CELL_SIZE + CELL_SIZE);

	SelectObject(hMemDC, hOldBrush);
	DeleteObject(hBrush);
	SelectObject(hMemDC, hOldPen);
}

void DrawEffects(HWND hWnd, unsigned short cx, unsigned short cy)
{
	for (auto it : effects) {
		switch (it.second->type)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			DrawPlayerStdSkillEffect(hWnd, cx, cy, it.second->x, it.second->y, it.second->type);
			break;
		case SKILL::PLAYER_CROSS_SKILL:
			DrawPlayerCrossSkillEffect(hWnd, cx, cy, it.second->x, it.second->y);
			break;
		case SKILL::PLAYER_ULTIMATE_SKILL:
			DrawPlayerUltimateSkillEffect(hWnd, cx, cy, it.second->x, it.second->y);
			break;
		case SKILL::NPC_SKILL:
			DrawNPCSkillEffect(hWnd, cx, cy, it.second->x, it.second->y);
			break;
		default:
			break;
		}
	}
}

void DrawRod(unsigned short x, unsigned short y, unsigned short cx, unsigned cy)
{
	// 주변에 물이 있는지 확인
	short dir[4][2] = { { 0, -1 },{ 1, 0 },{ 0, 1 },{ -1, 0 } };

	int d = -1;
	for (int i = 0; i < 4; ++i) {
		if (map[y + dir[i][1]][x + dir[i][0]] == 2) {
			d = i;
			break;
		}
	}
	if (d == -1) return;


	short dx = x - cx; short dy = y - cy;
	unsigned short rod_x = dx + dir[d][0] * 3;
	unsigned short rod_y = dy + dir[d][1] * 3;

	// 직선 그리기
	MoveToEx(hMemDC
		, dx*CELL_SIZE + (CELL_SIZE / 2), dy*CELL_SIZE + (CELL_SIZE / 2), NULL);
	LineTo(hMemDC, dx*CELL_SIZE + (CELL_SIZE / 2) + dir[d][0] * CELL_SIZE * 3
		, dy*CELL_SIZE + (CELL_SIZE / 2) + dir[d][1] * CELL_SIZE * 3);

	// 사각형 그리기
	HBRUSH hBrush = CreateSolidBrush(RGB(191, 0, 14));
	HPEN hPen = CreatePen(PS_DOT, 2, RGB(244, 56, 135));
	HGDIOBJ hOldBrush = SelectObject(hMemDC, hBrush);
	HGDIOBJ hOldPen = SelectObject(hMemDC, hPen);
	Rectangle(hMemDC
		, dx*CELL_SIZE + (CELL_SIZE / 2) + dir[d][0] * CELL_SIZE * 3 - INTERVAL_CUBE_RADIUS
		, dy*CELL_SIZE + (CELL_SIZE / 2) + dir[d][1] * CELL_SIZE * 3 - INTERVAL_CUBE_RADIUS
		, dx*CELL_SIZE + (CELL_SIZE / 2) + dir[d][0] * CELL_SIZE * 3 + INTERVAL_CUBE_RADIUS
		, dy*CELL_SIZE + (CELL_SIZE / 2) + dir[d][1] * CELL_SIZE * 3 + INTERVAL_CUBE_RADIUS);


	SelectObject(hMemDC, hOldBrush);
	DeleteObject(hBrush);
	SelectObject(hMemDC, hOldPen);
	DeleteObject(hPen);
}

void DrawArmor()
{
	if (nDurability > 0 && myHP > 0) {
		HBRUSH hBrush = CreateSolidBrush(RGB(175, 175, 175));
		HGDIOBJ hOldBrush = SelectObject(hMemDC, hBrush);
		HGDIOBJ hOldPen = SelectObject(hMemDC, GetStockObject(NULL_PEN));

		Ellipse(hMemDC
			, (VIEW_RADIUS)*CELL_SIZE + (CELL_SIZE / 2) - ARMOR_RADIUS
			, (VIEW_RADIUS)*CELL_SIZE + (CELL_SIZE / 2) - ARMOR_RADIUS
			, (VIEW_RADIUS)*CELL_SIZE + (CELL_SIZE / 2) + ARMOR_RADIUS
			, (VIEW_RADIUS)*CELL_SIZE + (CELL_SIZE / 2) + ARMOR_RADIUS);


		SelectObject(hMemDC, hOldBrush);
		DeleteObject(hBrush);
		SelectObject(hMemDC, hOldPen);
	}
}

void Draw(HWND hWnd)
{
	//	화면 클리어
	HGDIOBJ a = SelectObject(hMemDC, GetStockObject(WHITE_BRUSH));
	Rectangle(hMemDC, 0, 0, FRAME_SIZE * CELL_SIZE, FRAME_SIZE * CELL_SIZE);

	m.lock();
	if (myid != MAX_PLAYER) {
		//	배경 그리기
		unsigned short x = characters[myid]->x;
		unsigned short y = characters[myid]->y;

		HBRUSH hWaterBrush = CreateSolidBrush(RGB(153, 217, 234));
		HBRUSH hMineBrush = CreateSolidBrush(RGB(195, 195, 195));
		HBRUSH hTreeBrush = CreateSolidBrush(RGB(34, 177, 76));
		HBRUSH hRockBrush = CreateSolidBrush(RGB(127, 127, 127));
		HBRUSH hFenceBrush = CreateSolidBrush(RGB(185, 122, 87));
		HBRUSH hVillageBrush = CreateSolidBrush(RGB(255, 210, 210));
		HBRUSH hStoreBrush = CreateSolidBrush(RGB(255, 201, 14));

		HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 0));
		HGDIOBJ hOldBrush = SelectObject(hMemDC, hBrush);
		HGDIOBJ hOldPen = SelectObject(hMemDC, GetStockObject(NULL_PEN));
		
		int cur_y = y - VIEW_RADIUS;
		for (int i = 0; i < FRAME_SIZE; ++i) {
			int cur_x = x - VIEW_RADIUS;
			for (int j = 0; j < FRAME_SIZE; ++j) {
				if (0 <= cur_x && cur_x <= BOARD_WIDTH - 1 && 0 <= cur_y && cur_y <= BOARD_HEIGHT - 1) {
					if (map[cur_y][cur_x] != 1) {
						switch (map[cur_y][cur_x])
						{
						case 2:
							SelectObject(hMemDC, hWaterBrush);
							break;
						case 3:
							SelectObject(hMemDC, hMineBrush);
							break;
						case 4:
							SelectObject(hMemDC, hTreeBrush);
							break;
						case 5:
							SelectObject(hMemDC, hRockBrush);
							break;
						case 6:
							SelectObject(hMemDC, hFenceBrush);
							break;
						case 7:
							SelectObject(hMemDC, hVillageBrush);
							break;
						case 8:
							SelectObject(hMemDC, hStoreBrush);
							break;
						default:
							SelectObject(hMemDC, hBrush);
							break;
						}
						Rectangle(hMemDC, j * CELL_SIZE, i * CELL_SIZE, j * CELL_SIZE + CELL_SIZE, i * CELL_SIZE + CELL_SIZE);
					}
					if (cur_x%INTERVAL_SIZE == 0 && cur_y%INTERVAL_SIZE == 0) {
						SelectObject(hMemDC, hBrush);
						Rectangle(hMemDC, j * CELL_SIZE - INTERVAL_CUBE_RADIUS, i * CELL_SIZE - INTERVAL_CUBE_RADIUS, j * CELL_SIZE + INTERVAL_CUBE_RADIUS, i * CELL_SIZE + INTERVAL_CUBE_RADIUS);
					}
					
				}
				++cur_x;
			}
			++cur_y;
		}

		SelectObject(hMemDC, hOldBrush);
		DeleteObject(hWaterBrush);
		DeleteObject(hMineBrush);
		DeleteObject(hTreeBrush);
		DeleteObject(hRockBrush);
		DeleteObject(hFenceBrush);
		DeleteObject(hVillageBrush);
		DeleteObject(hStoreBrush);
		DeleteObject(hBrush);
		SelectObject(hMemDC, hOldPen);

		DrawEffects(hWnd, x - VIEW_RADIUS, y - VIEW_RADIUS);

		DrawArmor();

		// 플레이어 그리기
		for (auto it : characters) {
			it.second->Draw(hMemDC, x - VIEW_RADIUS, y - VIEW_RADIUS);
			if (it.second->hp > 0 && it.second->kind == KIND::PLAYER_KIND) 
				DrawRod(it.second->x, it.second->y
				, x - VIEW_RADIUS, y - VIEW_RADIUS);
		}
		if(myHP > 0)DrawRod(x, y , x - VIEW_RADIUS, y - VIEW_RADIUS);

		RECT rect;
		rect.left = 10 ;
		rect.right = CELL_SIZE + CELL_SIZE * VIEW_RADIUS;
		rect.top = 10;
		rect.bottom = CELL_SIZE + CELL_SIZE * VIEW_RADIUS;
		/*
		rect.right = VIEW_RADIUS * CELL_SIZE - CELL_SIZE;
		rect.left = VIEW_RADIUS * CELL_SIZE + CELL_SIZE + CELL_SIZE;
		rect.top = VIEW_RADIUS * CELL_SIZE - CELL_SIZE;
		rect.bottom = VIEW_RADIUS * CELL_SIZE;
		*/
		WCHAR buf[100];
		wsprintf(buf, L"%HP : %d \nEXP : %d \nLEVEL : %d \n X: %d Y: %d \n Fish: %d  Stone: %d \nPotion: %d  Armor: %d"
			, myHP, myEXP, myLEVEL, x, y, nFish, nStone, nPotion, nArmor);
		DrawText(hMemDC, buf, lstrlen(buf), &rect, DT_LEFT | DT_TOP);
		rect.left = 10;
		rect.right = CELL_SIZE * FRAME_SIZE;
		rect.top = CELL_SIZE * (FRAME_SIZE - 1);
		rect.bottom = CELL_SIZE * FRAME_SIZE -10;
		DrawText(hMemDC, msg_buf, lstrlen(msg_buf), &rect, DT_CENTER | DT_BOTTOM);
	}
	m.unlock();
}

void OnDestroy()
{
	Shutdown(hSock);
}


void CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	auto got = effects.find(idEvent);
	if (got != effects.end()) {
		EFFECT *del_c = got->second;
		delete del_c;
		effects.erase(got);
	}
	KillTimer(hWnd, idEvent);
	Draw(gHwnd);
	InvalidateRect(gHwnd, NULL, FALSE);
}

void ProcessPacket()
{
	byte msg_type = recv_packet_buf[sizeof(byte)];
	m.lock();
	switch (msg_type)
	{
	case S2C::LOGIN_OK: {
		sc_packet_login_ok *p = reinterpret_cast<sc_packet_login_ok *>(recv_packet_buf);
		myid = p->id;
		myHP = p->hp;
		myEXP = p->exp;
		myLEVEL = p->level;
		nFish = p->fish;
		nStone = p->stone;
		nPotion = p->potion;
		nArmor = p->armor;

		Character *new_c = new Character{p->id, p->x, p->y, KIND::PLAYER_KIND};
		characters[p->id] = new_c;
		break;
	}
	case S2C::ADD_OBJECT: {
		sc_packet_add_object *p = reinterpret_cast<sc_packet_add_object *>(recv_packet_buf);

		Character *new_c = new Character{ p->id, p->x, p->y, p->kind };
		characters[p->id] = new_c;
		break;
	}
	case S2C::POSITION_INFO: {
		sc_packet_position_info *p = reinterpret_cast<sc_packet_position_info *>(recv_packet_buf);
		auto got = characters.find(p->id);
		if (got != characters.end()) {
			got->second->SetPos(p->x, p->y);
		}
		break;
	}
	case S2C::REMOVE_OBJECT: {
		sc_packet_remove_object *p = reinterpret_cast<sc_packet_remove_object *>(recv_packet_buf);
		auto got = characters.find(p->id);
		if (got != characters.end()) {
			Character *del_c = got->second;
			delete del_c;
			characters.erase(got);
		}
		break;
	}
	case S2C::LOGIN_FAIL: {
		MessageBox(gHwnd, L"No Exist ID.", L"Error", MB_OK);
		exit(0);
		break;
	}
	case S2C::CHAT_S2C: {
		sc_packet_chat *p = reinterpret_cast<sc_packet_chat *>(recv_packet_buf);
		auto got = characters.find(p->id);
		if (got != characters.end()) {
			got->second->SetChat(p->chat);
		}
		break;
	}
	case S2C::STAT_CHANGE: {
		sc_packet_stat_change *p = reinterpret_cast<sc_packet_stat_change *>(recv_packet_buf);
		if (p->id >= MAX_PLAYER) {
			auto got = characters.find(p->id);
			if (got != characters.end()) {
				if (p->hp < got->second->hp)wsprintf(msg_buf, L"%d 데미지 입힘(->%d)"
					, (got->second->hp - p->hp), p->hp);
				got->second->hp = p->hp;
				got->second->HPChange(p->hp);
			}
		}
		else {
			auto got = characters.find(p->id);
			if (got != characters.end()) {
				got->second->hp = p->hp;
				if(p->hp <= 0)wsprintf(msg_buf, L"%d 플레이어 사망", got->second->id);
			}
		}

		if (p->id == myid) {
			if (myEXP < p->exp) {
				wsprintf(msg_buf, L"%d 경험치 획득", (p->exp - myEXP));
			}
			if (myHP > p->hp) {
				wsprintf(msg_buf, L"%d 데미지 받음", (myHP - p->hp));
			}
			if (myHP < p->hp) {
				wsprintf(msg_buf, L"HP %d 회복", (p->hp - myHP));
			}
			if (myLEVEL < p->level) {
				wsprintf(msg_buf, L"Level Up!!!");
				MessageBeep(NULL);
			}
			myHP = p->hp;
			myEXP = p->exp;
			myLEVEL = p->level;
			nDurability = p->durability;
		}
		
		break;
	}
	case S2C::ADD_EFFECT: {
		sc_packet_add_effect *p = reinterpret_cast<sc_packet_add_effect *>(recv_packet_buf);
		EFFECT *new_e = new EFFECT;
		new_e->type = p->skill;
		new_e->x = p->x;
		new_e->y = p->y;
		effects[(UINT)new_e] = new_e;
		SetTimer(gHwnd, (UINT)new_e, 300, TimerProc);
		break;
	}
	case S2C::ADD_ITEM: {
		sc_packet_add_item *p = reinterpret_cast<sc_packet_add_item *>(recv_packet_buf);
		switch (p->item)
		{
		case ITEM::FISH_ITEM:
			nFish += p->num;
			MessageBeep(NULL);
			wsprintf(msg_buf, L"Get Fish(->%d)!!!", nFish);
			break;
		case ITEM::STONE:
			nStone += p->num;
			MessageBeep(NULL);
			wsprintf(msg_buf, L"Get Stone(->%d)!!!", nStone);
			break;
		case ITEM::POTION:
			nPotion += p->num;
			break;
		case ITEM::ARMOR:
			nArmor += p->num;
			break;
		default:
			break;
		}
		break;
	}
	case S2C::DEAL_FAIL:
		MessageBox(gHwnd, L"deal fail!!", L"Error", MB_OK);
		break;
	case S2C::DEAL_OK: {
		sc_packet_deal_ok *p = reinterpret_cast<sc_packet_deal_ok *>(recv_packet_buf);
		nFish -= (p->potion - nPotion) * NUM_FISH_TO_POTION;
		nPotion = p->potion;
		nStone -= (p->armor - nArmor) * NUM_STONE_TO_ARMOR;
		nArmor = p->armor;
		MessageBox(gHwnd, L"deal Success!!", L"OK", MB_OK);
		break;
	}
	case S2C::ITEM_USED: {
		sc_packet_item_used *p = reinterpret_cast<sc_packet_item_used *>(recv_packet_buf);
		switch (p->item)
		{
		case ITEM::FISH_ITEM:
			--nFish;
			break;
		case ITEM::STONE:
			--nStone;
			break;
		case ITEM::POTION:
			//wsprintf(msg_buf, L"Use Potion!!!");
			--nPotion;
			break;
		case ITEM::ARMOR:
			//wsprintf(msg_buf, L"Use Armor!!!");
			--nArmor;
			break;
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
	m.unlock();
}

void RecvThread()
{
	while (true)
	{
		int len = recv(hSock, recv_buf, MAX_BUF_SIZE, 0);

		// 패킷 조립 + 패킷처리
		if (len == 0) {
			PostQuitMessage(0);
			return;
		}
		if (len < 0)
		{
			continue;
		}

		// packet 조립
		// ****이때 이 2byte도 끊겨서 올 수 있다.
		int rest_packet_size = len;
		int cur_recv_buf_idx = 0;

		while (rest_packet_size) {
			int required;
			if (remain_packet_size == 0) required = recv_buf[cur_recv_buf_idx];
			else required = recv_packet_buf[0] - remain_packet_size;
			if (required > rest_packet_size) {
				memcpy(recv_packet_buf + remain_packet_size, recv_buf + cur_recv_buf_idx, rest_packet_size);
				remain_packet_size += rest_packet_size;
				break;
			}


			memcpy(recv_packet_buf + remain_packet_size, recv_buf + cur_recv_buf_idx, required);
			cur_recv_buf_idx += required;
			rest_packet_size -= required;
			remain_packet_size = 0;

			ProcessPacket();
		}

		Draw(gHwnd);
		InvalidateRect(gHwnd, NULL, FALSE);
	}
}




void OnCreate(HWND hWnd) {
	//	메모리 DC를 생성
	HDC hdc = GetDC(hWnd);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, FRAME_SIZE * CELL_SIZE, FRAME_SIZE * CELL_SIZE);
	hMemDC = CreateCompatibleDC(hdc);
	SelectObject(hMemDC, hBitmap);
	ReleaseDC(hWnd, hdc);

	// map 읽기
	std::ifstream f("map_data.txt", std::ios::in);
	int row = 0, col = 0;
	while (!f.eof()) {
		int a;
		f >> a;
		if (a == 9) a = 1;
		else if (a == 10) a = 2;
		map[row][col] = a;
		++col;
		if (col == BOARD_WIDTH) {
			col = 0;
			++row;
		}
	}
	f.close();



	if (Init(hSock, ip_addr) != true) exit(1);

	cs_packet_login packet;
	packet.size = sizeof(cs_packet_login);
	packet.type = C2S::LOGIN;
	ZeroMemory(packet.name, 20);
	unsigned int n = lstrlen(lpszID);
	memcpy(packet.name, lpszID, n * sizeof(WCHAR));

	int len = send(hSock, reinterpret_cast<char *>(&packet), packet.size, 0);
	if (len <= 0) ErrDisplay(L"name send()");


	remain_packet_size = 0;
	recv_t = new thread{ RecvThread };

	gHwnd = hWnd;
	Draw(hWnd);
	InvalidateRect(hWnd, NULL, FALSE);
}

char CheckSpaceInput()
{
	m.lock();
	short dir[4][2] = { { 0, -1 },{ 1, 0 },{ 0, 1 },{ -1, 0 } };
	const unsigned short x = characters[myid]->x;
	const unsigned short y = characters[myid]->y;

	if (map[y][x] == 8) {
		m.unlock();
		return 0;
	}

	if (map[y][x] == 3) {
		m.unlock();
		return SKILL::MINING;
	}

	for (int i = 0; i < 4; ++i) {
		if (map[y + dir[i][1]][x + dir[i][0]] == 2) {
			m.unlock();
			return SKILL::FISHING;
		}
	}

	m.unlock();
	return -1;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_IP), hWnd, DialogProc);
		OnCreate(hWnd);
		break;
	case WM_PAINT:
		OnPaint(hWnd);
		break;
	case WM_KEYDOWN: {
		switch (wParam) {
		case 'w':
		case 'W':
			SendAttackInput(hSock, 0);
			break;
		case 'd':
		case 'D':
			SendAttackInput(hSock, 1);
			break;
		case 's':
		case 'S':
			SendAttackInput(hSock, 2);
			break;
		case 'a':
		case 'A':
			SendAttackInput(hSock, 3);
			break;
		case 'e':
		case 'E':
			SendAttackInput(hSock, SKILL::PLAYER_CROSS_SKILL);
			break;
		case 'q':
		case 'Q':
			SendAttackInput(hSock, SKILL::PLAYER_ULTIMATE_SKILL);
			break;
		case VK_LEFT:
			SendInput(hSock, 2);
			break;
		case VK_UP:
			SendInput(hSock, 0);
			break;
		case VK_RIGHT:
			SendInput(hSock, 3);
			break;
		case VK_DOWN:
			SendInput(hSock, 1);
			break;
		case VK_SPACE:{
			char in = CheckSpaceInput();
			if (in == 0) {
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_STORE), hWnd, DialogProcStore);
			}
			if (in == SKILL::FISHING || in == SKILL::MINING) {
				SendAttackInput(hSock, in);
			}
			break;
		}
		case '1':
			SendUseItemPacket(hSock, ITEM::POTION);
			break;
		case '2':
			SendUseItemPacket(hSock, ITEM::ARMOR);
			break;
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_QUIT:
		OnDestroy();
		//recv_t->join();
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}



BOOL CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	WORD cchPassword;

	switch (message)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg, IDC_IPADDRESS1, IPM_SETADDRESS, 0, MAKEIPADDRESS(127, 0, 0, 1));
		return TRUE;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDC_BUTTON_CANCEL:
			exit(0);
			return TRUE;
		case IDC_BUTTON_OK:
			SendDlgItemMessage(hDlg, IDC_IPADDRESS1, IPM_GETADDRESS, 0, (LPARAM)&ip_addr);

			cchPassword = (WORD)SendDlgItemMessage(hDlg,
				IDC_EDIT_ID, EM_LINELENGTH, (WPARAM)0, (LPARAM)0);
			if (cchPassword > 10)
			{
				MessageBox(hDlg, L"Too many characters in ID.", L"Error", MB_OK);
				return FALSE;
			}
			else if (cchPassword == 0)
			{
				MessageBox(hDlg, L"No characters entered in ID.", L"Error", MB_OK);
				return FALSE;
			}

			*((LPWORD)lpszID) = cchPassword;

			SendDlgItemMessage(hDlg, IDC_EDIT_ID, EM_GETLINE, (WPARAM)0, (LPARAM)lpszID);
			lpszID[cchPassword] = 0;

			EndDialog(hDlg, TRUE);
			return TRUE;
		}
		return 0;
	}
	return FALSE;

	UNREFERENCED_PARAMETER(lParam);
}


BOOL CALLBACK DialogProcStore(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		CheckRadioButton(hDlg, IDC_RADIO_POTION, IDC_RADIO_ARMOR, IDC_RADIO_POTION);
		return TRUE;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return TRUE;
		case IDOK:
		{
			BOOL is_ok;
			UINT number = GetDlgItemInt(hDlg, IDC_EDIT_NUMBER, &is_ok, FALSE);
			if (is_ok == false || number == 0)
			{
				MessageBox(hDlg, L"잘못된 입력", L"Error", MB_OK);
				return FALSE;
			}
			if (number > 125) {
				MessageBox(hDlg, L"too big number", L"Error", MB_OK);
				return FALSE;
			}

			char it = ITEM::POTION;
			if (SendDlgItemMessage(hDlg, IDC_RADIO_ARMOR, BM_GETCHECK, 0, 0) == BST_CHECKED)
				it = ITEM::ARMOR;
			SendBuyItemPacket(hSock, it, number);
			EndDialog(hDlg, TRUE);
			return TRUE;
		}
		}
		return 0;
	}
	return FALSE;

	UNREFERENCED_PARAMETER(lParam);
}
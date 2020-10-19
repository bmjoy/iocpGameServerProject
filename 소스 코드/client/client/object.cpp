#include "object.h"



Character::Character()
{
}

Character::Character(unsigned short id, unsigned short x, unsigned short y, byte kind)
	:id{id}, x{x}, y{y}, kind{kind}, chatting{false}, hp{DEFAULT_HP}
{
}


Character::~Character()
{
}

void Character::SetPos(unsigned short nx, unsigned short ny)
{
	x = nx; y = ny;
}

void Character::Draw(HDC hMemDC, unsigned short cx, unsigned short cy)
{
	if (hp <= 0) return;

	HBRUSH hBrush;
	switch (kind)
	{
	case KIND::PLAYER_KIND:
		hBrush = CreateSolidBrush(RGB(100, 100, 255));
		break;
	case KIND::AGGR_MONSTER:
		hBrush = CreateSolidBrush(RGB(249, 71, 71));
		break;
	case KIND::FISH:
		hBrush = CreateSolidBrush(RGB(100, 255, 100));
		break;
	case KIND::MILD_MONSTER:
		hBrush = CreateSolidBrush(RGB(246, 236, 83));
		break;
	case KIND::NEUTRAL_MONSTER:
		hBrush = CreateSolidBrush(RGB(252, 24, 255));
		break;
	default:
		hBrush = CreateSolidBrush(RGB(100, 100, 100));
		break;
	}
	HGDIOBJ hOldBrush = SelectObject(hMemDC, hBrush);
	HGDIOBJ hOldPen = SelectObject(hMemDC, GetStockObject(NULL_PEN));

	short dx = x - cx; short dy = y - cy;
	if (id < MAX_PLAYER)
		Ellipse(hMemDC, dx * CELL_SIZE, dy * CELL_SIZE, dx * CELL_SIZE + CELL_SIZE, dy * CELL_SIZE + CELL_SIZE);
	else
		Rectangle(hMemDC, dx * CELL_SIZE + (CELL_SIZE - NPC_CUBE_RADIUS) / 2
			, dy * CELL_SIZE + (CELL_SIZE - NPC_CUBE_RADIUS) / 2
			, dx * CELL_SIZE + NPC_CUBE_RADIUS + (CELL_SIZE - NPC_CUBE_RADIUS) / 2
			, dy * CELL_SIZE + NPC_CUBE_RADIUS + (CELL_SIZE - NPC_CUBE_RADIUS) / 2);

	SelectObject(hMemDC, hOldBrush);
	SelectObject(hMemDC, hOldPen);
	DeleteObject(hBrush);


	if (chatting) {
		RECT rect;
		rect.right = dx * CELL_SIZE - CELL_SIZE;
		rect.left = dx * CELL_SIZE + CELL_SIZE + CELL_SIZE;
		rect.top = dy * CELL_SIZE - CELL_SIZE;
		rect.bottom = dy * CELL_SIZE;
		WCHAR buf[100];
		swprintf_s(buf, L"%s", chat);
		DrawText(hMemDC, buf, lstrlen(buf), &rect, DT_SINGLELINE | DT_CENTER | DT_BOTTOM);
		if (std::chrono::system_clock::now() >= valid_chat_time) {
			chatting = false;
			chat[0] = 0;
		}
			
	}
}

void Character::SetChat(wchar_t * msg)
{
	chatting = true;
	valid_chat_time = std::chrono::system_clock::now() + std::chrono::milliseconds(CHAT_VALID_TIME);
	lstrcpyn(chat, msg, MAX_CHAT_SIZE-1);
}

void Character::HPChange(unsigned short hp)
{
	chatting = true;
	valid_chat_time = std::chrono::system_clock::now() + std::chrono::milliseconds(100);
	wsprintf(chat, L"%d", hp);
}

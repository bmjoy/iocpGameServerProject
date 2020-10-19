#include "network.h"


bool Init(SOCKET& hSock, DWORD ip_addr)
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	hSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, 0);
	if (hSock == INVALID_SOCKET){
		ErrDisplay(L"socket()");
		return false;
	}

	BOOL NoDelay = TRUE;
	setsockopt(hSock, IPPROTO_TCP, TCP_NODELAY, (const char FAR*)&NoDelay, sizeof(NoDelay));
	unsigned long noblock = 1; 
	int nRet = ioctlsocket(hSock, FIONBIO, &noblock);

	SOCKADDR_IN addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	//addr.sin_addr.s_addr = inet_addr(ip_addr);
	addr.sin_addr.s_addr = htonl(ip_addr);
	addr.sin_port = htons(SERVER_PORT);

	WSAConnect(hSock, (SOCKADDR *)&addr, sizeof(addr), NULL, NULL, NULL, NULL);

	return true;
}

void Shutdown(SOCKET hSock)
{
	closesocket(hSock);
	WSACleanup();
}


void ErrDisplay(LPCWSTR msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, WSAGetLastError(),
		MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, nullptr
	);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
}

void SendInput(SOCKET hSock, const byte msg)
{
	cs_packet_move packet;
	packet.dir = msg;
	packet.size = sizeof(cs_packet_move);
	packet.type = C2S::MOVE;

	int len = send(hSock, reinterpret_cast<char *>(&packet), packet.size, 0);
	if (len <= 0) ErrDisplay(L"send()");
}

void SendAttackInput(SOCKET hSock, const byte msg) 
{
	cs_packet_attack packet;
	packet.skill = msg;
	packet.size = sizeof(cs_packet_attack);
	packet.type = C2S::ATTACK;

	int len = send(hSock, reinterpret_cast<char *>(&packet), packet.size, 0);
	if (len <= 0) ErrDisplay(L"send()");
}

void SendBuyItemPacket(SOCKET hSock, char item, byte num)
{
	cs_packet_buy_item packet;
	packet.size = sizeof(cs_packet_buy_item);
	packet.type = C2S::BUY_ITEM;
	packet.item = item;
	packet.number = num;

	int len = send(hSock, reinterpret_cast<char *>(&packet), packet.size, 0);
	if (len <= 0) ErrDisplay(L"send()");
}

void SendUseItemPacket(SOCKET hSock, char item)
{
	cs_packet_use_item packet;
	packet.size = sizeof(cs_packet_use_item);
	packet.type = C2S::USE_ITEM;
	packet.item = item;

	int len = send(hSock, reinterpret_cast<char *>(&packet), packet.size, 0);
	if (len <= 0) ErrDisplay(L"send()");
}

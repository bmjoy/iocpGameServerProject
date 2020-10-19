#pragma once
#include "server.h"

struct EXOVERLAPPED {
	WSAOVERLAPPED wsaoverlapped;
	WSABUF wsabuf;
	char command;
};

struct RECV_INFO {
	EXOVERLAPPED recv_ovex;
	char recv_buf[MAX_BUF_SIZE];

	char recved_packet[MAX_PACKET_SIZE];
	int remain_packet_size;
};

struct SEND_PACKET_NODE {
	EXOVERLAPPED exov;
	char buf[MAX_PACKET_SIZE];

	SEND_PACKET_NODE *next{ nullptr };
};

class SendPacketPool {
	mutex head_access;
	SEND_PACKET_NODE head, tail;

public:
	SendPacketPool();
	~SendPacketPool();

	SEND_PACKET_NODE *GetSendMemory();
	void ReturnSendPacket(SEND_PACKET_NODE *packet);
};

struct OVEX_NODE {
	EXOVERLAPPED exov;

	OVEX_NODE *next;
};

class OVEXMemoryPool {
	mutex head_access;
	OVEX_NODE head, tail;

public:
	OVEXMemoryPool();
	~OVEXMemoryPool();

	OVEX_NODE *GetOVEX();
	void ReturnOVEX(OVEX_NODE *ovex);
};


struct RECV_NAME_PACKET_NODE {
	EXOVERLAPPED exov;
	int remain_packet_size;
	SOCKET sock;
	unsigned short id;

	char buf[RECV_NAME_PACKET_SIZE];
	RECV_NAME_PACKET_NODE *next{ nullptr };
};

class RecvNamePacketMemoryPool {
	mutex head_access;
	RECV_NAME_PACKET_NODE head;

public:
	RecvNamePacketMemoryPool();
	~RecvNamePacketMemoryPool();

	RECV_NAME_PACKET_NODE *GetMemory();
	void ReturnMemory(RECV_NAME_PACKET_NODE *ovex);
};


void ErrQuit(LPCWSTR msg);
void ErrDisplay(LPCWSTR msg);
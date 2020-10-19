#include "network.h"

void ErrQuit(LPCWSTR msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, nullptr
	);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
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

SendPacketPool::SendPacketPool()
{
	head.next = &tail;
	tail.next = nullptr;
}

SendPacketPool::~SendPacketPool()
{
	while (head.next != &tail) {
		SEND_PACKET_NODE *del = head.next;
		head.next = del->next;
		delete del;
	}
}

SEND_PACKET_NODE *SendPacketPool::GetSendMemory()
{
	head_access.lock();
	if (head.next == &tail) {
		head_access.unlock();
		SEND_PACKET_NODE *ret = new SEND_PACKET_NODE;
		ret->exov.command = COMMAND::SEND_CMD;
		ret->exov.wsabuf.buf = ret->buf;
		return ret;
	}
	else {
		SEND_PACKET_NODE *ret = head.next;
		head.next = ret->next;
		head_access.unlock();
		return ret;
	}
}

void SendPacketPool::ReturnSendPacket(SEND_PACKET_NODE *packet)
{
	head_access.lock();
	packet->next = head.next;
	head.next = packet;
	head_access.unlock();
}

OVEXMemoryPool::OVEXMemoryPool()
{
	head.next = &tail;
	tail.next = nullptr;
}

OVEXMemoryPool::~OVEXMemoryPool()
{
	while (head.next != &tail) {
		OVEX_NODE *del = head.next;
		head.next = del->next;
		delete del;
	}
}

OVEX_NODE * OVEXMemoryPool::GetOVEX()
{
	head_access.lock();
	if (head.next == &tail) {
		head_access.unlock();
		OVEX_NODE *ret = new OVEX_NODE;
		return ret;
	}
	else {
		OVEX_NODE *ret = head.next;
		head.next = ret->next;
		head_access.unlock();
		return ret;
	}
}

void OVEXMemoryPool::ReturnOVEX(OVEX_NODE * ovex)
{
	head_access.lock();
	ovex->next = head.next;
	head.next = ovex;
	head_access.unlock();
}

RecvNamePacketMemoryPool::RecvNamePacketMemoryPool()
{
	head.next = nullptr;
}

RecvNamePacketMemoryPool::~RecvNamePacketMemoryPool()
{
	while (head.next != nullptr) {
		RECV_NAME_PACKET_NODE *del = head.next;
		head.next = del->next;
		delete del;
	}
}

RECV_NAME_PACKET_NODE * RecvNamePacketMemoryPool::GetMemory()
{
	head_access.lock();
	if (head.next == nullptr) {
		head_access.unlock();
		RECV_NAME_PACKET_NODE *ret = new RECV_NAME_PACKET_NODE;
		return ret;
	}
	else {
		RECV_NAME_PACKET_NODE *ret = head.next;
		head.next = ret->next;
		head_access.unlock();
		return ret;
	}
}

void RecvNamePacketMemoryPool::ReturnMemory(RECV_NAME_PACKET_NODE * m)
{
	head_access.lock();
	m->next = head.next;
	head.next = m;
	head_access.unlock();
}

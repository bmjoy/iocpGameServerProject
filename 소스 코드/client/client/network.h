#pragma once
#include "object.h"

bool Init(SOCKET&, DWORD);
void Shutdown(SOCKET);

void ErrDisplay(LPCWSTR);

void RecvThread();
void SendInput(SOCKET, byte);
void SendAttackInput(SOCKET hSock, const byte msg);
void SendBuyItemPacket(SOCKET hSock, char item, byte num);
void SendUseItemPacket(SOCKET hSock, char item);
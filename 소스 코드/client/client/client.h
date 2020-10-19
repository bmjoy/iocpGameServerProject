#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <Windows.h>
#include <unordered_map>
#include <CommCtrl.h>
#include <fstream>
#include <chrono>
#include <mutex>
#include <thread>
#include "protocol.h"

using namespace std;

#define CHAT_VALID_TIME 1000
#define CELL_SIZE	40
static const int FRAME_SIZE = VIEW_RADIUS * 2 + 1;

#define INTERVAL_SIZE 8
#define INTERVAL_CUBE_RADIUS 5
#define ARMOR_RADIUS 22
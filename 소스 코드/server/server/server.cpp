#include <random>
#include <vector>
#include <thread>
#include "GameManager.h"

HANDLE g_iocp;

SendPacketPool send_packet_pool;
OVEXMemoryPool ovex_pool;
EvnetQueue event_q;
DBEvnetQueue db_event_q;
GameManager game{ send_packet_pool, ovex_pool, event_q, db_event_q };
RecvNamePacketMemoryPool recv_name_packet_pool;


void Initialize()
{
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	game.SetIOCPHande(g_iocp);
}

void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
	SQLSMALLINT iRec = 0;
	SQLINTEGER  iError;
	WCHAR       wszMessage[1000];
	WCHAR       wszState[SQL_SQLSTATE_SIZE + 1];

	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}
	while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT *)NULL) == SQL_SUCCESS) {
		// Hide data truncated..
		if (wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}


void RecvNamePacket(RECV_NAME_PACKET_NODE *m, unsigned long data_size)
{
	if (data_size == 0) {
		cout << "recv name error" << endl;
		game.ReturnID(m->id);
		closesocket(m->sock);
		recv_name_packet_pool.ReturnMemory(m);
		return;
	}
	int remain = m->remain_packet_size;
	unsigned int required = RECV_NAME_PACKET_SIZE - remain;
	if (required > data_size) {
		m->remain_packet_size += data_size;
		// recv 요청
		m->exov.wsabuf.buf = m->buf + m->remain_packet_size;
		m->exov.wsabuf.len = RECV_NAME_PACKET_SIZE - m->remain_packet_size;
		ZeroMemory(&(m->exov.wsaoverlapped), sizeof(WSAOVERLAPPED));

		unsigned long flag = 0;
		WSARecv(m->sock, &(m->exov.wsabuf), 1, NULL, &flag
			, &(m->exov.wsaoverlapped), NULL);
		return;
	}

	if (m->buf[sizeof(BYTE)] != C2S::LOGIN) {
		cout << "recv name error" << endl;
		game.ReturnID(m->id);
		closesocket(m->sock);
		recv_name_packet_pool.ReturnMemory(m);
		return;
	}
	
	// DB event request
	WCHAR *name = reinterpret_cast<cs_packet_login *>(m->buf)->name;
	db_event_q.EnqSelectByNameCMD(DB_COMMAND::GET_PLAYER_INFO_BY_NAME, 0, m->id, name, m->sock);
	recv_name_packet_pool.ReturnMemory(m);
}

void DBThread()
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLRETURN retcode;

	// 데이터를 저장할 변수
	// x(word), y(word), hp(word), level(byte), exp(dword)
	SQLINTEGER nID;
	SQLWCHAR szName[NAME_LEN];
	SQLUSMALLINT nX, nY, nHP, nFish, nStone, nPotion, nArmor;
	SQLSCHAR nLEVEL;
	SQLUINTEGER nEXP;
	SQLLEN cbID = 0,cbName = 0, cbX = 0, cbY = 0, cbHP = 0, cbLEVEL = 0, cbEXP = 0
			,cbFish = 0, cbStone = 0, cbPotion = 0, cbArmor = 0;
	

	setlocale(LC_ALL, "korean");

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"ODBC_GAME_SERVER", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						while (true)
						{
							Sleep(1);
							while (true)
							{
								DB_EVENT_NODE *e = db_event_q.peek();
								if (e == nullptr) break;
								if (system_clock::now() < e->when) break;
								e = db_event_q.deq();

								switch (e->db_command)
								{
								case DB_COMMAND::GET_PLAYER_INFO_BY_NAME: {
									WCHAR db_cmd[100];
									wsprintf(db_cmd, L"EXEC select_by_name %s", e->t_name);

									retcode = SQLExecDirect(hstmt, (SQLWCHAR *)db_cmd, SQL_NTS);
									if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
										// Bind columns 1, 2, and 3 
										// 1, 2, 3 column 번호
										/*
										SQLUINTEGER nEXP;
										*/
										retcode = SQLBindCol(hstmt, 1, SQL_C_LONG, &nID
											, sizeof(nID), &cbID);
										retcode = SQLBindCol(hstmt, 2, SQL_C_WCHAR, szName
											, NAME_LEN, &cbName);
										retcode = SQLBindCol(hstmt, 3, SQL_C_STINYINT, &nLEVEL
											, sizeof(nLEVEL), &cbLEVEL);
										retcode = SQLBindCol(hstmt, 4, SQL_C_USHORT, &nX
											, sizeof(nX), &cbX);
										retcode = SQLBindCol(hstmt, 5, SQL_C_USHORT, &nY
											, sizeof(nY), &cbY);
										retcode = SQLBindCol(hstmt, 6, SQL_C_USHORT
											, &nHP, sizeof(nHP), &cbHP);
										retcode = SQLBindCol(hstmt, 7, SQL_C_ULONG
											, &nEXP, sizeof(nEXP), &cbEXP);
										retcode = SQLBindCol(hstmt, 8, SQL_C_USHORT
											, &nFish, sizeof(nFish), &cbFish);
										retcode = SQLBindCol(hstmt, 9, SQL_C_USHORT
											, &nStone, sizeof(nStone), &cbStone);
										retcode = SQLBindCol(hstmt, 10, SQL_C_USHORT
											, &nPotion, sizeof(nPotion), &cbPotion);
										retcode = SQLBindCol(hstmt, 11, SQL_C_USHORT
											, &nArmor, sizeof(nArmor), &cbArmor);

										// Fetch and print each row of data. On an error, display a message and exit.  

										// 미리 bindcol 으로 셋팅 해놓는다
										// 어디에 데이터를 넣을지 -> fetch
										retcode = SQLFetch(hstmt);
										if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
											HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
										if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
										{
											//wprintf(L"%d %S %d %d\n", nID, szName, nX, nY);
											game.InPlayer(e->t_id, e->t_sock, 
												nX, nY, nHP, nLEVEL, nEXP, szName
												, nFish, nStone, nPotion, nArmor);
											//cout << e->t_id << " stat save event in" << endl;
											event_q.enq(e->t_id,
												COMMAND::STAT_SAVE_CMD, STAT_SAVE_TERM);
										}
										else {
											WSABUF wsabuf;
											sc_packet_login_fail lf;
											lf.size = sizeof(sc_packet_login_fail);
											lf.type = S2C::LOGIN_FAIL;
											wsabuf.buf = reinterpret_cast<char *>(&lf);
											wsabuf.len = lf.size;
											WSASend(e->t_sock, &wsabuf, 1, 
												NULL, 0, NULL, NULL);
											game.ReturnID(e->t_id);
											closesocket(e->t_sock);
											break;
										}
									}
									else {
										HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
									}
									break;
								}
								case DB_COMMAND::UPDATE_PLAYER_POS: {
									WCHAR db_cmd[200];
									wsprintf(db_cmd
									, L"EXEC update_player_stat %s, %d, %d, %d, %d, %d, %d, %d, %d, %d"
										, e->t_name, e->t_x, e->t_y, e->t_hp, e->t_level, e->t_exp
										, e->t_fish, e->t_stone, e->t_potion, e->t_armor);

									retcode = SQLExecDirect(hstmt, (SQLWCHAR *)db_cmd, SQL_NTS);
									if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
										/*
										retcode = SQLFetch(hstmt);
										if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
											HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
											*/
									}
									else {
										HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
									}
									break;
								}
								case DB_COMMAND::UPDATE_DEAL_DATA: {
									WCHAR db_cmd[200];
									wsprintf(db_cmd
									, L"EXEC update_player_stat %s, %d, %d, %d, %d, %d, %d, %d, %d, %d"
										, e->t_name, e->t_x, e->t_y, e->t_hp, e->t_level, e->t_exp
										, e->t_fish, e->t_stone, e->t_potion, e->t_armor);

									retcode = SQLExecDirect(hstmt, (SQLWCHAR *)db_cmd, SQL_NTS);
									if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
										game.SendDealOKPacket(e->t_id, e->t_potion, e->t_armor);
										/*
										retcode = SQLFetch(hstmt);
										if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
										HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
										*/
									}
									else {
										HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
									}
									break;
								}
								default:
									break;
								}
								db_event_q.ReturnMemory(e);
								SQLCancel(hstmt);
							}
							
						}
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}
					else {
						HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}

					SQLDisconnect(hdbc);
				}


				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}

}

void TimerThread()
{
	while (true)
	{
		Sleep(1);
		while (true)
		{
			EVENT_NODE *e = event_q.peek();
			if (e == nullptr) break;
			if (system_clock::now() < e->when) break;
			e = event_q.deq();
			OVEX_NODE *ovex = ovex_pool.GetOVEX();
			ZeroMemory(&(ovex->exov.wsaoverlapped), sizeof(WSAOVERLAPPED));
			ovex->exov.command = e->command;
			ovex->exov.wsabuf.len = e->target;
			PostQueuedCompletionStatus(g_iocp, 1, e->id, &(ovex->exov.wsaoverlapped));

			event_q.ReturnMemory(e);
		}
	}
}

void WorkerThread()
{
	while (true)
	{
		unsigned long data_size;
		unsigned long long key;
		WSAOVERLAPPED *p_over;

		BOOL is_success = GetQueuedCompletionStatus(g_iocp, &data_size, &key, &p_over, INFINITE);

		if (key < MAX_PLAYER) {
			// 1. error 처리 + 접속종료처리		2. send 처리		3. recv 처리
			EXOVERLAPPED *o = reinterpret_cast<EXOVERLAPPED *>(p_over);

			if (COMMAND::RECV_NAME_CMD == o->command) {
				RecvNamePacket(reinterpret_cast<RECV_NAME_PACKET_NODE *>(o), data_size);
				continue;
			}

			if (COMMAND::STAT_SAVE_CMD == o->command) {
				//cout << key << " stat save event out" << endl;
				game.RecordStat(key);
				//cout << key << " stat save event in" << endl;
				event_q.enq(key, COMMAND::STAT_SAVE_CMD, STAT_SAVE_TERM);
				ovex_pool.ReturnOVEX(reinterpret_cast<OVEX_NODE *>(o));
				continue;
			}

			if (COMMAND::SKILL_ON_CMD == o->command) {
				game.ActivateSkill(key, o->wsabuf.len);
				ovex_pool.ReturnOVEX(reinterpret_cast<OVEX_NODE *>(o));
				continue;
			}

			if (COMMAND::REVIVE_CMD == o->command) {
				game.RevivePlayer(key);
				ovex_pool.ReturnOVEX(reinterpret_cast<OVEX_NODE *>(o));
				continue;
			}

			// 1. error 처리 + 접속 종료 처리
			if (0 == is_success || 0 == data_size)
			{
				if (0 == is_success)
					cout << "Error in GQCS key[" << key << "]" << endl;
				if (COMMAND::SEND_CMD == o->command) {
					send_packet_pool.ReturnSendPacket(reinterpret_cast<SEND_PACKET_NODE *>(o));
					continue;
				}
				game.OutPlayer(key);
				continue;
			}

			// 2. send 처리 + recv 처리
			if (COMMAND::RECV_CMD == o->command) {
				game.RecvPacket(key, data_size);
			}
			else if (COMMAND::SEND_CMD == o->command) {
				send_packet_pool.ReturnSendPacket(reinterpret_cast<SEND_PACKET_NODE *>(o));
			}

		}
		// npc 일때
		else
		{
			EXOVERLAPPED *o = reinterpret_cast<EXOVERLAPPED *>(p_over);
			if (0 == is_success)
			{
				ovex_pool.ReturnOVEX(reinterpret_cast<OVEX_NODE *>(o));
				cout << "Error in GQCS key[" << key << "]" << endl;
				continue;
			}

			if (COMMAND::MOVE_CMD == o->command) game.MoveNPC(key);
			else if (COMMAND::CHECK_PLAYER_CMD == o->command)
				game.CheckPlayer(key, o->wsabuf.len);
			else if (COMMAND::REVIVE_CMD == o->command)
				game.ReviveNPC(key);
			ovex_pool.ReturnOVEX(reinterpret_cast<OVEX_NODE *>(o));
		}
	}

}

void AcceptThread()
{
	// listen socket이 overlapped 여야, client socket overlapped
	SOCKET listen_sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP
		, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (listen_sock == INVALID_SOCKET)
		ErrQuit(L"socket()");

	SOCKADDR_IN bind_addr;
	ZeroMemory(&bind_addr, sizeof(bind_addr));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(SERVER_PORT);


	if (::bind(listen_sock, (SOCKADDR*)&bind_addr, sizeof(bind_addr)) != 0)
		ErrQuit(L"bind()");
	if (listen(listen_sock, SOMAXCONN) != 0)
		ErrQuit(L"listen()");

	while (true) {
		SOCKADDR_IN client_addr;
		int addrlen = sizeof(client_addr);

		SOCKET client_sock = WSAAccept(listen_sock, (SOCKADDR*)&client_addr, &addrlen, NULL, NULL);
		if (client_sock == INVALID_SOCKET) {
			ErrDisplay(L"accept()");
			continue;
		}
		BOOL NoDelay = TRUE;
		setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (const char FAR*)&NoDelay, sizeof(NoDelay));

		if (game.GetEmptyPlayerSize() <= 0) {
			cout << "MAX USER EXCEED!!!" << endl;
			continue;
		}

		RECV_NAME_PACKET_NODE *m = recv_name_packet_pool.GetMemory();
		m->sock = client_sock;
		m->remain_packet_size = 0;
		m->exov.command = COMMAND::RECV_NAME_CMD;
		ZeroMemory(m->buf, RECV_NAME_PACKET_SIZE);
		m->exov.wsabuf.buf = m->buf;
		m->exov.wsabuf.len = RECV_NAME_PACKET_SIZE;
		ZeroMemory(&(m->exov.wsaoverlapped), sizeof(WSAOVERLAPPED));

		unsigned short id = game.GetID();
		m->id = id;
		unsigned long flag = 0;
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(m->sock), g_iocp, id, 0);
		WSARecv(m->sock, &(m->exov.wsabuf), 1, NULL, &flag
			, &(m->exov.wsaoverlapped), NULL);

	}
	closesocket(listen_sock);
}



int CAPI_send_message(lua_State * L)
{
	// 파라미터 3개를 받는다
	char *mess = (char *)lua_tostring(L, -1);
	int chatter = (int)lua_tointeger(L, -2);
	int to = (int)lua_tointeger(L, -3);
	lua_pop(L, 4);

	size_t len = strlen(mess);
	if (len >= MAX_CHAT_SIZE) len = MAX_CHAT_SIZE - 1;
	wchar_t wmess[MAX_CHAT_SIZE];
	size_t wlen;
	mbstowcs_s(&wlen, wmess, len, mess, _TRUNCATE);
	wmess[MAX_CHAT_SIZE - 1] = (wchar_t)0;	// 혹시 모르니깐

	game.SendPacket(S2C::CHAT_S2C, to, chatter, wmess);

	return 0;
}

int CAPI_get_x(lua_State * L)
{
	int id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2); //함수까지 있으니까 하나더 pop
	int x = game.GetX(id);
	lua_pushnumber(L, x);
	return 1;
}

int CAPI_get_y(lua_State * L)
{
	int id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2); //함수까지 있으니까 하나더 pop
	int y = game.GetY(id);
	lua_pushnumber(L, y);
	return 1;
}

int CAPI_move_neutral_npc(lua_State * L)
{
	int id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	game.NeutralNPCMove(id);
	return 0;
}

int CAPI_register_event(lua_State * L)
{
	// 파라미터 3개를 받는다
	int id = (int)lua_tointeger(L, -1);
	int e_num = (int)lua_tointeger(L, -2);
	int e_t = (int)lua_tointeger(L, -3);
	lua_pop(L, 4);

	// MOVE_CMD : 2, MOVE_TERM 1000
	//event_q.enq(id, COMMAND::MOVE_CMD, MOVE_TERM);
	event_q.enq(id, e_num, e_t);
	return 0;
}

int CAPI_set_npc_dir(lua_State * L)
{
	// 파라미터 2개를 받는다
	int id = (int)lua_tointeger(L, -1);
	int dir = (int)lua_tointeger(L, -2);
	lua_pop(L, 3);

	game.SetNeutralNPCDir(id, dir);
	return 0;
}


int main()
{
	vector <thread> threads;
	Initialize();

	for (int i = 0; i < 4; ++i)
		threads.push_back(thread{ WorkerThread });

	threads.push_back(thread{ AcceptThread });
	threads.push_back(thread{ TimerThread });
	threads.push_back(thread{ DBThread });


	for (auto &th : threads) th.join();


	WSACleanup();
}
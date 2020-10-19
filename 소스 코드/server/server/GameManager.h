#pragma once
#include "Astar.h"

class GameManager{
	RECV_INFO players_recv_info[MAX_PLAYER];
	PLAYER players[MAX_PLAYER];

	mutex player_in_out;
	unsigned short empty_idx[MAX_PLAYER];
	unsigned short nEmpty{ MAX_PLAYER };

	byte map[BOARD_HEIGHT][BOARD_WIDTH];
	NPC npcs[NUM_NPC];

	unordered_set<unsigned short> sectors[SECTOR_ROW_SIZE * SECTOR_COL_SIZE];
	// sector lock을 얻어야 상대방 view list에 접근가능하거나 나갈 수 있다
	mutex sectors_mutex[SECTOR_ROW_SIZE * SECTOR_COL_SIZE];
	
	HANDLE hIOCP;
	SendPacketPool &send_packet_pool;
	OVEXMemoryPool &ovex_pool;
	DBEvnetQueue &db_event_q;
	EvnetQueue &event_q;
public:
	
	GameManager(SendPacketPool &send_packet_pool
		,OVEXMemoryPool &ovex_pool, EvnetQueue &event_q, DBEvnetQueue &db_event_q);
	~GameManager();

	void SetIOCPHande(HANDLE g_iocp);

	unsigned short GetEmptyPlayerSize();

	void SendPacket(char type, unsigned short to, unsigned short target = -1, wchar_t *msg = nullptr);
	void SendEffectPacket(unsigned short to, unsigned short x, unsigned short y, char skill);
	void SendItemPacket(char type, unsigned short to, char item, short n);
	void SendDealOKPacket(unsigned short to, unsigned short nPotion, unsigned short nArmor);
	void SendItemUsedPacket(unsigned short to, char item);

	void ProcessPacket(unsigned short idx);
	void MovePlayer(unsigned short idx);

	void MakePlayerSkill(unsigned short idx);
	void MakeStandardSkill(unsigned short idx, char dir);
	void MakeCrossSkill(unsigned short idx);
	void MakeUltimateSkill(unsigned short idx);
	void Fishing(unsigned short idx);
	void Mining(unsigned short idx);

	void DealItem(unsigned short idx);
	void UseItem(unsigned short idx);
	void UsePotion(unsigned short idx);
	void UseArmor(unsigned short idx);
	
	void RecvPacket(unsigned short idx, unsigned long data_size);

	unsigned short GetID();
	void ReturnID(unsigned short);

	void InPlayer(unsigned short idx, SOCKET sock, unsigned short x, unsigned short y
		, unsigned short hp, byte level, unsigned long exp, WCHAR *name
		, unsigned short fish, unsigned short stone, unsigned short potion, unsigned short armor);
	void OutPlayer(unsigned short idx);

	void GetViewSectors(set<unsigned short> &view_sector, unsigned short x, unsigned short y);

	int GetSquareDistance(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2);

	void MoveNPC(unsigned short id);
	void MildMonsterMove(unsigned short id);
	unsigned short GetMostClosePlayer(set<unsigned short> &view_sectors
										, unsigned short x, unsigned short y);
	void AgrressiveMonsterMove(unsigned short id);
	void FishMove(unsigned short id);
	void IdleMove(unsigned short id);
	void ChasingMove(unsigned short id, unsigned short target);
	char OnAstar(unsigned short id, unsigned short target);
	bool AttackPlayer(unsigned short id, unsigned short target);
	bool IsInView(unsigned short x, unsigned short y, unsigned tx, unsigned short ty);
	void MakeNPCSkill(unsigned short id, unsigned short target);

	void RecordStat(unsigned short id, bool deal_record=false);


	void CheckPlayer(unsigned short id, unsigned short target);
	unsigned short GetX(unsigned short id);
	unsigned short GetY(unsigned short id);

	void NeutralNPCMove(unsigned short id);
	void SetNeutralNPCDir(unsigned short id, byte dir);

	void ActivateSkill(unsigned short id, char type);
	void ReviveNPC(unsigned short id);
	void RevivePlayer(unsigned short id);
};
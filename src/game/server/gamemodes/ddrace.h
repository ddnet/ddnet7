/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_GAMEMODES_DDRACE_H
#define GAME_SERVER_GAMEMODES_DDRACE_H
#include <game/server/gamecontroller.h>

#include <vector>
#include <map>

class CGameControllerDDRace: public IGameController
{
	//TODO: Replace this with a std::vector
	CGameWorld *m_apGameWorlds[MAX_CLIENTS];
	bool m_ResetWorlds[MAX_CLIENTS];

public:
	CGameControllerDDRace(class CGameContext *pGameServer);

	bool OnEntityInternal(int Index, vec2 Pos, CTile Tile, CGameWorld *pWorld);
	bool OnEntity(int Index, vec2 Pos, CTile Tile, CGameWorld *pWorld);
	void Tick();
	void Snap(int SnappingClient);
	void PostSnap();
	bool CanSpawn(int Team, vec2 *pPos, CGameWorld *pWorld) const;
	void OnReset(CGameWorld *pWorld);

	CGameWorld *GetGameWorld(int Team) { return m_apGameWorlds[Team]; };
	void SetTuning(CTuningParams &Tuning);
	int JoinTeam(CPlayer *pPlayer, int Team);
};
#endif // GAME_SERVER_GAMEMODES_DDRACE_H

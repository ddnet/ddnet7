/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
/* Based on Race mod stuff and tweaked by GreYFoX@GTi and others to fit our DDRace needs. */
#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>

#include "ddrace.h"

#define GAME_NAME "DDraceNetwork"

// TODO: Maybe move World stuff out of the controller :/
// TODO: Maybe add a ForEachWorld that takes a std::function

CGameControllerDDRace::CGameControllerDDRace(class CGameContext *pGameServer) :
		IGameController(pGameServer)
{
	m_apGameWorlds[0] = new CGameWorld();
	m_apGameWorlds[0]->SetGameServer(pGameServer);
	m_ResetWorlds[0] = false;
	m_apGameWorlds[1] = new CGameWorld();
	m_apGameWorlds[1]->SetGameServer(pGameServer);
	m_ResetWorlds[1] = false;
	m_apGameWorlds[2] = new CGameWorld();
	m_apGameWorlds[2]->SetGameServer(pGameServer);
	m_ResetWorlds[2] = false;
	for(int i = 3; i < MAX_CLIENTS; i++)
	{
		m_apGameWorlds[i] = nullptr;
		m_ResetWorlds[i] = false;
	}

	m_pGameType = GAME_NAME;
	SetGameState(IGS_GAME_RUNNING, TIMER_INFINITE);
}

bool CGameControllerDDRace::OnEntityInternal(int Index, vec2 Pos, CTile Tile, CGameWorld *pWorld)
{
	dbg_assert(!!pWorld, "World can't be null");
	if(IGameController::OnEntity(Index, Pos, Tile, pWorld))
		return true;

	// Handle our entities here

	return false;
}

bool CGameControllerDDRace::OnEntity(int Index, vec2 Pos, CTile Tile, CGameWorld *pWorld)
{
	if(!pWorld)
	{
		bool Result = true;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			pWorld = m_apGameWorlds[i];
			if(!pWorld)
				continue;

			Result = Result && OnEntityInternal(Index, Pos, Tile, pWorld);
		}
		return Result;
	}
	else
	{
		return OnEntityInternal(Index, Pos, Tile, pWorld);
	}

}

void CGameControllerDDRace::Tick()
{
	for(int i = 0; i < MAX_CLIENTS; i++) {
		CGameWorld *pWorld = m_apGameWorlds[i];
		if(!pWorld)
			continue;

		pWorld->Tick();
	}
	IGameController::Tick();
}

void CGameControllerDDRace::Snap(int SnappingClient)
{
	CPlayer *pPlayer = m_pGameServer->m_apPlayers[SnappingClient];
	if(pPlayer->GetTeam() == TEAM_SPECTATORS) {
		for(int i = 0; i < MAX_CLIENTS; i++) {
			CGameWorld *pWorld = m_apGameWorlds[i];
			if(!pWorld)
				continue;

			pWorld->Snap(SnappingClient);
		}
	}
	else {
		CGameWorld *pWorld = m_apGameWorlds[pPlayer->GetDDRaceTeam()];
		dbg_assert(!!pWorld, "Ingame players must have a gameworld");

		pWorld->Snap(SnappingClient);
	}

	IGameController::Snap(SnappingClient);
}

void CGameControllerDDRace::PostSnap()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CGameWorld *pWorld = m_apGameWorlds[i];
		if(!pWorld)
			continue;

		pWorld->PostSnap();
	}
}

bool CGameControllerDDRace::CanSpawn(int Team, vec2 *pPos, CGameWorld *pWorld) const
{
	CSpawnEval Eval;

	// spectators can't spawn
	if(Team == TEAM_SPECTATORS)
		return false;

	EvaluateSpawnType(&Eval, 0, pWorld);
	EvaluateSpawnType(&Eval, 1, pWorld);
	EvaluateSpawnType(&Eval, 2, pWorld);

	*pPos = Eval.m_Pos;
	return Eval.m_Got;
}

//TODO: Maybe drop this and butcher gamecontroller instead
void CGameControllerDDRace::OnReset(CGameWorld *pWorld)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apGameWorlds[i] == pWorld)
		{
			m_ResetWorlds[i] = true;
			break;
		}
	}

	bool Ready = false;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apGameWorlds[i])
		{
			Ready = m_ResetWorlds[i];
			if(!m_ResetWorlds[i])
				break;
		}
	}

	if(Ready)
		IGameController::OnReset(pWorld);
}

void CGameControllerDDRace::SetTuning(CTuningParams &Tuning)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CGameWorld *pWorld = m_apGameWorlds[i];
		if(!pWorld)
			continue;

		pWorld->m_Core.m_Tuning = Tuning;
	}
}

int CGameControllerDDRace::JoinTeam(CPlayer *pPlayer, int Team)
{
	CGameWorld *pNewWorld = m_apGameWorlds[Team];
	if(!pNewWorld)
	{
		//TODO: Create world here
	}

	//TODO: Handle invites and team lock here

	CGameWorld *pOldWorld = pPlayer->GameWorld();
	CCharacter *pChr = pPlayer->GetCharacter();

	dbg_msg("DEBUG", "Joining world %p from %p", pNewWorld, pOldWorld);
	// Remove the character from the old world
	pOldWorld->RemoveEntity(pChr);

	// Add him to the new one
	pNewWorld->InsertEntity(pPlayer->GetCharacter());

	return Team;
}

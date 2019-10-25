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
	m_apTeamWorlds[0] = new CGameWorld();
	m_apTeamWorlds[0]->SetGameServer(pGameServer);
	for(int i = 2; i < MAX_CLIENTS; i++)
		m_apTeamWorlds[i] = nullptr;

	m_pGameType = GAME_NAME;
	SetGameState(IGS_GAME_RUNNING, TIMER_INFINITE);
}

bool CGameControllerDDRace::OnEntity(int Index, vec2 Pos, CTile Tile)
{
	if(IGameController::OnEntity(Index, Pos, Tile))
		return true;

	// Handle our entities here

	return false;
}

void CGameControllerDDRace::Tick()
{
	for(int i = 0; i < MAX_CLIENTS; i++) {
		CGameWorld *pWorld = m_apTeamWorlds[i];
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
			CGameWorld *pWorld = m_apTeamWorlds[i];
			if(!pWorld)
				continue;

			pWorld->Snap(SnappingClient);
		}
	}
	else {
		CGameWorld *pWorld = m_apTeamWorlds[pPlayer->GetDDRaceTeam()];
		dbg_assert(!!pWorld, "Ingame players must have a gameworld");

		pWorld->Snap(SnappingClient);
	}

	IGameController::Snap(SnappingClient);
}

void CGameControllerDDRace::PostSnap()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CGameWorld *pWorld = m_apTeamWorlds[i];
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

void CGameControllerDDRace::SetTuning(CTuningParams &Tuning)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CGameWorld *pWorld = m_apTeamWorlds[i];
		if(!pWorld)
			continue;

		pWorld->m_Core.m_Tuning = Tuning;
	}
}

void CGameControllerDDRace::OnSetPaused(bool Paused)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CGameWorld *pWorld = m_apTeamWorlds[i];
		if(!pWorld)
			continue;

		pWorld->m_Paused = Paused;
	}
}

void CGameControllerDDRace::OnResetRequested()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CGameWorld *pWorld = m_apTeamWorlds[i];
		if(!pWorld)
			continue;

		pWorld->m_ResetRequested = true;
	}
}

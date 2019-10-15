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

CGameControllerDDRace::CGameControllerDDRace(class CGameContext *pGameServer) :
		IGameController(pGameServer)
{
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

}

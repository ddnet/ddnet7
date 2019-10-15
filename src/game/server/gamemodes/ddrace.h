/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_GAMEMODES_DDRACE_H
#define GAME_SERVER_GAMEMODES_DDRACE_H
#include <game/server/gamecontroller.h>

#include <vector>
#include <map>

class CGameControllerDDRace: public IGameController
{
public:

	CGameControllerDDRace(class CGameContext *pGameServer);
};
#endif // GAME_SERVER_GAMEMODES_DDRACE_H

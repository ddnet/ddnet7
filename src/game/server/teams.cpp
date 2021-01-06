/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#include "teams.h"
#include "score.h"
#include <engine/shared/config.h>
#include "gamecontroller.h"

CGameTeams::CGameTeams(CGameContext *pGameContext) :
		m_pGameContext(pGameContext)
{
	Reset();
}

void CGameTeams::Reset()
{
	m_Core.Reset();
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		m_TeamState[i] = TEAMSTATE_EMPTY;
		m_TeeFinished[i] = false;
		m_MembersCount[i] = 0;
		m_LastChat[i] = 0;
		m_TeamLocked[i] = false;
		m_IsSaving[i] = false;
		m_Invited[i] = 0;
		m_Practice[i] = false;
	}
}

void CGameTeams::OnCharacterStart(int ClientID)
{
	int Tick = Server()->Tick();
	CCharacter* pStartingChar = Character(ClientID);
	if(!pStartingChar)
		return;
	if(m_Core.Team(ClientID) != TEAM_FLOCK && pStartingChar->m_DDRaceState == DDRACE_FINISHED)
		return;
	if(m_Core.Team(ClientID) == TEAM_FLOCK
			|| m_Core.Team(ClientID) == TEAM_SUPER)
	{
		pStartingChar->m_DDRaceState = DDRACE_STARTED;
		pStartingChar->m_StartTime = Tick;
		return;
	}
	bool Waiting = false;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(m_Core.Team(ClientID) != m_Core.Team(i))
			continue;
		CPlayer* pPlayer = GetPlayer(i);
		if(!pPlayer || !pPlayer->IsPlaying())
			continue;
		if(GetDDRaceState(pPlayer) != DDRACE_FINISHED)
			continue;

		Waiting = true;
		pStartingChar->m_DDRaceState = DDRACE_NONE;

		if(m_LastChat[ClientID] + Server()->TickSpeed()
				+ g_Config.m_SvChatDelay < Tick)
		{
			char aBuf[128];
			str_format(
					aBuf,
					sizeof(aBuf),
					"%s has finished and didn't go through start yet, wait for him or join another team.",
					Server()->ClientName(i));
			GameServer()->SendChatTarget(ClientID, aBuf);
			m_LastChat[ClientID] = Tick;
		}
		if(m_LastChat[i] + Server()->TickSpeed()
				+ g_Config.m_SvChatDelay < Tick)
		{
			char aBuf[128];
			str_format(
					aBuf,
					sizeof(aBuf),
					"%s wants to start a new round, kill or walk to start.",
					Server()->ClientName(ClientID));
			GameServer()->SendChatTarget(i, aBuf);
			m_LastChat[i] = Tick;
		}
	}

	if(m_TeamState[m_Core.Team(ClientID)] < TEAMSTATE_STARTED && !Waiting)
	{
		ChangeTeamState(m_Core.Team(ClientID), TEAMSTATE_STARTED);

		char aBuf[512];
		str_format(
				aBuf,
				sizeof(aBuf),
				"Team %d started with these %d players: ",
				m_Core.Team(ClientID),
				Count(m_Core.Team(ClientID)));

		bool First = true;

		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(m_Core.Team(ClientID) == m_Core.Team(i))
			{
				CPlayer* pPlayer = GetPlayer(i);
				// TODO: THE PROBLEM IS THAT THERE IS NO CHARACTER SO START TIME CAN'T BE SET!
				if(pPlayer && (pPlayer->IsPlaying() || TeamLocked(m_Core.Team(ClientID))))
				{
					SetDDRaceState(pPlayer, DDRACE_STARTED);
					SetStartTime(pPlayer, Tick);

					if(First)
						First = false;
					else
						str_append(aBuf, ", ", sizeof(aBuf));

					str_append(aBuf, GameServer()->Server()->ClientName(i), sizeof(aBuf));
				}
			}
		}

		if(g_Config.m_SvTeam < 3 && g_Config.m_SvTeamMaxSize != 2 && g_Config.m_SvPauseable && m_MembersCount[m_Core.Team(ClientID)] > 1)
		{
			for(int i = 0; i < MAX_CLIENTS; ++i)
			{
				CPlayer* pPlayer = GetPlayer(i);
				if(m_Core.Team(ClientID) == m_Core.Team(i) && pPlayer && (pPlayer->IsPlaying() || TeamLocked(m_Core.Team(ClientID))))
				{
					GameServer()->SendChatTarget(i, aBuf);
				}
			}
		}
	}
}

void CGameTeams::OnCharacterFinish(int ClientID)
{
	if (m_Core.Team(ClientID) == TEAM_FLOCK
			|| m_Core.Team(ClientID) == TEAM_SUPER)
	{
		CPlayer* pPlayer = GetPlayer(ClientID);
		if (pPlayer && pPlayer->IsPlaying())
		{
			float Time = (float)(Server()->Tick() - GetStartTime(pPlayer))
					/ ((float)Server()->TickSpeed());
			if (Time < 0.000001f)
				return;
			char aTimestamp[TIMESTAMP_STR_LENGTH];
			str_timestamp_format(aTimestamp, sizeof(aTimestamp), FORMAT_SPACE); // 2019-04-02 19:41:58

			OnFinish(pPlayer, Time, aTimestamp);
		}
	}
	else
	{
		m_TeeFinished[ClientID] = true;

		CheckTeamFinished(m_Core.Team(ClientID));
	}
}

void CGameTeams::CheckTeamFinished(int Team)
{
	if (TeamFinished(Team))
	{
		CPlayer *TeamPlayers[MAX_CLIENTS];
		unsigned int PlayersCount = 0;

		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (Team == m_Core.Team(i))
			{
				CPlayer* pPlayer = GetPlayer(i);
				if (pPlayer && pPlayer->IsPlaying())
				{
					m_TeeFinished[i] = false;

					TeamPlayers[PlayersCount++] = pPlayer;
				}
			}
		}

		if (PlayersCount > 0)
		{
			float Time = (float)(Server()->Tick() - GetStartTime(TeamPlayers[0]))
					/ ((float)Server()->TickSpeed());
			if (Time < 0.000001f)
			{
				return;
			}

			if(m_Practice[Team])
			{
				ChangeTeamState(Team, TEAMSTATE_FINISHED);

				char aBuf[256];
				str_format(aBuf, sizeof(aBuf),
					"Your team would've finished in: %d minute(s) %5.2f second(s). Since you had practice mode enabled your rank doesn't count.",
					(int)Time / 60, Time - ((int)Time / 60 * 60));

				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(m_Core.Team(i) == Team && GameServer()->m_apPlayers[i])
					{
						GameServer()->SendChatTarget(i, aBuf);
					}
				}

				for(unsigned int i = 0; i < PlayersCount; ++i)
				{
					SetDDRaceState(TeamPlayers[i], DDRACE_FINISHED);
				}

				return;
			}

			char aTimestamp[TIMESTAMP_STR_LENGTH];
			str_timestamp_format(aTimestamp, sizeof(aTimestamp), FORMAT_SPACE); // 2019-04-02 19:41:58

			for (unsigned int i = 0; i < PlayersCount; ++i)
				OnFinish(TeamPlayers[i], Time, aTimestamp);
			ChangeTeamState(Team, TEAMSTATE_FINISHED); //TODO: Make it better
			//ChangeTeamState(Team, TEAMSTATE_OPEN);
			OnTeamFinish(TeamPlayers, PlayersCount, Time, aTimestamp);
		}
	}
}

bool CGameTeams::SetCharacterTeam(int ClientID, int Team)
{
	//Check on wrong parameters. +1 for TEAM_SUPER
	if (ClientID < 0 || ClientID >= MAX_CLIENTS || Team < 0
			|| Team >= MAX_CLIENTS + 1)
		return false;
	//You can join to TEAM_SUPER at any time, but any other group you cannot if it started
	if (Team != TEAM_SUPER && m_TeamState[Team] > TEAMSTATE_OPEN)
		return false;
	//No need to switch team if you there
	if (m_Core.Team(ClientID) == Team)
		return false;
	if (!Character(ClientID))
		return false;
	//You cannot be in TEAM_SUPER if you not super
	if (Team == TEAM_SUPER && !Character(ClientID)->m_Super)
		return false;
	//if you begin race
	if (Character(ClientID)->m_DDRaceState != DDRACE_NONE && Team != TEAM_SUPER)
		return false;
	//No cheating through noob filter with practice and then leaving team
	if (m_Practice[m_Core.Team(ClientID)])
		return false;

	SetForceCharacterTeam(ClientID, Team);

	//GameServer()->CreatePlayerSpawn(Character(id)->m_Core.m_Pos, TeamMask());
	return true;
}

void CGameTeams::SetForceCharacterTeam(int ClientID, int Team)
{
	int OldTeam = m_Core.Team(ClientID);

	if (Team != m_Core.Team(ClientID))
		ForceLeaveTeam(ClientID);
	else
	{
		m_TeeFinished[ClientID] = false;
		if (Count(m_Core.Team(ClientID)) > 0)
			m_MembersCount[m_Core.Team(ClientID)]--;
	}

	m_Core.Team(ClientID, Team);

	if (m_Core.Team(ClientID) != TEAM_SUPER)
		m_MembersCount[m_Core.Team(ClientID)]++;
	if (Team != TEAM_SUPER && (m_TeamState[Team] == TEAMSTATE_EMPTY || m_TeamLocked[Team]))
	{
		if (!m_TeamLocked[Team])
			ChangeTeamState(Team, TEAMSTATE_OPEN);

		if (GameServer()->Collision()->m_NumSwitchers > 0) {
			for (int i = 0; i < GameServer()->Collision()->m_NumSwitchers+1; ++i)
			{
				GameServer()->Collision()->m_pSwitchers[i].m_Status[Team] = GameServer()->Collision()->m_pSwitchers[i].m_Initial;
				GameServer()->Collision()->m_pSwitchers[i].m_EndTick[Team] = 0;
				GameServer()->Collision()->m_pSwitchers[i].m_Type[Team] = TILE_SWITCHOPEN;
			}
		}
	}

	if (OldTeam != Team)
	{
		for(int LoopClientID = 0; LoopClientID < MAX_CLIENTS; ++LoopClientID)
			if(GetPlayer(LoopClientID))
				SendTeamsState(LoopClientID);

		if(GetPlayer(ClientID))
			GetPlayer(ClientID)->m_VotedForPractice = false;
	}
}

void CGameTeams::ForceLeaveTeam(int ClientID)
{
	m_TeeFinished[ClientID] = false;

	if (m_Core.Team(ClientID) != TEAM_FLOCK
			&& m_Core.Team(ClientID) != TEAM_SUPER
			&& m_TeamState[m_Core.Team(ClientID)] != TEAMSTATE_EMPTY)
	{
		bool NoOneInOldTeam = true;
		for (int i = 0; i < MAX_CLIENTS; ++i)
			if (i != ClientID && m_Core.Team(ClientID) == m_Core.Team(i))
			{
				NoOneInOldTeam = false; //all good exists someone in old team
				break;
			}
		if (NoOneInOldTeam)
		{
			m_TeamState[m_Core.Team(ClientID)] = TEAMSTATE_EMPTY;

			// unlock team when last player leaves
			SetTeamLock(m_Core.Team(ClientID), false);
			ResetInvited(m_Core.Team(ClientID));
			m_Practice[m_Core.Team(ClientID)] = false;
		}
	}

	if (Count(m_Core.Team(ClientID)) > 0)
		m_MembersCount[m_Core.Team(ClientID)]--;
}

int CGameTeams::Count(int Team) const
{
	if (Team == TEAM_SUPER)
		return -1;
	return m_MembersCount[Team];
}

void CGameTeams::ChangeTeamState(int Team, int State)
{
	int OldState = m_TeamState[Team];
	m_TeamState[Team] = State;
	onChangeTeamState(Team, State, OldState);
}

void CGameTeams::onChangeTeamState(int Team, int State, int OldState)
{
	if (OldState != State && State == TEAMSTATE_STARTED)
	{
		// OnTeamStateStarting
	}
	if (OldState != State && State == TEAMSTATE_FINISHED)
	{
		// OnTeamStateFinishing
	}
}

bool CGameTeams::TeamFinished(int Team)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
		if (m_Core.Team(i) == Team && !m_TeeFinished[i])
			return false;
	return true;
}

int64_t CGameTeams::TeamMask(int Team, int ExceptID, int Asker)
{
	int64_t Mask = 0;

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (i == ExceptID)
			continue; // Explicitly excluded
		if (!GetPlayer(i))
			continue; // Player doesn't exist

		if (!(GetPlayer(i)->GetTeam() == -1 || GetPlayer(i)->IsPaused()))
		{ // Not spectator
			if (i != Asker)
			{ // Actions of other players
				if (!Character(i))
					continue; // Player is currently dead
				if (!GetPlayer(i)->m_ShowOthers)
				{
					if (m_Core.GetSolo(Asker))
						continue; // When in solo part don't show others
					if (m_Core.GetSolo(i))
						continue; // When in solo part don't show others
					if (m_Core.Team(i) != Team && m_Core.Team(i) != TEAM_SUPER)
						continue; // In different teams
				} // ShowOthers
			} // See everything of yourself
		}
		else if (GetPlayer(i)->GetSpecMode() == SPEC_PLAYER)
		{ // Spectating specific player
			if (GetPlayer(i)->GetSpectatorID() != Asker)
			{ // Actions of other players
				if (!Character(GetPlayer(i)->GetSpectatorID()))
					continue; // Player is currently dead
				if (!GetPlayer(i)->m_ShowOthers)
				{
					if (m_Core.GetSolo(Asker))
						continue; // When in solo part don't show others
					if (m_Core.GetSolo(GetPlayer(i)->GetSpectatorID()))
						continue; // When in solo part don't show others
					if (m_Core.Team(GetPlayer(i)->GetSpectatorID()) != Team && m_Core.Team(GetPlayer(i)->GetSpectatorID()) != TEAM_SUPER)
						continue; // In different teams
				} // ShowOthers
			} // See everything of player you're spectating
		}
		else
		{ // Freeview
			if (GetPlayer(i)->m_SpecTeam)
			{ // Show only players in own team when spectating
				if (m_Core.Team(i) != Team && m_Core.Team(i) != TEAM_SUPER)
					continue; // in different teams
			}
		}

		Mask |= 1LL << i;
	}
	return Mask;
}

void CGameTeams::SendTeamsState(int ClientID)
{
	if (g_Config.m_SvTeam == 3)
		return;

	if (!m_pGameContext->m_apPlayers[ClientID] || m_pGameContext->m_apPlayers[ClientID]->m_DDraceVersion < VERSION_DDRACE_TEAMS)
		return;

	CMsgPacker Msg(NETMSGTYPE_SV_TEAMSSTATE);

	for(unsigned i = 0; i < MAX_CLIENTS; i++)
		Msg.AddInt(m_Core.Team(i));

	Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

int CGameTeams::GetDDRaceState(CPlayer* Player)
{
	if (!Player)
		return DDRACE_NONE;

	CCharacter* pChar = Player->GetCharacter();
	if (pChar)
		return pChar->m_DDRaceState;
	return DDRACE_NONE;
}

void CGameTeams::SetDDRaceState(CPlayer* Player, int DDRaceState)
{
	if (!Player)
		return;

	CCharacter* pChar = Player->GetCharacter();
	if (pChar)
		pChar->m_DDRaceState = DDRaceState;
}

int CGameTeams::GetStartTime(CPlayer* Player)
{
	if (!Player)
		return 0;

	CCharacter* pChar = Player->GetCharacter();
	if (pChar)
		return pChar->m_StartTime;
	return 0;
}

void CGameTeams::SetStartTime(CPlayer* Player, int StartTime)
{
	if (!Player)
		return;

	CCharacter* pChar = Player->GetCharacter();
	if (pChar)
		pChar->m_StartTime = StartTime;
}

void CGameTeams::SetCpActive(CPlayer* Player, int CpActive)
{
	if (!Player)
		return;

	CCharacter* pChar = Player->GetCharacter();
	if (pChar)
		pChar->m_CpActive = CpActive;
}

float *CGameTeams::GetCpCurrent(CPlayer* Player)
{
	if (!Player)
		return NULL;

	CCharacter* pChar = Player->GetCharacter();
	if (pChar)
		return pChar->m_CpCurrent;
	return NULL;
}

void CGameTeams::OnTeamFinish(CPlayer** Players, unsigned int Size, float Time, const char *pTimestamp)
{
	bool CallSaveScore = false;

#if defined(CONF_SQL)
	CallSaveScore = g_Config.m_SvUseSQL;
#endif

	int PlayerCIDs[MAX_CLIENTS];

	for(unsigned int i = 0; i < Size; i++)
	{
		PlayerCIDs[i] = Players[i]->GetCID();

		if(g_Config.m_SvRejoinTeam0 && g_Config.m_SvTeam != 3 && (m_Core.Team(Players[i]->GetCID()) >= TEAM_SUPER || !m_TeamLocked[m_Core.Team(Players[i]->GetCID())]))
		{
			SetForceCharacterTeam(Players[i]->GetCID(), 0);
			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "%s joined team 0",
					GameServer()->Server()->ClientName(Players[i]->GetCID()));
			GameServer()->SendChat(-1, CHAT_ALL, -1, aBuf);
		}
	}

	if (CallSaveScore && Size >= 2)
		GameServer()->Score()->SaveTeamScore(PlayerCIDs, Size, Time, pTimestamp);
}

void CGameTeams::OnFinish(CPlayer* Player, float Time, const char *pTimestamp)
{
	if (!Player || !Player->IsPlaying())
		return;
	//TODO:DDRace:btd: this ugly
	CPlayerData *pData = GameServer()->Score()->PlayerData(Player->GetCID());
	char aBuf[128];
	SetCpActive(Player, -2);
	str_format(aBuf, sizeof(aBuf),
			"%s finished in: %d minute(s) %5.2f second(s)",
			Server()->ClientName(Player->GetCID()), (int)Time / 60,
			Time - ((int)Time / 60 * 60));
	if (g_Config.m_SvHideScore || !g_Config.m_SvSaveWorseScores)
		GameServer()->SendChatTarget(Player->GetCID(), aBuf);
	else
		GameServer()->SendChat(-1, CHAT_ALL, -1, aBuf);

	float Diff = fabs(Time - pData->m_BestTime);

	if (Time - pData->m_BestTime < 0)
	{
		// new record \o/

		if (Diff >= 60)
			str_format(aBuf, sizeof(aBuf), "New record: %d minute(s) %5.2f second(s) better.",
					(int)Diff / 60, Diff - ((int)Diff / 60 * 60));
		else
			str_format(aBuf, sizeof(aBuf), "New record: %5.2f second(s) better.",
					Diff);
		if (g_Config.m_SvHideScore || !g_Config.m_SvSaveWorseScores)
			GameServer()->SendChatTarget(Player->GetCID(), aBuf);
		else
			GameServer()->SendChat(-1, CHAT_ALL, -1, aBuf);
	}
	else if (pData->m_BestTime != 0) // tee has already finished?
	{
		if (Diff <= 0.005f)
		{
			GameServer()->SendChatTarget(Player->GetCID(),
					"You finished with your best time.");
		}
		else
		{
			if (Diff >= 60)
				str_format(aBuf, sizeof(aBuf), "%d minute(s) %5.2f second(s) worse, better luck next time.",
						(int)Diff / 60, Diff - ((int)Diff / 60 * 60));
			else
				str_format(aBuf, sizeof(aBuf),
						"%5.2f second(s) worse, better luck next time.",
						Diff);
			GameServer()->SendChatTarget(Player->GetCID(), aBuf); //this is private, sent only to the tee
		}
	}

	bool CallSaveScore = false;
#if defined(CONF_SQL)
	CallSaveScore = g_Config.m_SvUseSQL && g_Config.m_SvSaveWorseScores;
#endif

	if (!pData->m_BestTime || Time < pData->m_BestTime)
	{
		// update the score
		pData->Set(Time, GetCpCurrent(Player));
		CallSaveScore = true;
	}

	if (CallSaveScore)
		if (g_Config.m_SvNamelessScore || !str_startswith(Server()->ClientName(Player->GetCID()), "nameless tee"))
			GameServer()->Score()->SaveScore(Player->GetCID(), Time, pTimestamp,
					GetCpCurrent(Player), Player->m_NotEligibleForFinish);

	// update server best time
	if (GameServer()->m_pController->m_CurrentRecord == 0
			|| Time < GameServer()->m_pController->m_CurrentRecord)
	{
		// check for nameless
		if (g_Config.m_SvNamelessScore || !str_startswith(Server()->ClientName(Player->GetCID()), "nameless tee"))
		{
			GameServer()->m_pController->m_CurrentRecord = Time;
			//dbg_msg("character", "Finish");
		}
	}

	SetDDRaceState(Player, DDRACE_FINISHED);
	// set player score
	if (!pData->m_CurrentTime || pData->m_CurrentTime > Time)
	{
		pData->m_CurrentTime = Time;
	}

	int TTime = 0 - (int)Time;
	if (Player->m_Score < TTime || !Player->m_HasFinishScore)
	{
		Player->m_Score = TTime;
		Player->m_HasFinishScore = true;
	}

	Player->m_Score = GameServer()->Score()->PlayerData(Player->GetCID())->m_BestTime;
}

void CGameTeams::OnCharacterSpawn(int ClientID)
{
	m_Core.SetSolo(ClientID, false);

	if (m_Core.Team(ClientID) >= TEAM_SUPER || !m_TeamLocked[m_Core.Team(ClientID)])
		SetForceCharacterTeam(ClientID, 0);
}

void CGameTeams::OnCharacterDeath(int ClientID, int Weapon)
{
	m_Core.SetSolo(ClientID, false);

	int Team = m_Core.Team(ClientID);
	bool Locked = TeamLocked(Team) && Weapon != WEAPON_GAME;

	if(!Locked)
	{
		SetForceCharacterTeam(ClientID, 0);
		CheckTeamFinished(Team);
	}
	else
	{
		SetForceCharacterTeam(ClientID, Team);

		if(GetTeamState(Team) != TEAMSTATE_OPEN)
		{
			ChangeTeamState(Team, CGameTeams::TEAMSTATE_OPEN);

			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "Everyone in your locked team was killed because '%s' %s.", Server()->ClientName(ClientID), Weapon == WEAPON_SELF ? "killed" : "died");

			m_Practice[Team] = false;

			for(int i = 0; i < MAX_CLIENTS; i++)
				if(m_Core.Team(i) == Team && GameServer()->m_apPlayers[i])
				{
					GameServer()->m_apPlayers[i]->m_VotedForPractice = false;

					if(i != ClientID)
					{
						GameServer()->m_apPlayers[i]->KillCharacter(WEAPON_SELF);
						if (Weapon == WEAPON_SELF)
							GameServer()->m_apPlayers[i]->Respawn(true); // spawn the rest of team with weak hook on the killer
					}
					if(m_MembersCount[Team] > 1)
						GameServer()->SendChatTarget(i, aBuf);
				}
		}
	}
}

void CGameTeams::SetTeamLock(int Team, bool Lock)
{
	if(Team > TEAM_FLOCK && Team < TEAM_SUPER)
		m_TeamLocked[Team] = Lock;
}

void CGameTeams::ResetInvited(int Team)
{
	m_Invited[Team] = 0;
}

void CGameTeams::SetClientInvited(int Team, int ClientID, bool Invited)
{
	if(Team > TEAM_FLOCK && Team < TEAM_SUPER)
	{
		if(Invited)
			m_Invited[Team] |= 1ULL << ClientID;
		else
			m_Invited[Team] &= ~(1ULL << ClientID);
	}
}

void CGameTeams::KillSavedTeam(int Team)
{
	// Set so that no finish is accidentally given to some of the players
	ChangeTeamState(Team, CGameTeams::TEAMSTATE_OPEN);

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_Core.Team(i) == Team && GameServer()->m_apPlayers[i])
		{
			// Set so that no finish is accidentally given to some of the players
			GameServer()->m_apPlayers[i]->GetCharacter()->m_DDRaceState = DDRACE_NONE;
			m_TeeFinished[i] = false;
		}
	}

	for (int i = 0; i < MAX_CLIENTS; i++)
		if(m_Core.Team(i) == Team && GameServer()->m_apPlayers[i])
			GameServer()->m_apPlayers[i]->ThreadKillCharacter(-2);

	ChangeTeamState(Team, CGameTeams::TEAMSTATE_EMPTY);

	// unlock team when last player leaves
	SetTeamLock(Team, false);
	ResetInvited(Team);

	m_Practice[Team] = false;
}

// DoD GAMERULES restore - dod/dlls/dod_gamerules.cpp
// GhidraAddr = link + 0x10000. Compared against SDK ~/rev/halflife
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "items.h"
#include "game.h"
#include "scripted.h"
#include <skill.h>
#include <objective.h>
#include <voice_gamemgr.h>
#include "dod_shared.h"
extern DodClassInfo_t g_ClassInfo[];
extern DodClassInfo_t g_ParaClassInfo[];
extern DLL_GLOBAL CGameRules *g_pGameRules;
extern DLL_GLOBAL BOOL g_fGameOver;
extern float g_flIntermissionStartTime;
extern int m_SaveRoundState;
struct mapcycle_item_s
{
	struct mapcycle_item_s *next;
	char mapname[32];
	int minplayers;
	int maxplayers;
	char rulebuffer[1024];
};
typedef struct mapcycle_item_s mapcycle_item_t;
struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
};
typedef struct mapcycle_s mapcycle_t;
extern void DestroyMapCycle( mapcycle_t *cycle );
extern int ReloadMapCycleFile( const char *filename, mapcycle_t *cycle );
extern void GetNextMap( mapcycle_t cycle, int cur, char *nextMap, char *firstMap );
struct TESTSTATS
{
	int lastAmmo;
	float lastHealth;
	float lastOutputTime;
	float nextOutputTime;
	float dataTime;
	float gameTime;
	float lastGameTime;
};
extern TESTSTATS gStats;
extern void SET_CLIENT_KEYVALUE( int idx, char *buf, const char *key, const char *val );
extern void Host_Say( edict_t *pEntity, int teamonly );
extern char *GET_INFOKEYBUFFER( edict_t *pEntity );
extern char *INFOKEY_VALUE( char *buf, const char *key );
extern void UpdateStatsFile( float t, char *m, float h, float a, int s );
extern float AmmoDamage( const char *psz );
extern void FireDODCamera( const char *p1, CBaseEntity *p2, CBaseEntity *p3, USE_TYPE t, float f );
extern int get_maptime_left( void );
extern CVoiceGameMgr g_VoiceGameMgr;
extern cvar_t *pcvar_mp_clan_match;
extern cvar_t *pcvar_mp_log_scores;
extern cvar_t *pcvar_mp_timelimit;
extern cvar_t *pcvar_mp_spawnbazookas;
extern cvar_t *pcvar_mp_combinemglimits;
extern cvar_t *pcvar_mp_alliesclasses;
extern cvar_t *pcvar_mp_axisclasses;
extern int gmsgGameRules;
extern void ExtractCommandString( char *s, char *szCommand );
extern cvar_t *pcvar_classlimitcvars[28];
extern char classlimitcvarNames[27][32];
extern cvar_t *pcvar_mp_chattime;
extern cvar_t *pcvar_mp_tkpenalty;
extern int gmsgScoreShort;
extern int gmsgHLTV;
extern int gmsgDeathMsg;
extern int gmsgScoreInfoLong;
extern int gmsgScoreInfo;
extern int gmsgSpectator;
extern int gmsgRoundState;
extern int gmsgTeamScore;
extern int gmsgWaveStatus;
extern int gmsgWaveTime;
extern cvar_t cvar_timelimit;
extern cvar_t cvar_weaponstay;
extern int g_teamplay;
extern cvar_t *pcvar_mp_teamlimit;
extern cvar_t *pcvar_mp_allowspectators;
extern cvar_t *pcvar_mp_friendlyfire;
extern cvar_t *pcvar_mp_deathmsg;
extern cvar_t *pcvar_mp_clan_timer;
extern cvar_t *pcvar_mp_clan_restartround;
extern cvar_t *pcvar_mp_clan_readyrestart;
extern cvar_t *pcvar_mp_clan_match_warmup;
extern cvar_t *pcvar_mp_clan_respawntime;
extern int gmsgPClass;
extern int gmsgPStatus;
extern int gmsgPTeam;
extern int gmsgClanTimer;
extern int gmsgServerName;
extern int gmsgMOTD;
extern void ShowVGUI( CBasePlayer *pPlayer, int iMenu, int iParam );
// .data:0x187460 size 0x200 (4 * 128) - team names, index = team number
char g_szTeamNames[4][128] = { "", "Allies", "Axis", "Spectators" };
void RespawnAllies( void );
void RespawnAxis( void );
void ResetEnts( void );
// SDK-IDENTICAL: dlls/multiplay_gamerules.cpp IsMultiplayer TRUE
// SDK-IDENTICAL: dlls/teamplay_gamerules.cpp IsTeamplay TRUE
// SDK-IDENTICAL: dlls/teamplay_gamerules.cpp FPlayerCanTakeDamage teammate check
// SDK-IDENTICAL: dlls/gamerules.cpp CGameRules CanHaveAmmo
// SDK-IDENTICAL: dlls/gamerules.cpp CGameRules CanHavePlayerItem base
// SDK-IDENTICAL: dlls/stats.cpp UpdateStatsFile
// SDK-IDENTICAL: dlls/stats.cpp InitStats
// SDK-IDENTICAL: dlls/singleplay_gamerules.cpp Think empty stub
// .text:000e3cf0 void CDoDTeamPlay::RefreshSkillData() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::RefreshSkillData( void )
{
	CGameRules::RefreshSkillData();
	gSkillData.plrDmgCOLT = 40.0f;
	gSkillData.plrDmgLUGER = 40.0f;
	gSkillData.plrDmgCARBINE = 40.0f;
	gSkillData.plrDmgTHOMPSON = 40.0f;
	gSkillData.plrDmgMP40 = 40.0f;
	gSkillData.plrDmgGREASEGUN = 40.0f;
	gSkillData.plrDmgSten = 40.0f;
	gSkillData.plrDmgWebley = 40.0f;
	gSkillData.plrDmgBAR = 85.0f;
	gSkillData.plrDmg30CAL = 85.0f;
	gSkillData.plrDmgMG42 = 85.0f;
	gSkillData.plrDmgMG34 = 85.0f;
	gSkillData.plrDmgFG42 = 85.0f;
	gSkillData.plrDmgBren = 85.0f;
	gSkillData.plrDmgCrowbar = 60.0f;
	gSkillData.plrDmgBayonet = 60.0f;
	gSkillData.plrDmgGarandButt = 60.0f;
	gSkillData.plrDmgEnfieldButt = 60.0f;
	gSkillData.plrDmgK43Butt = 60.0f;
	gSkillData.plrDmgGARAND = 120.0f;
	gSkillData.plrDmgK43 = 120.0f;
	gSkillData.plrDmgSCOPEDKAR = 160.0f;
	gSkillData.plrDmgMP44 = 50.0f;
	gSkillData.plrDmgSPRING = 160.0f;
	gSkillData.plrDmgKAR = 160.0f;
	gSkillData.plrDmgEnfield = 160.0f;
	gSkillData.plrDmgHandGrenade = 180.0f;
	gSkillData.plrDmgStickGrenade = 180.0f;
	gSkillData.plrDmgBazookaShell = 100.0f;
	gSkillData.plrDmgPiatShell = 100.0f;
	gSkillData.plrDmgPschreckShell = 100.0f;
	gSkillData.plrDmgMortarShell = 180.0f;
}
// .text:000e4910 BOOL CDoDTeamPlay::CanHavePlayerItem(CBasePlayer*, CBasePlayerItem*) (dod/dlls/dod_gamerules.cpp:0)
BOOL CDoDTeamPlay::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pItem )
{
	if ( cvar_weaponstay.value <= 0.0f || ( CBasePlayerItem::ItemInfoArray[pItem->m_iId].iFlags & ITEM_FLAG_LIMITINWORLD ) == 0 )
	{
		unsigned int cls = pItem->Classify();
		unsigned int mask = cls & 8;
		if ( ( cls & 0x3F0 ) != 0 )
			mask = 0x3F0;
		if ( ( cls & 1 ) != 0 )
			mask = 1;
		if ( ( cls & 6 ) == 0 )
		{
			for ( int i = 0; i < 6; i++ )
				for ( CBasePlayerItem *p = pPlayer->m_rgpPlayerItems[i]; p; p = p->m_pNext )
					if ( p->Classify() & mask )
						return FALSE;
		}
	}
	return CGameRules::CanHavePlayerItem( pPlayer, pItem );
}
// .text:000e49e0 edict_t* CDoDTeamPlay::GetPlayerSpawnSpot(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:0)
edict_t *CDoDTeamPlay::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *spot = CGameRules::GetPlayerSpawnSpot( pPlayer );
	if ( spot && IsMultiplayer() && spot->v.target != 0 )
		FireTargets( STRING( spot->v.target ), pPlayer, pPlayer, USE_TOGGLE, 0.0f );
	return spot;
}
void CDoDTeamPlay::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags )
{
	if ( (unsigned int)( flags - 3 ) >= 4 )
		flags = 0;

	int team = pVictim->pev->team;

	if ( ( team == TEAM_ALLIES && m_GamePlayRules.m_bAlliesInfiniteLives ) || ( team == TEAM_AXIS && m_GamePlayRules.m_bAxisInfiniteLives ) )
	{
		if ( !( pVictim->m_flDisplayHistory & HINT_KILLED_WAVETIME_FIRST ) )
		{
			pVictim->m_flDisplayHistory |= HINT_KILLED_WAVETIME_FIRST;
			pVictim->HintMessage( "#Hint_player_killed_wavetime", 0 );
		}
	}
	else if ( !( pVictim->m_flDisplayHistory & HINT_KILLED_WAVETIME_AGAIN ) )
	{
		pVictim->m_flDisplayHistory |= HINT_KILLED_WAVETIME_AGAIN;
		pVictim->HintMessage( "#Hint_player_killed_onelife", 0 );
	}

	if ( m_DisableDeathPenalty )
		return;

	DeathNotice( pVictim, pKiller, pInflictor, flags );

	if ( !m_RoundDoesCount )
		return;

	pVictim->m_iDeaths++;

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreShort );
		WRITE_BYTE( ENTINDEX( pVictim->edict() ) );
		WRITE_SHORT( pVictim->m_iObjScore );
		WRITE_SHORT( (int)pVictim->pev->frags );
		WRITE_SHORT( pVictim->m_iDeaths );
		WRITE_BYTE( 1 );
	MESSAGE_END();

	float flTKPenalty = -1.0f;

	if ( pcvar_mp_tkpenalty )
		flTKPenalty = pcvar_mp_tkpenalty->value;

	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );

	CBasePlayer *pAttacker = (CBasePlayer *)CBaseEntity::Instance( pKiller );

	if ( !pAttacker || pAttacker->Classify() != CLASS_PLAYER || !pAttacker->IsPlayer() )
		return;

	if ( pVictim->pev == pKiller )
		return;

	pAttacker->AddFrags( IPointsForKill( pAttacker, pVictim ), 1 );

	FireTargets( "game_playerkill", pAttacker, pAttacker, USE_TOGGLE, 0 );

	BOOL bTeamKill = pVictim->pev->team == pAttacker->pev->team;
	BOOL bPenalty = flTKPenalty > 0;

	if ( bTeamKill && bPenalty )
	{
		// counted here and once more below, as the original does
		pAttacker->m_iNumTKs++;
	}
	else
	{
		pAttacker->m_iNumTKs = 0;
	}

	if ( bTeamKill )
	{
		if ( bPenalty )
			pAttacker->m_iNumTKs++;

		if ( !( pAttacker->m_flDisplayHistory & HINT_CAREFUL_AROUND_TEAMMATES ) )
		{
			pAttacker->m_flDisplayHistory |= HINT_CAREFUL_AROUND_TEAMMATES;
			pAttacker->HintMessage( "#Hint_careful_around_teammates", 1 );
		}
	}
	else
	{
		pAttacker->m_iNumTKs = 0;

		if ( !( pAttacker->m_flDisplayHistory & HINT_KILLING_ENEMIES ) )
		{
			pAttacker->m_flDisplayHistory |= HINT_KILLING_ENEMIES;
			pAttacker->HintMessage( "#Hint_killing_enemies_is_good", 0 );
		}
	}

	pAttacker->m_flNextDecalTime = gpGlobals->time;

	if ( bTeamKill && bPenalty && (float)pAttacker->m_iNumTKs >= flTKPenalty )
	{
		// the host of a listen server is never kicked
		if ( ENTINDEX( pAttacker->edict() ) != 1 || IS_DEDICATED_SERVER() )
		{
			const char *pszName = "unconnected";

			if ( pAttacker->pev->netname )
			{
				pszName = STRING( pAttacker->pev->netname );

				if ( !*pszName )
					pszName = "unconnected";
			}

			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_kick_tk", (char *)pszName, NULL, NULL, NULL );

			UTIL_LogPrintf( "\"%s<%i>\" has been auto kicked from the game for TKing\n", STRING( pAttacker->pev->netname ),
							GETPLAYERUSERID( pAttacker->edict() ) );

			ClientPrint( pAttacker->pev, HUD_PRINTCONSOLE, "#dod_kicked_reason_tk" );

			CLIENT_COMMAND( pAttacker->edict(), "disconnect\n" );
		}
	}
}
void CDoDTeamPlay::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor, int flags )
{
	BOOL bDeathMsg = FALSE;

	if ( pcvar_mp_deathmsg )
		bDeathMsg = pcvar_mp_deathmsg->value > 0.0;

	// what killed the victim: the index of the killer, the id and the name of the weapon
	int iKillerIndex = 0;
	int iKillerWeapon = 0;
	const char *pszKillerWeapon = "world";
	const char *pszClassname = NULL;
	CBasePlayerWeapon *pWpn = NULL;

	if ( pKiller->flags & FL_CLIENT )
	{
		iKillerIndex = ENTINDEX( pKiller->pContainingEntity );

		if ( pevInflictor )
		{
			if ( pevInflictor == pKiller )
			{
				// the killer used his own weapon
				pWpn = (CBasePlayerWeapon *)( (CBasePlayer *)CBaseEntity::Instance( pKiller ) )->m_pActiveItem;

				if ( pWpn )
				{
					iKillerWeapon = pWpn->m_iId;
					pszClassname = CBasePlayerItem::ItemInfoArray[iKillerWeapon].pszName;
				}
			}
			else
			{
				pszClassname = STRING( pevInflictor->classname );

				if ( !strcmp( pszClassname, "grenade" ) )
					iKillerWeapon = 13;
				else
					iKillerWeapon = !strcmp( pszClassname, "grenade2" ) ? 14 : 0;
			}
		}
	}

	if ( pszClassname )
	{
		if ( !memcmp( pszClassname, "weapon_", 7 ) )
			pszKillerWeapon = pszClassname + 7;
		else if ( !strncmp( pszClassname, "monster_", 8 ) )
			pszKillerWeapon = pszClassname + 8;
		else if ( !strncmp( pszClassname, "func_", 5 ) )
			pszKillerWeapon = pszClassname + 5;
		else
			pszKillerWeapon = strncmp( pszClassname, "shell_", 6 ) ? pszClassname : pszClassname + 6;
	}

	// the name and the id the death message uses depend on how the weapon was used
	BOOL bResolved = FALSE;
	BOOL bSkipBayonet = FALSE;

	if ( pszClassname && iKillerWeapon == 15 )
	{
		pszKillerWeapon = "stick";
		bSkipBayonet = TRUE;
	}
	else if ( pszClassname && iKillerWeapon == 6 )
	{
		pszKillerWeapon = "scopedkar";
		bSkipBayonet = TRUE;
	}

	if ( !bSkipBayonet && !strcmp( pszKillerWeapon, "kar" ) && flags == 3 )
	{
		iKillerWeapon = 37;
		pszKillerWeapon = "bayonet";
		bResolved = TRUE;
	}

	if ( !bResolved && !strcmp( pszKillerWeapon, "garand" ) && flags == 4 )
	{
		iKillerWeapon = 42;
		pszKillerWeapon = "garandbutt";
		bResolved = TRUE;
	}

	if ( !bResolved && !strcmp( pszKillerWeapon, "k43" ) && flags == 6 )
	{
		iKillerWeapon = 42;
		pszKillerWeapon = "k43butt";
		bResolved = TRUE;
	}

	BOOL bNotEnfield = strcmp( pszKillerWeapon, "enfield" ) != 0;

	if ( !bResolved && !bNotEnfield && flags == 5 )
	{
		iKillerWeapon = 43;
		pszKillerWeapon = "enf_bayonet";
		bResolved = TRUE;
	}

	if ( !bResolved && !strcmp( pszKillerWeapon, "m1carbine" ) &&
		 ( ( pKiller->team == TEAM_ALLIES && m_GamePlayRules.m_bAlliesArePara ) ||
		   ( pKiller->team == TEAM_AXIS && m_GamePlayRules.m_bAxisArePara ) ) )
	{
		iKillerWeapon = 36;
		pszKillerWeapon = "fcarbine";
		bResolved = TRUE;
	}

	if ( !bResolved )
	{
		if ( pWpn && !strcmp( pszKillerWeapon, "fg42" ) )
		{
			if ( pWpn->m_iWeaponState & 1 )
			{
				iKillerWeapon = 35;
				pszKillerWeapon = "scoped_fg42";
			}
		}
		else if ( pWpn && !bNotEnfield )
		{
			if ( pWpn->m_iWeaponState & 1 )
			{
				iKillerWeapon = 38;
				pszKillerWeapon = "scoped_enfield";
			}
		}
		else if ( !strcmp( pszKillerWeapon, "grenade" ) && m_GamePlayRules.m_bAlliesAreBrit )
		{
			iKillerWeapon = 39;
			pszKillerWeapon = "mills_bomb";
		}
		else if ( !strcmp( pszKillerWeapon, "amerknife" ) && m_GamePlayRules.m_bAlliesAreBrit )
		{
			iKillerWeapon = 40;
			pszKillerWeapon = "brit_knife";
		}
		else if ( !strcmp( pszKillerWeapon, "bazooka" ) )
		{
			iKillerWeapon = 29;
		}
		else if ( FStrEq( pszKillerWeapon, "pschreck" ) )
		{
			iKillerWeapon = 30;
		}
		else if ( FStrEq( pszKillerWeapon, "piat" ) )
		{
			iKillerWeapon = 31;
		}
		else if ( FStrEq( pszKillerWeapon, "mortar" ) )
		{
			iKillerWeapon = 32;
		}
	}

	if ( bDeathMsg )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
			WRITE_BYTE( iKillerIndex );
			WRITE_BYTE( ENTINDEX( pVictim->edict() ) );
			WRITE_BYTE( iKillerWeapon );
		MESSAGE_END();
	}

	// the log and the code of the director message
	int iDirectorMsg;

	if ( pVictim->pev == pKiller )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n", STRING( pVictim->pev->netname ),
						GETPLAYERUSERID( pVictim->edict() ), GETPLAYERAUTHID( pKiller->pContainingEntity ),
						pKiller->team == TEAM_ALLIES ? "Allies" : "Axis", pszKillerWeapon );

		iDirectorMsg = 0;
	}
	else if ( !( pKiller->flags & FL_CLIENT ) )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n", STRING( pVictim->pev->netname ),
						GETPLAYERUSERID( pVictim->edict() ), GETPLAYERAUTHID( pVictim->edict() ),
						pVictim->pev->team == TEAM_ALLIES ? "Allies" : "Axis", pszKillerWeapon );

		iDirectorMsg = 4;
	}
	else
	{
		CBasePlayer *pKillerPlayer = (CBasePlayer *)CBaseEntity::Instance( pKiller );

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n", STRING( pKillerPlayer->pev->netname ),
						GETPLAYERUSERID( pKillerPlayer->pev->pContainingEntity ),
						GETPLAYERAUTHID( pKillerPlayer->pev->pContainingEntity ), pKiller->team == TEAM_ALLIES ? "Allies" : "Axis",
						STRING( pVictim->pev->netname ), GETPLAYERUSERID( pVictim->edict() ),
						GETPLAYERAUTHID( pVictim->edict() ), pVictim->pev->team == TEAM_ALLIES ? "Allies" : "Axis", pszKillerWeapon );

		if ( !pevInflictor )
		{
			iDirectorMsg = 0;
		}
		else if ( !pKillerPlayer->m_pActiveItem )
		{
			iDirectorMsg = 0;
		}
		else if ( (unsigned int)pKillerPlayer->m_pActiveItem->m_iId > 19 )
		{
			iDirectorMsg = 4;
		}
		else
		{
			int iMask = 1 << pKillerPlayer->m_pActiveItem->m_iId;

			if ( iMask & 0x1E240 )
				iDirectorMsg = 5;
			else if ( iMask & 0x80006 )
				iDirectorMsg = 7;
			else
				iDirectorMsg = 4;
		}
	}

	// the director follows the inflictor, or the killer when there is none
	CBaseEntity *pOther = NULL;

	if ( pevInflictor && g_engfuncs.pfnEntOffsetOfPEntity( pevInflictor->pContainingEntity ) )
		pOther = CBaseEntity::Instance( pevInflictor );
	else if ( g_engfuncs.pfnEntOffsetOfPEntity( pKiller->pContainingEntity ) )
		pOther = CBaseEntity::Instance( pKiller );

	UTIL_SendDirectorMessage( pVictim, pOther, iDirectorMsg );
}
// .text:000ee590 void CDoDTeamPlay::RestartRound() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::RestartRound( void )
{
	m_RoundDoesCount = TRUE;
	SetRoundState( ROUNDSTATE_ALLIES_WON );
	MESSAGE_BEGIN( MSG_ALL, gmsgHLTV );
	WRITE_BYTE( 0 );
	WRITE_BYTE( 228 );
	MESSAGE_END();
	MESSAGE_BEGIN( MSG_ALL, gmsgHLTV );
	WRITE_BYTE( 0 );
	WRITE_BYTE( 0 );
	MESSAGE_END();
	m_RoundParaCanJoin = TRUE;
	m_rRoundStartCount = gpGlobals->time;
	m_rReadyToUnFreeze = 2;
	ResetEnts();
	CBaseEntity *pr = NULL;
	while ( ( pr = UTIL_FindEntityByClassname( pr, "dod_preround" ) ) != NULL )
		( (CDoDPreRound *) pr )->Launch( m_SaveRoundState );
	if ( cvar_timelimit.value < 0.0f )
		CVAR_SET_FLOAT( "timelimit", 0.0f );
	int allies = 0;
	int axis = 0;
	CBaseEntity *first = NULL;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *p = (CBasePlayer *) UTIL_PlayerByIndex( i );
		if ( !p || p->IsObserver() || p->has_disconnected )
			continue;
		int t = p->pev->team;
		if ( t == 0 || t == 3 || p->m_iNextClass == -1 )
			continue;
		if ( ( t == 1 && m_GamePlayRules.m_bAlliesInfiniteLives ) ||
			 ( t == 2 && m_GamePlayRules.m_bAxisInfiniteLives ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgWaveStatus, NULL, p->pev );
			WRITE_BYTE( 0 );
			MESSAGE_END();
		}
		p->HasObject = FALSE;
		p->DelayedSpawn();
		if ( p->pev->team == TEAM_ALLIES )
			allies++;
		else if ( p->pev->team == TEAM_AXIS )
			axis++;
		if ( !first )
			first = p;
	}
	if ( ( !m_GamePlayRules.m_bAlliesInfiniteLives || !m_GamePlayRules.m_bAxisInfiniteLives ) &&
		 ( allies == 0 || axis == 0 ) )
	{
		m_RoundDoesCount = FALSE;
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#game_bogus_round", 0, 0, 0, 0 );
	}
	UTIL_SendDirectorMessage( first, NULL, 2 );
	m_flAlliesWaveTime = 0.0f;
	m_flAxisWaveTime = 0.0f;
	m_flRoundTime = 0.0f;
	m_flCanJoinTime = gpGlobals->time + 10.0f;
	int idx = 0;
	CBaseEntity *e = NULL;
	while ( ( e = UTIL_FindEntityByClassname( e, "dod_capture_area" ) ) != NULL )
	{
		e->Reset( idx );
		idx++;
	}
	e = NULL;
	while ( ( e = UTIL_FindEntityByClassname( e, "dod_object_goal" ) ) != NULL )
	{
		e->Reset( idx );
		idx++;
	}
	e = NULL;
	while ( ( e = UTIL_FindEntityByClassname( e, "func_breakable" ) ) != NULL )
	{
		e->Reset( idx );
		idx++;
	}
}
// .text:000ea470 void CDoDTeamPlay::GoToIntermission() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::GoToIntermission( void )
{
	UTIL_LogPrintf( "Final Scores: Allies: %d   Axis: %d\n", m_iTeamScores[1], m_iTeamScores[2] );
	if ( g_fGameOver )
		return;
	if ( IsClanMatch() && m_bClanMatch )
	{
		m_bClanMatchActive = FALSE;
		SetClanMatch( FALSE );
		SendClientGamerules( NULL );
	}
	MESSAGE_BEGIN( MSG_ALL, SVC_INTERMISSION );
	MESSAGE_END();
	int ct = pcvar_mp_chattime ? (int) pcvar_mp_chattime->value : 10;
	if ( ct < 10 )
	{
		CVAR_SET_FLOAT( "mp_chattime", 10.0f );
		ct = 10;
	}
	else if ( ct > 120 )
	{
		CVAR_SET_FLOAT( "mp_chattime", 120.0f );
		ct = 120;
	}
	m_flIntermissionEndTime = gpGlobals->time + (float) ct;
	g_flIntermissionStartTime = gpGlobals->time;
	g_fGameOver = TRUE;
	m_iEndIntermissionButtonHit = TRUE;
}
// .text:000ea1d0 void CDoDTeamPlay::TeamWon(int) (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::TeamWon( int iTeamWon )
{
	int allies = 0;
	int axis = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *p = (CBasePlayer *) UTIL_PlayerByIndex( i );
		if ( !p )
			continue;
		if ( p->pev->team == TEAM_ALLIES )
			allies++;
		else if ( p->pev->team == TEAM_AXIS )
			axis++;
	}
	edict_t *ed = NULL;
	while ( ( ed = FIND_ENTITY_BY_STRING( ed, "classname", "grenade" ) ) != NULL )
		if ( OFFSET( ed ) != 0 )
			VARS( ed )->iuser1 = 1;
	ed = NULL;
	while ( ( ed = FIND_ENTITY_BY_STRING( ed, "classname", "grenade2" ) ) != NULL )
		if ( OFFSET( ed ) != 0 )
			VARS( ed )->iuser1 = 1;
	UTIL_LogPrintf( "\"%s\" scored \"%i\" with \"%i\" players\n", "Allies", m_iTeamScores[1], allies );
	UTIL_LogPrintf( "\"%s\" scored \"%i\" with \"%i\" players\n", "Axis", m_iTeamScores[2], axis );
	SetRoundState( ROUNDSTATE_PREROUND );
	GoodToGo();
}
// .text:000ea660 void CDoDTeamPlay::EnumerateTeams(int*,int*,int*,int*) (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::EnumerateTeams( int *aA, int *nA, int *aX, int *nX )
{
	int aa = 0;
	int na = 0;
	int ax = 0;
	int nx = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *p = (CBasePlayer *) UTIL_PlayerByIndex( i );
		if ( !p || p->has_disconnected )
			continue;
		if ( p->pev->team == TEAM_ALLIES )
		{
			na++;
			if ( p->IsAlive() )
				aa++;
		}
		else if ( p->pev->team == TEAM_AXIS )
		{
			nx++;
			if ( p->IsAlive() )
				ax++;
		}
	}
	if ( aA )
		*aA = aa;
	if ( nA )
		*nA = na;
	if ( aX )
		*aX = ax;
	if ( nX )
		*nX = nx;
}
void CDoDTeamPlay::TimerExpired( void )
{
	if ( !m_GamePlayRules.m_bUseTimer )
		return;

	if ( m_GamePlayRules.m_iTimerTeam == 1 )
	{
		SetRoundState( ROUNDSTATE_ALLIES_WON );
		AddToTeamScores( 1, m_GamePlayRules.m_iPointsTimerExpires );

		FireDODCamera( "allies_win_cut", NULL, NULL, USE_TOGGLE, 0 );
		FireDODCamera( "axis_loose_cut", NULL, NULL, USE_TOGGLE, 0 );
	}
	else if ( m_GamePlayRules.m_iTimerTeam == 2 )
	{
		SetRoundState( ROUNDSTATE_AXIS_WON );
		AddToTeamScores( 2, m_GamePlayRules.m_iPointsTimerExpires );

		FireDODCamera( "axis_win_cut", NULL, NULL, USE_TOGGLE, 0 );
		FireDODCamera( "allies_loose_cut", NULL, NULL, USE_TOGGLE, 0 );
	}
	else
	{
		SetRoundState( ROUNDSTATE_DRAW );

		CCineMonster *pCine = NULL;

		while ( ( pCine = (CCineMonster *)UTIL_FindEntityByClassname( pCine, "scripted_sequence" ) ) != NULL )
			pCine->DoDReset();

		CBaseEntity *pTrain = NULL;

		while ( ( pTrain = UTIL_FindEntityByClassname( pTrain, "func_train" ) ) != NULL )
			pTrain->RoundRespawn();

		FireTargets( "para_draw", NULL, NULL, USE_TOGGLE, 0 );

		FireDODCamera( "axis_draw_cut", NULL, NULL, USE_TOGGLE, 0 );
		FireDODCamera( "allies_draw_cut", NULL, NULL, USE_TOGGLE, 0 );
	}

	FireTargets( STRING( m_GamePlayRules.m_sTimerExpireTarget ), NULL, NULL, USE_TOGGLE, 0 );

	m_SaveRoundState = m_RoundState;

	SetRestartTime( gpGlobals->time + 7.0 );
}
void CDoDTeamPlay::Think( void )
{
	g_VoiceGameMgr.Update( gpGlobals->frametime );

	if ( g_fGameOver )
	{
		// the dead players watch the intermission, not the respawn countdown
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

			if ( !pPlayer || pPlayer->IsAlive() )
				continue;

			pPlayer->StopObserver();

			if ( ( pPlayer->pev->team == TEAM_ALLIES && m_GamePlayRules.m_bAlliesInfiniteLives ) ||
				 ( pPlayer->pev->team == TEAM_AXIS && m_GamePlayRules.m_bAxisInfiniteLives ) )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgWaveStatus, NULL, pPlayer->edict() );
					WRITE_BYTE( 0 );
				MESSAGE_END();
			}
		}

		float flChatTime = pcvar_mp_chattime ? pcvar_mp_chattime->value : 0;

		if ( pcvar_mp_chattime )
		{
			int iChatTime = (int)flChatTime;

			if ( iChatTime <= 9 )
			{
				CVAR_SET_FLOAT( "mp_chattime", 10 );
				flChatTime = pcvar_mp_chattime->value;
			}
			else if ( iChatTime > 120 )
			{
				CVAR_SET_FLOAT( "mp_chattime", 120 );
				flChatTime = pcvar_mp_chattime->value;
			}
		}

		m_flIntermissionEndTime = flChatTime + g_flIntermissionStartTime;

		if ( gpGlobals->time > m_flIntermissionEndTime &&
			 ( m_iEndIntermissionButtonHit || gpGlobals->time > g_flIntermissionStartTime + 120.0 ) )
		{
			int iAllies = 0;
			int iAxis = 0;

			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

				if ( !pPlayer )
					continue;

				if ( pPlayer->pev->team == TEAM_ALLIES )
					iAllies++;
				else if ( pPlayer->pev->team == TEAM_AXIS )
					iAxis++;
			}

			UTIL_LogPrintf( "\"%s\" scored \"%i\" with \"%i\" players\n", "Allies", TeamScores( 1 ), iAllies );
			UTIL_LogPrintf( "\"%s\" scored \"%i\" with \"%i\" players\n", "Axis", TeamScores( 2 ), iAxis );

			SetMapChangeFromTime( 0 );
			ChangeLevel();
		}

		return;
	}

	float flTimeLimit = 3600.0f;

	if ( pcvar_mp_timelimit )
		flTimeLimit = 60.0f * pcvar_mp_timelimit->value;

	if ( get_maptime_left() <= 0 && (int)flTimeLimit )
	{
		GoToIntermission();
		return;
	}

	if ( gpGlobals->time > m_fLogScoresTime && pcvar_mp_log_scores && pcvar_mp_log_scores->value > 0.0 )
	{
		float flDelay = 1.0f;

		if ( pcvar_mp_log_scores->value >= 1.0 )
		{
			flDelay = pcvar_mp_log_scores->value;

			if ( flDelay > 600.0 )
				flDelay = 600.0;
		}

		UTIL_LogPrintf( "Team Scores: Allies: %d, Axis: %d\n", TeamScores( 1 ), TeamScores( 2 ) );

		m_fLogScoresTime = gpGlobals->time + flDelay;
	}

	if ( m_RoundState == ROUNDSTATE_RUNNING && ( !m_GamePlayRules.m_bAlliesInfiniteLives || !m_GamePlayRules.m_bAxisInfiniteLives ) )
	{
		m_flRoundTime += gpGlobals->frametime;
		m_RoundParaCanJoin = m_flRoundTime <= 30.0;
	}

	if ( IsClanMatch() )
	{
		if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value <= 0.0 )
		{
			SetClanMatch( FALSE );
			setMapTime();
		}
		else if ( pcvar_mp_clan_restartround && pcvar_mp_clan_restartround->value > 0.0 )
		{
			StartClanRestart();
			CVAR_SET_FLOAT( "mp_clan_restartround", 0 );
		}
		else if ( pcvar_mp_clan_readyrestart && pcvar_mp_clan_readyrestart->value > 0.0 )
		{
			m_bAwaitingReadyRestart = TRUE;
			m_bHeardAlliesReady = FALSE;
			m_bHeardAxisReady = FALSE;

			UTIL_ClientPrintAll( HUD_PRINTCENTER, "#clan_ready_rules", (char *)CVAR_GET_STRING( "mp_clan_ready_signal" ), NULL, NULL, NULL );

			CVAR_SET_FLOAT( "mp_clan_readyrestart", 0 );
		}
		else if ( pcvar_mp_clan_match_warmup && pcvar_mp_clan_match_warmup->value > 0.0 )
		{
			UTIL_ClientPrintAll( HUD_PRINTCENTER, "#clan_warmup_rules", NULL, NULL, NULL, NULL );

			ReturnToWarmup();

			CVAR_SET_FLOAT( "mp_clan_match_warmup", 0 );
		}
		else if ( m_bAwaitingReadyRestart && m_bHeardAlliesReady && m_bHeardAxisReady )
		{
			StartClanRestart();

			m_bAwaitingReadyRestart = FALSE;
			m_bHeardAlliesReady = FALSE;
			m_bHeardAxisReady = FALSE;
		}
		else if ( m_bRoundRestarting && gpGlobals->time > m_flRestartRoundTime )
		{
			StartClanMatch();
		}
	}
	else if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value > 0.0 )
	{
		SetClanMatch( TRUE );

		UTIL_ClientPrintAll( HUD_PRINTCENTER, "#clan_warmup_rules", NULL, NULL, NULL, NULL );
	}

	if ( m_RoundState == ROUNDSTATE_PREROUND )
	{
		if ( gpGlobals->time >= m_flGoodToGoTime && !m_rReadyToUnFreeze )
		{
			GoodToGo();
		}
		else if ( gpGlobals->time >= m_rRoundStartCount )
		{
			if ( m_rReadyToUnFreeze == 2 )
				RoundFreeze();
			else if ( m_rReadyToUnFreeze == 1 && m_DoDCameraState <= 0 )
				RoundUnfreeze();
		}
	}

	if ( !g_fGameOver )
	{
		BOOL bContinue = TRUE;

		if ( m_flAlliesWaveTime != 0 && m_RoundState == ROUNDSTATE_RUNNING && (int)m_flAlliesRespawn <= (int)gpGlobals->time )
		{
			RespawnAllies();

			m_flAlliesWaveTime = 0;
			bContinue = !g_fGameOver;
		}

		if ( bContinue && m_flAxisWaveTime != 0 && m_RoundState == ROUNDSTATE_RUNNING && (int)m_flAxisRespawn <= (int)gpGlobals->time )
		{
			RespawnAxis();

			m_flAxisWaveTime = 0;
		}
	}

	if ( gpGlobals->time >= m_flRestartTime && (unsigned int)( m_RoundState - 3 ) <= 2 && !g_fGameOver && m_DoDCameraState <= 0 )
	{
		m_SaveRoundState = m_RoundState;

		TeamWon( m_RoundState );

		m_flRestartTime = gpGlobals->time + 3.0;
	}
}
// .text:000e9f70 void CDoDTeamPlay::DetectGameRules() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::DetectGameRules( void )
{
	CBaseEntity *det = UTIL_FindEntityByClassname( NULL, "info_doddetect" );
	BOOL found = FALSE;
	while ( det )
	{
		string_t master = ( (CDoDDetect *) det )->m_sMaster;
		if ( master == 0 || UTIL_IsMasterTriggered( master, det ) )
		{
			gr_gameRules = 1;
			CopyGamePlayLogic( ( (CDoDDetect *) det )->m_GamePlayRules );
			found = TRUE;
		}
		det = UTIL_FindEntityByClassname( det, "info_doddetect" );
	}
	if ( !found )
	{
		gr_gameRules = 0;
		m_GamePlayRules.m_iPointsAlliesElim = 5;
		m_GamePlayRules.m_iPointsAxisElim = 5;
		m_GamePlayRules.m_bAlliesInfiniteLives = TRUE;
		m_GamePlayRules.m_bAxisInfiniteLives = TRUE;
		m_GamePlayRules.m_bAlliesArePara = FALSE;
		m_GamePlayRules.m_bAlliesAreBrit = FALSE;
		m_GamePlayRules.m_bAxisArePara = FALSE;
		m_GamePlayRules.m_bUseTimer = FALSE;
		m_GamePlayRules.m_iTimerTeam = 0;
		m_GamePlayRules.m_sAlliesElimTarget = 0;
		m_GamePlayRules.m_sAxisElimTarget = 0;
		m_GamePlayRules.m_sTimerExpireTarget = 0;
		m_GamePlayRules.m_iPointsTimerExpires = 0;
		m_GamePlayRules.m_fAlliesRespawnFactor = 1.0f;
		m_GamePlayRules.m_fAxisRespawnFactor = 1.0f;
	}
	SendClientGamerules( NULL );
}
// .text:000ea0e0 void CDoDTeamPlay::GoodToGo() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::GoodToGo( void )
{
	DetectGameRules();
	int ready = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *p = (CBasePlayer *) UTIL_PlayerByIndex( i );
		if ( !p || p->pev->team < 1 || p->pev->team > 2 )
			continue;
		if ( p->m_iJoiningState == JOINED )
			ready++;
	}
	if ( ready != 0 )
	{
		RestartRound();
		return;
	}
	UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_not_enough", 0, 0, 0, 0 );
	SetRoundState( ROUNDSTATE_PREROUND );
	m_flGoodToGoTime = gpGlobals->time + 20.0f;
}
void CDoDTeamPlay::ChangeLevel( void )
{
	static char szPreviousMapCycleFile[256];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[60];
	char szCommands[1500];
	char szRules[1500];
	int minplayers = 0;
	int maxplayers = 0;
	int curplayers = 0;
	BOOL do_cycle = TRUE;

	szNextMap[0] = 0;

	strcpy( szFirstMapInList, "dod_caen" );

	// find the map to change to
	char *mapcfile = (char *)CVAR_GET_STRING( "mapcyclefile" );

	szCommands[0] = 0;
	szRules[0] = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		if ( UTIL_PlayerByIndex( i ) )
			curplayers++;
	}

	// has the map cycle filename changed?
	if ( stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		strcpy( szPreviousMapCycleFile, mapcfile );

		DestroyMapCycle( &mapcycle );

		mapcycle.items = NULL;
		mapcycle.next_item = NULL;

		if ( !ReloadMapCycleFile( mapcfile, &mapcycle ) || !mapcycle.items )
		{
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
			do_cycle = FALSE;
		}
	}
	else if ( !mapcycle.items )
	{
		do_cycle = FALSE;
	}

	if ( do_cycle )
	{
		BOOL keeplooking = FALSE;
		BOOL found = FALSE;
		mapcycle_item_s *item;

		// assume the current map
		strcpy( szNextMap, STRING( gpGlobals->mapname ) );
		strcpy( szFirstMapInList, STRING( gpGlobals->mapname ) );

		// traverse the list
		for ( item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next )
		{
			keeplooking = FALSE;

			if ( item->minplayers != 0 )
			{
				if ( curplayers >= item->minplayers )
				{
					found = TRUE;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( item->maxplayers != 0 )
			{
				if ( curplayers <= item->maxplayers )
				{
					found = TRUE;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( keeplooking )
				continue;

			found = TRUE;
			break;
		}

		if ( !found )
			item = mapcycle.next_item;

		// increment the next item pointer
		mapcycle.next_item = item->next;

		strcpy( szNextMap, item->mapname );

		ExtractCommandString( item->rulebuffer, szCommands );
		strcpy( szRules, item->rulebuffer );
	}

	if ( !IS_MAP_VALID( szNextMap ) )
		strcpy( szNextMap, szFirstMapInList );

	g_fGameOver = TRUE;

	// the clan match is back in the warmup on the next map
	if ( IsClanMatch() && m_bClanMatch )
	{
		m_bClanMatchActive = FALSE;
		setMapTime();
		SendClientGamerules( NULL );
	}

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );

	if ( minplayers || maxplayers )
		ALERT( at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers );

	if ( strlen( szRules ) > 0 )
		ALERT( at_console, "RULES:  %s\n", szRules );

	CHANGE_LEVEL( szNextMap, NULL );

	if ( strlen( szCommands ) > 0 )
		SERVER_COMMAND( szCommands );
}
// .text:000e96c0 void CDoDTeamPlay::InitHUD(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::InitHUD( CBasePlayer *pPlayer )
{
	const char *name = "unconnected";

	if ( pPlayer->pev->netname )
	{
		name = STRING( pPlayer->pev->netname );

		if ( !*name )
			name = "unconnected";
	}

	UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_joined_game", (char *)name, NULL, NULL, NULL );

	SendMOTDToClient( pPlayer->edict() );

	// send the scores of the others to the new player
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( plr && plr != pPlayer )
		{
			if ( plr->m_iObjScore <= 255 && plr->pev->frags <= 255 && plr->m_iDeaths <= 255 )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pPlayer->edict() );
					WRITE_BYTE( i );
					WRITE_BYTE( plr->m_iObjScore );
					WRITE_BYTE( plr->pev->frags );
					WRITE_BYTE( plr->m_iDeaths );
			}
			else
			{
				ALERT( at_aiconsole, "Init HUD data requiring short values - sending with WRITE_SHORT()\n" );

				MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfoLong, NULL, pPlayer->edict() );
					WRITE_BYTE( i );
					WRITE_SHORT( plr->m_iObjScore );
					WRITE_SHORT( plr->pev->frags );
					WRITE_SHORT( plr->m_iDeaths );
			}

			WRITE_BYTE( plr->pev->playerclass );
			WRITE_BYTE( plr->pev->team );
			WRITE_BYTE( plr->PlayerStatus() );
			MESSAGE_END();
		}
	}

	// tell everybody about the new player, it spectates until it has chosen a team
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX( pPlayer->edict() ) );
		WRITE_BYTE( pPlayer->m_iObjScore );
		WRITE_BYTE( pPlayer->pev->frags );
		WRITE_BYTE( pPlayer->m_iDeaths );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 3 );
		WRITE_BYTE( pPlayer->PlayerStatus() );
	MESSAGE_END();

	if ( g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ONE, SVC_INTERMISSION, NULL, pPlayer->edict() );
		MESSAGE_END();
	}

	UTIL_LogPrintf( "\"%s<%i><%s><>\" entered the game\n", STRING( pPlayer->pev->netname ),
					GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ) );

	LevelChangeResets( pPlayer );

	// the teams of the players that already joined one
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if ( plr && (unsigned int)( plr->pev->team - 1 ) <= 2 )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgPTeam, NULL, pPlayer->edict() );
				WRITE_BYTE( ENTINDEX( plr->edict() ) );
				WRITE_BYTE( plr->pev->team );
			MESSAGE_END();
		}
	}

	SendClientGamerules( pPlayer );

	ShowVGUI( pPlayer, 2, 0 );

	pPlayer->SetThink( &CBasePlayer::ServerSpawnCheck );
	pPlayer->pev->nextthink = 2.0;

	pPlayer->SetMapMarker( 0 );
}
void CDoDTeamPlay::ClientDisconnected( edict_t *pClient )
{
	if ( !pClient )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE( pClient );

	if ( pPlayer )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
			WRITE_BYTE( ENTINDEX( pClient ) );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

		const char *szTeam = pPlayer->pev->team == TEAM_ALLIES ? "Allies" : "Axis";

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" disconnected\n", STRING( pPlayer->pev->netname ),
						GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ), szTeam );

		pPlayer->DropPlayerItem( "weapon_bazooka", TRUE );
		pPlayer->DropPlayerItem( "weapon_pschreck", TRUE );
		pPlayer->DropPlayerItem( "weapon_piat", TRUE );
		pPlayer->RemoveAllItems( TRUE );

		// the players that watched him stop watching
		CBasePlayer *pObserver = NULL;

		while ( ( pObserver = (CBasePlayer *)UTIL_FindEntityByClassname( pObserver, "player" ) ) != NULL )
		{
			if ( FNullEnt( pObserver->edict() ) )
				break;

			if ( pObserver->pev && pObserver != pPlayer && pObserver->m_hObserverTarget == pPlayer )
			{
				int iMode = pObserver->pev->iuser1;

				pObserver->pev->iuser1 = 0;
				pObserver->m_hObserverTarget = NULL;
				pObserver->Observer_SetMode( iMode );
			}
		}

		if ( pPlayer->HasObject )
			pPlayer->DropObject();

		pPlayer->ClientDisconnect();
	}
}
// .text:000e80e0 void CDoDTeamPlay::RoundUnfreeze() (dod/dlls/dod_gamerules.cpp:6187)
void CDoDTeamPlay::RoundUnfreeze( void )
{
	SetRoundState( ROUNDSTATE_RUNNING );
	m_rReadyToUnFreeze = 0;
	m_bIsRoundFrozen = false;
	CDodRoundTimer *pTimer =
		(CDodRoundTimer *) UTIL_FindEntityByClassname( NULL, "dod_round_timer" );
	while ( pTimer )
	{
		if ( ( pTimer->pev->spawnflags & SF_ROUNDTIMER_MANUAL ) == 0 )
			pTimer->StartTimer();
		pTimer = (CDodRoundTimer *) UTIL_FindEntityByClassname( pTimer, "dod_round_timer" );
	}
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *p = (CBasePlayer *) UTIL_PlayerByIndex( i );
		if ( !p || p->Classify() != 2 || p->pev->team == TEAM_SPECTATOR )
			continue;
		p->CheckPlayerSpeed();
		int r = RANDOM_LONG( 0, 3 );
		int team = p->pev->team;
		const char *msg;
		int bparam1;
		if ( team == TEAM_ALLIES )
		{
			if ( m_GamePlayRules.m_bAlliesAreBrit )
			{
				msg = ( r == 1 || r == 3 ) ? "#game_roundstart_brit2" : "#game_roundstart_brit1";
				bparam1 = 1;
			}
			else
			{
				msg = ( r == 1 || r == 3 ) ? "#game_roundstart_allie2" : "#game_roundstart_allie1";
				bparam1 = 0;
			}
		}
		else
		{
			msg = ( r == 1 || r == 3 ) ? "#game_roundstart_axis2" : "#game_roundstart_axis1";
			bparam1 = 0;
		}
		entvars_t *client = p->pev;
		ClientPrint( client, HUD_PRINTTALK, msg, 0, 0, 0, 0 );
		PLAYBACK_EVENT_FULL( 0, client->pContainingEntity, m_roundRestartSound, 0.0, (float *) &g_vecZero,
							 (float *) &g_vecZero, 0.0, 0.0, team, r, bparam1, 0 );
	}
}
void CDoDTeamPlay::RoundFreeze( void )
{
	m_rReadyToUnFreeze = 1;
	m_bIsRoundFrozen = TRUE;
	m_rRoundStartCount = gpGlobals->time + 5.0;

	CDodRoundTimer *pTimer = NULL;

	while ( ( pTimer = (CDodRoundTimer *)UTIL_FindEntityByClassname( pTimer, "dod_round_timer" ) ) != NULL )
		pTimer->ResetTimer();

	// the teams that have not seen their spawn cutscene yet
	BOOL bAlliesCut = FALSE;
	BOOL bAxisCut = FALSE;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( !pPlayer || pPlayer->Classify() != CLASS_PLAYER || pPlayer->pev->team == TEAM_SPECTATOR )
			continue;

		if ( pPlayer->pev->team == TEAM_ALLIES )
		{
			if ( !pPlayer->i_seenAllieSpawnScene )
				bAlliesCut = TRUE;
		}
		else if ( pPlayer->pev->team == TEAM_AXIS && !pPlayer->i_seenAxisSpawnScene )
		{
			bAxisCut = TRUE;
		}

		(*g_engfuncs.pfnSetClientMaxspeed)( pPlayer->edict(), 1.0f );
	}

	CCineMonster *pCine = NULL;

	while ( ( pCine = (CCineMonster *)UTIL_FindEntityByClassname( pCine, "scripted_sequence" ) ) != NULL )
		pCine->DoDReset();

	CBaseEntity *pTrain = NULL;

	while ( ( pTrain = UTIL_FindEntityByClassname( pTrain, "func_train" ) ) != NULL )
		pTrain->RoundRespawn();

	if ( bAlliesCut )
		FireDODCamera( "allie_spawn_cut", NULL, NULL, USE_TOGGLE, 0 );

	if ( bAxisCut )
		FireDODCamera( "axis_spawn_cut", NULL, NULL, USE_TOGGLE, 0 );
}
void CDoDTeamPlay::SendClientGamerules( CBasePlayer *pPlayer )
{
	int iAllies;
	int iAlliesInfinite;

	if ( m_GamePlayRules.m_bAlliesArePara )
	{
		iAllies = 1;
		iAlliesInfinite = 3;
	}
	else
	{
		iAllies = m_GamePlayRules.m_bAlliesAreBrit ? 4 : 0;
		iAlliesInfinite = m_GamePlayRules.m_bAlliesAreBrit ? 6 : 2;
	}

	int iAxis = m_GamePlayRules.m_bAxisArePara;

	if ( m_GamePlayRules.m_bAlliesInfiniteLives )
		iAllies = iAlliesInfinite;

	if ( m_GamePlayRules.m_bAxisInfiniteLives )
		iAxis |= 2;

	// a clan match that is still in the warmup
	if ( IsClanMatch() && !IsClanMatchActive() )
		iAxis |= 4;

	if ( pPlayer )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgGameRules, NULL, pPlayer->edict() );
			WRITE_BYTE( iAllies );
			WRITE_BYTE( iAxis );
		MESSAGE_END();

		return;
	}

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEntity = UTIL_PlayerByIndex( i );

		if ( pEntity && (unsigned int)( pEntity->pev->team - 1 ) <= 2 )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgGameRules, NULL, pEntity->edict() );
				WRITE_BYTE( iAllies );
				WRITE_BYTE( iAxis );
			MESSAGE_END();
		}
	}
}
// .text:000e4c60 void CDoDTeamPlay::SendTeamScores(int) (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::SendTeamScores( int m_iTeam )
{
	if ( m_iTeam == 0 )
	{
		m_iTeamScores[1] = 0;
		m_iTeamScores[2] = 0;
		return;
	}
	if ( m_iTeam == TEAM_ALLIES || m_iTeam == 3 )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
		WRITE_BYTE( 1 );
		WRITE_SHORT( m_iTeamScores[1] );
		MESSAGE_END();
	}
	if ( m_iTeam == TEAM_AXIS || m_iTeam == 3 )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
		WRITE_BYTE( 2 );
		WRITE_SHORT( m_iTeamScores[2] );
		MESSAGE_END();
	}
}
void CDoDTeamPlay::SendObjectiveStatus( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgRoundState, NULL, pPlayer->edict() );
		WRITE_BYTE( RoundState() );
	MESSAGE_END();

	// the control points of the master that is in use
	CControlPointMaster *pMaster = (CControlPointMaster *)UTIL_FindEntityByClassname( NULL, "dod_control_point_master" );

	while ( pMaster )
	{
		CBaseEntity *pNext = UTIL_FindEntityByClassname( pMaster, "dod_control_point_master" );

		if ( pMaster->IsMasteredOn() )
		{
			pMaster->SendObjectiveStatus( pPlayer );
			break;
		}

		pMaster = (CControlPointMaster *)pNext;
	}

	CBaseEntity *pEntity = NULL;

	while ( ( pEntity = UTIL_FindEntityByClassname( pEntity, "dod_capture_area" ) ) != NULL )
		pEntity->area_SendStatus( pPlayer );

	pEntity = NULL;

	while ( ( pEntity = UTIL_FindEntityByClassname( pEntity, "dod_object_goal" ) ) != NULL )
		pEntity->area_SendStatus( pPlayer );

	pEntity = NULL;

	while ( ( pEntity = UTIL_FindEntityByClassname( pEntity, "func_breakable" ) ) != NULL )
		pEntity->area_SendStatus( pPlayer );

	CDodRoundTimer *pTimer = NULL;

	while ( ( pTimer = (CDodRoundTimer *)UTIL_FindEntityByClassname( pTimer, "dod_round_timer" ) ) != NULL )
		pTimer->SendStatus( pPlayer );

	CParticleShooter *pShooter = NULL;

	while ( ( pShooter = (CParticleShooter *)UTIL_FindEntityByClassname( pShooter, "particle_shooter" ) ) != NULL )
		pShooter->SendUpdate( pPlayer );
}
void CDoDTeamPlay::SetNextMap( void )
{
	static char szPreviousMapCycleFile[256];
	static mapcycle_t mapcycle;

	char szFirstMapInList[44];

	strcpy( szFirstMapInList, "dod_caen" );

	char *mapcfile = (char *)CVAR_GET_STRING( "mapcyclefile" );

	BOOL bAdvance;

	if ( !stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		// the cycle is loaded already
		bAdvance = mapcycle.items != NULL;
	}
	else
	{
		strcpy( szPreviousMapCycleFile, mapcfile );

		DestroyMapCycle( &mapcycle );

		mapcycle.items = NULL;
		mapcycle.next_item = NULL;

		bAdvance = ReloadMapCycleFile( mapcfile, &mapcycle ) && mapcycle.items;

		if ( !bAdvance )
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
	}

	if ( bAdvance )
	{
		strcpy( m_szNextMap, STRING( gpGlobals->mapname ) );
		strcpy( szFirstMapInList, STRING( gpGlobals->mapname ) );

		const char *pszNext = mapcycle.next_item->mapname;

		mapcycle.next_item = mapcycle.next_item->next;

		strcpy( m_szNextMap, pszNext );
	}

	if ( !IS_MAP_VALID( m_szNextMap ) )
		strcpy( m_szNextMap, szFirstMapInList );
}
// .text:000eae00 void CDoDTeamPlay::StartClanMatch() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::StartClanMatch( void )
{
	m_bClanMatchActive = TRUE;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *) UTIL_PlayerByIndex( i );
		if ( !plr )
			continue;
		plr->m_iObjScore = 0;
		plr->pev->frags = 0;
		plr->m_iDeaths = 0;
		MESSAGE_BEGIN( MSG_ALL, gmsgScoreShort );
		WRITE_BYTE( ENTINDEX( plr->edict() ) );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
		WRITE_BYTE( plr->PlayerStatus() );
		MESSAGE_END();
	}
	SendClientGamerules( NULL );
	SetRoundState( ROUNDSTATE_PREROUND );
	m_bRoundRestarting = FALSE;
}
// .text:000eaf50 void CDoDTeamPlay::CheckClanRestart() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::CheckClanRestart( void )
{
	if ( m_bRoundRestarting && m_flRestartRoundTime < gpGlobals->time )
		StartClanMatch();
}
// .text:000eaca0 void CDoDTeamPlay::VoiceCommSelect(CBasePlayer*, int) (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::VoiceCommSelect( CBasePlayer *pPlayer, int v )
{
	if ( v != 10 && pPlayer->lastMenuSelected > 0 )
		pPlayer->VoiceCom( v + pPlayer->lastMenuSelected * 9 - 9 );
}
int CDoDTeamPlay::GetWeaponId( char *weaponname )
{
	if ( !strcmp( weaponname, "WEAPON_AMERKNIFE" ) )
		return 1;

	if ( !strcmp( weaponname, "WEAPON_GERKNIFE" ) )
		return 2;

	if ( !strcmp( weaponname, "WEAPON_COLT" ) )
		return 3;

	if ( !strcmp( weaponname, "WEAPON_LUGER" ) )
		return 4;

	if ( !strcmp( weaponname, "WEAPON_GARAND" ) )
		return 5;

	if ( !strcmp( weaponname, "WEAPON_SCOPEDKAR" ) )
		return 6;

	if ( !strcmp( weaponname, "WEAPON_THOMPSON" ) )
		return 7;

	if ( !strcmp( weaponname, "WEAPON_MP44" ) )
		return 8;

	if ( !strcmp( weaponname, "WEAPON_SPRING" ) )
		return 9;

	if ( !strcmp( weaponname, "WEAPON_KAR" ) )
		return 10;

	if ( !strcmp( weaponname, "WEAPON_BAR" ) )
		return 11;

	if ( !strcmp( weaponname, "WEAPON_MP40" ) )
		return 12;

	if ( !strcmp( weaponname, "WEAPON_MG42" ) )
		return 17;

	if ( !strcmp( weaponname, "WEAPON_30CAL" ) )
		return 18;

	if ( !strcmp( weaponname, "WEAPON_SPADE" ) )
		return 19;

	if ( !strcmp( weaponname, "WEAPON_M1CARBINE" ) )
		return 20;

	if ( !strcmp( weaponname, "WEAPON_MG34" ) )
		return 21;

	if ( !strcmp( weaponname, "WEAPON_GREASEGUN" ) )
		return 22;

	if ( !strcmp( weaponname, "WEAPON_FG42" ) )
		return 23;

	if ( !strcmp( weaponname, "WEAPON_K43" ) )
		return 24;

	if ( !strcmp( weaponname, "WEAPON_ENFIELD" ) )
		return 25;

	if ( !strcmp( weaponname, "WEAPON_STEN" ) )
		return 26;

	if ( !strcmp( weaponname, "WEAPON_BREN" ) )
		return 27;

	if ( !strcmp( weaponname, "WEAPON_WEBLEY" ) )
		return 28;

	if ( !strcmp( weaponname, "WEAPON_FOLDINGCARBINE" ) )
		return 20;

	if ( !strcmp( weaponname, "WEAPON_GERPARAKNIFE" ) )
		return 2;

	if ( !strcmp( weaponname, "WEAPON_SCOPED_FG42" ) )
		return 65;

	return !strcmp( weaponname, "WEAPON_SCOPEDENFIELD" ) ? 25 : -1;
}
// .text:000e4030 BOOL CDoDTeamPlay::ClientConnected(edict_t*,char*,char*,char*) (dod/dlls/dod_gamerules.cpp:0)
BOOL CDoDTeamPlay::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *szRejectReason )
{
	g_VoiceGameMgr.ClientConnected( pEntity );
	char *buf = GET_INFOKEYBUFFER( pEntity );
	char *nm = INFOKEY_VALUE( buf, "name" );
	if ( nm[0] == '#' )
	{
		char sName[256];
		strncpy( sName, nm, 255 );
		sName[0] = '*';
		sName[255] = 0;
		SET_CLIENT_KEYVALUE( ENTINDEX( pEntity ), buf, "name", sName );
	}
	return TRUE;
}
// .text:000ec630 BOOL CDoDTeamPlay::ClientCommand(CBasePlayer*, char*) (dod/dlls/dod_gamerules.cpp:1673)
// wrapper link 0xddf20 size 0x65 Ghidra 0xedf20: VoiceGameMgr gate before main 0xdc630
extern char *s_VoiceCommands[29][4];

BOOL CDoDTeamPlay::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	if ( g_VoiceGameMgr.ClientCommand( pPlayer, pcmd ) )
		return TRUE;

	// the spectate command is sent by the HLTV proxy
	if ( !strcmp( pcmd, "spectate" ) && ( pPlayer->pev->flags & FL_PROXY ) )
	{
		pPlayer->pev->flags |= FL_PROXY;
		pPlayer->m_iNextClass = -1;
		pPlayer->pev->playerclass = 0;
		pPlayer->m_bIsRandomClass = FALSE;
		pPlayer->pev->team = TEAM_SPECTATOR;

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" joined team \"%s\"\n", STRING( pPlayer->pev->netname ),
						GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ), g_szTeamNames[0],
						g_szTeamNames[pPlayer->pev->team] );

		pPlayer->m_iJoiningState = JOINED;

		ChangePlayerTeam( pPlayer, pPlayer->pev->team, 0, 0 );

		Vector color = g_vecZero;
		pPlayer->UTIL_EdictScreenFade( pPlayer->edict(), color, 1.0f, 15.0f, 0, 5 );

		pPlayer->pev->deadflag = DEAD_DEAD;
		pPlayer->pev->effects |= EF_NODRAW;
		pPlayer->m_bIsObserver = 1;
		pPlayer->StartObserver( pPlayer->pev->origin, pPlayer->pev->angles );

		return FALSE;
	}

	if ( !strcmp( pcmd, "specmode" ) )
	{
		if ( pPlayer->pev->iuser1 > 0 )
			pPlayer->Observer_SetMode( atoi( CMD_ARGV( 1 ) ) );

		return FALSE;
	}

	if ( !strcmp( pcmd, "follownext" ) )
	{
		if ( pPlayer->pev->iuser1 > 0 )
			pPlayer->Observer_FindNextPlayer( atoi( CMD_ARGV( 1 ) ) != 0 );

		return FALSE;
	}

	if ( !strcmp( pcmd, "spectateperson" ) )
	{
		if ( CMD_ARGC() > 1 )
		{
			pPlayer->Observer_SpectatePlayer( atoi( CMD_ARGV( 1 ) ) );
			return FALSE;
		}

		return TRUE;
	}

	if ( !strcmp( pcmd, "fullupdate" ) )
	{
		pPlayer->ForceClientDllUpdate();
		return FALSE;
	}

	if ( !strcmp( pcmd, "timeleft" ) )
	{
		if ( cvar_timelimit.value == 0.0 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_time_left2" );
		}
		else
		{
			int iTimeLeft = get_maptime_left();
			int iMinutes = iTimeLeft / 60;
			char szSeconds[8];

			sprintf( szSeconds, "%02d", iTimeLeft - 60 * iMinutes );

			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_time_left1", UTIL_dtos1( iMinutes ), szSeconds );
		}

		return FALSE;
	}

	if ( !strcmp( pcmd, "dod_version" ) )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "Version 1.2f" );
		return FALSE;
	}

	if ( !strcmp( pcmd, "chatmsg" ) )
	{
		if ( pPlayer->m_chatMsg )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_chat_off" );
			pPlayer->m_chatMsg = 0;
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_chat_on" );
			pPlayer->m_chatMsg = 1;
		}

		return FALSE;
	}

	if ( !strcmp( pcmd, "deathmsg" ) )
	{
		if ( pPlayer->m_deathMsg )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_deathmsg_off" );
			pPlayer->m_deathMsg = 0;
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_deathmsg_on" );
			pPlayer->m_deathMsg = 1;
		}

		return FALSE;
	}

	if ( !strcmp( pcmd, "clientcutscenes" ) )
	{
		// a client that does not want the cutscenes has seen all of them
		if ( CMD_ARGC() > 1 && !atoi( CMD_ARGV( 1 ) ) )
		{
			pPlayer->i_seenAllieSpawnScene = 1;
			pPlayer->i_seenAxisSpawnScene = 1;
			pPlayer->i_seenAllieWinScene = 1;
			pPlayer->i_seenAxisWinScene = 1;
			pPlayer->i_seenAllieLooseScene = 1;
			pPlayer->i_seenAxisLooseScene = 1;
			pPlayer->i_seenAllieDrawScene = 1;
			pPlayer->i_seenAxisDrawScene = 1;
		}

		return FALSE;
	}

	if ( !strcmp( pcmd, "vote" ) )
	{
		if ( (unsigned int)( pPlayer->lastMenuSelected - 5 ) > 4 )
		{
			if ( m_vKickSessionState == 1 )
			{
				pPlayer->VoteMenu( m_vKick, 5 );
				return FALSE;
			}

			if ( m_vKickSessionState == 0 )
			{
				pPlayer->VoteMenu( -1, 6 );
				return FALSE;
			}

			return FALSE;
		}

		return TRUE;
	}

	if ( !strcmp( pcmd, "fov" ) )
	{
		pPlayer->pev->fov = (float)pPlayer->m_iFOV;

		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_dont_cheat" );

		return FALSE;
	}

	if ( !strcmp( pcmd, "say" ) )
	{
		int iChatMsg = pPlayer->m_chatMsg;
		char readyString[64];

		snprintf( readyString, sizeof( readyString ), "\"%s\"", CVAR_GET_STRING( "mp_clan_ready_signal" ) );

		if ( m_bAwaitingReadyRestart && CMD_ARGC() > 1 )
		{
			const char *pszText = CMD_ARGS();
			int team = pPlayer->pev->team;

			if ( team == TEAM_ALLIES )
			{
				if ( !m_bHeardAlliesReady && !strcmp( pszText, readyString ) )
				{
					m_bHeardAlliesReady = TRUE;
					UTIL_ShowMessageAll( "#Clan_allies_ready", FALSE );
				}
			}
			else if ( team == TEAM_AXIS && !m_bHeardAxisReady && !strcmp( pszText, readyString ) )
			{
				m_bHeardAxisReady = TRUE;
				UTIL_ShowMessageAll( "#Clan_axis_ready", FALSE );
			}
		}

		if ( iChatMsg )
		{
			Host_Say( pPlayer->edict(), 0 );
			return FALSE;
		}

		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_chat_off" );
		return FALSE;
	}

	if ( !strcmp( pcmd, "say_team" ) )
	{
		if ( pPlayer->m_chatMsg )
		{
			Host_Say( pPlayer->edict(), 1 );
			return FALSE;
		}

		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_chat_off" );
		return FALSE;
	}

	if ( FStrEq( pcmd, "menuselect" ) )
	{
		if ( CMD_ARGC() > 1 )
		{
			int iSelection = atoi( CMD_ARGV( 1 ) );

			switch ( pPlayer->lastMenuSelected )
			{
			case 5:
				if ( iSelection == 1 )
					VoteKick( pPlayer, m_vKick, true );
				break;

			case 6:
				if ( iSelection == 9 )
					pPlayer->VoteMenu( -1, 7 );
				else if ( iSelection != 10 )
					VoteKick( pPlayer, iSelection, true );
				break;

			case 7:
				if ( iSelection == 9 )
					pPlayer->VoteMenu( -1, 8 );
				else if ( iSelection != 10 )
					VoteKick( pPlayer, iSelection + 8, true );
				break;

			case 8:
				if ( iSelection == 9 )
					pPlayer->VoteMenu( -1, 9 );
				else if ( iSelection != 10 )
					VoteKick( pPlayer, iSelection + 16, true );
				break;

			case 9:
				if ( iSelection != 10 && iSelection != 9 )
					VoteKick( pPlayer, iSelection + 24, true );
				break;

			default:
				if ( (unsigned int)( pPlayer->lastMenuSelected - 1 ) <= 3 )
					VoiceCommSelect( pPlayer, iSelection );
				break;
			}

			pPlayer->lastMenuSelected = -1;
		}

		return TRUE;
	}

	if ( FStrEq( pcmd, "jointeam" ) )
	{
		if ( CMD_ARGC() > 1 )
			TeamSelect( pPlayer, atoi( CMD_ARGV( 1 ) ) );

		return TRUE;
	}

	if ( FStrEq( pcmd, "changeclass" ) )
	{
		ShowClassSelectMenu( pPlayer );
		return TRUE;
	}

	if ( FStrEq( pcmd, "changeteam" ) )
	{
		ShowVGUI( pPlayer, 2, 0 );
		return TRUE;
	}

	if ( !strncmp( pcmd, "cls_", 4 ) )
	{
		int iClass = 0;

		while ( strcmp( g_ClassInfo[iClass].selectcmd, pcmd ) )
		{
			if ( ++iClass == 28 )
			{
				iClass = 0;
				break;
			}
		}

		ClassSelect( pPlayer, iClass );
		return FALSE;
	}

	if ( FStrEq( pcmd, "nextmap" ) )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_nextmap", m_szNextMap );
		return TRUE;
	}

	if ( FStrEq( pcmd, "debug" ) || FStrEq( pcmd, "debug2" ) )
		return TRUE;

	// the commands below are for the living
	if ( pPlayer->pev->deadflag )
		return FALSE;

	if ( FStrEq( pcmd, "sprone" ) )
	{
		if ( pPlayer->IsSandbagDeployed() )
			return TRUE;

		if ( pPlayer->IsInMortarDeploy() )
			return TRUE;

		if ( pPlayer->pev->button & IN_ATTACK )
			return TRUE;

		if ( pPlayer->pev->movetype == MOVETYPE_NOCLIP )
			return TRUE;

		if ( !pPlayer->IsProne() && pPlayer->pev->waterlevel > 1 && pPlayer->pev->watertype != -19 )
			return TRUE;

		if ( gpGlobals->time > pPlayer->m_inextprone )
		{
			int iProneState = pPlayer->pev->iuser3;

			if ( iProneState == STANCE_PRONE )
			{
				pPlayer->UnProne();
			}
			else if ( iProneState == STANCE_STANDING )
			{
				if ( ( pPlayer->pev->button & IN_DUCK ) || pPlayer->IsDucking() || ( pPlayer->m_afPhysicsFlags & PFLAG_DUCKING ) )
					return TRUE;

				pPlayer->GoProne();
				pPlayer->CheckPlayerSpeed();

				if ( !( pPlayer->m_flDisplayHistory & HINT_PRONE ) )
				{
					pPlayer->m_flDisplayHistory |= HINT_PRONE;
					pPlayer->HintMessage( "#Hint_prone", 0 );
				}

				return FALSE;
			}
		}

		pPlayer->CheckPlayerSpeed();
		return FALSE;
	}

	if ( FStrEq( pcmd, "give" ) )
	{
		if ( CVAR_GET_FLOAT( "sv_cheats" ) == 0.0 )
			return FALSE;

		if ( pPlayer->pev->iuser1 )
			return FALSE;

		int iCount = atoi( CMD_ARGV( 2 ) );
		int iszItem = ALLOC_STRING( CMD_ARGV( 1 ) );

		if ( iCount <= 0 )
			iCount = 1;

		for ( int i = 0; i < iCount; i++ )
			pPlayer->GiveNamedItem( STRING( iszItem ) );

		return FALSE;
	}

	if ( FStrEq( pcmd, "use" ) )
	{
		pPlayer->SelectItem( CMD_ARGV( 1 ) );
		return FALSE;
	}

	if ( strstr( pcmd, "weapon_" ) == pcmd )
	{
		pPlayer->SelectItem( pcmd );
		return FALSE;
	}

	if ( FStrEq( pcmd, "lastinv" ) )
	{
		pPlayer->SelectLastItem();
		return FALSE;
	}

	if ( FStrEq( pcmd, "dropammo" ) || FStrEq( pcmd, "dropmgammo" ) )
	{
		pPlayer->DropGenericAmmo();
		return FALSE;
	}

	if ( FStrEq( pcmd, "maim" ) )
	{
		CMD_ARGS();

		if ( CMD_ARGC() > 1 )
		{
			atoi( CMD_ARGV( 1 ) );
			return FALSE;
		}

		return TRUE;
	}

	if ( FStrEq( pcmd, "drop" ) )
	{
		pPlayer->DropPlayerItem( (char *)CMD_ARGV( 1 ), FALSE );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_sticktogether" ) )
	{
		pPlayer->HandSignal( 0 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_fallback" ) )
	{
		pPlayer->HandSignal( 1 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_no" ) )
	{
		pPlayer->HandSignal( 2 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_yes" ) )
	{
		pPlayer->HandSignal( 3 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_sniper" ) )
	{
		pPlayer->HandSignal( 4 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_backup" ) )
	{
		pPlayer->HandSignal( 5 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_enemyright" ) )
	{
		pPlayer->HandSignal( 6 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_enemyleft" ) )
	{
		pPlayer->HandSignal( 7 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_grenade" ) )
	{
		pPlayer->HandSignal( 8 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_flankleft" ) )
	{
		pPlayer->HandSignal( 9 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_moveout" ) )
	{
		pPlayer->HandSignal( 10 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_flankright" ) )
	{
		pPlayer->HandSignal( 11 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_areaclear" ) )
	{
		pPlayer->HandSignal( 12 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_coveringfire" ) )
	{
		pPlayer->HandSignal( 13 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_holdposition" ) )
	{
		pPlayer->HandSignal( 15 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "signal_enemyspotted" ) )
	{
		pPlayer->HandSignal( 17 );
		return FALSE;
	}

	if ( !strncmp( pcmd, "dod_is_toned", 12 ) )
	{
		int iSignal = atoi( pcmd + 12 );

		if ( (unsigned int)( iSignal - 2 ) <= 3 )
			pPlayer->HandSignal( iSignal + 24 );

		return FALSE;
	}

	if ( FStrEq( pcmd, "voice_menu1" ) )
	{
		pPlayer->VoiceMenu( 1 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "voice_menu2" ) )
	{
		pPlayer->VoiceMenu( 2 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "voice_menu3" ) )
	{
		pPlayer->VoiceMenu( 3 );
		return FALSE;
	}

	if ( FStrEq( pcmd, "dropobject" ) )
	{
		if ( pPlayer->HasObject )
			pPlayer->DropObject();

		return TRUE;
	}

	if ( FStrEq( pcmd, "chattoplayer" ) )
	{
		CBaseEntity *pTarget = UTIL_PlayerByIndex( atoi( CMD_ARGV( 1 ) ) );

		if ( pTarget )
		{
			char text[201];
			char word[127];

			sprintf( text, "%c%s: ", 2, STRING( pTarget->pev->netname ) );

			for ( int i = 2; i != CMD_ARGC() + 1; i++ )
			{
				sprintf( word, "%s ", CMD_ARGV( i ) );
				strcat( text, word );
			}

			ClientPrint( pTarget->pev, HUD_PRINTTALK, text );
		}

		return TRUE;
	}

	if ( FStrEq( pcmd, "firemarker" ) )
	{
		pPlayer->FireMapMarker();
		return FALSE;
	}

	if ( FStrEq( pcmd, "nextmarker" ) )
	{
		pPlayer->NextMapMarker();
		return FALSE;
	}

	if ( FStrEq( pcmd, "prevmarker" ) )
	{
		pPlayer->PrevMapMarker();
		return FALSE;
	}

	for ( int i = 1; i != 28; i++ )
	{
		if ( FStrEq( pcmd, s_VoiceCommands[i][0] ) )
		{
			pPlayer->VoiceCom( i );
			return TRUE;
		}
	}

	// no percent signs in what is printed back
	char szCmd[127];

	strncpy( szCmd, pcmd, sizeof( szCmd ) - 1 );
	szCmd[sizeof( szCmd ) - 1] = 0;

	for ( char *p = szCmd; *p; p++ )
	{
		if ( *p == '%' )
			*p = ' ';
	}

	ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "Unknown command: %s\n", szCmd );

	return FALSE;
}

// .text:000e3610 char* CDoDTeamPlay::GetIndexedTeamName(int) (dod/dlls/dod_gamerules.cpp:3329)
const char *CDoDTeamPlay::GetIndexedTeamName( int teamIndex )
{
	if ( teamIndex == 1 )
		return "Allies";
	if ( teamIndex == 2 )
		return "Axis";
	if ( teamIndex == 3 )
		return "Spectators";
	return "";
}
// .text:000e3650 float CDoDTeamPlay::getWaveTime(int) (dod/dlls/dod_gamerules.cpp:4071)
float CDoDTeamPlay::getWaveTime( int iTeam )
{
	float dt;
	if ( iTeam == TEAM_ALLIES )
	{
		dt = m_flAlliesRespawn - gpGlobals->time;
		return dt > 0.0f ? dt : 0.0f;
	}
	if ( iTeam == TEAM_AXIS )
	{
		dt = m_flAxisRespawn - gpGlobals->time;
		return dt > 0.0f ? dt : 0.0f;
	}
	return 0.0f;
}
// .text:000e3760 void CDoDTeamPlay::SetTeamScores(int,int) (dod/dlls/dod_gamerules.cpp:4870)
void CDoDTeamPlay::SetTeamScores( int iTeam, int iScore )
{
	if ( m_RoundDoesCount != TRUE )
		return;
	m_iTeamScores[iTeam] = iScore;
}
// .text:000e38c0-000e3b30 setters (dod/dlls/dod_gamerules.cpp:4992-5075)
void CDoDTeamPlay::SetusVoice( short s ) { m_usVoice = s; }
void CDoDTeamPlay::SetbodyDamage( short s ) { m_bodyDamage = s; }
void CDoDTeamPlay::SetroundRestartSound( short s ) { m_roundRestartSound = s; }
void CDoDTeamPlay::SetgerVoice( short s ) { m_gerVoice = s; }
void CDoDTeamPlay::SetPain( short s ) { m_Pain = s; }
void CDoDTeamPlay::SetSmoke( short s ) { m_Smoke = s; }
void CDoDTeamPlay::SetProne( short s ) { m_Prone = s; }
void CDoDTeamPlay::SetBloodSprite( short s ) { m_BloodSprite = s; }
void CDoDTeamPlay::SetBloodStream( short s ) { m_BloodStream = s; }
void CDoDTeamPlay::SetBulletTracers( short s ) { m_BulletTracers = s; }
void CDoDTeamPlay::SetSparkShower( short s ) { m_SparkShower = s; }
void CDoDTeamPlay::SetScreenFades( short s ) { m_ScreenFades = s; }
void CDoDTeamPlay::SetBubbleTrails( short s ) { m_BubbleTrails = s; }
void CDoDTeamPlay::SetBubbles( short s ) { m_Bubbles = s; }
void CDoDTeamPlay::SetShards( short s ) { m_Shards = s; }
void CDoDTeamPlay::SetExplosion( short s ) { m_Explosion = s; }
void CDoDTeamPlay::SetPopHelmet( short s ) { m_PopHelmet = s; }
void CDoDTeamPlay::SetRoundReset( short s ) { m_RoundReset = s; }
void CDoDTeamPlay::SetRocketTrail( short s ) { m_RocketTrail = s; }
void CDoDTeamPlay::SetCamera( short s ) { m_Camera = s; }
// .text:000e33e0 void CDoDTeamPlay::Init() (dod/dlls/dod_gamerules.cpp:1228)
void CDoDTeamPlay::Init( void )
{
	m_GamePlayRules.m_iTimerTeam = 0;
	m_GamePlayRules.m_sAxisElimTarget = 0;
	m_GamePlayRules.m_sAlliesElimTarget = 0;
	m_GamePlayRules.m_iPointsAlliesElim = 5;
	m_GamePlayRules.m_sTimerExpireTarget = 0;
	m_GamePlayRules.m_iPointsTimerExpires = 0;
	m_GamePlayRules.m_bAlliesInfiniteLives = TRUE;
	m_GamePlayRules.m_bAxisInfiniteLives = TRUE;
	m_GamePlayRules.m_bAlliesArePara = FALSE;
	m_GamePlayRules.m_bAlliesAreBrit = FALSE;
	m_GamePlayRules.m_bAxisArePara = FALSE;
	m_GamePlayRules.m_bUseTimer = FALSE;
	m_GamePlayRules.m_iPointsAxisElim = 5;
	m_GamePlayRules.m_fAlliesRespawnFactor = 1.0f;
	m_GamePlayRules.m_fAxisRespawnFactor = 1.0f;
}
// .text:000e34a0 float CDoDTeamPlay::FlPlayerFallDamage(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:2477)
float CDoDTeamPlay::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	pPlayer->m_flFallVelocity -= 500.0f;
	return pPlayer->m_flFallVelocity * 100.0f / 400.0f;
}
// .text:000e3240 Vector CDoDTeamPlay::VecWeaponRespawnSpot(CBasePlayerItem*) (dod/dlls/dod_gamerules.cpp:865)
Vector CDoDTeamPlay::VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}
// .text:000e32d0 Vector CDoDTeamPlay::VecItemRespawnSpot(CItem*) (dod/dlls/dod_gamerules.cpp:968)
Vector CDoDTeamPlay::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}
// .text:000e3320 Vector CDoDTeamPlay::VecAmmoRespawnSpot(CBasePlayerAmmo*) (dod/dlls/dod_gamerules.cpp:1016)
Vector CDoDTeamPlay::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}
// .text:000e32a0 int CDoDTeamPlay::ItemShouldRespawn(CItem*) (dod/dlls/dod_gamerules.cpp:899)
int CDoDTeamPlay::ItemShouldRespawn( CItem *pItem )
{
	if ( ( pItem->pev->spawnflags & SF_NORESPAWN ) == 0 )
		return GR_ITEM_RESPAWN_NO;
	return GR_ITEM_RESPAWN_YES;
}
// .text:000e4810 void CDoDTeamPlay::ClientUserInfoChanged(CBasePlayer*,char*) (dod/dlls/dod_gamerules.cpp:2760)
void CDoDTeamPlay::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	char *mdls = INFOKEY_VALUE( infobuffer, "model" );
	if ( mdls && mdls[0] )
	{
		char *buf = GET_INFOKEYBUFFER( pPlayer->edict() );
		SET_CLIENT_KEYVALUE( ENTINDEX( pPlayer->edict() ), buf, "model", mdls );
	}
	char *lw = INFOKEY_VALUE( infobuffer, "cl_lw" );
	if ( lw[0] < '1' )
	{
		char *buf = GET_INFOKEYBUFFER( pPlayer->edict() );
		SET_CLIENT_KEYVALUE( ENTINDEX( pPlayer->edict() ), buf, "cl_lw", "1" );
	}
	pPlayer->SetPrefsFromUserinfo( infobuffer );
}
// .text:000e4d50 BOOL CDoDTeamPlay::PlayFootstepSounds(CBasePlayer*,float) (dod/dlls/dod_gamerules.cpp:3245)
BOOL CDoDTeamPlay::PlayFootstepSounds( CBasePlayer *pl, float fvol )
{
	extern cvar_t *g_footsteps;
	if ( g_footsteps && g_footsteps->value == 0.0f )
		return FALSE;
	if ( pl->pev->movetype == MOVETYPE_FLY )
		return TRUE;
	float vel = sqrtf( pl->pev->velocity.x * pl->pev->velocity.x + pl->pev->velocity.y * pl->pev->velocity.y );
	return ( vel > 30.0f );
}
// .text:000e3bf0 float CDoDTeamPlay::GetMaxWaveTime(int) (dod/dlls/dod_gamerules.cpp:4026)
float CDoDTeamPlay::GetMaxWaveTime( int nTeam )
{
	float fTime = 6.0f;
	int nNumPlayers = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *p = UTIL_PlayerByIndex( i );
		if ( p && p->pev->team == nTeam )
			nNumPlayers++;
	}
	if ( nNumPlayers > 2 ) fTime = 8.0f;
	if ( nNumPlayers > 5 ) fTime = 10.0f;
	if ( nNumPlayers > 7 ) fTime = 11.0f;
	if ( nNumPlayers > 9 ) fTime = 12.0f;
	if ( nNumPlayers > 11 ) fTime = 13.0f;
	if ( nNumPlayers > 13 ) fTime = 14.0f;
	if ( nTeam == 1 )
		return fTime * m_GamePlayRules.m_fAlliesRespawnFactor;
	if ( nTeam == 2 )
		return fTime * m_GamePlayRules.m_fAxisRespawnFactor;
	return fTime;
}

// .text:000e50b0 void CDoDTeamPlay::setWaveTime(int,float) (dod/dlls/dod_gamerules.cpp:4094)
void CDoDTeamPlay::setWaveTime( int iTeam, float fTime )
{
	if ( iTeam == TEAM_ALLIES )
	{
		if ( !m_GamePlayRules.m_bAlliesInfiniteLives )
			return;
		m_flAlliesRespawn = gpGlobals->time + fTime;
		m_flAlliesWaveTime = fTime;
	}
	else if ( iTeam == TEAM_AXIS )
	{
		if ( !m_GamePlayRules.m_bAxisInfiniteLives )
			return;
		m_flAxisRespawn = gpGlobals->time + fTime;
		m_flAxisWaveTime = fTime;
	}
	else
		return;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *p = UTIL_PlayerByIndex( i );
		if ( p && p->pev->team == iTeam )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgWaveTime, NULL, p->pev );
			WRITE_CHAR( (int) fTime );
			MESSAGE_END();
		}
	}
}
int CDoDTeamPlay::GetClassLimit( int cls )
{
	if ( cls <= 28 && pcvar_classlimitcvars[cls] )
		return (int)pcvar_classlimitcvars[cls]->value;

	return -1;
}
// .text:000e73c0 char* WeaponIDToChar(int) (dod/dlls/dod_gamerules.cpp:3383)
extern char weaponnames[][64];
char *WeaponIDToChar( int id )
{
	if ( (unsigned)id < 64 )
		return weaponnames[id];
	return NULL;
}
// .text:000e7b40 int CountTeamMembers(int) (dod/dlls/dod_gamerules.cpp:4128)
int CountTeamMembers( int team )
{
	int n = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *p = UTIL_PlayerByIndex( i );
		if ( p && p->pev->team == team )
			n++;
	}
	return n;
}
// .text:000e7bc0 int DeadTeammates(entvars_t*) (dod/dlls/dod_gamerules.cpp:4182)
int DeadTeammates( entvars_t *pevOrg )
{
	int n = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *p = UTIL_PlayerByIndex( i );
		if ( p && p->pev->team == pevOrg->team && p->pev->deadflag != DEAD_NO )
			n++;
	}
	return n;
}
// .text:000e7c60 int AliveTeammates(entvars_t*) (dod/dlls/dod_gamerules.cpp:4216)
int AliveTeammates( entvars_t *pevOrg )
{
	int n = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *p = UTIL_PlayerByIndex( i );
		if ( p && p->pev->team == pevOrg->team && p->pev->deadflag == DEAD_NO )
			n++;
	}
	return n;
}
// .text:000ea600 int CDoDTeamPlay::IsTeamPara(int) (dod/dlls/dod_gamerules.cpp:5574)
BOOL CDoDTeamPlay::IsTeamPara( int team )
{
	if ( team == TEAM_ALLIES )
		return m_GamePlayRules.m_bAlliesArePara;
	if ( team == TEAM_AXIS )
		return m_GamePlayRules.m_bAxisArePara;
	return FALSE;
}
// .text:000ea630 int CDoDTeamPlay::IsTeamInfinite(int) (dod/dlls/dod_gamerules.cpp:5580)
BOOL CDoDTeamPlay::IsTeamInfinite( int team )
{
	if ( team == TEAM_ALLIES )
		return m_GamePlayRules.m_bAlliesInfiniteLives;
	if ( team == TEAM_AXIS )
		return m_GamePlayRules.m_bAxisInfiniteLives;
	return FALSE;
}
// .text:000e4dd0 BOOL CDoDTeamPlay::FShouldSwitchWeapon(CBasePlayer*,CBasePlayerItem*) (dod/dlls/dod_gamerules.cpp:3354)
BOOL CDoDTeamPlay::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pPlayer->m_pActiveItem )
		return TRUE;
	if ( !pPlayer->m_pActiveItem->CanHolster() )
		return FALSE;
	return TRUE;
}
// .text:000e4e60 BOOL CDoDTeamPlay::GetNextBestWeapon(CBasePlayer*,CBasePlayerItem*) (dod/dlls/dod_gamerules.cpp:1075)
BOOL CDoDTeamPlay::GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{
	if ( pCurrentWeapon && ( pCurrentWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE ) )
	{
		CBasePlayerItem *pBest = NULL;
		int bestW = -1;
		for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
		{
			for ( CBasePlayerItem *p = pPlayer->m_rgpPlayerItems[i]; p; p = p->m_pNext )
			{
				if ( p == pCurrentWeapon )
					continue;
				if ( p->iWeight() > bestW && p->CanDeploy() )
				{
					bestW = p->iWeight();
					pBest = p;
				}
			}
		}
		if ( pBest )
		{
			pPlayer->SwitchWeapon( pBest, FALSE );
			return TRUE;
		}
	}
	return FALSE;
}
BOOL CDoDTeamPlay::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	if ( !pcvar_mp_spawnbazookas )
		pcvar_mp_spawnbazookas = CVAR_GET_POINTER( "mp_spawnbazookas" );

	if ( pcvar_mp_spawnbazookas->value > 0.0 )
		return TRUE;

	// without mp_spawnbazookas the launchers are not spawned
	const char *pszClassname = STRING( pEntity->pev->classname );

	if ( !strcmp( pszClassname, "weapon_piat" ) || !strcmp( pszClassname, "weapon_pschreck" ) )
		return FALSE;

	return strcmp( pszClassname, "weapon_bazooka" ) != 0;
}

// .text:000e6830 void ResetGamePlayRules(gameplay_rules_t*) (dod/dlls/dod_gamerules.cpp:169)
void ResetGamePlayRules( gameplay_rules_t *pRules )
{
	pRules->m_bAlliesInfiniteLives = TRUE;
	pRules->m_bAxisInfiniteLives = TRUE;
	pRules->m_bAlliesArePara = FALSE;
	pRules->m_bAlliesAreBrit = FALSE;
	pRules->m_bAxisArePara = FALSE;
	pRules->m_bUseTimer = FALSE;
	pRules->m_iTimerTeam = 0;
	pRules->m_sAlliesElimTarget = 0;
	pRules->m_sAxisElimTarget = 0;
	pRules->m_sTimerExpireTarget = 0;
	pRules->m_iPointsAlliesElim = 5;
	pRules->m_iPointsAxisElim = 5;
	pRules->m_iPointsTimerExpires = 0;
	pRules->m_fAlliesRespawnFactor = 1.0f;
	pRules->m_fAxisRespawnFactor = 1.0f;
}
// .text:000e68e0 int CountPlayers() (dod/dlls/dod_gamerules.cpp:421)
int CountPlayers( void )
{
	int n = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *p = UTIL_PlayerByIndex( i );
		if ( p )
			n++;
	}
	return n;
}
// .text:000e6a10 char* COM_Parse(char*) (dod/dlls/dod_gamerules.cpp:500)
extern char com_token[1024];
char *COM_Parse( char *data )
{
	com_token[0] = 0;
	if ( !data )
		return NULL;
	while ( *data <= ' ' )
	{
		if ( !*data )
			return NULL;
		data++;
	}
	if ( data[0] == '/' && data[1] == '/' )
	{
		while ( *data && *data != '\n' )
			data++;
		return COM_Parse( data );
	}
	if ( *data == '\"' )
	{
		int len = 0;
		data++;
		while ( *data && *data != '\"' )
			com_token[len++] = *data++;
		com_token[len] = 0;
		return *data ? data + 1 : data;
	}
	if ( *data == '{' || *data == '}' || *data == '(' || *data == ')' || *data == ',' || *data == '\'' )
	{
		com_token[0] = *data++;
		com_token[1] = 0;
		return data;
	}
	int len = 0;
	while ( *data > ' ' && *data != '{' && *data != '}' && *data != '(' && *data != ')' && *data != ',' && *data != '\'' )
		com_token[len++] = *data++;
	com_token[len] = 0;
	return data;
}
// .text:000e6b30 BOOL COM_TokenWaiting(char*) (dod/dlls/dod_gamerules.cpp:577)
BOOL COM_TokenWaiting( char *buffer )
{
	char *p = COM_Parse( buffer );
	return ( p && com_token[0] );
}
// .text:000e8d10 char* CDoDTeamPlay::COM_Parse(char*) (dod/dlls/dod_gamerules.cpp:4734)
char *CDoDTeamPlay::COM_Parse( char *data )
{
	int c, len = 0;
	com_token[0] = 0;
	if ( !data )
		return NULL;
	while ( *data <= ' ' )
	{
		if ( *data == 0 )
			return NULL;
		data++;
	}
	if ( data[0] == '/' && data[1] == '/' )
	{
		while ( *data && *data != '\n' )
			data++;
		return COM_Parse( data );
	}
	if ( *data == '\"' )
	{
		data++;
		while ( *data && *data != '\"' )
			com_token[len++] = *data++;
		com_token[len] = 0;
		return *data ? data + 1 : data;
	}
	if ( *data == '{' || *data == '}' || *data == '(' || *data == ')' || *data == ',' || *data == '\'' )
	{
		com_token[0] = *data++;
		com_token[1] = 0;
		return data;
	}
	while ( *data > ' ' && *data != '{' && *data != '}' && *data != '(' && *data != ')' && *data != ',' && *data != '\'' )
		com_token[len++] = *data++;
	com_token[len] = 0;
	return data;
}
// .text:000e8e30 BOOL CDoDTeamPlay::COM_TokenWaiting(char*) (dod/dlls/dod_gamerules.cpp:4804)
BOOL CDoDTeamPlay::COM_TokenWaiting( char *buffer )
{
	char *p = COM_Parse( buffer );
	return ( p && com_token[0] );
}
// .text:000e6930 void ExtractCommandString(char*,char*) (dod/dlls/dod_gamerules.cpp:446)
void ExtractCommandString( char *s, char *szCommand )
{
	char pkey[512], value[512];
	if ( s[0] == '\\' )
		s++;
	while ( 1 )
	{
		if ( s[0] == '\\' )
		{
			s++;
			char *o = pkey;
			*o = 0;
			char *d = value;
			*d = 0;
			while ( s[0] && s[0] != '\\' )
				*o++ = *s++;
			*o = 0;
			strcat( szCommand, pkey );
			if ( value[0] )
			{
				strcat( szCommand, " " );
				strcat( szCommand, value );
			}
			strcat( szCommand, "\n" );
			if ( !s[0] )
				return;
		}
		else if ( !s[0] )
			return;
		else
			s++;
	}
}
void CDoDTeamPlay::PreparePlayerForSpawn( CBasePlayer *pPlayer )
{
	if ( pPlayer->m_bIsObserver && pPlayer->m_imissedwave )
		pPlayer->m_irdytospawn = 1;

	int iClass = pPlayer->m_iNextClass;
	char *pszClassName;

	if ( (unsigned int)( iClass - 10 ) > 10 )
		pszClassName = m_GamePlayRules.m_bAlliesArePara ? g_ParaClassInfo[iClass].classname : g_ClassInfo[iClass].classname;
	else
		pszClassName = m_GamePlayRules.m_bAxisArePara ? g_ParaClassInfo[iClass].classname : g_ClassInfo[iClass].classname;

	ClientPrint( pPlayer->pev, HUD_PRINTTALK, pPlayer->IsAlive() ? "#game_respawn_as" : "#game_spawn_as", pszClassName );

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed role to \"%s\"\n", STRING( pPlayer->pev->netname ),
					GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ), "Allies", pszClassName );

	if ( pPlayer->m_irdytospawn )
	{
		if ( ( pPlayer->pev->team == TEAM_ALLIES && m_GamePlayRules.m_bAlliesInfiniteLives ) ||
			 ( pPlayer->pev->team == TEAM_AXIS && m_GamePlayRules.m_bAxisInfiniteLives ) || m_RoundState != ROUNDSTATE_RUNNING )
			pPlayer->DelayedSpawn();
	}
}
// .text:000e7340 int CDoDTeamPlay::CountClasses(int) (dod/dlls/dod_gamerules.cpp:2581)
int CDoDTeamPlay::CountClasses( int targetClass )
{
	int n = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *p = UTIL_PlayerByIndex( i );
		if ( p && p->IsPlayer() &&
			 ( ( (CBasePlayer *) p )->m_iNextClass == targetClass || p->pev->playerclass == targetClass ) )
			n++;
	}
	return n;
}
BOOL CDoDTeamPlay::ReachedClassLimit( int targetClass )
{
	if ( targetClass == 27 )
	{
		if ( pcvar_mp_clan_match )
			return pcvar_mp_clan_match->value > 0.0;

		return FALSE;
	}

	if ( (unsigned int)( targetClass - 1 ) > 26 )
		return FALSE;

	int iLimit = GetClassLimit( targetClass );
	int iExisting = CountClasses( targetClass );

	// the two machine gun classes can share their limits
	if ( pcvar_mp_combinemglimits && pcvar_mp_combinemglimits->value > 0.0 )
	{
		if ( targetClass == 17 )
		{
			iLimit += GetClassLimit( 18 );
			iExisting += CountClasses( 18 );
		}
		else if ( targetClass == 18 )
		{
			iLimit += GetClassLimit( 17 );
			iExisting += CountClasses( 17 );
		}
	}

	return iLimit <= iExisting && iLimit >= 0;
}
// .text:000e73e0 BOOL CDoDTeamPlay::CanPlayerJoinClass(int) (dod/dlls/dod_gamerules.cpp:3393)
// link 0xd73e0 size 0x6a Ghidra 0xe73e0 + part 0xd51a0 size 0xad Ghidra 0xe51a0
BOOL CDoDTeamPlay::CanPlayerJoinClass( int playerclass )
{
	if ( playerclass == 27 )
	{
		if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value > 0.0f )
			return FALSE;
	}
	else if ( (unsigned)( playerclass - 1 ) < 26 )
	{
		if ( ReachedClassLimit( playerclass ) )
			return FALSE;
	}
	else if ( (unsigned)( playerclass - 1 ) > 26 && playerclass != 27 )
	{
		// class 0 / >27: skip limit check, fall through to mask check
	}
	if ( playerclass == 9 || playerclass == 20 || playerclass == 26 )
		return FALSE;
	if ( (unsigned)( playerclass - 10 ) < 11 )
	{
		int v = (int)CVAR_GET_FLOAT( "mp_axisclasses" );
		int mask = ( v > -1 ) ? v : -1;
		int shift = playerclass - 10;
		if ( ( ( mask >> ( shift & 31 ) ) & 1 ) == 0 )
			return FALSE;
		if ( m_GamePlayRules.m_bAxisArePara )
			return TRUE;
		return (unsigned)( playerclass - 15 ) > 1;
	}
	int v = (int)CVAR_GET_FLOAT( "mp_alliesclasses" );
	int mask = ( v > -1 ) ? v : -1;
	int shift = m_GamePlayRules.m_bAlliesAreBrit ? playerclass - 21 : playerclass - 1;
	if ( ( ( mask >> ( shift & 31 ) ) & 1 ) == 0 )
		return FALSE;
	return TRUE;
}
void CDoDTeamPlay::ChooseRandomClass( CBasePlayer *pPlayer )
{
	int iFirst;
	int iLast;

	if ( pPlayer->pev->team == TEAM_ALLIES )
	{
		iFirst = m_GamePlayRules.m_bAlliesAreBrit ? 21 : 1;
		iLast = m_GamePlayRules.m_bAlliesAreBrit ? 26 : 9;
	}
	else if ( pPlayer->pev->team == TEAM_AXIS )
	{
		iFirst = 10;
		iLast = 20;
	}
	else
	{
		iFirst = 0;
		iLast = 0;
	}

	// the classes the player may take now
	int choices[17];
	int iChoices = 0;

	for ( int iClass = iFirst; iClass <= iLast; iClass++ )
	{
		if ( (unsigned int)( iClass - 1 ) <= 26 && ReachedClassLimit( iClass ) )
			continue;

		if ( !CanPlayerJoinClass( iClass ) )
			continue;

		choices[iChoices++] = iClass;
	}

	if ( iChoices )
	{
		pPlayer->m_iNextClass = choices[RANDOM_LONG( 0, iChoices - 1 )];
	}
	else
	{
		ALERT( at_console, "Random class found that all classes were full - ignoring class limits for this spawn\n" );

		pPlayer->m_iNextClass = RANDOM_LONG( iFirst, iLast );
	}
}
void CDoDTeamPlay::UpdateData( CBasePlayer *pPlayer )
{
	if ( pPlayer->m_iObjScore <= 255 && pPlayer->pev->frags <= 255 && pPlayer->m_iDeaths <= 255 )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( ENTINDEX( pPlayer->edict() ) );
			WRITE_BYTE( pPlayer->m_iObjScore );
			WRITE_BYTE( pPlayer->pev->frags );
			WRITE_BYTE( pPlayer->m_iDeaths );
	}
	else
	{
		ALERT( at_aiconsole, "Init HUD data requiring short values - sending with WRITE_SHORT()\n" );

		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfoLong );
			WRITE_BYTE( ENTINDEX( pPlayer->edict() ) );
			WRITE_SHORT( pPlayer->m_iObjScore );
			WRITE_SHORT( pPlayer->pev->frags );
			WRITE_SHORT( pPlayer->m_iDeaths );
	}

	WRITE_BYTE( pPlayer->pev->playerclass );
	WRITE_BYTE( pPlayer->pev->team );
	WRITE_BYTE( pPlayer->PlayerStatus() );
	MESSAGE_END();
}
void CDoDTeamPlay::LevelChangeResets( CBasePlayer *pPlayer )
{
	pPlayer->m_iFOV = 0;
	pPlayer->pev->fov = 0;

	pPlayer->CheckPlayerSpeed();

	MESSAGE_BEGIN( MSG_ONE, gmsgClientAreas, NULL, pPlayer->edict() );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 2 );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgWaveTime, NULL, pPlayer->edict() );
		WRITE_CHAR( 0 );
	MESSAGE_END();
}
// .text:000ea730 void CDoDTeamPlay::ResetForDoDCamera() link 0xda730 size 0x83 Ghidra 0xea730: DoDReset all scripted_sequence + RoundRespawn all func_train
void CDoDTeamPlay::ResetForDoDCamera( void )
{
	CBaseEntity *pEnt = UTIL_FindEntityByClassname( NULL, "scripted_sequence" );
	while ( pEnt )
	{
		((CCineMonster *)pEnt)->DoDReset();
		pEnt = UTIL_FindEntityByClassname( pEnt, "scripted_sequence" );
	}
	pEnt = UTIL_FindEntityByClassname( NULL, "func_train" );
	while ( pEnt )
	{
		pEnt->RoundRespawn();
		pEnt = UTIL_FindEntityByClassname( pEnt, "func_train" );
	}
}
// .text:000eacf0 void CDoDTeamPlay::StartClanRestart() (dod/dlls/dod_gamerules.cpp:6177)
void CDoDTeamPlay::StartClanRestart( void )
{
	int iRestartDelay = 0;
	float fRestartDelay = 0.0f;
	if ( pcvar_mp_clan_timer != NULL )
	{
		fRestartDelay = pcvar_mp_clan_timer->value;
		iRestartDelay = (int) fRestartDelay;
	}
	char szMins[8];
	char szSecs[8];
	strcpy( szMins, UTIL_dtos1( iRestartDelay / 60 ) );
	strcpy( szSecs, UTIL_dtos1( iRestartDelay % 60 ) );
	m_bRoundRestarting = TRUE;
	m_flRestartRoundTime = fRestartDelay + gpGlobals->time;
	MESSAGE_BEGIN( MSG_ALL, gmsgClanTimer );
	WRITE_BYTE( iRestartDelay );
	MESSAGE_END();
	UTIL_ClientPrintAll( HUD_PRINTCENTER, "#Clan_time_remaining", szMins, szSecs, 0, 0 );
}
int CDoDTeamPlay::GetClassMask( int team )
{
	int iFirst;
	int iLast;

	if ( team == TEAM_ALLIES )
	{
		iFirst = m_GamePlayRules.m_bAlliesAreBrit ? 21 : 1;
		iLast = m_GamePlayRules.m_bAlliesAreBrit ? 26 : 9;
	}
	else
	{
		iFirst = 10;
		iLast = 20;
	}

	// the classes that reached their limit go into the upper 16 bits
	int iLimitBits = 0;

	for ( int iClass = iFirst; iClass <= iLast; iClass++ )
	{
		if ( (unsigned int)( iClass - 1 ) <= 26 && ReachedClassLimit( iClass ) )
			iLimitBits |= 1 << ( iClass - iFirst );
	}

	int iMask = iLimitBits << 16;

	if ( team == TEAM_ALLIES )
	{
		int iClasses = (int)pcvar_mp_alliesclasses->value;

		iMask |= iClasses >= 0 ? (unsigned short)iClasses : 0xFFFF;

		if ( !m_GamePlayRules.m_bAlliesAreBrit )
			return iMask & 0xFFFFFEFF;

		return iMask & 0xFFFFFFDF;
	}

	int iClasses = (int)pcvar_mp_axisclasses->value;

	iMask |= iClasses >= 0 ? (unsigned short)iClasses : 0xFFFF;

	return iMask & 0xFFFFFBFF;
}
void CDoDTeamPlay::ShowClassSelectMenu( CBasePlayer *pPlayer )
{
	int team = pPlayer->pev->team;

	if ( team == TEAM_ALLIES )
	{
		if ( m_GamePlayRules.m_bAlliesArePara )
			ShowVGUI( pPlayer, 11, GetClassMask( 1 ) );
		else if ( m_GamePlayRules.m_bAlliesAreBrit )
			ShowVGUI( pPlayer, 12, GetClassMask( 1 ) );
		else
			ShowVGUI( pPlayer, 10, GetClassMask( 1 ) );
	}
	else if ( team == TEAM_AXIS )
	{
		if ( m_GamePlayRules.m_bAxisArePara )
			ShowVGUI( pPlayer, 14, GetClassMask( 2 ) );
		else
			ShowVGUI( pPlayer, 13, GetClassMask( 2 ) );
	}
}
// .text:000edfd0 BOOL CDoDTeamPlay::AllowSpawnableBazookas() (dod/dlls/dod_gamerules.cpp:6368)
BOOL CDoDTeamPlay::AllowSpawnableBazookas( void )
{
	return (int)CVAR_GET_FLOAT( "mp_spawnbazookas" ) != 0;
}
// NOTE: CDoDDetect ctor lives in objective.h zone (not our files) - see binary 000ee020 sz=132
class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker );
};
bool CMultiplayGameMgrHelper::CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker )
{
	if ( g_teamplay )
	{
		// only the team hears, and the living are heard by the dead but not the other way round
		if ( pTalker->pev->team != pListener->pev->team )
			return false;

		if ( pTalker->pev->deadflag != pListener->pev->deadflag )
			return pTalker->pev->deadflag == DEAD_NO;
	}

	return true;
}

CMultiplayGameMgrHelper g_GameMgrHelper;
// .text:000e4b90 void CDoDTeamPlay::AddToTeamScores(int, int) (dod/dlls/dod_gamerules.cpp:4876)
void CDoDTeamPlay::AddToTeamScores( int iTeam, int iScore )
{
	if ( ( !IsClanMatch() || IsClanMatchActive() ) && m_RoundDoesCount == TRUE )
	{
		m_iTeamScores[iTeam] = m_iTeamScores[iTeam] + iScore;
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
		WRITE_BYTE( iTeam );
		WRITE_SHORT( m_iTeamScores[iTeam] );
		MESSAGE_END();
	}
}
// .text:000e3490 BOOL CDoDTeamPlay::AllowAutoTargetCrosshair() (dod/dlls/dod_gamerules.cpp:1360)
BOOL CDoDTeamPlay::AllowAutoTargetCrosshair( void )
{
	return FALSE;
}
// .text:000ec170 void CDoDTeamPlay::ClassSelect(CBasePlayer*,int) (dod/dlls/dod_gamerules.cpp:5757)
void CDoDTeamPlay::ClassSelect( CBasePlayer *pPlayer, int iClassSelect )
{
	int team = pPlayer->pev->team;
	if ( team == 3 )
		return;

	pPlayer->m_bIsRandomClass = false;
	BOOL bIsBrit = m_GamePlayRules.m_bAlliesAreBrit;
	BOOL bIsAlly = ( team == TEAM_ALLIES );
	int iVar10;
	DodClassInfo_t *pClassInfo = NULL;

	if ( iClassSelect == 0x1b )
	{
		if ( pPlayer->m_iNextClass == -1 && pPlayer->m_imissedwave != 0 && m_RoundState == ROUNDSTATE_RUNNING )
			pPlayer->m_irdytospawn = 1;
		pPlayer->m_iNextClass = 0x1b;
		pPlayer->m_bIsRandomClass = true;
		if ( !pPlayer->IsAlive() )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_spawn_asrandom", 0, 0, 0, 0 );
			if ( pPlayer->m_iJoiningState != JOINED )
				pPlayer->m_iJoiningState = READYTOGO;
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_respawn_asrandom", 0, 0, 0, 0 );
		}
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed role to \"Random\"\n", STRING( pPlayer->pev->netname ),
						GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ),
						bIsAlly ? "Allies" : "Axis" );
		iVar10 = pPlayer->m_irdytospawn;
	}
	else
	{
		if ( bIsAlly && bIsBrit && ( iClassSelect - 0x15 ) > 5 )
			return;
		if ( !bIsBrit && bIsAlly )
		{
			if ( ( iClassSelect - 1 ) > 8 )
				return;
			if ( (unsigned) ( iClassSelect - 10 ) < 0xb )
				pClassInfo = m_GamePlayRules.m_bAxisArePara ? g_ParaClassInfo : g_ClassInfo;
			else
				pClassInfo = m_GamePlayRules.m_bAlliesArePara ? g_ParaClassInfo : g_ClassInfo;
		}
		else if ( team != TEAM_AXIS )
		{
			if ( iClassSelect == 0 )
				return;
			if ( (unsigned) ( iClassSelect - 10 ) < 0xb )
				pClassInfo = m_GamePlayRules.m_bAxisArePara ? g_ParaClassInfo : g_ClassInfo;
			else
				pClassInfo = m_GamePlayRules.m_bAlliesArePara ? g_ParaClassInfo : g_ClassInfo;
		}
		else
		{
			if ( ( iClassSelect - 10 ) > 10 )
				return;
			pClassInfo = m_GamePlayRules.m_bAxisArePara ? g_ParaClassInfo : g_ClassInfo;
		}
		const char *classname = pClassInfo[iClassSelect].classname;

		if ( iClassSelect != pPlayer->pev->playerclass &&
			 ( ( (unsigned) ( iClassSelect - 1 ) < 0x1b && ReachedClassLimit( iClassSelect ) ) ||
			   !CanPlayerJoinClass( iClassSelect ) ) )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_class_limit", classname, 0, 0, 0 );
			ShowClassSelectMenu( pPlayer );
			return;
		}
		pPlayer->m_iNextClass = iClassSelect;
		if ( pPlayer->m_imissedwave != 0 && m_RoundState == ROUNDSTATE_RUNNING )
			pPlayer->m_irdytospawn = 1;
		if ( !pPlayer->IsAlive() )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_spawn_as", classname, 0, 0, 0 );
			if ( pPlayer->m_iJoiningState != JOINED )
				pPlayer->m_iJoiningState = READYTOGO;
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_respawn_as", classname, 0, 0, 0 );
		}
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed role to \"%s\"\n", STRING( pPlayer->pev->netname ),
						GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ),
						bIsAlly ? "Allies" : "Axis", classname );
		iVar10 = pPlayer->m_irdytospawn;
	}

	if ( iVar10 == 0 )
		return;
	int team2 = pPlayer->pev->team;
	BOOL infinite;
	if ( team2 == 1 )
		infinite = m_GamePlayRules.m_bAlliesInfiniteLives;
	else if ( team2 == 2 )
		infinite = m_GamePlayRules.m_bAxisInfiniteLives;
	else
		infinite = FALSE;
	if ( !infinite && m_RoundState == ROUNDSTATE_RUNNING )
		return;
	pPlayer->DelayedSpawn();
}
// .text:000e3370 void CDoDTeamPlay::PlayerGotWeapon(CBasePlayer*,CBasePlayerItem*) (dod/dlls/dod_gamerules.cpp:1146)
// NOTE: binary also sets pWeapon->m_bTriggerSpawnOnDie (member absent from this tree's CBasePlayerItem)
void CDoDTeamPlay::PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	ItemInfo ii;
	memset( &ii, 0, sizeof( ii ) );
	pWeapon->GetItemInfo( &ii );
}
// .text:000e36a0 void CDoDTeamPlay::VoteKick(CBasePlayer*,int,bool) (dod/dlls/dod_gamerules.cpp:4422)
void CDoDTeamPlay::VoteKick( CBasePlayer *pPlayer, int vote, bool bAdmin )
{
	return;
}
// .text:000e80d0 void CDoDTeamPlay::VoteKickTimeUp() (dod/dlls/dod_gamerules.cpp:4426)
void CDoDTeamPlay::VoteKickTimeUp( void )
{
	return;
}
// .text:000e70f0 void CDoDTeamPlay::CheckOneLifeRoundConditions() (dod/dlls/dod_gamerules.cpp:1334)
void CDoDTeamPlay::CheckOneLifeRoundConditions( void )
{
	return;
}

// ============================================================
// Missing CDoDTeamPlay / free bodies restored from dod.so (Ghidra addr = link + 0x10000)
// ============================================================

// .text:000e3260 int CDoDTeamPlay::DeadPlayerWeapons(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:873)
int CDoDTeamPlay::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return 8;
}
// .text:000e3290 int CDoDTeamPlay::DeadPlayerAmmo(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:893)
int CDoDTeamPlay::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return 11;
}
// .text:000e32c0 float CDoDTeamPlay::FlItemRespawnTime(CItem*) (dod/dlls/dod_gamerules.cpp:968)
float CDoDTeamPlay::FlItemRespawnTime( CItem *pItem )
{
	return 30.0f + gpGlobals->time;
}
// .text:000e32f0 int CDoDTeamPlay::AmmoShouldRespawn(CBasePlayerAmmo*) (dod/dlls/dod_gamerules.cpp:997)
int CDoDTeamPlay::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	if ( ( pAmmo->pev->spawnflags & SF_NORESPAWN ) != 0 )
		return 4;
	return 3;
}
// .text:000e3310 float CDoDTeamPlay::FlAmmoRespawnTime(CBasePlayerAmmo*) (dod/dlls/dod_gamerules.cpp:1016)
float CDoDTeamPlay::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return 20.0f + gpGlobals->time;
}
// .text:000e3360 float CDoDTeamPlay::FlPlayerSpawnTime(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:1056)
float CDoDTeamPlay::FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;
}
// .text:000e33c0 float CDoDTeamPlay::FlHealthChargerRechargeTime() (dod/dlls/dod_gamerules.cpp:1168)
float CDoDTeamPlay::FlHealthChargerRechargeTime( void )
{
	return 60.0f;
}
// .text:000e33d0 float CDoDTeamPlay::FlHEVChargerRechargeTime() (dod/dlls/dod_gamerules.cpp:1174)
float CDoDTeamPlay::FlHEVChargerRechargeTime( void )
{
	return 30.0f;
}
// .text:000e34d0 const char* CDoDTeamPlay::SetDefaultPlayerTeam(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:2610)
const char *CDoDTeamPlay::SetDefaultPlayerTeam( CBasePlayer *pPlayer )
{
	return NULL;
}
// .text:000e34f0 BOOL CDoDTeamPlay::FPlayerCanTakeDamage(CBasePlayer*,CBaseEntity*) (dod/dlls/dod_gamerules.cpp:3229)
BOOL CDoDTeamPlay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	BOOL bRet = TRUE;
	if ( pAttacker != NULL )
	{
		if ( CGameRules::FPlayerCanTakeDamage( pPlayer, pAttacker ) == TRUE && pcvar_mp_friendlyfire )
			bRet = ( (CBasePlayer *) pAttacker == pPlayer || pcvar_mp_friendlyfire->value > 0.0f );
	}
	return bRet;
}
// .text:000e3570 BOOL CDoDTeamPlay::ShouldAutoAim(CBasePlayer*,edict_t*) (dod/dlls/dod_gamerules.cpp:3279)
BOOL CDoDTeamPlay::ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target )
{
	return FALSE;
}
// .text:000e3580 int CDoDTeamPlay::IPointsForKill(CBasePlayer*,CBasePlayer*) (dod/dlls/dod_gamerules.cpp:3284)
int CDoDTeamPlay::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	int pts = 0;
	if ( pKilled != NULL )
	{
		pts = 1;
		if ( pAttacker != NULL && pAttacker != pKilled )
			pts = ( FPlayerCanTakeDamage( pAttacker, pKilled ) != TRUE );
	}
	return pts;
}
// .text:000e35d0 const char* CDoDTeamPlay::GetTeamID(CBaseEntity*) (dod/dlls/dod_gamerules.cpp:3301)
const char *CDoDTeamPlay::GetTeamID( CBaseEntity *pEntity )
{
	if ( pEntity != NULL && pEntity->pev != NULL )
		return pEntity->TeamID();
	return "";
}
// .text:000e36f0 void CDoDTeamPlay::SetRoundState(int) (dod/dlls/dod_gamerules.cpp:4825)
void CDoDTeamPlay::SetRoundState( int i_State )
{
	m_RoundState = i_State;
	MESSAGE_BEGIN( MSG_ALL, gmsgRoundState );
	WRITE_BYTE( m_RoundState );
	MESSAGE_END();
}
// .text:000e3740 void CDoDTeamPlay::SetRestartTime(float) (dod/dlls/dod_gamerules.cpp:4861)
void CDoDTeamPlay::SetRestartTime( float f_Time )
{
	m_flRestartTime = f_Time;
}
// .text:000e3b20 void CDoDTeamPlay::SetMapChangeFromTime(int) (dod/dlls/dod_gamerules.cpp:5072)
void CDoDTeamPlay::SetMapChangeFromTime( int iTime )
{
	MapChangeFromTime = iTime;
}
// .text:000e3b50 void CDoDTeamPlay::setMapTime() (dod/dlls/dod_gamerules.cpp:5176)
void CDoDTeamPlay::setMapTime( void )
{
	m_flDoDMapTime = gpGlobals->time;
}
// .text:000e3b70 bool CDoDTeamPlay::IsClanMatch() (dod/dlls/dod_gamerules.cpp:6160)
bool CDoDTeamPlay::IsClanMatch( void )
{
	return m_bClanMatch;
}
// .text:000e3b80 bool CDoDTeamPlay::IsClanMatchActive() (dod/dlls/dod_gamerules.cpp:6165)
bool CDoDTeamPlay::IsClanMatchActive( void )
{
	return m_bClanMatchActive;
}
// .text:000ea5c0 bool CDoDTeamPlay::IsAxisInfinite() (dod/dlls/dod_gamerules.cpp:5555)
BOOL CDoDTeamPlay::IsAxisInfinite( void )
{
	return m_GamePlayRules.m_bAxisInfiniteLives;
}
// .text:000ea5d0 bool CDoDTeamPlay::IsAxisPara() (dod/dlls/dod_gamerules.cpp:5560)
BOOL CDoDTeamPlay::IsAxisPara( void )
{
	return m_GamePlayRules.m_bAxisArePara;
}
// .text:000ea5e0 bool CDoDTeamPlay::IsAlliesInfinite() (dod/dlls/dod_gamerules.cpp:5565)
BOOL CDoDTeamPlay::IsAlliesInfinite( void )
{
	return m_GamePlayRules.m_bAlliesInfiniteLives;
}
// .text:000ea5f0 bool CDoDTeamPlay::IsAlliesPara() (dod/dlls/dod_gamerules.cpp:5570)
BOOL CDoDTeamPlay::IsAlliesPara( void )
{
	return m_GamePlayRules.m_bAlliesArePara;
}
// .text:000e4aa0 void CDoDTeamPlay::PlayerThink(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:1651)
void CDoDTeamPlay::PlayerThink( CBasePlayer *pPlayer )
{
	if ( g_fGameOver )
	{
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}
// .text:000e4a60 int CDoDTeamPlay::WeaponShouldRespawn(CBasePlayerItem*) (dod/dlls/dod_gamerules.cpp:1025)
int CDoDTeamPlay::WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	if ( ( pWeapon->pev->spawnflags & SF_NORESPAWN ) != 0 )
		return 2;
	return 2 - ( ( pWeapon->pev->flags & FL_IMMUNE_LAVA ) == 0 );
}
// .text:000e5050 float CDoDTeamPlay::FlWeaponRespawnTime(CBasePlayerItem*) (dod/dlls/dod_gamerules.cpp:0)
float CDoDTeamPlay::FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	if ( ( CBasePlayerItem::ItemInfoArray[pWeapon->m_iId].iFlags & ITEM_FLAG_LAUNCHER ) == 0 )
	{
		if ( cvar_weaponstay.value > 0.0f &&
			 ( CBasePlayerItem::ItemInfoArray[pWeapon->m_iId].iFlags & ITEM_FLAG_LIMITINWORLD ) == 0 )
			return gpGlobals->time;
		return 20.0f + gpGlobals->time;
	}
	return 0.0f;
}
// .text:000e4fb0 float CDoDTeamPlay::FlWeaponTryRespawn(CBasePlayerItem*) (dod/dlls/dod_gamerules.cpp:980)
float CDoDTeamPlay::FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon == NULL )
		return 0.0f;
	if ( pWeapon->m_iId != 0 && ( CBasePlayerItem::ItemInfoArray[pWeapon->m_iId].iFlags & ITEM_FLAG_LIMITINWORLD ) != 0 &&
		 (int) NUMBER_OF_ENTITIES() >= gpGlobals->maxEntities - 100 )
		return FlWeaponRespawnTime( pWeapon );
	return 0.0f;
}
// .text:000e4ad0 int CDoDTeamPlay::PlayerRelationship(CBaseEntity*,CBaseEntity*) (dod/dlls/dod_gamerules.cpp:3260)
int CDoDTeamPlay::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	if ( pTarget == NULL || pPlayer == NULL )
		return 0;
	if ( !pTarget->IsPlayer() )
		return 0;
	if ( GetTeamID( pPlayer )[0] == 0 )
		return 0;
	if ( GetTeamID( pTarget )[0] == 0 )
		return 0;
	return ( stricmp( GetTeamID( pTarget ), GetTeamID( pPlayer ) ) == 0 );
}
// .text:000e9090 void CDoDTeamPlay::SendWaveStatus(CBasePlayer*,int) (dod/dlls/dod_gamerules.cpp:5162)
void CDoDTeamPlay::SendWaveStatus( CBasePlayer *pPlayer, int status )
{
	if ( pPlayer == NULL )
		return;
	int team = pPlayer->pev->team;
	BOOL infinite;
	if ( team == TEAM_ALLIES )
		infinite = m_GamePlayRules.m_bAlliesInfiniteLives;
	else if ( team == TEAM_AXIS )
		infinite = m_GamePlayRules.m_bAxisInfiniteLives;
	else
		return;
	if ( infinite )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWaveStatus, NULL, pPlayer->pev );
		WRITE_BYTE( status );
		MESSAGE_END();
	}
}
// .text:000e94c0 void CDoDTeamPlay::CopyGamePlayLogic(gameplay_rules_t const&) (dod/dlls/dod_gamerules.cpp:5497)
void CDoDTeamPlay::CopyGamePlayLogic( const gameplay_rules_t &rules )
{
	memcpy( &m_GamePlayRules, &rules, sizeof( gameplay_rules_t ) );
}
// .text:000e9680 void CDoDTeamPlay::SetClanMatch(bool) (dod/dlls/dod_gamerules.cpp:6142)
void CDoDTeamPlay::SetClanMatch( bool bMatchOn )
{
	m_bClanMatch = bMatchOn;
	m_bClanMatchActive = FALSE;
	if ( !bMatchOn )
	{
		m_bRoundRestarting = FALSE;
		m_bAwaitingReadyRestart = FALSE;
		m_bHeardAlliesReady = FALSE;
		m_bHeardAxisReady = FALSE;
	}
	SendClientGamerules( NULL );
}
void CDoDTeamPlay::ReturnToWarmup( void )
{
	if ( m_bClanMatch )
	{
		m_bClanMatchActive = FALSE;
		setMapTime();
		SendClientGamerules( NULL );
	}
}
// .text:000e9680 void CDoDTeamPlay::SetClanMatch(bool) (dod/dlls/dod_gamerules.cpp:6142)
const char *CDoDTeamPlay::GetPlayerClassName( int playerclass )
{
	BOOL para;
	if ( (unsigned) ( playerclass - 10 ) < 11 )
		para = m_GamePlayRules.m_bAxisArePara;
	else
		para = m_GamePlayRules.m_bAlliesArePara;
	if ( para )
		return g_ParaClassInfo[playerclass].classname;
	return g_ClassInfo[playerclass].classname;
}
// .text:000e5860 void CDoDTeamPlay::PlayerDeathThink(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:5410)
void CDoDTeamPlay::PlayerDeathThink( CBasePlayer *pPlayer )
{
	if ( gpGlobals->time <= pPlayer->m_fDeadTime + 1.0f || g_fGameOver )
		return;

	if ( RoundState() != ROUNDSTATE_RUNNING )
	{
		pPlayer->StartObserver( pPlayer->pev->origin, pPlayer->pev->angles );
		return;
	}

	pPlayer->m_bIsObserver = TRUE;
	int team = pPlayer->pev->team;
	BOOL infinite;
	if ( team == TEAM_ALLIES )
		infinite = m_GamePlayRules.m_bAlliesInfiniteLives;
	else if ( team == TEAM_AXIS )
		infinite = m_GamePlayRules.m_bAxisInfiniteLives;
	else
		infinite = FALSE;

	if ( infinite )
	{
		if ( GetMaxWaveTime( pPlayer->pev->team ) <= 0.0f )
		{
			float waveTime = getWaveTime( pPlayer->pev->team );
			if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value > 0.0f && pcvar_mp_clan_respawntime &&
				 pcvar_mp_clan_respawntime->value > 0.0f )
				waveTime = pcvar_mp_clan_respawntime->value;
			float t = waveTime;
			if ( 3.0f > t )
				t = 3.0f;
			setWaveTime( pPlayer->pev->team, t );
		}
		if ( IsTeamInfinite( pPlayer->pev->team ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgWaveStatus, NULL, pPlayer->pev );
			WRITE_BYTE( 1 );
			MESSAGE_END();
		}
	}
	pPlayer->StartObserver( pPlayer->pev->origin, pPlayer->pev->angles );
}
// .text:000e6f80 void CDoDTeamPlay::SendMOTDToClient(edict_t*) (dod/dlls/dod_gamerules.cpp:911)
void CDoDTeamPlay::SendMOTDToClient( edict_t *client )
{
	int length = 0;
	char *pFileList = (char *) LOAD_FILE_FOR_ME( CVAR_GET_STRING( "motdfile" ), &length );
	MESSAGE_BEGIN( MSG_ONE, gmsgServerName, NULL, client );
	WRITE_STRING( CVAR_GET_STRING( "hostname" ) );
	MESSAGE_END();
	if ( pFileList && pFileList[0] != 0 )
	{
		int char_count = 0;
		char *aFileList = pFileList;
		do
		{
			char chunk[61];
			if ( (int) strlen( aFileList ) < 60 )
				strcpy( chunk, aFileList );
			else
			{
				strncpy( chunk, aFileList, 60 );
				chunk[60] = 0;
			}
			char_count += (int) strlen( chunk );
			if ( char_count < 0x600 )
				aFileList = pFileList + char_count;
			else
				*aFileList = 0;
			MESSAGE_BEGIN( MSG_ONE, gmsgMOTD, NULL, client );
			WRITE_BYTE( ( *aFileList == 0 ) );
			WRITE_STRING( chunk );
			MESSAGE_END();
		} while ( *aFileList != 0 && char_count < 0x600 );
	}
	FREE_FILE( pFileList );
}
// .text:000e7d00 void RespawnAllies() (dod/dlls/dod_gamerules.cpp:4249)
void RespawnAllies( void )
{
	ALERT( at_console, "Respawning Allies\n" );
	CBaseEntity *ent1 = NULL;
	FireTargets( "dod_respawn_allies", NULL, NULL, USE_TOGGLE, 0.0f );
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *p = (CBasePlayer *) UTIL_PlayerByIndex( i );
		if ( p && !p->has_disconnected && p->pev->team == TEAM_ALLIES && !p->IsAlive() )
		{
			if ( p->m_iNextClass == -1 )
			{
				ClientPrint( p->pev, HUD_PRINTCENTER, "#game_will_spawn", 0, 0, 0, 0 );
				p->m_imissedwave = 1;
			}
			else
			{
				p->DelayedSpawn();
				if ( !ent1 )
					ent1 = p;
			}
		}
	}
	UTIL_SendDirectorMessage( ent1, NULL, 2 | DRC_FLAG_FACEPLAYER );
}
// .text:000e7e20 void RespawnAxis() (dod/dlls/dod_gamerules.cpp:4291)
void RespawnAxis( void )
{
	CBaseEntity *ent1 = NULL;
	ALERT( at_console, "Respawning Axis\n" );
	FireTargets( "dod_respawn_axis", NULL, NULL, USE_TOGGLE, 0.0f );
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *p = (CBasePlayer *) UTIL_PlayerByIndex( i );
		if ( p && !p->has_disconnected && p->pev->team == TEAM_AXIS && !p->IsAlive() )
		{
			if ( p->m_iNextClass == -1 )
			{
				ClientPrint( p->pev, HUD_PRINTCENTER, "#game_will_spawn", 0, 0, 0, 0 );
				p->m_imissedwave = 1;
			}
			else
			{
				p->DelayedSpawn();
				if ( !ent1 )
					ent1 = p;
			}
		}
	}
	UTIL_SendDirectorMessage( ent1, NULL, 2 );
}

// .text:000e4110 void CDoDTeamPlay::ChangePlayerTeam(CBasePlayer*, int, int, int) (dod/dlls/dod_gamerules.cpp:2729)
void CDoDTeamPlay::ChangePlayerTeam( CBasePlayer *pPlayer, int iTeam, int bKill, int bGib )
{
	int clientIndex = ENTINDEX( pPlayer->edict() );
	if ( bKill )
	{
		entvars_t *pevWorld = VARS( INDEXENT( 0 ) );
		pPlayer->TakeDamage( pevWorld, pevWorld, 900.0f, 0x1000 );
	}
	strncpy( pPlayer->m_szTeamName, g_szTeamNames[iTeam], 0x10 );
	SET_CLIENT_KEYVALUE( clientIndex, GET_INFOKEYBUFFER( pPlayer->edict() ), "team", pPlayer->m_szTeamName );
	MESSAGE_BEGIN( MSG_ALL, gmsgPTeam );
	WRITE_BYTE( clientIndex );
	WRITE_BYTE( iTeam );
	MESSAGE_END();
}

void CDoDTeamPlay::PlayerSpawn( CBasePlayer *pPlayer )
{
	if ( pPlayer->has_disconnected || pPlayer->pev->iuser1 != 0 || pPlayer->pev->team == TEAM_SPECTATOR || pPlayer->pev->team == 0 ||
		 pPlayer->m_iNextClass == 0 )
		return;

	int entindex = ENTINDEX( pPlayer->edict() );

	if ( pPlayer->m_bIsRandomClass )
	{
		ChooseRandomClass( pPlayer );

		DodClassInfo_t *pInfo;

		if ( (unsigned int)( pPlayer->m_iNextClass - 10 ) <= 10 )
			pInfo = m_GamePlayRules.m_bAxisArePara ? g_ParaClassInfo : g_ClassInfo;
		else
			pInfo = m_GamePlayRules.m_bAlliesArePara ? g_ParaClassInfo : g_ClassInfo;

		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_now_as", pInfo[pPlayer->m_iNextClass].classname );
	}

	pPlayer->pev->playerclass = pPlayer->m_iNextClass;
	pPlayer->m_ilastteam = 0;
	pPlayer->SetBodygroup( 1, 2 );

	int cls = pPlayer->pev->playerclass;

	if ( cls == 0 )
	{
		pPlayer->m_sPlayerModel = NULL;
	}
	else
	{
		if ( pPlayer->pev->team == TEAM_ALLIES )
			pPlayer->m_sPlayerModel = "us-inf";
		else if ( pPlayer->pev->team == TEAM_AXIS )
			pPlayer->m_sPlayerModel = "axis-inf";

		if ( (unsigned int)( cls - 1 ) <= 26 )
		{
			DodClassInfo_t *pClass;

			if ( pPlayer->pev->team == TEAM_ALLIES && m_GamePlayRules.m_bAlliesArePara )
				pClass = &g_ParaClassInfo[cls];
			else if ( pPlayer->pev->team == TEAM_AXIS && m_GamePlayRules.m_bAxisArePara )
				pClass = &g_ParaClassInfo[cls];
			else
				pClass = &g_ClassInfo[cls];

			pPlayer->m_sPlayerModel = pClass->modelname;

			for ( int i = 0; i < 32; i++ )
			{
				if ( !( ( pClass->weapons >> i ) & 1 ) )
					continue;

				if ( i == 23 || i == 25 )
				{
					// the scoped class of the FG42 and of the Enfield starts with the scope up
					edict_t *pent = pPlayer->GiveNamedItem( i == 23 ? "weapon_fg42" : "weapon_enfield" );

					if ( pent && pPlayer->pev->playerclass == ( i == 23 ? 16 : 23 ) )
					{
						CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)CBaseEntity::Instance( pent );

						pWeapon->m_iWeaponState |= 1;
						pWeapon->Deploy();
					}
				}
				else
				{
					pPlayer->GiveNamedItem( weaponnames[i] );
				}
			}

			for ( int i = 0; i < 32; i++ )
			{
				if ( ( pClass->weapons2 >> i ) & 1 )
					pPlayer->GiveNamedItem( weaponnames[i + 32] );
			}

			if ( pPlayer->pev->team == TEAM_ALLIES )
			{
				for ( int i = 0; i < pClass->numgrenades; i++ )
					pPlayer->GiveNamedItem( "weapon_handgrenade" );
			}
			else if ( pPlayer->pev->team == TEAM_AXIS )
			{
				for ( int i = 0; i < pClass->numgrenades; i++ )
					pPlayer->GiveNamedItem( "weapon_stickgrenade" );
			}

			pPlayer->SetBodygroup( 2, pClass->headgroup );
			pPlayer->SetBodygroup( 1, pClass->helmetgroup );
			pPlayer->SetBodygroup( 3, pClass->geargroup );
			pPlayer->SetBodygroup( 0, pClass->bodygroup );
			pPlayer->SetBodygroup( 4, 0 );
		}

		pPlayer->m_iClassSpeed = 600;
		pPlayer->CheckPlayerSpeed();

		SET_CLIENT_KEYVALUE( ENTINDEX( pPlayer->edict() ), GET_INFOKEYBUFFER( pPlayer->edict() ), "model", pPlayer->m_sPlayerModel );
	}

	SendTeamScores( 3 );

	MESSAGE_BEGIN( MSG_ALL, gmsgPStatus );
		WRITE_BYTE( entindex );
		WRITE_BYTE( pPlayer->PlayerStatus() );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgPClass );
		WRITE_BYTE( entindex );
		WRITE_BYTE( pPlayer->pev->playerclass );
	MESSAGE_END();
}

// .text:000eba30 void CDoDTeamPlay::TeamSelect(CBasePlayer*, int) (dod/dlls/dod_gamerules.cpp:5402)
void CDoDTeamPlay::TeamSelect( CBasePlayer *pPlayer, int iTeamSelect )
{
	if ( (unsigned) ( iTeamSelect - 1 ) > 4 )
		return;
	if ( iTeamSelect == 4 )
		return;

	BOOL bShowVGUI = FALSE;
	if ( iTeamSelect == 3 )
	{
		if ( pcvar_mp_allowspectators != NULL && pcvar_mp_allowspectators->value <= 0.0f )
			bShowVGUI = TRUE;
	}
	else if ( iTeamSelect == 5 )
	{
		int iAllies = 0;
		int iAxis = 0;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *p = UTIL_PlayerByIndex( i );
			if ( p )
			{
				if ( p->pev->team == TEAM_ALLIES )
					iAllies++;
				else if ( p->pev->team == TEAM_AXIS )
					iAxis++;
			}
		}
		if ( iAllies - iAxis < 0 )
			iTeamSelect = 1;
		else if ( iAllies != iAxis )
			iTeamSelect = 2;
		else
			iTeamSelect = ( m_iTeamScores[2] <= m_iTeamScores[1] ) + 1;
	}

	int team = pPlayer->pev->team;
	if ( team == iTeamSelect )
		return;

	if ( pcvar_mp_teamlimit != NULL && pcvar_mp_teamlimit->value > 0.0f && iTeamSelect != 3 )
	{
		int iAllies = 0;
		int iAxis = 0;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *p = UTIL_PlayerByIndex( i );
			if ( p )
			{
				if ( p->pev->team == TEAM_ALLIES )
					iAllies++;
				else if ( p->pev->team == TEAM_AXIS )
					iAxis++;
			}
		}
		int diff = ( iTeamSelect == 1 ) ? ( iAllies - iAxis ) : ( iAxis - iAllies );
		if ( pcvar_mp_teamlimit->value <= (float) diff )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#game_uneven_teams", 0, 0, 0, 0 );
			bShowVGUI = TRUE;
		}
		else
			team = pPlayer->pev->team;
	}

	if ( !bShowVGUI )
	{
		if ( pPlayer->m_ilastteam == 1 && team != 3 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#game_switch_teams_once", 0, 0, 0, 0 );
			return;
		}
		if ( team > 0 && iTeamSelect != team )
			pPlayer->m_ilastteam = 1;

		pPlayer->m_iNextClass = -1;
		if ( pPlayer->pev->team != TEAM_SPECTATOR )
		{
			pPlayer->SetThink( &CBasePlayer::ServerSpawnCheck );
			pPlayer->pev->nextthink = gpGlobals->time;
		}
		pPlayer->pev->playerclass = 0;
		pPlayer->m_bIsRandomClass = false;

		int team = pPlayer->pev->team;
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" joined team \"%s\"\n", STRING( pPlayer->pev->netname ),
						GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ),
						g_szTeamNames[team], g_szTeamNames[iTeamSelect] );
		ChangePlayerTeam( pPlayer, iTeamSelect, 1, 0 );
		pPlayer->pev->team = iTeamSelect;

		if ( iTeamSelect == 3 )
		{
			pPlayer->m_iJoiningState = JOINED;
			Vector color = g_vecZero;
			pPlayer->UTIL_EdictScreenFade( pPlayer->edict(), color, 1.0f, 15.0f, 0, 5 );
			pPlayer->pev->deadflag = DEAD_DEAD;
			pPlayer->pev->effects |= EF_NOINTERP;
			pPlayer->m_bIsObserver = 1;
			pPlayer->StartObserver( pPlayer->pev->origin, pPlayer->pev->angles );
			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_joined_team", STRING( pPlayer->pev->netname ),
								 g_szTeamNames[iTeamSelect], 0, 0 );
			return;
		}

		BOOL b_playableteam = (unsigned) ( team - 1 ) < 3;
		int myTeam = pPlayer->pev->team;
		BOOL infinite = ( myTeam == 1 ) ? m_GamePlayRules.m_bAlliesInfiniteLives
										: ( myTeam == 2 ) ? m_GamePlayRules.m_bAxisInfiniteLives : FALSE;
		if ( infinite && b_playableteam )
		{
			float tempTime;
			if ( pcvar_mp_clan_match != NULL && pcvar_mp_clan_match->value > 0.0f &&
				 pcvar_mp_clan_respawntime != NULL && pcvar_mp_clan_respawntime->value > 0.0f )
				tempTime = pcvar_mp_clan_respawntime->value;
			else
				tempTime = GetMaxWaveTime( myTeam );
			float lVar13 = ( m_RoundState == ROUNDSTATE_RUNNING ) ? tempTime : 0.0f;
			MESSAGE_BEGIN( MSG_ONE, gmsgWaveStatus, NULL, pPlayer->pev );
			WRITE_BYTE( 1 );
			MESSAGE_END();
			setWaveTime( myTeam, lVar13 );
		}
		else if ( !infinite )
		{
			float waveTime = getWaveTime( myTeam );
			if ( waveTime > 0.0f )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgWaveStatus, NULL, pPlayer->pev );
				WRITE_BYTE( 1 );
				MESSAGE_END();
			}
		}

		ShowClassSelectMenu( pPlayer );
		MESSAGE_BEGIN( MSG_ALL, gmsgPClass );
		WRITE_BYTE( ENTINDEX( pPlayer->edict() ) );
		WRITE_BYTE( pPlayer->pev->playerclass );
		MESSAGE_END();

		UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_joined_team", STRING( pPlayer->pev->netname ),
							 g_szTeamNames[iTeamSelect], 0, 0 );
		return;
	}

	ShowVGUI( pPlayer, 2, 0 );
}

// LINK entities
LINK_ENTITY_TO_CLASS( para_roundtimer, CDodParaRoundTimer );
LINK_ENTITY_TO_CLASS( dod_preround, CDoDPreRound );
LINK_ENTITY_TO_CLASS( info_doddetect, CDoDDetect );

// .text:000e5250 void CDoDPreRound::KeyValue(KeyValueData*) (dod/dlls/dod_gamerules.cpp:6673)
void CDoDPreRound::KeyValue( KeyValueData *pkvd )
{
	pkvd->fHandled = TRUE;
	if ( !strcmp( pkvd->szKeyName, "allieswin" ) )
		m_allieswin = ALLOC_STRING( pkvd->szValue );
	else if ( !strcmp( pkvd->szKeyName, "alliesloose" ) )
		m_alliesloose = ALLOC_STRING( pkvd->szValue );
	else if ( !strcmp( pkvd->szKeyName, "axiswin" ) )
		m_axiswin = ALLOC_STRING( pkvd->szValue );
	else if ( !strcmp( pkvd->szKeyName, "axisloose" ) )
		m_axisloose = ALLOC_STRING( pkvd->szValue );
	else if ( !strcmp( pkvd->szKeyName, "draw" ) )
		m_draw = ALLOC_STRING( pkvd->szValue );
	else
		pkvd->fHandled = FALSE;
}

// .text:000e3640 CDoDTeamPlay::IsValidTeam (file:dod_gamerules.cpp)
BOOL CDoDTeamPlay::IsValidTeam( char *pTeamName )
{
	return 1;
}

// .text:000e3600 CDoDTeamPlay::GetTeamIndex (file:dod_gamerules.cpp)
int CDoDTeamPlay::GetTeamIndex( char *pTeamName )
{
	return -1;
}

// .text:000e9120 CDoDTeamPlay::ReadInClassInfo (file:dod_gamerules.cpp)
void CDoDTeamPlay::ReadInClassInfo( void )
{
}

extern float g_flTimeLimit;
extern cvar_t *pcvar_mp_log_scores_delay;

CDoDTeamPlay::CDoDTeamPlay( void )
{
	g_VoiceGameMgr.Init( &g_GameMgrHelper, gpGlobals->maxClients );

	CGameRules::RefreshSkillData();

	gSkillData.plrDmgCOLT = 40.0f;
	gSkillData.plrDmgLUGER = 40.0f;
	gSkillData.plrDmgGARAND = 120.0f;
	gSkillData.plrDmgSCOPEDKAR = 160.0f;
	gSkillData.plrDmgCARBINE = 40.0f;
	gSkillData.plrDmgTHOMPSON = 40.0f;
	gSkillData.plrDmgMP44 = 50.0f;
	gSkillData.plrDmgSPRING = 160.0f;
	gSkillData.plrDmgKAR = 160.0f;
	gSkillData.plrDmgBAR = 85.0f;
	gSkillData.plrDmg30CAL = 85.0f;
	gSkillData.plrDmgMG42 = 85.0f;
	gSkillData.plrDmgMG34 = 85.0f;
	gSkillData.plrDmgMP40 = 40.0f;
	gSkillData.plrDmgGREASEGUN = 40.0f;
	gSkillData.plrDmgFG42 = 85.0f;
	gSkillData.plrDmgK43 = 120.0f;
	gSkillData.plrDmgEnfield = 160.0f;
	gSkillData.plrDmgSten = 40.0f;
	gSkillData.plrDmgBren = 85.0f;
	gSkillData.plrDmgWebley = 40.0f;
	gSkillData.plrDmgHandGrenade = 180.0f;
	gSkillData.plrDmgStickGrenade = 180.0f;
	gSkillData.plrDmgCrowbar = 60.0f;
	gSkillData.plrDmgBayonet = 60.0f;
	gSkillData.plrDmgGarandButt = 60.0f;
	gSkillData.plrDmgEnfieldButt = 60.0f;
	gSkillData.plrDmgK43Butt = 60.0f;
	gSkillData.plrDmgBazookaShell = 100.0f;
	gSkillData.plrDmgPiatShell = 100.0f;
	gSkillData.plrDmgPschreckShell = 100.0f;
	gSkillData.plrDmgMortarShell = 180.0f;

	m_flIntermissionEndTime = 0.0f;
	g_flIntermissionStartTime = 0.0f;
	g_flTimeLimit = gpGlobals->time;
	m_flDoDMapTime = gpGlobals->time;

	if ( !IS_DEDICATED_SERVER() )
	{
		const char *lservercfgfile = CVAR_GET_STRING( "lservercfgfile" );

		if ( lservercfgfile && *lservercfgfile )
		{
			char szCommand[256];

			ALERT( at_console, "Executing listen server config file\n" );

			sprintf( szCommand, "exec %s\n", lservercfgfile );
			SERVER_COMMAND( szCommand );
		}
	}

	char szTemp[44];

	sprintf( szTemp, "exec %s.cfg\n", STRING( gpGlobals->mapname ) );
	SERVER_COMMAND( szTemp );

	m_pParaTimer = NULL;
	m_iNumAlliesAlive = 0;
	m_iNumAxisAlive = 0;
	m_iNumAlliesOnTeam = 0;
	m_iNumAxisOnTeam = 0;
	m_DisableDeathMessages = 0;
	m_bAwaitingReadyRestart = 0;
	m_bHeardAlliesReady = 0;
	m_bHeardAxisReady = 0;
	gr_gameRules = 0;
	m_rReadyToUnFreeze = 0;
	m_RoundState = 0;
	m_DisableDeathPenalty = 0;
	m_vKickSessionState = 0;
	m_flIntermissionEndTime = 0.0f;
	g_flIntermissionStartTime = 0.0f;
	m_flDoDMapTime = 0.0f;
	m_DoDCameraState = 0;
	m_flRoundTime = 0.0f;
	m_bClanMatch = false;
	m_bClanMatchActive = false;
	m_bRoundRestarting = 0;
	m_flRestartRoundTime = 0.0f;
	m_iTeamScores[2] = 0;
	m_iTeamScores[1] = 0;
	m_bIsRoundFrozen = false;
	m_SaveRoundState = 0;
	m_RoundDoesCount = 1;
	m_RoundParaCanJoin = 1;
	m_flGoodToGoTime = gpGlobals->time + 20.0f;
	m_vKickTime = 0.0f;
	m_flSpamResetTime = 0.0f;
	m_vKickENT = NULL;

	if ( MapChangeFromTime != 1 )
		SetNextMap();

	MapChangeFromTime = 1;

	m_flAlliesWaveTime = 0.0f;
	m_flAxisWaveTime = 0.0f;
	m_flAlliesRespawn = 0.0f;
	m_flAxisRespawn = 0.0f;

	float flLogDelay = 10.0f;

	if ( pcvar_mp_log_scores_delay )
	{
		if ( pcvar_mp_log_scores_delay->value >= 1.0 )
		{
			flLogDelay = pcvar_mp_log_scores_delay->value;

			if ( flLogDelay > 600.0 )
				flLogDelay = 600.0;
		}
		else
		{
			flLogDelay = 1.0f;
		}
	}

	m_fLogScoresTime = gpGlobals->time + flLogDelay;

	// the cvars of the class limits, class 0 has none
	pcvar_classlimitcvars[0] = NULL;

	for ( int i = 1; i != 27; i++ )
		pcvar_classlimitcvars[i] = CVAR_GET_POINTER( classlimitcvarNames[i] );
}

// .text:000de460 CDoDPreRound::Launch (file:dod_gamerules.cpp)
void CDoDPreRound::Launch( int iRoundState )
{
	if ( iRoundState == 3 )
	{
		if ( m_allieswin != 0 )
			FireTargets( STRING( m_allieswin ), NULL, this, USE_TOGGLE, 0 );
		if ( m_axisloose == 0 )
			return;
		FireTargets( STRING( m_axisloose ), NULL, this, USE_TOGGLE, 0 );
	}
	else if ( iRoundState == 4 )
	{
		if ( m_axiswin != 0 )
			FireTargets( STRING( m_axiswin ), NULL, this, USE_TOGGLE, 0 );
		if ( m_alliesloose == 0 )
			return;
		FireTargets( STRING( m_alliesloose ), NULL, this, USE_TOGGLE, 0 );
	}
	else if ( iRoundState == 5 )
	{
		if ( m_draw == 0 )
			return;
		FireTargets( STRING( m_draw ), NULL, this, USE_TOGGLE, 0 );
	}
}

// .text:000eace0 CDoDTeamPlay::IsAlliesBrit (file:dod_gamerules.cpp)
bool CDoDTeamPlay::IsAlliesBrit( void )
{
	return m_GamePlayRules.m_bAlliesAreBrit;
}

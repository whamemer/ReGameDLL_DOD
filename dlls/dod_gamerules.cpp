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
	if ( cvar_weaponstay.value <= 0.0f || ( CBasePlayerItem::ItemInfoArray[pItem->m_iId].iFlags & 8 ) == 0 )
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
// .text:000e4240 void CDoDTeamPlay::PlayerKilled(CBasePlayer*, entvars_t*, entvars_t*, int) (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags )
{
	if ( flags < 3 || flags > 6 )
		flags = 0;
	int team = pVictim->pev->team;
	BOOL infinite = ( team == 1 ) ? m_GamePlayRules.m_bAlliesInfiniteLives : m_GamePlayRules.m_bAxisInfiniteLives;
	if ( ( team == 1 || team == 2 ) && infinite )
	{
		if ( ( pVictim->m_flDisplayHistory & 0x10000 ) == 0 )
		{
			pVictim->m_flDisplayHistory |= 0x10000;
			pVictim->HintMessage( "#Hint_player_killed_wavetime", 0 );
		}
	}
	else if ( ( pVictim->m_flDisplayHistory & 0x20000 ) == 0 )
	{
		pVictim->m_flDisplayHistory |= 0x20000;
		pVictim->HintMessage( "#Hint_player_killed_onelife", 0 );
	}
	if ( m_DisableDeathPenalty )
		return;
	CGameRules::PlayerKilled( pVictim, pKiller, pInflictor, flags );
	if ( !m_RoundDoesCount )
		return;
	pVictim->m_iDeaths++;
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreShort );
	WRITE_BYTE( ENTINDEX( pVictim->edict() ) );
	WRITE_SHORT( pVictim->m_iObjScore );
	WRITE_SHORT( (int) pVictim->pev->frags );
	WRITE_SHORT( pVictim->m_iDeaths );
	WRITE_BYTE( 1 );
	MESSAGE_END();
	float tkPenalty = pcvar_mp_tkpenalty ? pcvar_mp_tkpenalty->value : -1.0f;
	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0.0f );
	if ( pVictim->pev == pKiller )
		return;
	CBaseEntity *pEnt = pKiller->pContainingEntity ? (CBaseEntity *) GET_PRIVATE( pKiller->pContainingEntity ) : NULL;
	if ( !pEnt || !pEnt->IsPlayer() )
		return;
	CBasePlayer *pk = (CBasePlayer *) pEnt;
	if ( pVictim->pev->team == pk->pev->team )
	{
		if ( tkPenalty > 0.0f )
			pk->m_iNumTKs++;
		if ( ( pk->m_flDisplayHistory & 0x10 ) == 0 )
		{
			pk->m_flDisplayHistory |= 0x10;
			pk->HintMessage( "#Hint_careful_around_teammates", 1 );
		}
		if ( tkPenalty > 0.0f && (float) pk->m_iNumTKs >= tkPenalty )
		{
			const char *nn = "unconnected";
			if ( pk->pev->netname != 0 && STRING( pk->pev->netname )[0] != 0 )
				nn = STRING( pk->pev->netname );
			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_kick_tk", (char *) nn, 0, 0, 0 );
			UTIL_LogPrintf( "%s<%i> has been auto kicked from the game for TKing\n", nn,
							GETPLAYERUSERID( pk->edict() ) );
			ClientPrint( pk->pev, HUD_PRINTCONSOLE, "#dod_kicked_reason_tk", 0, 0, 0, 0 );
			CLIENT_COMMAND( pk->edict(), "disconnect\n" );
			return;
		}
	}
	else
	{
		pk->m_iNumTKs = 0;
		if ( ( pk->m_flDisplayHistory & 0x20 ) == 0 )
		{
			pk->m_flDisplayHistory |= 0x20;
			pk->HintMessage( "#Hint_killing_enemies_is_good", 0 );
		}
	}
	int pts = IPointsForKill( pk, pVictim );
	pk->AddFrags( pts, 1 );
	FireTargets( "game_playerkill", pk, pk, USE_TOGGLE, 0.0f );
	pk->m_flNextDecalTime = gpGlobals->time;
}
// .text:000e5c80 void CDoDTeamPlay::DeathNotice(CBasePlayer*, entvars_t*, entvars_t*, int) (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::DeathNotice( CBasePlayer *pVictim, entvars_t *pevKiller, entvars_t *pevInflictor, int flags )
{
	CGameRules::DeathNotice( pVictim, pevKiller, pevInflictor, flags );
	const char *vic = GetPlayerClassName( pVictim->pev->playerclass );
	const char *kil = "";
	CBaseEntity *ke =
		pevKiller && pevKiller->pContainingEntity ? (CBaseEntity *) GET_PRIVATE( pevKiller->pContainingEntity ) : NULL;
	if ( ke && ke->IsPlayer() )
		kil = GetPlayerClassName( ( (CBasePlayer *) ke )->pev->playerclass );
	MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
	WRITE_BYTE( ENTINDEX( pVictim->edict() ) );
	WRITE_BYTE( ke ? ENTINDEX( ke->edict() ) : -1 );
	WRITE_STRING( kil );
	WRITE_STRING( vic ? vic : "" );
	WRITE_BYTE( flags );
	MESSAGE_END();
}
// .text:000ee590 void CDoDTeamPlay::RestartRound() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::RestartRound( void )
{
	m_RoundDoesCount = TRUE;
	SetRoundState( 3 );
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
		if ( p->pev->team == 1 )
			allies++;
		else if ( p->pev->team == 2 )
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
		if ( p->pev->team == 1 )
			allies++;
		else if ( p->pev->team == 2 )
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
	SetRoundState( 0 );
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
		if ( p->pev->team == 1 )
		{
			na++;
			if ( p->IsAlive() )
				aa++;
		}
		else if ( p->pev->team == 2 )
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
// .text:000ea7c0 void CDoDTeamPlay::TimerExpired() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::TimerExpired( void )
{
	if ( !m_GamePlayRules.m_bUseTimer )
		return;
	if ( m_GamePlayRules.m_iTimerTeam == 1 )
	{
		SetRoundState( 3 );
		AddToTeamScores( 1, m_GamePlayRules.m_iPointsTimerExpires );
		FireDODCamera( "allies_win_cut", NULL, NULL, USE_TOGGLE, 0.0f );
		FireDODCamera( "axis_loose_cut", NULL, NULL, USE_TOGGLE, 0.0f );
	}
	else if ( m_GamePlayRules.m_iTimerTeam == 2 )
	{
		SetRoundState( 4 );
		AddToTeamScores( 2, m_GamePlayRules.m_iPointsTimerExpires );
		FireDODCamera( "axis_win_cut", NULL, NULL, USE_TOGGLE, 0.0f );
		FireDODCamera( "allies_loose_cut", NULL, NULL, USE_TOGGLE, 0.0f );
	}
	else
	{
		SetRoundState( 5 );
		FireTargets( "para_draw", NULL, NULL, USE_TOGGLE, 0.0f );
		FireDODCamera( "axis_draw_cut", NULL, NULL, USE_TOGGLE, 0.0f );
		FireDODCamera( "allies_draw_cut", NULL, NULL, USE_TOGGLE, 0.0f );
	}
	FireTargets( STRING( m_GamePlayRules.m_sTimerExpireTarget ), NULL, NULL, USE_TOGGLE, 0.0f );
}
// .text:000eaf80 void CDoDTeamPlay::Think() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::Think( void )
{
	g_VoiceGameMgr.Update( gpGlobals->frametime );
	if ( g_fGameOver )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *p = (CBasePlayer *) UTIL_PlayerByIndex( i );
			if ( !p || !p->IsObserver() )
				continue;
			if ( ( p->pev->team == 1 && m_GamePlayRules.m_bAlliesInfiniteLives ) ||
				 ( p->pev->team == 2 && m_GamePlayRules.m_bAxisInfiniteLives ) )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgWaveStatus, NULL, p->pev );
				WRITE_BYTE( 0 );
				MESSAGE_END();
			}
		}
		float ct = pcvar_mp_chattime ? pcvar_mp_chattime->value : 10.0f;
		if ( ct < 10.0f )
			ct = 10.0f;
		if ( ct > 120.0f )
			ct = 120.0f;
		if ( pcvar_mp_chattime )
			CVAR_SET_FLOAT( "mp_chattime", ct );
		m_flIntermissionEndTime = g_flIntermissionStartTime + ct;
		if ( gpGlobals->time <= m_flIntermissionEndTime )
			return;
		if ( !m_iEndIntermissionButtonHit && gpGlobals->time <= g_flIntermissionStartTime + 10.0f )
			return;
		int allies = 0;
		int axis = 0;
		EnumerateTeams( NULL, &allies, NULL, &axis );
		UTIL_LogPrintf( "%s scored %i with %i players\n", "Allies", TeamScores( 1 ), allies );
		UTIL_LogPrintf( "%s scored %i with %i players\n", "Axis", TeamScores( 2 ), axis );
		ChangeLevel();
		return;
	}
	int left = get_maptime_left();
	float lim = pcvar_mp_timelimit ? pcvar_mp_timelimit->value * 60.0f : 0.0f;
	if ( left < 1 && (int) lim != 0 )
	{
		GoToIntermission();
		return;
	}
	if ( m_fLogScoresTime < gpGlobals->time && pcvar_mp_log_scores && pcvar_mp_log_scores->value > 0.0f )
	{
		float d = pcvar_mp_log_scores->value;
		if ( d < 1.0f )
			d = 1.0f;
		if ( d > 3600.0f )
			d = 3600.0f;
		UTIL_LogPrintf( "Team Scores: Allies: %d, Axis: %d\n", TeamScores( 1 ), TeamScores( 2 ) );
		m_fLogScoresTime = gpGlobals->time + d;
	}
	if ( m_RoundState == 1 && ( !m_GamePlayRules.m_bAlliesInfiniteLives || !m_GamePlayRules.m_bAxisInfiniteLives ) )
	{
		m_flRoundTime += gpGlobals->frametime;
		m_RoundParaCanJoin = ( m_flRoundTime <= 30.0f );
	}
	if ( !IsClanMatch() )
	{
		if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value > 0.0f )
		{
			SetClanMatch( TRUE );
			UTIL_ClientPrintAll( HUD_PRINTCENTER, "#clan_warmup_rules", 0, 0, 0, 0 );
		}
	}
	else if ( !pcvar_mp_clan_match || pcvar_mp_clan_match->value > 0.0f )
	{
		if ( !pcvar_mp_clan_restartround || pcvar_mp_clan_restartround->value <= 0.0f )
		{
			if ( !pcvar_mp_clan_readyrestart || pcvar_mp_clan_readyrestart->value <= 0.0f )
			{
				if ( !pcvar_mp_clan_match_warmup || pcvar_mp_clan_match_warmup->value <= 0.0f )
				{
					if ( !m_bAwaitingReadyRestart || !m_bHeardAlliesReady || !m_bHeardAxisReady )
					{
						if ( m_bRoundRestarting && m_flRestartRoundTime < gpGlobals->time )
							StartClanMatch();
					}
					else
					{
						StartClanRestart();
						m_bAwaitingReadyRestart = FALSE;
						m_bHeardAlliesReady = FALSE;
						m_bHeardAxisReady = FALSE;
					}
				}
				else
				{
					UTIL_ClientPrintAll( HUD_PRINTCENTER, "#clan_warmup_rules", 0, 0, 0, 0 );
					ReturnToWarmup();
					CVAR_SET_FLOAT( "mp_clan_match_warmup", 0.0f );
				}
			}
			else
			{
				m_bAwaitingReadyRestart = TRUE;
				m_bHeardAlliesReady = FALSE;
				m_bHeardAxisReady = FALSE;
				UTIL_ClientPrintAll( HUD_PRINTCENTER, "#clan_ready_rules", CVAR_GET_STRING( "mp_clan_ready_signal" ), 0,
									 0, 0 );
				CVAR_SET_FLOAT( "mp_clan_readyrestart", 0.0f );
			}
		}
		else
		{
			StartClanRestart();
			CVAR_SET_FLOAT( "mp_clan_restartround", 0.0f );
		}
	}
	else
	{
		SetClanMatch( FALSE );
		ReturnToWarmup();
	}
	if ( m_RoundState == 0 )
	{
		if ( m_flGoodToGoTime <= gpGlobals->time && m_rReadyToUnFreeze == 0 )
			GoodToGo();
		else if ( m_rRoundStartCount <= gpGlobals->time )
		{
			if ( m_rReadyToUnFreeze == 2 )
				RoundFreeze();
			else if ( m_rReadyToUnFreeze == 1 && m_DoDCameraState == 0 )
				RoundUnfreeze();
		}
	}
	if ( g_fGameOver )
		return;
	if ( m_flAlliesWaveTime != 0.0f && m_RoundState == 1 && (int) m_flAlliesRespawn <= (int) gpGlobals->time )
	{
		RespawnAllies();
		m_flAlliesWaveTime = 0.0f;
		if ( g_fGameOver )
			return;
	}
	if ( m_flAxisWaveTime != 0.0f && m_RoundState == 1 && (int) m_flAxisRespawn <= (int) gpGlobals->time )
	{
		RespawnAxis();
		m_flAxisWaveTime = 0.0f;
	}
	if ( m_flRestartTime <= gpGlobals->time && m_RoundState >= 3 && m_RoundState <= 5 && !g_fGameOver &&
		 m_DoDCameraState < 1 )
	{
		m_SaveRoundState = m_RoundState;
		TeamWon( m_RoundState );
		m_flRestartTime = gpGlobals->time + 10.0f;
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
	SetRoundState( 0 );
	m_flGoodToGoTime = gpGlobals->time + 20.0f;
}
// .text:000e9b80 void CDoDTeamPlay::ChangeLevel() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::ChangeLevel( void )
{
	static char prevFile[64];
	static mapcycle_t cycle;
	int cur = 0;
	const char *cf = CVAR_GET_STRING( "mapcyclefile" );
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		if ( UTIL_PlayerByIndex( i ) )
			cur++;
	if ( stricmp( cf, prevFile ) != 0 )
	{
		strcpy( prevFile, cf );
		DestroyMapCycle( &cycle );
		if ( !ReloadMapCycleFile( cf, &cycle ) || !cycle.items )
			return;
	}
	if ( !cycle.items )
		return;
	char nextMap[32];
	char firstMap[32];
	GetNextMap( cycle, cur, nextMap, firstMap );
	CHANGE_LEVEL( nextMap, NULL );
}
// .text:000e96c0 void CDoDTeamPlay::InitHUD(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::InitHUD( CBasePlayer *pPlayer )
{
	const char *nm = "unconnected";
	if ( pPlayer->pev->netname != 0 && STRING( pPlayer->pev->netname )[0] != 0 )
		nm = STRING( pPlayer->pev->netname );
	UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_joined_game", (char *) nm, 0, 0, 0 );
	SendMOTDToClient( pPlayer->edict() );
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *p = (CBasePlayer *) UTIL_PlayerByIndex( i );
		if ( !p || p == pPlayer )
			continue;
		int big = ( p->m_iObjScore > 255 || p->pev->frags > 255.0f || p->m_iDeaths > 255 );
		MESSAGE_BEGIN( MSG_ONE, big ? gmsgScoreInfoLong : gmsgScoreInfo, NULL, pPlayer->pev );
		WRITE_BYTE( i );
		WRITE_SHORT( p->m_iObjScore );
		WRITE_SHORT( (int) p->pev->frags );
		WRITE_SHORT( p->m_iDeaths );
		MESSAGE_END();
	}
	SendObjectiveStatus( pPlayer );
	SendWaveStatus( pPlayer, 0 );
	SendClientGamerules( pPlayer );
	SendTeamScores( 0 );
}
// .text:000e3de0 void CDoDTeamPlay::ClientDisconnected(edict_t*) (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::ClientDisconnected( edict_t *pClient )
{
	if ( !pClient )
		return;
	CBasePlayer *p = (CBasePlayer *) GET_PRIVATE( pClient );
	if ( !p )
		return;
	MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
	WRITE_BYTE( ENTINDEX( pClient ) );
	WRITE_BYTE( 0 );
	MESSAGE_END();
	FireTargets( "game_playerleave", p, p, USE_TOGGLE, 0.0f );
	const char *team = ( p->pev->team == 1 ) ? "Allies" : "Axis";
	UTIL_LogPrintf( "%s<%i><%s><%s> disconnected\n", STRING( p->pev->netname ), GETPLAYERUSERID( pClient ),
					GETPLAYERAUTHID( pClient ), team );
	p->DropPlayerItem( (char *) "weapon_bazooka", TRUE );
	p->DropPlayerItem( (char *) "weapon_pschreck", TRUE );
	p->DropPlayerItem( (char *) "weapon_piat", TRUE );
	p->RemoveAllItems( TRUE );
}
// .text:000e80e0 void CDoDTeamPlay::RoundUnfreeze() (dod/dlls/dod_gamerules.cpp:6187)
void CDoDTeamPlay::RoundUnfreeze( void )
{
	SetRoundState( 1 );
	m_rReadyToUnFreeze = 0;
	m_bIsRoundFrozen = false;
	CDodRoundTimer *pTimer =
		(CDodRoundTimer *) UTIL_FindEntityByClassname( NULL, "dod_round_timer" );
	while ( pTimer )
	{
		if ( ( pTimer->pev->spawnflags & 2 ) == 0 )
			pTimer->StartTimer();
		pTimer = (CDodRoundTimer *) UTIL_FindEntityByClassname( pTimer, "dod_round_timer" );
	}
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *p = (CBasePlayer *) UTIL_PlayerByIndex( i );
		if ( !p || p->Classify() != 2 || p->pev->team == 3 )
			continue;
		p->CheckPlayerSpeed();
		int r = RANDOM_LONG( 0, 3 );
		int team = p->pev->team;
		const char *msg;
		int bparam1;
		if ( team == 1 )
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
// .text:000e8470 void CDoDTeamPlay::RoundFreeze() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::RoundFreeze( void )
{
	m_RoundState = 0;
	m_rReadyToUnFreeze = 1;
	m_rRoundStartCount = gpGlobals->time + 5.0f;
	UTIL_ClientPrintAll( HUD_PRINTCENTER, "#game_round_freeze", 0, 0, 0, 0 );
}
// .text:000e94f0 void CDoDTeamPlay::SendClientGamerules(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::SendClientGamerules( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( pPlayer ? MSG_ONE : MSG_ALL, gmsgRoundState, NULL, pPlayer ? pPlayer->pev : NULL );
	WRITE_BYTE( m_RoundState );
	WRITE_BYTE( m_GamePlayRules.m_bAlliesInfiniteLives );
	WRITE_BYTE( m_GamePlayRules.m_bAxisInfiniteLives );
	WRITE_BYTE( m_GamePlayRules.m_bAlliesArePara );
	WRITE_BYTE( m_GamePlayRules.m_bAxisArePara );
	MESSAGE_END();
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
	if ( m_iTeam == 1 || m_iTeam == 3 )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
		WRITE_BYTE( 1 );
		WRITE_SHORT( m_iTeamScores[1] );
		MESSAGE_END();
	}
	if ( m_iTeam == 2 || m_iTeam == 3 )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
		WRITE_BYTE( 2 );
		WRITE_SHORT( m_iTeamScores[2] );
		MESSAGE_END();
	}
}
// .text:000e8e90 void CDoDTeamPlay::SendObjectiveStatus(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::SendObjectiveStatus( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgRoundState, NULL, pPlayer->pev );
	WRITE_BYTE( m_RoundState );
	MESSAGE_END();
	CBaseEntity *mst = UTIL_FindEntityByClassname( NULL, "dod_control_point_master" );
	while ( mst )
	{
		if ( ( (CControlPointMaster *) mst )->IsMasteredOn() )
		{
			( (CControlPointMaster *) mst )->SendObjectiveStatus( pPlayer );
			break;
		}
		mst = UTIL_FindEntityByClassname( mst, "dod_control_point_master" );
	}
	CBaseEntity *e = NULL;
	while ( ( e = UTIL_FindEntityByClassname( e, "dod_capture_area" ) ) != NULL )
		e->SendStatus( pPlayer );
	e = NULL;
	while ( ( e = UTIL_FindEntityByClassname( e, "dod_object_goal" ) ) != NULL )
		e->SendStatus( pPlayer );
}
// .text:000e8750 void CDoDTeamPlay::SetNextMap() (dod/dlls/dod_gamerules.cpp:0)
void CDoDTeamPlay::SetNextMap( void )
{
	const char *nx = CVAR_GET_STRING( "mp_nextmap" );
	if ( nx && nx[0] != 0 )
		strcpy( m_szNextMap, nx );
	else
		strcpy( m_szNextMap, STRING( gpGlobals->mapname ) );
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
	SetRoundState( 0 );
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
// .text:000e9130 int CDoDTeamPlay::GetWeaponId(char*) (dod/dlls/dod_gamerules.cpp:0)
int CDoDTeamPlay::GetWeaponId( char *wn )
{
	if ( !wn || !wn[0] )
		return 0;
	for ( int i = 0; i < 32; i++ )
		if ( CBasePlayerItem::ItemInfoArray[i].pszName && !stricmp( CBasePlayerItem::ItemInfoArray[i].pszName, wn ) )
			return i;
	return 0;
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
BOOL CDoDTeamPlay::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	if ( g_VoiceGameMgr.ClientCommand( pPlayer, pcmd ) )
		return TRUE;
	if ( !stricmp( pcmd, "spectate" ) )
	{
		if ( ( pPlayer->pev->flags & FL_SPECTATOR ) == 0 )
		{
			pPlayer->pev->flags |= FL_SPECTATOR;
			pPlayer->m_iNextClass = -1;
			pPlayer->pev->playerclass = 0;
			pPlayer->m_bIsRandomClass = FALSE;
			pPlayer->pev->team = 3;
			UTIL_LogPrintf( "%s<%i><%s><%s> joined team %s\n", STRING( pPlayer->pev->netname ),
							GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ), "SPECTATOR",
							"SPECTATOR" );
			pPlayer->m_iJoiningState = JOINED;
			ChangePlayerTeam( pPlayer, 3, 1, 0 );
			Vector color = g_vecZero;
			pPlayer->UTIL_EdictScreenFade( pPlayer->edict(), color, 1.0f, 15.0f, 0, 5 );
			pPlayer->pev->deadflag = 2;
			pPlayer->pev->effects |= 0x20;
			pPlayer->m_bIsObserver = 1;
			pPlayer->StartObserver( pPlayer->pev->origin, pPlayer->pev->angles );
		}
		return FALSE;
	}
	if ( !stricmp( pcmd, "specmode" ) )
	{
		if ( pPlayer->pev->iuser1 > 0 )
			pPlayer->Observer_SetMode( atoi( CMD_ARGV( 1 ) ) );
		return FALSE;
	}
	if ( !stricmp( pcmd, "follownext" ) )
	{
		if ( pPlayer->pev->iuser1 > 0 )
			pPlayer->Observer_FindNextPlayer( atoi( CMD_ARGV( 1 ) ) != 0 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "spectateperson" ) )
	{
		if ( CMD_ARGC() > 1 )
			pPlayer->Observer_SpectatePlayer( atoi( CMD_ARGV( 1 ) ) );
		return TRUE;
	}
	if ( !stricmp( pcmd, "fullupdate" ) )
	{
		pPlayer->ForceClientDllUpdate();
		return FALSE;
	}
	if ( !stricmp( pcmd, "timeleft" ) )
	{
		if ( CVAR_GET_FLOAT( "timelimit" ) == 0.0f )
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_time_left2", 0, 0, 0, 0 );
		else
		{
			int l = get_maptime_left();
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_time_left1", UTIL_dtos1( l / 60 ), UTIL_dtos2( l % 60 ), 0,
						 0 );
		}
		return FALSE;
	}
	if ( !stricmp( pcmd, "dod_version" ) )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "Version 1.2f", 0, 0, 0, 0 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "chatmsg" ) )
	{
		pPlayer->m_chatMsg = !pPlayer->m_chatMsg;
		return FALSE;
	}
	if ( !stricmp( pcmd, "deathmsg" ) )
	{
		pPlayer->m_deathMsg = !pPlayer->m_deathMsg;
		return FALSE;
	}
	if ( !stricmp( pcmd, "clientcutscenes" ) )
	{
		if ( CMD_ARGC() < 2 )
			return FALSE;
		if ( atoi( CMD_ARGV( 1 ) ) == 0 )
		{
			pPlayer->i_seenAllieSpawnScene = 0;
			pPlayer->i_seenAxisSpawnScene = 0;
			pPlayer->i_seenAllieWinScene = 0;
			pPlayer->i_seenAxisWinScene = 0;
			pPlayer->i_seenAllieLooseScene = 0;
			pPlayer->i_seenAxisLooseScene = 0;
			pPlayer->i_seenAllieDrawScene = 0;
			pPlayer->i_seenAxisDrawScene = 0;
		}
		return FALSE;
	}
	if ( !stricmp( pcmd, "vote" ) )
	{
		if ( (unsigned) ( pPlayer->lastMenuSelected - 5 ) < 5 )
			return TRUE;
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
	if ( !stricmp( pcmd, "fov" ) )
	{
		pPlayer->pev->fov = (float) pPlayer->m_iFOV;
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_dont_cheat", 0, 0, 0, 0 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "say" ) )
	{
		int chatOn = pPlayer->m_chatMsg;
		const char *readyCvar = CVAR_GET_STRING( "mp_clan_ready_signal" );
		char readyString[64];
		snprintf( readyString, sizeof( readyString ), "\"%s\"", readyCvar );
		if ( m_bAwaitingReadyRestart && CMD_ARGC() > 1 )
		{
			const char *text = CMD_ARGS();
			int team = pPlayer->pev->team;
			if ( team == 1 )
			{
				if ( !m_bHeardAlliesReady && strcmp( text, readyString ) == 0 )
				{
					m_bHeardAlliesReady = TRUE;
					UTIL_ShowMessageAll( "#Clan_allies_ready", FALSE );
				}
			}
			else if ( team == 2 && !m_bHeardAxisReady && strcmp( text, readyString ) == 0 )
			{
				m_bHeardAxisReady = TRUE;
				UTIL_ShowMessageAll( "#Clan_axis_ready", FALSE );
			}
		}
		if ( chatOn != 0 )
		{
			Host_Say( pPlayer->edict(), 0 );
			return FALSE;
		}
		return CGameRules::ClientCommand( pPlayer, pcmd );
	}
	if ( !stricmp( pcmd, "say_team" ) )
	{
		if ( pPlayer->m_chatMsg != 0 )
		{
			Host_Say( pPlayer->edict(), 1 );
			return FALSE;
		}
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_chat_off", 0, 0, 0, 0 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "menuselect" ) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;
		int sel = atoi( CMD_ARGV( 1 ) );
		int menu = pPlayer->lastMenuSelected;
		if ( menu == 5 )
		{
			if ( sel == 1 )
				VoteKick( pPlayer, sel, FALSE );
		}
		else if ( menu == 6 )
		{
			if ( sel == 9 )
				pPlayer->VoteMenu( -1, 7 );
			else if ( sel != 10 )
				VoteKick( pPlayer, sel, FALSE );
		}
		else if ( menu == 7 )
		{
			if ( sel == 9 )
				pPlayer->VoteMenu( -1, 8 );
			else if ( sel != 10 )
				VoteKick( pPlayer, sel, FALSE );
		}
		else if ( menu == 8 )
		{
			if ( sel == 9 )
				pPlayer->VoteMenu( -1, 9 );
			else if ( sel != 10 )
				VoteKick( pPlayer, sel, FALSE );
		}
		else if ( menu == 9 )
		{
			if ( sel != 10 && sel != 9 )
				VoteKick( pPlayer, sel, FALSE );
		}
		else if ( (unsigned) ( menu - 1 ) < 4 )
		{
			VoiceCommSelect( pPlayer, sel );
		}
		pPlayer->lastMenuSelected = -1;
		return TRUE;
	}
	if ( !stricmp( pcmd, "jointeam" ) )
	{
		if ( CMD_ARGC() > 1 )
			TeamSelect( pPlayer, atoi( CMD_ARGV( 1 ) ) );
		return TRUE;
	}
	if ( !stricmp( pcmd, "changeclass" ) )
	{
		ShowClassSelectMenu( pPlayer );
		return TRUE;
	}
	if ( !stricmp( pcmd, "changeteam" ) )
	{
		ShowVGUI( pPlayer, 2, 0 );
		return TRUE;
	}
	if ( !strnicmp( pcmd, "cls_", 4 ) )
	{
		int cls = 0;
		while ( cls != 0x1c )
		{
			if ( strcmp( g_ClassInfo[cls].selectcmd, pcmd ) == 0 )
				break;
			cls++;
		}
		if ( cls == 0x1c )
			cls = 0;
		ClassSelect( pPlayer, cls );
		return FALSE;
	}
	if ( !stricmp( pcmd, "nextmap" ) )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_nextmap", m_szNextMap, 0, 0, 0 );
		return TRUE;
	}
	if ( !stricmp( pcmd, "debug" ) || !stricmp( pcmd, "debug2" ) )
		return TRUE;
	if ( pPlayer->pev->deadflag != 0 )
		return FALSE;
	if ( !stricmp( pcmd, "sprone" ) )
	{
		if ( pPlayer->IsSandbagDeployed() )
			return TRUE;
		if ( pPlayer->IsInMortarDeploy() )
			return TRUE;
		if ( ( pPlayer->pev->button & 1 ) != 0 )
			return TRUE;
		if ( pPlayer->pev->movetype == 5 )
			return TRUE;
		BOOL prone = pPlayer->IsProne();
		if ( !prone && pPlayer->pev->waterlevel > 1 && pPlayer->pev->watertype != -0x13 )
			return TRUE;
		if ( pPlayer->m_inextprone < gpGlobals->time )
		{
			int mode = pPlayer->pev->iuser3;
			if ( mode == 1 )
			{
				pPlayer->UnProne();
			}
			else if ( mode == 0 )
			{
				if ( ( pPlayer->pev->button & 4 ) != 0 )
					return TRUE;
				if ( pPlayer->IsDucking() )
					return TRUE;
				if ( ( pPlayer->m_afPhysicsFlags & 8 ) != 0 )
					return TRUE;
				pPlayer->GoProne();
				pPlayer->CheckPlayerSpeed();
				if ( ( pPlayer->m_flDisplayHistory & 0x2000 ) != 0 )
					return FALSE;
				pPlayer->m_flDisplayHistory |= 0x2000;
				pPlayer->HintMessage( "#Hint_prone", 0 );
				return FALSE;
			}
		}
		pPlayer->CheckPlayerSpeed();
		return FALSE;
	}
	if ( !stricmp( pcmd, "use" ) )
	{
		pPlayer->SelectItem( CMD_ARGV( 1 ) );
		return FALSE;
	}
	if ( !strnicmp( pcmd, "weapon_", 7 ) )
	{
		pPlayer->SelectItem( pcmd );
		return FALSE;
	}
	if ( !stricmp( pcmd, "lastinv" ) )
	{
		pPlayer->SelectLastItem();
		return FALSE;
	}
	if ( !stricmp( pcmd, "dropammo" ) || !stricmp( pcmd, "dropmgammo" ) )
	{
		pPlayer->DropGenericAmmo();
		return FALSE;
	}
	if ( !stricmp( pcmd, "drop" ) )
	{
		pPlayer->DropPlayerItem( (char *) CMD_ARGV( 1 ), FALSE );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_sticktogether" ) )
	{
		pPlayer->HandSignal( 0 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_fallback" ) )
	{
		pPlayer->HandSignal( 1 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_no" ) )
	{
		pPlayer->HandSignal( 2 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_yes" ) )
	{
		pPlayer->HandSignal( 3 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_sniper" ) )
	{
		pPlayer->HandSignal( 4 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_backup" ) )
	{
		pPlayer->HandSignal( 5 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_enemyright" ) )
	{
		pPlayer->HandSignal( 6 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_enemyleft" ) )
	{
		pPlayer->HandSignal( 7 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_grenade" ) )
	{
		pPlayer->HandSignal( 8 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_flankleft" ) )
	{
		pPlayer->HandSignal( 9 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_moveout" ) )
	{
		pPlayer->HandSignal( 10 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_flankright" ) )
	{
		pPlayer->HandSignal( 0xb );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_areaclear" ) )
	{
		pPlayer->HandSignal( 0xc );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_coveringfire" ) )
	{
		pPlayer->HandSignal( 0xd );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_holdposition" ) )
	{
		pPlayer->HandSignal( 0xf );
		return FALSE;
	}
	if ( !stricmp( pcmd, "signal_enemyspotted" ) )
	{
		pPlayer->HandSignal( 0x11 );
		return FALSE;
	}
	if ( !strncmp( pcmd, "dod_is_oned", 11 ) )
	{
		int num = atoi( pcmd + 0xc );
		if ( (unsigned) ( num - 2 ) < 4 )
			pPlayer->HandSignal( num + 0x18 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "voice_menu1" ) )
	{
		pPlayer->VoiceMenu( 1 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "voice_menu2" ) )
	{
		pPlayer->VoiceMenu( 2 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "voice_menu3" ) )
	{
		pPlayer->VoiceMenu( 3 );
		return FALSE;
	}
	if ( !stricmp( pcmd, "dropobject" ) )
	{
		if ( pPlayer->HasObject != 0 )
			pPlayer->DropObject();
		return TRUE;
	}
	if ( !stricmp( pcmd, "chattoplayer" ) )
	{
		int idx = atoi( CMD_ARGV( 1 ) );
		CBaseEntity *plr = UTIL_PlayerByIndex( idx );
		if ( plr )
		{
			char text[201];
			sprintf( text, "%c%s: ", 1, STRING( pPlayer->pev->netname ) );
			for ( int i = 2; i != CMD_ARGC() + 1; i++ )
			{
				char text2[50];
				sprintf( text2, "%s ", CMD_ARGV( i ) );
				strcat( text, text2 );
			}
			ClientPrint( plr->pev, HUD_PRINTTALK, text, 0, 0, 0, 0 );
		}
		return TRUE;
	}
	if ( !stricmp( pcmd, "firemarker" ) )
	{
		pPlayer->FireMapMarker();
		return FALSE;
	}
	if ( !stricmp( pcmd, "nextmarker" ) )
	{
		pPlayer->NextMapMarker();
		return FALSE;
	}
	if ( !stricmp( pcmd, "prevmarker" ) )
	{
		pPlayer->PrevMapMarker();
		return FALSE;
	}
	return CGameRules::ClientCommand( pPlayer, pcmd );
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
	if ( iTeam == 1 )
	{
		dt = m_flAlliesRespawn - gpGlobals->time;
		return dt > 0.0f ? dt : 0.0f;
	}
	if ( iTeam == 2 )
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
	if ( iTeam == 1 )
	{
		if ( !m_GamePlayRules.m_bAlliesInfiniteLives )
			return;
		m_flAlliesRespawn = gpGlobals->time + fTime;
		m_flAlliesWaveTime = fTime;
	}
	else if ( iTeam == 2 )
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
// .text:000e72d0 int CDoDTeamPlay::GetClassLimit(int) (dod/dlls/dod_gamerules.cpp:2521)
int CDoDTeamPlay::GetClassLimit( int cls )
{
	char name[32];
	sprintf( name, "mp_classlimit_%d", cls );
	float v = CVAR_GET_FLOAT( name );
	if ( cls < 0 || cls >= 29 )
		return -1;
	return (int)v;
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
	if ( team == 1 )
		return m_GamePlayRules.m_bAlliesArePara;
	if ( team == 2 )
		return m_GamePlayRules.m_bAxisArePara;
	return FALSE;
}
// .text:000ea630 int CDoDTeamPlay::IsTeamInfinite(int) (dod/dlls/dod_gamerules.cpp:5580)
BOOL CDoDTeamPlay::IsTeamInfinite( int team )
{
	if ( team == 1 )
		return m_GamePlayRules.m_bAlliesInfiniteLives;
	if ( team == 2 )
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
// .text:000e5360 BOOL CDoDTeamPlay::IsAllowedToSpawn(CBaseEntity*) (dod/dlls/dod_gamerules.cpp:4638)
BOOL CDoDTeamPlay::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	const char *cn = STRING( pEntity->pev->classname );
	if ( !strcmp( cn, "weaponbox" ) )
		return FALSE;
	return TRUE;
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
// .text:000e7100 void CDoDTeamPlay::PreparePlayerForSpawn(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:2483)
void CDoDTeamPlay::PreparePlayerForSpawn( CBasePlayer *pPlayer )
{
	pPlayer->m_iNumTKs = 0;
	pPlayer->m_flNextDecalTime = 0;
	pPlayer->RemoveAllWeaponFlags();
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
// .text:000e72f0 int CDoDTeamPlay::ReachedClassLimit(int) (dod/dlls/dod_gamerules.cpp:2534)
BOOL CDoDTeamPlay::ReachedClassLimit( int cls )
{
	int lim = GetClassLimit( cls );
	if ( lim < 0 )
		return FALSE;
	return CountClasses( cls ) >= lim;
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
// .text:000e7450 void CDoDTeamPlay::ChooseRandomClass(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:3447)
void CDoDTeamPlay::ChooseRandomClass( CBasePlayer *pPlayer )
{
	int cls = ( rand() % 10 ) + 1;
	ClassSelect( pPlayer, cls );
}
// .text:000e7f40 void CDoDTeamPlay::UpdateData(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:4338)
void CDoDTeamPlay::UpdateData( CBasePlayer *pPlayer )
{
	SendTeamScores( 0 );
	SendObjectiveStatus( pPlayer );
}
// .text:000e86a0 void CDoDTeamPlay::LevelChangeResets(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:4612)
void CDoDTeamPlay::LevelChangeResets( CBasePlayer *pPlayer )
{
	pPlayer->m_iDeaths = 0;
	pPlayer->pev->frags = 0;
	pPlayer->m_iObjScore = 0;
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
// .text:000eb830 int CDoDTeamPlay::GetClassMask(int) (dod/dlls/dod_gamerules.cpp:6259)
int CDoDTeamPlay::GetClassMask( int team )
{
	if ( team == 1 )
		return (int)CVAR_GET_FLOAT( "mp_alliesclasses" );
	if ( team == 2 )
		return (int)CVAR_GET_FLOAT( "mp_axisclasses" );
	return 0;
}
// .text:000eb920 void CDoDTeamPlay::ShowClassSelectMenu(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:6331)
void CDoDTeamPlay::ShowClassSelectMenu( CBasePlayer *pPlayer )
{
	char cmd[64];
	sprintf( cmd, "classmenu %d\n", pPlayer->pev->team );
	CLIENT_COMMAND( pPlayer->edict(), cmd );
}
// .text:000edfd0 BOOL CDoDTeamPlay::AllowSpawnableBazookas() (dod/dlls/dod_gamerules.cpp:6368)
BOOL CDoDTeamPlay::AllowSpawnableBazookas( void )
{
	return (int)CVAR_GET_FLOAT( "mp_spawnbazookas" ) != 0;
}
// NOTE: CDoDDetect ctor lives in objective.h zone (not our files) - see binary 000ee020 sz=132
class CMultiplayGameMgrHelper
{
public:
	virtual BOOL CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker );
};
// .text:000eeac0 BOOL CMultiplayGameMgrHelper::CanPlayerHearPlayer(CBasePlayer*,CBasePlayer*) (dod/dlls/dod_gamerules.cpp:149)
BOOL CMultiplayGameMgrHelper::CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker )
{
	if ( pListener->pev->team != pTalker->pev->team )
		return FALSE;
	return TRUE;
}
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
	BOOL bIsAlly = ( team == 1 );
	int iVar10;
	DodClassInfo_t *pClassInfo = NULL;

	if ( iClassSelect == 0x1b )
	{
		if ( pPlayer->m_iNextClass == -1 && pPlayer->m_imissedwave != 0 && m_RoundState == 1 )
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
		else if ( team != 2 )
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
		if ( pPlayer->m_imissedwave != 0 && m_RoundState == 1 )
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
	if ( !infinite && m_RoundState == 1 )
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
	if ( ( pAmmo->pev->spawnflags & 0x40000000 ) != 0 )
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
	if ( ( pWeapon->pev->spawnflags & 0x40000000 ) != 0 )
		return 2;
	return 2 - ( ( pWeapon->pev->flags & 0x80000 ) == 0 );
}
// .text:000e5050 float CDoDTeamPlay::FlWeaponRespawnTime(CBasePlayerItem*) (dod/dlls/dod_gamerules.cpp:0)
float CDoDTeamPlay::FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	if ( ( CBasePlayerItem::ItemInfoArray[pWeapon->m_iId].iFlags & 0x200 ) == 0 )
	{
		if ( cvar_weaponstay.value > 0.0f &&
			 ( CBasePlayerItem::ItemInfoArray[pWeapon->m_iId].iFlags & 8 ) == 0 )
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
	if ( pWeapon->m_iId != 0 && ( CBasePlayerItem::ItemInfoArray[pWeapon->m_iId].iFlags & 8 ) != 0 &&
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
	if ( team == 1 )
		infinite = m_GamePlayRules.m_bAlliesInfiniteLives;
	else if ( team == 2 )
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
// .text:000eb7f0 void CDoDTeamPlay::ReturnToWarmup() (dod/dlls/dod_gamerules.cpp:6244)
void CDoDTeamPlay::ReturnToWarmup( void )
{
	if ( m_bClanMatch )
	{
		m_bClanMatchActive = FALSE;
		SetClanMatch( FALSE );
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

	if ( RoundState() != 1 )
	{
		pPlayer->StartObserver( pPlayer->pev->origin, pPlayer->pev->angles );
		return;
	}

	pPlayer->m_bIsObserver = TRUE;
	int team = pPlayer->pev->team;
	BOOL infinite;
	if ( team == 1 )
		infinite = m_GamePlayRules.m_bAlliesInfiniteLives;
	else if ( team == 2 )
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
		if ( p && !p->has_disconnected && p->pev->team == 1 && !p->IsAlive() )
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
	UTIL_SendDirectorMessage( ent1, NULL, 0x82 );
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
		if ( p && !p->has_disconnected && p->pev->team == 2 && !p->IsAlive() )
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

// .text:000e7560 void CDoDTeamPlay::PlayerSpawn(CBasePlayer*) (dod/dlls/dod_gamerules.cpp:3499)
void CDoDTeamPlay::PlayerSpawn( CBasePlayer *pPlayer )
{
	if ( pPlayer->has_disconnected || pPlayer->pev->iuser1 != 0 || pPlayer->pev->team == 3 ||
		 pPlayer->pev->team == 0 || pPlayer->m_iNextClass == 0 )
		return;

	int entindex = ENTINDEX( pPlayer->edict() );

	if ( pPlayer->m_bIsRandomClass )
	{
		ChooseRandomClass( pPlayer );
		DodClassInfo_t *pInfo;
		if ( (unsigned) ( pPlayer->m_iNextClass - 10 ) < 0xb )
			pInfo = m_GamePlayRules.m_bAxisArePara ? g_ParaClassInfo : g_ClassInfo;
		else
			pInfo = m_GamePlayRules.m_bAlliesArePara ? g_ParaClassInfo : g_ClassInfo;
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_now_as", pInfo[pPlayer->m_iNextClass].classname, 0, 0, 0 );
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
		if ( pPlayer->pev->team == 1 )
			pPlayer->m_sPlayerModel = (char *) "us-inf";
		else if ( pPlayer->pev->team == 2 )
			pPlayer->m_sPlayerModel = (char *) "axis-inf";

		if ( (unsigned) ( cls - 1 ) < 0x1b )
		{
			DodClassInfo_t *pInfo;
			if ( pPlayer->pev->team == 1 )
				pInfo = m_GamePlayRules.m_bAlliesArePara ? g_ParaClassInfo : g_ClassInfo;
			else
				pInfo = ( pPlayer->pev->team == 2 && m_GamePlayRules.m_bAxisArePara ) ? g_ParaClassInfo
																					 : g_ClassInfo;
			DodClassInfo_t *pClass = &pInfo[cls];
			pPlayer->m_sPlayerModel = pClass->modelname;

			for ( int i = 0; i < 0x20; i++ )
				if ( ( pClass->weapons >> ( i & 31 ) ) & 1 )
					pPlayer->GiveNamedItem( weaponnames[i] );
			for ( int i = 0; i < 0x20; i++ )
				if ( ( pClass->weapons2 >> ( i & 31 ) ) & 1 )
					pPlayer->GiveNamedItem( weaponnames[i + 0x20] );

			int team = pPlayer->pev->team;
			if ( team == 1 )
			{
				for ( int i = 0; i < pClass->numgrenades; i++ )
					pPlayer->GiveNamedItem( "weapon_handgrenade" );
			}
			else if ( team == 2 )
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
		SET_CLIENT_KEYVALUE( entindex, GET_INFOKEYBUFFER( pPlayer->edict() ), "model", pPlayer->m_sPlayerModel );
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
				if ( p->pev->team == 1 )
					iAllies++;
				else if ( p->pev->team == 2 )
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
				if ( p->pev->team == 1 )
					iAllies++;
				else if ( p->pev->team == 2 )
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
		if ( pPlayer->pev->team != 3 )
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
			pPlayer->pev->deadflag = 2;
			pPlayer->pev->effects |= 0x20;
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
			float lVar13 = ( m_RoundState == 1 ) ? tempTime : 0.0f;
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

// .text:000d8900 CDoDTeamPlay::CDoDTeamPlay (file:dod_gamerules.cpp)
CDoDTeamPlay::CDoDTeamPlay( void )
{
	// this codebase has no CMultiplayGameMgrHelper; voice is inited elsewhere.
	(void)0;
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

	if ( !( *g_engfuncs.pfnIsDedicatedServer )() )
	{
		const char *lservercfgfile = ( *g_engfuncs.pfnCVarGetString )( "lservercfgfile" );
		if ( lservercfgfile != NULL && *lservercfgfile != '\0' )
		{
			char szCommand[256];
			sprintf( szCommand, "exec %s\n", lservercfgfile );
			SERVER_COMMAND( szCommand );
		}
	}
	{
		char szCommand[256];
		sprintf( szCommand, "exec %s.cfg\n", STRING( gpGlobals->mapname ) );
		SERVER_COMMAND( szCommand );
	}

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

	float fLogDelay = 30.0f;
	if ( pcvar_mp_log_scores_delay != NULL )
	{
		float v = pcvar_mp_log_scores_delay->value;
		if ( v >= 1.0f )
			fLogDelay = ( v > 120.0f ) ? 120.0f : v;
	}
	m_fLogScoresTime = gpGlobals->time + fLogDelay;
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

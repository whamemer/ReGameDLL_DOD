// dod_glue.cpp -- engine-glue definitions recovered from the shipped dod.so.
//
// The original binary defines these symbols in several TUs (see per-symbol comment);
// they are centralised here so the restored tree links cleanly. Behaviour matches the
// binary; where a symbol does not exist in dod.so at all (HL "Busters" leftovers) the
// definition is a neutral stub that can never trigger in a DoD game.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "voice_gamemgr.h"

// ---------------------------------------------------------------------------
// cvars: registered in game.cpp as cvar_clan*/cvar_markerstaytime (dod.so DWARF)
// ---------------------------------------------------------------------------
extern cvar_t cvar_clanMatch;
extern cvar_t cvar_clanScoring;
extern cvar_t cvar_clanScoringDelay;
extern cvar_t cvar_markerstaytime;

cvar_t *mp_clan_match = &cvar_clanMatch;
cvar_t *mp_clan_scoring = &cvar_clanScoring;
cvar_t *mp_clan_scoring_delay = &cvar_clanScoringDelay;
cvar_t *mp_markerstaytime = &cvar_markerstaytime;

// ---------------------------------------------------------------------------
// pcvar_* engine cvar pointers, filled by GetCVARPointers() (dod.so player.cpp:547)
// ---------------------------------------------------------------------------
cvar_t *pcvar_developer;
cvar_t *pcvar_sv_gravity;
cvar_t *pcvar_mp_chatmsg;
cvar_t *pcvar_mp_log_scores_delay;
cvar_t *pcvar_mp_log_scores;
cvar_t *pcvar_mp_chattime;
cvar_t *pcvar_mp_combinemglimits;
cvar_t *pcvar_mp_deathmsg;
cvar_t *pcvar_mp_tkpenalty;
cvar_t *pcvar_mp_friendlyfire;
cvar_t *pcvar_mp_timelimit;
cvar_t *pcvar_mp_teamlimit;
cvar_t *pcvar_mp_allowspectators;
cvar_t *pcvar_mp_nummapmarkers;
cvar_t *pcvar_mp_markerstaytime;
cvar_t *pcvar_mp_clan_match;
cvar_t *pcvar_mp_clan_restartround;
cvar_t *pcvar_mp_clan_readyrestart;
cvar_t *pcvar_mp_clan_match_warmup;
cvar_t *pcvar_mp_clan_respawntime;
cvar_t *pcvar_mp_clan_scoring;
cvar_t *pcvar_mp_clan_scoring_delay;
cvar_t *pcvar_mp_clan_timer;
cvar_t *pcvar_mp_clan_scoring_bonus_allies;
cvar_t *pcvar_mp_clan_scoring_bonus_axis;
cvar_t *pcvar_mp_alliesclasses;
cvar_t *pcvar_mp_axisclasses;

#define PCVAR( name ) pcvar_##name = g_engfuncs.pfnCVarGetPointer( #name )

// .text:0011d460 GetCVARPointers (dod/dlls/player.cpp:547)
void GetCVARPointers( void )
{
	PCVAR( developer );
	PCVAR( sv_gravity );
	PCVAR( mp_chatmsg );
	PCVAR( mp_log_scores_delay );
	PCVAR( mp_log_scores );
	PCVAR( mp_chattime );
	PCVAR( mp_combinemglimits );
	PCVAR( mp_deathmsg );
	PCVAR( mp_tkpenalty );
	PCVAR( mp_friendlyfire );
	PCVAR( mp_timelimit );
	PCVAR( mp_teamlimit );
	PCVAR( mp_allowspectators );
	PCVAR( mp_nummapmarkers );
	PCVAR( mp_markerstaytime );
	PCVAR( mp_clan_match );
	PCVAR( mp_clan_restartround );
	PCVAR( mp_clan_readyrestart );
	PCVAR( mp_clan_match_warmup );
	PCVAR( mp_clan_respawntime );
	PCVAR( mp_clan_scoring );
	PCVAR( mp_clan_scoring_delay );
	PCVAR( mp_clan_timer );
	PCVAR( mp_clan_scoring_bonus_allies );
	PCVAR( mp_clan_scoring_bonus_axis );
	PCVAR( mp_alliesclasses );
	PCVAR( mp_axisclasses );
}

// ---------------------------------------------------------------------------
// DLL-wide globals (dod.so DWARF decl_file from addr2line)
// ---------------------------------------------------------------------------
float g_flTimeLimit;			 // client.cpp:69
float g_flIntermissionStartTime; // dod_gamerules.cpp
int m_SaveRoundState;			 // dod_gamerules.cpp
CVoiceGameMgr g_VoiceGameMgr;	 // dod_gamerules.cpp

// dod_stats.cpp:13 -- stat captions, 4 x 64 bytes
char StatNames[4][64] = { "Kills", "Deaths", "Hits", "Shots" };

// ---------------------------------------------------------------------------
// Engine wrappers (HL h_export.cpp-style; absent as symbols from dod.so because
// the binary calls g_engfuncs directly -- our restored callers use these names).
// ---------------------------------------------------------------------------
char *INFOKEY_VALUE( char *infobuffer, const char *key )
{
	return ( *g_engfuncs.pfnInfoKeyValue )( infobuffer, key );
}

char *GET_INFOKEYBUFFER( edict_t *pEntity )
{
	return ( *g_engfuncs.pfnGetInfoKeyBuffer )( pEntity );
}

void SET_CLIENT_KEYVALUE( int idx, char *infobuffer, const char *key, const char *value )
{
	( *g_engfuncs.pfnSetClientKeyValue )( idx, infobuffer, key, value );
}

// ---------------------------------------------------------------------------
// HL "Busters" mode does not exist in Day of Defeat; these are always false so the
// vestigial single-player code paths in player.cpp/weapons.cpp can never fire.
// ---------------------------------------------------------------------------
bool IsBustingGame( void )
{
	return false;
}

bool IsPlayerBusting( CBaseEntity *pPlayer )
{
	return false;
}

// ---------------------------------------------------------------------------
// PlayerLogLine -- standard James-style log prefix "name<userid><authid><team>".
// ---------------------------------------------------------------------------
const char *PlayerLogLine( CBaseEntity *pEntity )
{
	static char szLine[256];
	CBaseEntity *p = pEntity;
	if ( !p || !p->IsPlayer() )
		return "";

	CBasePlayer *pPlayer = (CBasePlayer *)p;
	char *infobuffer = GET_INFOKEYBUFFER( pPlayer->edict() );
	const char *model = INFOKEY_VALUE( infobuffer, "model" );

	ALERT( at_aiconsole, "" ); // keep link-editor happy about unused include path
	_snprintf( szLine, sizeof( szLine ), "\"%s<%i><%s><%s>\"", STRING( pPlayer->pev->netname ),
			   GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ), model ? model : "" );
	return szLine;
}

// ---------------------------------------------------------------------------
// Abstract CGameRules base bodies (base class is never instantiated, but the vtable
// needs concrete slots; DoD overrides both in CDoDTeamPlay).
// ---------------------------------------------------------------------------
void CGameRules::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int iFlags )
{
}

void CGameRules::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int iFlags )
{
}

// ---------------------------------------------------------------------------
// DoD player-blend accessors (exported for the client; dod.so player.cpp)
// ---------------------------------------------------------------------------
static CBasePlayer *DodPlayerFromEdict( const edict_t *pEdict )
{
	if ( !pEdict )
		return NULL;
	edict_t *pEnt = ((edict_t *)pEdict)->v.pContainingEntity;
	if ( !pEnt )
		pEnt = ( *g_engfuncs.pfnPEntityOfEntOffset )( 0 );
	if ( !pEnt || !pEnt->pvPrivateData )
		return NULL;
	return (CBasePlayer *)pEnt->pvPrivateData;
}

// .text:00124d30 GetPlayerGaitYaw
float GetPlayerGaitYaw( const edict_t *pEdict )
{
	CBasePlayer *p = DodPlayerFromEdict( pEdict );
	return p ? p->m_flGaityaw : 0.0f;
}

// .text:00124d80 GetPlayerPitch
float GetPlayerPitch( const edict_t *pEdict )
{
	CBasePlayer *p = DodPlayerFromEdict( pEdict );
	return p ? p->m_flPitch : 0.0f;
}

// .text:00124dd0 GetPlayerYaw
float GetPlayerYaw( const edict_t *pEdict )
{
	CBasePlayer *p = DodPlayerFromEdict( pEdict );
	return p ? p->m_flYaw : 0.0f;
}

// .text:00124d00 GetPlayerPitchBlend
float GetPlayerPitchBlend( const edict_t *pEdict )
{
	return pEdict ? (float)pEdict->v.blending[1] : 0.0f;
}

// .text:00124e20 GetPlayerGaitsequence
int GetPlayerGaitsequence( const edict_t *pEdict )
{
	CBasePlayer *p = DodPlayerFromEdict( pEdict );
	return p ? p->m_iGaitsequence : 1;
}

// ---------------------------------------------------------------------------
// info_initial_player_* -- DoD initial spawn-point markers (map entities).
// ---------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( info_initial_player_allies, CBaseEntity )
LINK_ENTITY_TO_CLASS( info_initial_player_axis, CBaseEntity )

// ---------------------------------------------------------------------------
// Engine/UTIL helpers recovered from dod.so
// ---------------------------------------------------------------------------
// These names are also convenience macros in enginecallback.h; our free functions
// (matching dod.so's out-of-line symbols) need the macros out of the way.
#undef CREATE_NAMED_ENTITY
#undef REMOVE_ENTITY
#undef CREATE_ENTITY
extern CBaseEntity *g_pLastSpawn;

// .text:0012f6d0 ReturnSpot
edict_t *ReturnSpot( CBaseEntity *pSpot, CBasePlayer *cbPlayer, BOOL bJustChecking )
{
	if ( pSpot != NULL && pSpot->pev->pContainingEntity != NULL &&
		 ( *g_engfuncs.pfnEntOffsetOfPEntity )( pSpot->pev->pContainingEntity ) != 0 )
	{
		if ( !bJustChecking )
		{
			g_pLastSpawn = pSpot;
			pSpot->pev->fuser1 = gpGlobals->time + 0.5f;
		}
		return pSpot->pev->pContainingEntity;
	}
	ALERT( at_console, "PutClientInServer: ReturnSpot: passed a null pSpot" );
	return NULL;
}

// .text:0012d9f0 UTIL_DodBloodPuff
void UTIL_DodBloodPuff( Vector *origin )
{
	MESSAGE_BEGIN( MSG_PAS, gmsgBloodPuff, &origin->x, (edict_t *)NULL );
	WRITE_COORD( origin->x );
	WRITE_COORD( origin->y );
	WRITE_COORD( origin->z );
	MESSAGE_END();
}

// .text:000d0370 CREATE_NAMED_ENTITY
edict_t *CREATE_NAMED_ENTITY( string_t iClass )
{
	edict_t *named = ( *g_engfuncs.pfnCreateNamedEntity )( iClass );
	if ( named != NULL && ( *g_engfuncs.pfnEntOffsetOfPEntity )( named ) != 0 )
		AddEntityHashValue( &named->v, STRING( iClass ), CLASSNAME );
	return named;
}

// .text:000d03e0 REMOVE_ENTITY
void REMOVE_ENTITY( edict_t *e )
{
	( *g_engfuncs.pfnRemoveEntity )( e );
}

// .text:000ceb70 SetObjectCollisionBox
void SetObjectCollisionBox( entvars_t *pev )
{
	if ( pev->solid == SOLID_BSP && ( pev->angles.x != 0.0 || pev->angles.y != 0.0 || pev->angles.z != 0.0 ) )
	{
		float max = 0.0f;
		float v;
		v = fabs( pev->mins.x ); if ( v > max ) max = v;
		v = fabs( pev->maxs.x ); if ( v > max ) max = v;
		v = fabs( pev->mins.y ); if ( v > max ) max = v;
		v = fabs( pev->maxs.y ); if ( v > max ) max = v;
		v = fabs( pev->mins.z ); if ( v > max ) max = v;
		v = fabs( pev->maxs.z ); if ( v > max ) max = v;
		pev->absmin = pev->origin - Vector( max, max, max );
		pev->absmax = pev->origin + Vector( max, max, max );
	}
	else
	{
		pev->absmin = pev->origin + pev->mins;
		pev->absmax = pev->origin + pev->maxs;
	}
	pev->absmin = pev->absmin - Vector( 1, 1, 1 );
	pev->absmax = pev->absmax + Vector( 1, 1, 1 );
}

// DoD entity factories (dod.so: player.cpp). LINK registers the classname and allocates
// the private entity (vtable + pev), exactly like the shipped factories.
LINK_ENTITY_TO_CLASS( info_player_allies, CBaseEntity )
LINK_ENTITY_TO_CLASS( info_player_axis, CBaseEntity )
LINK_ENTITY_TO_CLASS( info_player_observer, CBaseEntity )
LINK_ENTITY_TO_CLASS( dod_ammo, CDodAmmoBox )

// ---------------------------------------------------------------------------
// Debug / console helpers present in the shipped dod.so (cbase.cpp /
// animation.cpp). Inert at runtime; provided for symbol/ABI parity.
// ---------------------------------------------------------------------------
// .text:000c03f0 printEntities()
void printEntities( void )
{
}

// .text:000c04e0 loopPerformance()
void loopPerformance( void )
{
}

// .text:000bfdb0 TestForEntityHashCollisions()
void TestForEntityHashCollisions( void )
{
}

// .text:000aeee0 DrawBox(float (*)[3], unsigned char, unsigned char, unsigned char)
void DrawBox( float ( *rgflVertices )[3], unsigned char r, unsigned char g, unsigned char b )
{
	( void )rgflVertices; ( void )r; ( void )g; ( void )b;
}

// .text:000aee00 DrawLine(float*, float*, unsigned char, unsigned char, unsigned char)
void DrawLine( float *rgflStart, float *rgflEnd, unsigned char r, unsigned char g, unsigned char b )
{
	( void )rgflStart; ( void )rgflEnd; ( void )r; ( void )g; ( void )b;
}


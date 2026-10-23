/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== player.cpp ========================================================

  functions dealing with the player

*/

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#include "trains.h"
#include "nodes.h"
#include "weapons.h"
#include "soundent.h"
#include "monsters.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "game.h"
#include "pm_shared.h"
#include "hltv.h"

#include "objective.h"
#include "mapmarker.h"
#include "dod_shared.h"
#include "combat.h"

extern DodClassInfo_t g_ClassInfo[];
extern DodClassInfo_t g_ParaClassInfo[];

void UTIL_EdictScreenFadeBuild( ScreenFade &fade, const Vector &color, float fadeTime, float fadeHold, int alpha,
								int flags );
void UTIL_EdictScreenFadeWrite( const ScreenFade &fade, edict_t *pEntity );
void UTIL_DodBloodPuff( Vector &origin );
extern cvar_t *pcvar_mp_friendlyfire;
extern cvar_t *pcvar_mp_nummapmarkers;

// #define DUCKFIX

extern DLL_GLOBAL ULONG g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL g_fGameOver;
extern DLL_GLOBAL BOOL g_fDrawLines;
int gEvilImpulse101;
extern DLL_GLOBAL int g_iSkillLevel, gDisplayTitle;

BOOL gInitHUD = TRUE;

extern void respawn( entvars_t *pev, BOOL fCopyCorpse );
extern Vector VecBModelOrigin( entvars_t *pevBModel );
extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer, BOOL bJustChecking );

extern cvar_t cvar_fadetoblack;

// the world node graph
extern CGraph WorldGraph;

#define TRAIN_ACTIVE 0x80
#define TRAIN_NEW 0xc0
#define TRAIN_OFF 0x00
#define TRAIN_NEUTRAL 0x01
#define TRAIN_SLOW 0x02
#define TRAIN_MEDIUM 0x03
#define TRAIN_FAST 0x04
#define TRAIN_BACK 0x05

#define FLASH_DRAIN_TIME 1.2   //100 units/3 minutes
#define FLASH_CHARGE_TIME 0.2  // 100 units/20 seconds  (seconds per unit)

// Global Savedata for player (EXACT: dod.so CBasePlayer::m_playerSaveData @0x1e0c00, 22 entries)
TYPEDESCRIPTION CBasePlayer::m_playerSaveData[] = {
	DEFINE_FIELD( CBasePlayer, m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonReleased, FIELD_INTEGER ),
	DEFINE_ARRAY( CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS ),
	DEFINE_FIELD( CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME ),
	DEFINE_ARRAY( CBasePlayer, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES ),
	DEFINE_FIELD( CBasePlayer, m_pActiveItem, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayer, m_pLastItem, FIELD_CLASSPTR ),
	DEFINE_ARRAY( CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS ),
	DEFINE_FIELD( CBasePlayer, m_idrowndmg, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_idrownrestored, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iTrain, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flFallVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iTargetVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_fInitHUD, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_pTank, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_iHideHUD, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iFOV, FIELD_INTEGER ),
};

int giPrecacheGrunt = 0;
int gmsgShake = 0;
int gmsgFade = 0;
int gmsgSelAmmo = 0;
int gmsgFlashlight = 0;
int gmsgFlashBattery = 0;
int gmsgResetHUD = 0;
int gmsgInitHUD = 0;
int gmsgShowGameTitle = 0;
int gmsgCurWeapon = 0;
int gmsgHealth = 0;
int gmsgDamage = 0;
int gmsgBattery = 0;
int gmsgTrain = 0;
int gmsgLogo = 0;
int gmsgWeaponList = 0;
int gmsgAmmoX = 0;
int gmsgHudText = 0;
int gmsgVGUIMenu = 0; // DoD VGUIMenu (dod.so BSS 0x2161c4)
int gmsgDeathMsg = 0;
int gmsgScoreInfo = 0;
int gmsgTeamInfo = 0;
int gmsgTeamScore = 0;
int gmsgGameMode = 0;
int gmsgMOTD = 0;
int gmsgServerName = 0;
int gmsgAmmoPickup = 0;
int gmsgWeapPickup = 0;
int gmsgItemPickup = 0;
int gmsgHideWeapon = 0;
int gmsgSetCurWeap = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
int gmsgSetFOV = 0;
int gmsgShowMenu = 0;
int gmsgGeigerRange = 0;
int gmsgTeamNames = 0;

int gmsgStatusText = 0;
int gmsgStatusValue = 0;
int gmsgScope = 0;
int gmsgFrags = 0;
int gmsgObjScore = 0;
int gmsgObject = 0;
int gmsgClientAreas = 0;
int gmsgYouDied = 0;
int gmsgBloodPuff = 0;
int gmsgResetSens = 0;
int gmsgPStatus = 0;
int gmsgPClass = 0;
int gmsgPTeam = 0;
int gmsgPShoot = 0;
int gmsgMapMarker = 0;
int gmsgCurMarker = 0;
int gmsgHandSignal = 0;
int gmsgSendCorpse = 0;
int gmsgWeather = 0;
int gmsgCapMsg = 0;
int gmsgHLTV = 0;
int gmsgSpectator = 0;
int gmsgScoreShort = 0;
int gmsgScoreInfoLong = 0;
int gmsgGameRules = 0;
int gmsgInitObjDisplay = 0;
int gmsgSetObjOwner = 0;
int gmsgStartProgressBar = 0;
int gmsgStartProgressBarF = 0;
int gmsgCancelProgressBar = 0;
int gmsgProgBarUpdate = 0;
int gmsgPlayersInArea = 0;
int gmsgTimerStatus = 0;
int gmsgClanTimer = 0;
int gmsgTimeLeft = 0;
int gmsgRoundState = 0;
int gmsgWaveStatus = 0;
int gmsgWaveTime = 0;
int gmsgCameraView = 0;
int gmsgUseSound = 0;
int gmsgReloadDone = 0;
int gmsgAmmoShort = 0;

extern const char *g_PlayerAnimationNames[34];
extern const char *sIdleAnimations[5];
extern char *sHandSignals[30];	// defined in dod_shared.cpp
extern const char *g_PlayerAnimationList[88];


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
cvar_t *pcvar_mp_spawnbazookas;
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

void LinkUserMessages( void )
{
	gmsgCurWeapon = REG_USER_MSG( "CurWeapon", 3 );
	gmsgHealth = REG_USER_MSG( "Health", 1 );
	gmsgHudText = REG_USER_MSG( "HudText", -1 );
	gmsgSayText = REG_USER_MSG( "SayText", -1 );
	gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );
	gmsgWeaponList = REG_USER_MSG( "WeaponList", -1 );
	gmsgResetHUD = REG_USER_MSG( "ResetHUD", 0 );
	gmsgInitHUD = REG_USER_MSG( "InitHUD", 0 );
	gmsgDeathMsg = REG_USER_MSG( "DeathMsg", -1 );
	gmsgScoreInfo = REG_USER_MSG( "ScoreInfo", 7 );
	gmsgScoreInfoLong = REG_USER_MSG( "ScoreInfoLong", 10 );
	gmsgGameRules = REG_USER_MSG( "GameRules", 2 );
	gmsgResetSens = REG_USER_MSG( "ResetSens", 0 );
	gmsgTeamScore = REG_USER_MSG( "TeamScore", 3 );
	gmsgMOTD = REG_USER_MSG( "MOTD", -1 );
	gmsgServerName = REG_USER_MSG( "ServerName", -1 );
	gmsgAmmoPickup = REG_USER_MSG( "AmmoPickup", 2 );
	gmsgWeapPickup = REG_USER_MSG( "WeapPickup", 1 );
	gmsgHideWeapon = REG_USER_MSG( "HideWeapon", 1 );
	gmsgSetFOV = REG_USER_MSG( "SetFOV", 1 );
	gmsgShake = REG_USER_MSG( "ScreenShake", 6 );
	gmsgFade = REG_USER_MSG( "ScreenFade", 10 );
	gmsgAmmoX = REG_USER_MSG( "AmmoX", 2 );
	gmsgAmmoShort = REG_USER_MSG( "AmmoShort", 3 );
	gmsgBloodPuff = REG_USER_MSG( "BloodPuff", 6 );
	gmsgVGUIMenu = REG_USER_MSG( "VGUIMenu", 5 );
	gmsgSpectator = REG_USER_MSG( "Spectator", 2 );
	gmsgWaveTime = REG_USER_MSG( "WaveTime", 1 );
	gmsgWaveStatus = REG_USER_MSG( "WaveStatus", 1 );
	gmsgScope = REG_USER_MSG( "Scope", 1 );
	gmsgObject = REG_USER_MSG( "Object", -1 );
	gmsgCameraView = REG_USER_MSG( "CameraView", -1 );
	gmsgInitObjDisplay = REG_USER_MSG( "InitObj", -1 );
	gmsgSetObjOwner = REG_USER_MSG( "SetObj", 3 );
	gmsgStartProgressBar = REG_USER_MSG( "StartProg", 4 );
	gmsgStartProgressBarF = REG_USER_MSG( "StartProgF", 4 );
	gmsgProgBarUpdate = REG_USER_MSG( "ProgUpdate", 3 );
	gmsgCancelProgressBar = REG_USER_MSG( "CancelProg", 2 );
	gmsgTimerStatus = REG_USER_MSG( "TimerStatus", 3 );
	gmsgHLTV = REG_USER_MSG( "HLTV", 2 );
	gmsgClientAreas = REG_USER_MSG( "ClientAreas", -1 );
	gmsgPlayersInArea = REG_USER_MSG( "PlayersIn", 4 );
	gmsgStatusValue = REG_USER_MSG( "StatusValue", 1 );
	gmsgClanTimer = REG_USER_MSG( "ClanTimer", 1 );
	gmsgMapMarker = REG_USER_MSG( "MapMarker", 6 );
	gmsgShowMenu = REG_USER_MSG( "ShowMenu", -1 );
	gmsgFrags = REG_USER_MSG( "Frags", 3 );
	gmsgPStatus = REG_USER_MSG( "PStatus", 2 );
	gmsgScoreShort = REG_USER_MSG( "ScoreShort", 8 );
	gmsgPClass = REG_USER_MSG( "PClass", 2 );
	gmsgPTeam = REG_USER_MSG( "PTeam", 2 );
	gmsgYouDied = REG_USER_MSG( "YouDied", 1 );
	gmsgPShoot = REG_USER_MSG( "PShoot", -1 );
	gmsgWeather = REG_USER_MSG( "Weather", 2 );
	gmsgObjScore = REG_USER_MSG( "ObjScore", 3 );
	gmsgReloadDone = REG_USER_MSG( "ReloadDone", 0 );
	gmsgRoundState = REG_USER_MSG( "RoundState", 1 );
	gmsgUseSound = REG_USER_MSG( "UseSound", 1 );
	gmsgCapMsg = REG_USER_MSG( "CapMsg", -1 );
	gmsgHandSignal = REG_USER_MSG( "HandSignal", 2 );
	gmsgCurMarker = REG_USER_MSG( "CurMarker", 1 );
	gmsgSendCorpse = REG_USER_MSG( "ClCorpse", -1 );
	gmsgTimeLeft = REG_USER_MSG( "TimeLeft", 2 );
}


LINK_ENTITY_TO_CLASS( player, CBasePlayer );

/* 
 *
 */
Vector VecVelocityForDamage( float flDamage )
{
	Vector vec( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );

	if ( flDamage > -50 )
		vec = vec * 0.7;
	else if ( flDamage > -200 )
		vec = vec * 2;
	else
		vec = vec * 10;

	return vec;
}

#if 0 /*
static void ThrowGib(entvars_t *pev, char *szGibModel, float flDamage)
{
	edict_t *pentNew = CREATE_ENTITY();
	entvars_t *pevNew = VARS(pentNew);

	pevNew->origin = pev->origin;
	SET_MODEL(ENT(pevNew), szGibModel);
	UTIL_SetSize(pevNew, g_vecZero, g_vecZero);

	pevNew->velocity		= VecVelocityForDamage(flDamage);
	pevNew->movetype		= MOVETYPE_BOUNCE;
	pevNew->solid			= SOLID_NOT;
	pevNew->avelocity.x		= RANDOM_FLOAT(0,600);
	pevNew->avelocity.y		= RANDOM_FLOAT(0,600);
	pevNew->avelocity.z		= RANDOM_FLOAT(0,600);
	CHANGE_METHOD(ENT(pevNew), em_think, SUB_Remove);
	pevNew->ltime		= gpGlobals->time;
	pevNew->nextthink	= gpGlobals->time + RANDOM_FLOAT(10,20);
	pevNew->frame		= 0;
	pevNew->flags		= 0;
}
	
	
static void ThrowHead(entvars_t *pev, char *szGibModel, floatflDamage)
{
	SET_MODEL(ENT(pev), szGibModel);
	pev->frame			= 0;
	pev->nextthink		= -1;
	pev->movetype		= MOVETYPE_BOUNCE;
	pev->takedamage		= DAMAGE_NO;
	pev->solid			= SOLID_NOT;
	pev->view_ofs		= Vector(0,0,8);
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,56));
	pev->velocity		= VecVelocityForDamage(flDamage);
	pev->avelocity		= RANDOM_FLOAT(-1,1) * Vector(0,600,0);
	pev->origin.z -= 24;
	ClearBits(pev->flags, FL_ONGROUND);
}


*/
#endif

int TrainSpeed( int iSpeed, int iMax )
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float) iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed / fMax;

	if ( iSpeed < 0 )
		iRet = TRAIN_BACK;
	else if ( iSpeed == 0 )
		iRet = TRAIN_NEUTRAL;
	else if ( fSpeed < 0.33 )
		iRet = TRAIN_SLOW;
	else if ( fSpeed < 0.66 )
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

// DoD DeathSound - PORTED from restore (dod.so). See restore/ NOTES.
void CBasePlayer ::DeathSound( void )
{
	int i = RANDOM_LONG( 1, 4 );
	const char *sample = 0;
	if ( i == 1 )
		sample = "player/dmbullo1.wav";
	else if ( i == 2 )
		sample = "player/dmbullo2.wav";
	else if ( i == 3 )
		sample = "player/dmbullo3.wav";
	else
		return;
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, sample, 1.0, ATTN_NORM, 0, 100 );
}

// override takehealth
// bitsDamageType indicates type of damage healed.

int CBasePlayer ::TakeHealth( float flHealth, int bitsDamageType )
{
	return CBaseMonster ::TakeHealth( flHealth, bitsDamageType );
}

Vector CBasePlayer ::GetGunPosition()
{
	//	UTIL_MakeVectors(pev->v_angle);
	//	m_HackedGunPos = pev->view_ofs;
	Vector origin;

	origin = pev->origin + pev->view_ofs;

	return origin;
}

//=========================================================
// TraceAttack
//=========================================================
// DoD TraceAttack - PORTED from restore (dod.so). See restore/ NOTES.
void CBasePlayer ::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr,
								int bitsDamageType )
{
	float flDot;
	int fBehind;
	CBaseEntity *pAttacker = CBaseEntity::Instance( pevAttacker );
	float ff = 0;

	if ( pcvar_mp_friendlyfire )
		ff = pcvar_mp_friendlyfire->value;

	if ( ff <= 0 && pAttacker->pev->team == pev->team && !( bitsDamageType & DMG_DOD_SELFSPLASH ) )
		return;

	if ( this == pAttacker && ( bitsDamageType & DMG_BULLET ) )
		return;

	if ( pev->takedamage == DAMAGE_NO )
		return;

	m_LastHitGroup = ptr->iHitgroup;

	MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
		WRITE_BYTE( m_iClientHealth );
	MESSAGE_END();

	if ( bitsDamageType & DMG_CLUB )
	{
		UTIL_MakeVectors( pev->angles );

		flDot = DotProduct( g_vecAttackDir * -1, gpGlobals->v_forward );
		fBehind = flDot > 0.3;

		if ( fBehind )
			flDamage = 200;
	}

	if ( pAttacker->pev->team != pev->team || ff > 0 )
	{
		m_bSlowedByHit = true;
		m_flUnslowTime = gpGlobals->time + 0.5;
	}

	int hitgroup = ptr->iHitgroup;
	char *szHitbox;
	float originalDmg = flDamage;

	if ( bitsDamageType & DMG_SONIC )
		flDamage *= 1.5;

	switch ( hitgroup )
	{
	case HITGROUP_GENERIC:
		szHitbox = "(error - hit generic)";
		break;

	case HITGROUP_HEAD:
		flDamage *= 2.5;

		if ( IsAlive() && ( pAttacker->pev->team != pev->team || ff == 1 ) )
		{
			UTIL_EdictScreenFade( edict(), Vector( 128, 0, 0 ), 3.0f, 2.0f, 200, FFADE_IN );

			PLAYBACK_EVENT_FULL( 0, edict(), g_pGameRules->bodyDamage(), 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0,
								 0, 0 );
		}

		UTIL_SendDirectorMessage( this, pAttacker, 5 );

		if ( pAttacker->pev->team != pev->team || ff == 1 )
		{
			Vector dir = vecDir * 100;

			dir.z = dir.z + 100;

			PopHelmet( &dir );
		}

		szHitbox = "head";
		break;

	case HITGROUP_CHEST:
		szHitbox = "chest";
		break;

	case HITGROUP_STOMACH:
		szHitbox = "stomach";
		break;

	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
		flDamage *= 0.75;
		szHitbox = "arm";
		break;

	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		if ( pAttacker->pev->team != pev->team || ff == 1 )
		{
			m_bSlowedByHit = true;
			m_flUnslowTime = gpGlobals->time + 2.0;
		}

		flDamage *= 0.75;
		szHitbox = "leg";
		break;

	default:
		szHitbox = "(error - hit default)";
		break;
	}

	ALERT( at_aiconsole, "%s hit %s in the %s hitgroup for %f damage ( %f base damage )\n",
		   STRING( pAttacker->pev->netname ), STRING( pev->netname ), szHitbox, flDamage, originalDmg );

	UTIL_DodBloodPuff( ptr->vecEndPos );

	if ( bitsDamageType & DMG_BLAST )
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
	else
		TakeDamage( pevAttacker, pevAttacker, flDamage, bitsDamageType );
}

/*
	Take some damage.  
	NOTE: each call to TakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to TakeDamage using DMG_GENERIC.
*/

#define ARMOR_RATIO 0.2	 // Armor Takes 80% of the damage
#define ARMOR_BONUS 0.5	 // Each Point of Armor is work 1/x points of health

// DoD TakeDamage - PORTED from restore (dod.so). See restore/ NOTES.

int CBasePlayer ::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( !IsAlive() )
		return 0;

	if ( bitsDamageType & DMG_DOD_NOBLOOD )
		return 0;

	if ( pev->health <= 0 )
		return 0;

	if ( bitsDamageType & DMG_BLAST )
		g_pGameRules->IsMultiplayer();

	CBasePlayer *pAttacker = (CBasePlayer *) CBaseEntity::Instance( pevAttacker );

	if ( !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker ) )
		return 0;

	if ( pAttacker && pAttacker->IsPlayer() )
	{
		if ( pAttacker != this && pAttacker->pev->team == pev->team )
		{
			if ( !( pAttacker->m_flDisplayHistory & HINT_INJURED_TEAMMATE ) )
			{
				pAttacker->m_flDisplayHistory |= HINT_INJURED_TEAMMATE;
				pAttacker->HintMessage( "#Hint_try_not_to_injure_teammates", 0 );
			}
		}
	}

	PLAYBACK_EVENT_FULL( 0, edict(), g_pGameRules->Pain(), 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 1, 0, 0, 0 );

	int fTookDamage = CBaseMonster::TakeDamage( pevInflictor, pevAttacker, (int) flDamage, bitsDamageType );

	CBaseEntity *pInflictor = NULL;

	if ( !FNullEnt( pevInflictor ) )
		pInflictor = CBaseEntity::Instance( pevInflictor );

	UTIL_SendDirectorMessage( this, pInflictor, 4 );

	m_bitsDamageType = bitsDamageType;

	MESSAGE_BEGIN( MSG_SPEC, gmsgHLTV );
		WRITE_BYTE( ENTINDEX( edict() ) );
		WRITE_BYTE( pev->health > 0 ? (int) pev->health | 128 : 128 );
	MESSAGE_END();

	return fTookDamage;
}


//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerItems( void )
{
	int iWeaponRules;
	int iAmmoRules;
	int i;
	CBasePlayerWeapon *rgpPackWeapons[20];	// 20 hardcoded for now. How to determine exactly how many weapons we have?
	int iPackAmmo[MAX_AMMO_SLOTS + 1];
	int iPW = 0;  // index into packweapons array
	int iPA = 0;  // index into packammo array

	memset( rgpPackWeapons, 0, sizeof( rgpPackWeapons ) );
	memset( iPackAmmo, -1, sizeof( iPackAmmo ) );

	// get the game rules
	iWeaponRules = g_pGameRules->DeadPlayerWeapons( this );
	iAmmoRules = g_pGameRules->DeadPlayerAmmo( this );

	if ( iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO )
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllItems( TRUE );
		return;
	}

	// go through all of the weapons and make a list of the ones to pack
	for ( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if ( m_rgpPlayerItems[i] )
		{
			// there's a weapon here. Should I pack it?
			CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[i];

			while ( pPlayerItem )
			{
				switch ( iWeaponRules )
				{
				case GR_PLR_DROP_GUN_ACTIVE:
					if ( m_pActiveItem && pPlayerItem == m_pActiveItem )
					{
						// this is the active item. Pack it.
						rgpPackWeapons[iPW++] = (CBasePlayerWeapon *)pPlayerItem;
					}
					break;

				case GR_PLR_DROP_GUN_ALL:
					rgpPackWeapons[iPW++] = (CBasePlayerWeapon *)pPlayerItem;
					break;

				default:
					break;
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

	// now go through ammo and make a list of which types to pack.
	if ( iAmmoRules != GR_PLR_DROP_AMMO_NO )
	{
		for ( i = 0; i < MAX_AMMO_SLOTS; i++ )
		{
			if ( m_rgAmmo[i] > 0 )
			{
				// player has some ammo of this type.
				switch ( iAmmoRules )
				{
				case GR_PLR_DROP_AMMO_ALL:
					iPackAmmo[iPA++] = i;
					break;

				case GR_PLR_DROP_AMMO_ACTIVE:
					if ( m_pActiveItem && i == m_pActiveItem->PrimaryAmmoIndex() )
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					else if ( m_pActiveItem && i == m_pActiveItem->SecondaryAmmoIndex() )
					{
						// this is the secondary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					break;

				default:
					break;
				}
			}
		}
	}

	// create a box to pack the stuff into.
	CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create( "weaponbox", pev->origin, pev->angles, edict() );

	pWeaponBox->pev->angles.x = 0;	// don't let weaponbox tilt.
	pWeaponBox->pev->angles.z = 0;

	pWeaponBox->SetThink( &CWeaponBox::Kill );
	pWeaponBox->pev->nextthink = gpGlobals->time + 120;

	// back these two lists up to their first elements
	iPA = 0;
	iPW = 0;

	// pack the ammo
	while ( iPackAmmo[iPA] != -1 )
	{
		pWeaponBox->PackAmmo( MAKE_STRING( CBasePlayerItem::AmmoInfoArray[iPackAmmo[iPA]].pszName ), m_rgAmmo[iPackAmmo[iPA]] );
		iPA++;
	}

	// now pack all of the items in the lists
	while ( rgpPackWeapons[iPW] )
	{
		// weapon unhooked from the player. Pack it into der box.
		pWeaponBox->PackWeapon( rgpPackWeapons[iPW] );

		iPW++;
	}

	pWeaponBox->pev->velocity = pev->velocity * 1.2;  // weaponbox has player's velocity, then some.

	RemoveAllItems( TRUE );	 // now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::RemoveAllItems( BOOL removeSuit )
{
	if ( m_pActiveItem )
	{
		m_pActiveItem->Holster();
	}

	m_pLastItem = NULL;

	int i;
	CBasePlayerItem *pPendingItem;
	for ( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		m_pActiveItem = m_rgpPlayerItems[i];
		while ( m_pActiveItem )
		{
			pPendingItem = m_pActiveItem->m_pNext;
			m_pActiveItem->Drop();
			m_pActiveItem = pPendingItem;
		}
		m_rgpPlayerItems[i] = NULL;
	}
	m_pActiveItem = NULL;

	pev->viewmodel = 0;
	pev->weaponmodel = 0;

	pev->weapons = 0;
	m_iWeapons2 = 0;

	for ( i = 0; i < MAX_AMMO_SLOTS; i++ )
		m_rgAmmo[i] = 0;

	UpdateClientData();
	// send Selected Weapon Message to our client
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
	WRITE_BYTE( 0 );
	WRITE_BYTE( 0 );
	WRITE_BYTE( 0 );
	MESSAGE_END();
}

/*
 * GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
 *
 * ENTITY_METHOD(PlayerDie)
 */
entvars_t *g_pevLastInflictor;	// Set in combat.cpp.  Used to pass the damage inflictor for death messages.
								// Better solution:  Add as parameter to all Killed() functions.

// DoD Killed - PORTED from restore (dod.so). See restore/ NOTES.

void CBasePlayer::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( m_pActiveItem )
	{
		m_pActiveItem->DropGren();
		m_pActiveItem->Holster();
	}

	DropItemByClass( CLASS_DROPPED_ON_DEATH );

	RemoveAllItems( 1 );
	if ( pevAttacker->pContainingEntity == NULL )
		pevAttacker = VARS( INDEXENT( 0 ) );
	g_pGameRules->PlayerKilled( this, pevAttacker, g_pevLastInflictor, iGib );
	if ( lastMenuSelected > 0 )
	{
		lastMenuSelected = -1;
		MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, pev );
		WRITE_SHORT( 0 );
		WRITE_CHAR( 0 );
		WRITE_BYTE( 0 );
		MESSAGE_END();
	}
	if ( m_pTank )
	{
		m_pTank->Use( this, this, USE_OFF, 0 );
		m_pTank = NULL;
	}
	CSound *pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
	if ( pSound )
		pSound->Reset();
	UTIL_MakeVectors( pev->angles );
	// a blast throws the player back or forward, from the side it is a normal death
	BOOL bThrown = FALSE;

	if ( m_bitsDamageType & DMG_BLAST )
	{
		float flDot = -DotProduct( g_vecAttackDir, gpGlobals->v_forward );

		if ( flDot <= -0.3f )
		{
			SetAnimation( PLAYER_EXPLODE_BACK );
			bThrown = TRUE;
		}
		else if ( flDot > 0.3f )
		{
			SetAnimation( PLAYER_EXPLODE_FORWARD );
			bThrown = TRUE;
		}
	}

	if ( bThrown )
		UTIL_SendDirectorMessage( this, NULL, 7 | DRC_FLAG_SLOWMOTION );
	else
		SetAnimation( PLAYER_DIE );

	m_iRespawnFrames = 0;
	pev->modelindex = g_ulModelIndexPlayer;
	pev->deadflag = DEAD_DYING;
	pev->movetype = MOVETYPE_TOSS;
	pev->flags &= ~FL_ONGROUND;
	if ( cvar_fadetoblack.value != 0.0f && g_pGameRules->IsClanMatch() && g_pGameRules->IsClanMatchActive() )
	{
		UTIL_ScreenFade( this, Vector( 0, 0, 0 ), 3.0, 3.0, 0xff, 5 );
	}
	else
	{
		pev->iuser1 = OBS_IN_EYE;
		pev->iuser2 = ENTINDEX( edict() );
		pev->iuser3 = ENTINDEX( ENT( pevAttacker ) );
		m_hObserverTarget = UTIL_PlayerByIndex( pev->iuser3 );
	}
	MESSAGE_BEGIN( MSG_ONE, gmsgYouDied, NULL, pev );
	WRITE_BYTE( 0 );
	MESSAGE_END();
	m_iClientFOV = 0;
	m_iFOV = 0;
	( (CDoDTeamPlay *) g_pGameRules )->CheckOneLifeRoundConditions();
	pev->vuser1.x = SANDBAG_NONE;
	pev->vuser1.y = 0;
	SetThink( &CBasePlayer::PlayerDeathThink );
	pev->nextthink = gpGlobals->time + 0.1;
}


// Set the activity based on an event or current state

void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;
	float speed;
	char szAnim[64];
	char szHS[60];
	bool bTimeToDeepIdle = false;
	bool bIdle = false;
	int iState;

	if ( gpGlobals->time > m_flIdleTimer )
	{
		bTimeToDeepIdle = true;
		m_flIdleTimer = RANDOM_LONG( 20, 30 ) + gpGlobals->time;
	}

	speed = pev->velocity.Length2D();

	BOOL bProne = !pev->deadflag && STANCE_IS_PRONE( pev->iuser3 );

	if ( pev->flags & FL_FROZEN )
	{
		speed = 0;
		bIdle = true;
	}
	else
	{
		switch ( playerAnim )
		{
		case PLAYER_IDLE:
		case PLAYER_WALK:
			bIdle = true;
			break;

		case PLAYER_JUMP:
			m_IdealActivity = ACT_HOP;
			break;

		case PLAYER_SUPERJUMP:
			m_IdealActivity = ACT_LEAP;
			break;

		case PLAYER_DIE:
			m_IdealActivity = GetDeathActivity();
			break;

		case PLAYER_ATTACK1:
			m_IdealActivity = SetIdealActivity( ACT_RANGE_ATTACK1 );
			break;

		case PLAYER_PRONE1:
			m_IdealActivity = DOD_PRONE_GETDOWN;
			break;

		case PLAYER_PRONE2:
			m_IdealActivity = DOD_PRONE_IDLE;
			break;

		case PLAYER_PRONE3:
			m_IdealActivity = DOD_PRONE_GETUP;
			break;

		case PLAYER_RELOAD:
			m_IdealActivity = SetIdealActivity( ACT_RELOAD );
			break;

		case PLAYER_SIGNAL:
			m_IdealActivity = SetIdealActivity( ACT_SIGNAL1 );
			break;

		case PLAYER_ATTACK2:
			m_IdealActivity = SetIdealActivity( ACT_RANGE_ATTACK2 );
			break;

		case PLAYER_ROLLGRENADE:
			m_IdealActivity = SetIdealActivity( ACT_SPECIAL_ATTACK1 );
			break;

		case PLAYER_EXPLODE_BACK:
			m_IdealActivity = DOD_EXPLOSION_BACK;
			break;

		case PLAYER_EXPLODE_FORWARD:
			m_IdealActivity = DOD_EXPLOSION_FORWARD;
			break;

		case PLAYER_BUT_SWING:
			m_IdealActivity = SetIdealActivity( ACT_SPECIAL_ATTACK2 );
			break;

		default:
			break;
		}
	}

	if ( bIdle )
	{
		// stay in the air animation until the player lands
		if ( ( pev->flags & FL_ONGROUND ) || (unsigned int) ( m_Activity - ACT_HOP ) > 1 )
		{
			if ( pev->waterlevel <= 1 || pev->watertype == -19 || pev->movetype == MOVETYPE_FLY )
			{
				if ( pev->deadflag || !STANCE_IS_PRONE( pev->iuser3 ) )
					m_IdealActivity = ACT_WALK;
				else
					m_IdealActivity = DOD_PRONE_IDLE;
			}
			else
			{
				m_IdealActivity = speed < 20.0 ? ACT_HOVER : ACT_SWIM;
			}
		}
		else
		{
			m_IdealActivity = m_Activity;
		}
	}

	if ( !bProne && !ShouldGunLower() && m_IdealActivity == ACT_WALK && speed < 250.0 &&
		 ( m_Activity == ACT_RANGE_ATTACK1 || !m_fSequenceFinished ) )
	{
		animDesired = pev->sequence;
	}
	else
	{
		switch ( m_IdealActivity )
		{
		case ACT_WALK:
		case DOD_PRONE_IDLE:
		case DOD_PRONE_CRAWL:
		{
			Activity cur = m_Activity;

			if ( ( (unsigned int) ( cur - ACT_RANGE_ATTACK1 ) > 1 && cur != ACT_SPECIAL_ATTACK1 && cur != ACT_RELOAD &&
				   cur != ACT_SIGNAL1 && cur != DOD_PRONE_GETUP && cur != DOD_PRONE_GETDOWN ) ||
				 m_fSequenceFinished || m_ianimupdate == 1 )
			{
				bool bKeepSequence = false;

				m_ianimupdate = 0;

				if ( m_bBazookaDeployed )
				{
					if ( !bProne )
						iState = ( pev->flags & FL_DUCKING ) ? 8 : 7;
					else
						iState = 9;
				}
				else if ( pev->iuser3 == STANCE_PRONE_DEPLOYED )
				{
					iState = 5;
				}
				else if ( bProne )
				{
					iState = 4;

					if ( m_iFOV <= 0 && speed > 0.0 )
						iState = 19;
				}
				else
				{
					iState = 6;

					if ( pev->vuser1.x != SANDBAG_DEPLOYED )
					{
						if ( ( pev->button & IN_RUN ) && speed > 250.0 )
						{
							iState = 2;
						}
						else if ( ( pev->flags & FL_DUCKING ) && !m_iFOV )
						{
							iState = 3;
						}
						else if ( m_iCurrentAnimationState == 20 && !m_fSequenceFinished )
						{
							bKeepSequence = true;
						}
						else if ( m_iFOV <= 0 )
						{
							iState = 1;

							if ( speed == 0.0 && bTimeToDeepIdle )
								iState = pev->deadflag == DEAD_NO ? 20 : 1;
						}
						else
						{
							iState = ( pev->flags & FL_DUCKING ) ? 8 : 7;
						}
					}
				}

				if ( bKeepSequence )
				{
					animDesired = pev->sequence;
				}
				else
				{
					animDesired = m_iCurrentAnimationSequence;

					if ( m_iCurrentAnimationState != iState || animDesired == 0 || m_pszSavedWeaponModel != pev->weaponmodel )
					{
						sprintf( szAnim, g_PlayerAnimationNames[iState], m_szAnimExtention );

						m_pszSavedWeaponModel = pev->weaponmodel - (unsigned int) (size_t) gpGlobals->pStringBase;

						if ( iState == 20 )
							animDesired = LookupSequence( sIdleAnimations[RANDOM_LONG( 1, 4 )] );
						else
							animDesired = LookupSequence( szAnim );

						m_iCurrentAnimationSequence = animDesired;
						m_iCurrentAnimationState = iState;
					}

					if ( animDesired == -1 )
						animDesired = 0;

					m_Activity = ACT_WALK;
				}
			}
			else
			{
				animDesired = pev->sequence;
			}

			break;
		}

		case ACT_SIGNAL1:
		{
			if ( pev->deadflag || !STANCE_IS_PRONE( pev->iuser3 ) )
				strcpy( szAnim, "hs_" );
			else
				strcpy( szAnim, "hs_prone_" );

			const char *pszSignal = sHandSignals[m_handSignal];

			animDesired = pev->sequence;

			if ( memcmp( pszSignal, "dod_idle", 8 ) || !bProne )
			{
				if ( !memcmp( pszSignal, "dod_idle", 8 ) )
					szAnim[0] = 0;

				strcpy( szHS, pszSignal );
				strcat( szAnim, szHS );

				animDesired = LookupSequence( szAnim );

				if ( animDesired == -1 )
					animDesired = 0;

				if ( pev->sequence != animDesired || !m_fSequenceLoops )
				{
					pev->frame = 0;

					if ( !m_fSequenceLoops )
						pev->effects |= EF_NOINTERP;
				}

				m_Activity = m_IdealActivity;
				pev->sequence = animDesired;

				ResetSequenceInfo();
			}

			break;
		}

		case ACT_RANGE_ATTACK1:
		case ACT_RANGE_ATTACK2:
		case ACT_SPECIAL_ATTACK1:
		case ACT_SPECIAL_ATTACK2:
		{
			switch ( m_IdealActivity )
			{
			case ACT_RANGE_ATTACK1:
				if ( ( pev->flags & FL_DUCKING ) && !bProne )
				{
					iState = 12;
				}
				else if ( pev->iuser3 == STANCE_PRONE_DEPLOYED )
				{
					iState = 14;
				}
				else if ( !bProne )
				{
					iState = 15;

					if ( pev->vuser1.x != SANDBAG_DEPLOYED )
					{
						iState = 10;

						if ( pev->button & IN_RUN )
							iState = ( speed > 0.0 ) + 10;
					}
				}
				else
				{
					iState = 13;
				}

				break;

			case ACT_SPECIAL_ATTACK1:
				if ( !bProne )
					iState = ( pev->flags & FL_DUCKING ) ? 27 : 26;
				else
					iState = 13;

				break;

			case ACT_SPECIAL_ATTACK2:
				if ( !bProne )
					iState = ( pev->flags & FL_DUCKING ) ? 32 : 31;
				else
					iState = 33;

				break;

			default:
				if ( !bProne )
					iState = ( pev->flags & FL_DUCKING ) ? 17 : 16;
				else
					iState = 18;

				break;
			}

			if ( !m_bBazookaDeployed )
				m_fGunDropTime = gpGlobals->time + 2.0;

			animDesired = m_iCurrentAnimationSequence;

			if ( m_iCurrentAnimationState != iState || animDesired == 0 )
			{
				sprintf( szAnim, g_PlayerAnimationNames[iState], m_szAnimExtention );

				animDesired = LookupSequence( szAnim );

				m_iCurrentAnimationState = iState;
				m_iCurrentAnimationSequence = animDesired;
			}

			if ( animDesired == -1 )
				animDesired = 0;

			if ( pev->sequence != animDesired || !m_fSequenceLoops )
			{
				pev->frame = 0;

				if ( !m_fSequenceLoops )
					pev->effects |= EF_NOINTERP;
			}

			m_Activity = m_IdealActivity;
			pev->sequence = animDesired;

			ResetSequenceInfo();
			break;
		}

		case ACT_RELOAD:
			if ( m_bBazookaDeployed )
			{
				if ( !bProne )
					iState = ( pev->flags & FL_DUCKING ) ? 29 : 28;
				else
					iState = 9;
			}
			else
			{
				iState = 25;

				if ( pev->vuser1.x != SANDBAG_DEPLOYED )
				{
					iState = 24;

					if ( pev->iuser3 != STANCE_PRONE_DEPLOYED )
					{
						if ( !bProne )
							iState = ( pev->flags & FL_DUCKING ) ? 22 : 21;
						else
							iState = 23;
					}
				}
			}

			animDesired = m_iCurrentAnimationSequence;

			if ( m_iCurrentAnimationState != iState || animDesired == 0 )
			{
				sprintf( szAnim, g_PlayerAnimationNames[iState], m_szAnimReloadExt );

				animDesired = LookupSequence( szAnim );

				m_iCurrentAnimationState = iState;
				m_iCurrentAnimationSequence = animDesired;
			}

			if ( animDesired == -1 )
				animDesired = 0;

			if ( pev->sequence != animDesired || !m_fSequenceLoops )
			{
				pev->frame = 0;

				if ( !m_fSequenceLoops )
					pev->effects |= EF_NOINTERP;
			}

			m_Activity = m_IdealActivity;
			pev->sequence = animDesired;

			ResetSequenceInfo();
			break;

		case DOD_PRONE_GETUP:
			if ( m_Activity == DOD_PRONE_GETUP && m_fSequenceFinished )
				return;

			animDesired = LookupSequence( "get_up" );

			if ( pev->sequence != animDesired )
			{
				m_Activity = m_IdealActivity;
				pev->frame = 0;
				pev->gaitsequence = 0;
				pev->sequence = animDesired;

				ResetSequenceInfo();
			}

			return;

		case DOD_PRONE_GETDOWN:
			if ( m_Activity == DOD_PRONE_GETDOWN && m_fSequenceFinished )
				return;

			animDesired = LookupSequence( "get_down" );

			if ( pev->sequence != animDesired )
			{
				m_Activity = m_IdealActivity;
				pev->frame = 0;
				pev->gaitsequence = 0;
				pev->sequence = animDesired;

				ResetSequenceInfo();
			}

			return;

		default:
			if ( m_Activity != m_IdealActivity )
			{
				m_Activity = m_IdealActivity;

				if ( *g_PlayerAnimationList[m_IdealActivity] )
					animDesired = LookupSequence( g_PlayerAnimationList[m_IdealActivity] );
				else
					animDesired = LookupActivity( m_IdealActivity );

				if ( pev->sequence != animDesired )
				{
					pev->gaitsequence = 0;
					pev->sequence = animDesired;
					pev->frame = 0;

					ResetSequenceInfo();
				}
			}

			return;
		}
	}

	// gait sequence
	if ( pev->deadflag || !STANCE_IS_PRONE( pev->iuser3 ) )
	{
		if ( pev->flags & FL_DUCKING )
		{
			if ( speed == 0.0 )
				pev->gaitsequence = LookupSequence( "dod_crouch_idle" );
			else
				pev->gaitsequence = LookupSequence( "dod_crawl" );
		}
		else if ( speed <= 0.0 )
		{
			pev->gaitsequence = LookupSequence( "dod_idle1" );
		}
		else if ( speed > 250.0 )
		{
			pev->gaitsequence = LookupSequence( "dod_sprint" );
		}
		else if ( speed > 73.0 )
		{
			pev->gaitsequence = LookupSequence( "dod_jog" );
		}
		else
		{
			pev->gaitsequence = LookupSequence( "dod_walk" );
		}
	}
	else
	{
		if ( speed == 0.0 )
			pev->gaitsequence = LookupSequence( "prone_idle" );
		else
			pev->gaitsequence = LookupSequence( "prone_forward" );
	}

	if ( pev->sequence != animDesired )
	{
		pev->sequence = animDesired;
		pev->frame = 0;

		ResetSequenceInfo();
	}
}


/*
===========
TabulateAmmo
This function is used to find and store 
all the ammo we have into the ammo vars.
============
*/
void CBasePlayer::TabulateAmmo()
{
	ammo_12mm = AmmoInventory( GetAmmoIndex( "ammo_12mm" ) );
	ammo_16mm = AmmoInventory( GetAmmoIndex( "ammo_16mm" ) );
	ammo_22mm = AmmoInventory( GetAmmoIndex( "ammo_22mm" ) );
	ammo_44mm = AmmoInventory( GetAmmoIndex( "ammo_44mm" ) );
	ammo_55mm = AmmoInventory( GetAmmoIndex( "ammo_55mm" ) );
	ammo_66mm = AmmoInventory( GetAmmoIndex( "ammo_66mm" ) );
	ammo_agrens = AmmoInventory( GetAmmoIndex( "ammo_agrens" ) );
	ammo_agrensex = AmmoInventory( GetAmmoIndex( "ammo_agrensex" ) );
	ammo_ggrens = AmmoInventory( GetAmmoIndex( "ammo_ggrens" ) );
	ammo_ggrensex = AmmoInventory( GetAmmoIndex( "ammo_ggrensex" ) );
	ammo_rockets = AmmoInventory( GetAmmoIndex( "rockets" ) );
}

/*
===========
WaterMove
============
*/
#define AIRTIME 12	// lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air;

	if ( pev->movetype == MOVETYPE_NOCLIP )
		return;

	if ( pev->health < 0 )
		return;

	if ( !IsAlive() )
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if ( pev->waterlevel != 3 || pev->watertype < -16 )
	{
		// not underwater

		// play 'up for air' sound
		if ( pev->air_finished < gpGlobals->time )
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM );
		else if ( pev->air_finished < gpGlobals->time + 9 )
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM );

		pev->air_finished = gpGlobals->time + AIRTIME;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if ( m_idrowndmg > m_idrownrestored )
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.

			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}
	}
	else
	{  // fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if ( pev->air_finished < gpGlobals->time )	// drown!
		{
			if ( pev->pain_finished < gpGlobals->time )
			{
				// take drowning damage
				pev->dmg += 1;
				if ( pev->dmg > 5 )
					pev->dmg = 5;
				TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), pev->dmg, DMG_DROWN );
				pev->pain_finished = gpGlobals->time + 1;

				// track drowning damage, give it back when
				// player finally takes a breath

				m_idrowndmg += pev->dmg;
			}
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if ( !pev->waterlevel || pev->watertype < -16 )
	{
		if ( FBitSet( pev->flags, FL_INWATER ) )
		{
			ClearBits( pev->flags, FL_INWATER );
		}
		return;
	}

	// make bubbles

	air = (int) ( pev->air_finished - gpGlobals->time );
	if ( !RANDOM_LONG( 0, 0x1f ) && RANDOM_LONG( 0, AIRTIME - 1 ) >= air )
	{
		switch ( RANDOM_LONG( 0, 3 ) )
		{
		case 0:
			EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM );
			break;
		case 3:
			EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM );
			break;
		}
	}

	if ( pev->watertype == CONTENT_LAVA )  // do damage
	{
		if ( pev->dmgtime < gpGlobals->time )
			TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), 10 * pev->waterlevel, DMG_BURN );
	}
	else if ( pev->watertype == CONTENT_SLIME )	 // do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), 4 * pev->waterlevel, DMG_ACID );
	}

	if ( !FBitSet( pev->flags, FL_INWATER ) )
	{
		SetBits( pev->flags, FL_INWATER );
		pev->dmgtime = 0;
	}
}

// TRUE if the player is attached to a ladder
BOOL CBasePlayer::IsOnLadder( void )
{
	return ( pev->movetype == MOVETYPE_FLY );
}


void CBasePlayer::PlayerDeathThink( void )
{
	float flForward;

	if ( m_iJoiningState != JOINED )
		return;

	if ( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		flForward = pev->velocity.Length() - 20;

		if ( flForward <= 0 )
			pev->velocity = g_vecZero;
		else
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	SetTouch( NULL );

	if ( m_rgpPlayerItems[0] || m_rgpPlayerItems[1] || m_rgpPlayerItems[2] || m_rgpPlayerItems[3] || m_rgpPlayerItems[4] ||
		 m_rgpPlayerItems[5] )
		RemoveAllItems( 1 );

	if ( HasObject )
		DropObject();

	if ( pev->movetype != MOVETYPE_NONE && FBitSet( pev->flags, FL_ONGROUND ) )
		pev->movetype = MOVETYPE_NONE;

	if ( pev->modelindex && !m_fSequenceFinished && pev->deadflag == DEAD_DYING )
	{
		StudioFrameAdvance();

		m_iRespawnFrames = m_iRespawnFrames + 1;  // Note, these aren't necessarily real "frames", so behavior is dependent on # of client movement commands

		if ( m_iRespawnFrames < 240 )  // Animations should be no longer than this
			return;
	}

	if ( pev->deadflag == DEAD_DYING )
		pev->deadflag = DEAD_DEAD;

	pev->framerate = 0.0;
	pev->effects |= EF_NOINTERP;

	pev->button = 0;

	if ( pev->deadflag == DEAD_DEAD )
	{
		if ( g_pGameRules->FPlayerCanRespawn( this ) )
		{
			m_fDeadTime = gpGlobals->time;
			pev->deadflag = DEAD_RESPAWNABLE;
		}
	}
	else
	{
		if ( g_pGameRules->IsMultiplayer() )
			g_pGameRules->PlayerDeathThink( this );

		pev->button = 0;
		m_iRespawnFrames = 0;

		if ( !g_pGameRules->IsMultiplayer() )
		{
			respawn( pev, !( m_afPhysicsFlags & PFLAG_OBSERVER ) );  // don't copy a corpse if we're in deathcam.
			pev->nextthink = -1;
		}
	}
}


//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam( void )
{
	if ( pev->view_ofs == g_vecZero )
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	// fade the screen out while the player is dead
	ScreenFade fade;
	UTIL_EdictScreenFadeBuild( fade, Vector( 0, 0, 0 ), 1.0f, 15.0f, 0, FFADE_OUT | FFADE_STAYOUT );
	UTIL_EdictScreenFadeWrite( fade, edict() );

	edict_t *pSpot = FIND_ENTITY_BY_CLASSNAME( NULL, "info_intermission" );

	if ( !FNullEnt( pSpot ) )
	{
		// at least one intermission spot in the world.
		int iRand = RANDOM_LONG( 0, 3 );

		while ( iRand > 0 )
		{
			edict_t *pNewSpot = FIND_ENTITY_BY_CLASSNAME( pSpot, "info_intermission" );

			if ( pNewSpot )
				pSpot = pNewSpot;

			iRand--;
		}

		StartObserver( pSpot->v.origin, pSpot->v.v_angle );
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, 128 ), ignore_monsters, edict(), &tr );

		StartObserver( tr.vecEndPos, UTIL_VecToAngles( tr.vecEndPos - pev->origin ) );
	}
}

// CBasePlayer::StartObserver moved to observer.cpp (binary link 0x121ee0
// observer.cpp:424) — HL leftover duplicate removed to fix multiple definition.
//
// PlayerUse - handles USE keypress
//
#define PLAYER_SEARCH_RADIUS (float) 64

void CBasePlayer::PlayerUse( void )
{
	// Was use pressed or released?
	if ( !( ( pev->button | m_afButtonPressed | m_afButtonReleased ) & IN_USE ) )
		return;

	// Hit Use on a train?
	if ( m_afButtonPressed & IN_USE )
	{
		if ( m_pTank )
		{
			// Stop controlling the tank
			m_pTank->Use( this, this, USE_OFF, 0 );
			m_pTank = NULL;
			return;
		}
		else
		{
			if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
			else
			{  // Start controlling the train!
				CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );

				if ( pTrain && !( pev->button & IN_JUMP ) && FBitSet( pev->flags, FL_ONGROUND ) &&
					 ( pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE ) && pTrain->OnControls( pev ) )
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed( pTrain->pev->speed, pTrain->pev->impulse );
					m_iTrain |= TRAIN_NEW;
					return;
				}
			}
		}
	}

	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	Vector vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;
	TraceResult tr;

	UTIL_MakeVectors( pev->v_angle );  // so we know which way we are facing

	// the thing right in front of the player is used first
	UTIL_TraceLine( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 64,
					dont_ignore_monsters, ENT( pev ), &tr );

	if ( !tr.pHit || ( pObject = (CBaseEntity *) tr.pHit->pvPrivateData ) == NULL ||
		 !( pObject->ObjectCaps() & ( FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE ) ) )
	{
		pObject = NULL;

		while ( ( pObject = UTIL_FindEntityInSphere( pObject, pev->origin, PLAYER_SEARCH_RADIUS ) ) != NULL )
		{
			if ( pObject->ObjectCaps() & ( FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE ) )
			{
				// This essentially moves the origin of the target to the corner nearest the player to test to see
				// if it's "hull" is in the view cone
				vecLOS = ( VecBModelOrigin( pObject->pev ) - ( pev->origin + pev->view_ofs ) );
				vecLOS = UTIL_ClampVectorToBox( vecLOS, pObject->pev->size * 0.5 );

				flDot = DotProduct( vecLOS, gpGlobals->v_forward );

				if ( flDot > flMaxDot )
				{  // only if the item is in front of the user
					pClosest = pObject;
					flMaxDot = flDot;
				}
			}
		}

		pObject = pClosest;

		if ( !pObject )
		{
			if ( m_afButtonPressed & IN_USE )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgUseSound, NULL, pev );
					WRITE_BYTE( 0 );
				MESSAGE_END();
			}

			return;
		}
	}

	// Found an object
	int caps = pObject->ObjectCaps();

	if ( m_afButtonPressed & IN_USE )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgUseSound, NULL, pev );
			WRITE_BYTE( 1 );
		MESSAGE_END();
	}

	if ( ( ( pev->button & IN_USE ) && ( caps & FCAP_CONTINUOUS_USE ) ) ||
		 ( ( m_afButtonPressed & IN_USE ) && ( caps & ( FCAP_IMPULSE_USE | FCAP_ONOFF_USE ) ) ) )
	{
		if ( caps & FCAP_CONTINUOUS_USE )
			m_afPhysicsFlags |= PFLAG_USING;

		pObject->Use( this, this, USE_SET, 1 );
	}
	// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
	else if ( ( m_afButtonReleased & IN_USE ) && ( pObject->ObjectCaps() & FCAP_ONOFF_USE ) )  // BUGBUG This is an "off" use
	{
		pObject->Use( this, this, USE_SET, 0 );
	}
}


void CBasePlayer::Jump()
{
	if ( FBitSet( pev->flags, FL_WATERJUMP ) )
		return;
	if ( !( ( pev->waterlevel < 2 || pev->watertype == -19 ) && ( m_afButtonPressed & IN_JUMP ) &&
			FBitSet( pev->flags, FL_ONGROUND ) && pev->groundentity && pev->iuser3 != STANCE_MORTAR ) )
		return;
	if ( pev->deadflag == DEAD_NO && STANCE_IS_PRONE( pev->iuser3 ) )
	{
		float f = pev->fuser4 - 40.0f;
		if ( f < 0.0f )
			f = 0.0f;
		else if ( f > 100.0f )
			f = 100.0f;
		pev->fuser4 = f;
	}
	if ( m_iFOV > 0 )
	{
		for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
		{
			for ( CBasePlayerItem *pItem = m_rgpPlayerItems[i]; pItem; pItem = pItem->m_pNext )
			{
				if ( CBasePlayerItem::ItemInfoArray[pItem->m_iId].iFlags & ITEM_FLAG_PRIMARY )
				{
					m_iFOV = ( (CBasePlayerWeapon *) pItem )->GetFOV();
					goto done;
				}
			}
		}
	}
done:
	UTIL_MakeVectors( pev->angles );
	SetAnimation( PLAYER_JUMP );
	if ( pev->groundentity && FBitSet( pev->groundentity->v.flags, FL_CONVEYOR ) )
		pev->velocity = pev->velocity + pev->basevelocity;
}


// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
void FixPlayerCrouchStuck( edict_t *pPlayer )
{
	TraceResult trace;

	// Move up as many as 18 pixels if the player is stuck.
	for ( int i = 0; i < 18; i++ )
	{
		UTIL_TraceHull( pPlayer->v.origin, pPlayer->v.origin, dont_ignore_monsters, head_hull, pPlayer, &trace );
		if ( trace.fStartSolid )
			pPlayer->v.origin.z++;
		else
			break;
	}
}


void CBasePlayer::Duck()
{
	if ( ( pev->button & IN_DUCK ) && m_IdealActivity != ACT_LEAP )
		SetAnimation( PLAYER_WALK );
}


//
// ID's player as such.
//
int CBasePlayer::Classify( void )
{
	return CLASS_PLAYER;
}

void CBasePlayer::AddPoints( int score, BOOL bAllowNegativeScore )
{
	// the points count outside of a clan match, or when the match is on
	if ( !g_pGameRules->IsClanMatch() || g_pGameRules->IsClanMatchActive() )
	{
		// Positive score always adds
		if ( score < 0 )
		{
			if ( !bAllowNegativeScore )
			{
				if ( pev->frags < 0 )  // Can't go more negative
					return;

				if ( -score > pev->frags )	// Will this go negative?
				{
					score = -pev->frags;  // Sum will be 0
				}
			}
		}

		pev->frags += score;

		MESSAGE_BEGIN( MSG_ALL, gmsgFrags );
		WRITE_BYTE( ENTINDEX( edict() ) );
		WRITE_SHORT( (int) pev->frags );
		MESSAGE_END();
	}
}

void CBasePlayer::AddPointsToTeam( int score, BOOL bAllowNegativeScore )
{
	if ( g_pGameRules->IsClanMatch() && !g_pGameRules->IsClanMatchActive() )
		return;

	int index = entindex();

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *) UTIL_PlayerByIndex( i );

		if ( pPlayer && i != index )
		{
			if ( g_pGameRules->PlayerRelationship( this, pPlayer ) == GR_TEAMMATE &&
				 ( !g_pGameRules->IsClanMatch() || g_pGameRules->IsClanMatchActive() ) )
			{
				// the teammates get the points as objective score
				pPlayer->m_iObjScore += score;

				MESSAGE_BEGIN( MSG_ALL, gmsgObjScore );
				WRITE_BYTE( ENTINDEX( pPlayer->edict() ) );
				WRITE_SHORT( pPlayer->m_iObjScore );
				MESSAGE_END();
			}
		}
	}
}

//Player ID
void CBasePlayer::InitStatusBar()
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString1[0] = m_SbarString0[0] = 0;
}

void CBasePlayer::UpdateStatusBar()
{
	CBaseEntity *pEntity;
	int iIndex;
	int iHealth = 0;
	float flDelay;

	if ( m_hObserverTarget && ( pev->iuser1 == 1 || pev->iuser1 == 2 || pev->iuser1 == 4 ) )
	{
		// spectators see the health of the player they are watching
		pEntity = UTIL_PlayerByIndex( ENTINDEX( m_hObserverTarget->edict() ) );

		if ( pEntity && !FNullEnt( pEntity->edict() ) )
		{
			iHealth = pEntity->pev->health / pEntity->pev->max_health * 100;
			iIndex = ENTINDEX( pEntity->edict() );
		}
		else
		{
			iIndex = -1;

			if ( m_flStatusBarDisappearDelay > gpGlobals->time )
			{
				iIndex = m_izSBarState[SBAR_ID_TARGETNAME];
				iHealth = m_izSBarState[SBAR_ID_TARGETHEALTH];
			}
		}

		flDelay = gpGlobals->time + 10.0;
	}
	else
	{
		// find an ID target
		TraceResult tr;

		UTIL_MakeVectors( pev->v_angle + pev->punchangle );

		Vector vecSrc = EyePosition();
		Vector vecEnd = vecSrc + gpGlobals->v_forward * 2048;

		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr );

		if ( tr.flFraction == 1.0 || FNullEnt( tr.pHit ) )
			return;

		pEntity = CBaseEntity::Instance( tr.pHit );

		if ( pEntity->Classify() != CLASS_PLAYER )
			return;

		if ( pev->team == pEntity->pev->team || pev->iuser1 > 0 )
		{
			if ( !( m_flDisplayHistory & HINT_SPOTTED_FRIEND ) )
			{
				if ( IsAlive() )
				{
					m_flDisplayHistory |= HINT_SPOTTED_FRIEND;

					if ( m_bShowHints )
					{
						MESSAGE_BEGIN( MSG_ONE, gmsgHudText, NULL, pev );
							WRITE_STRING( "#Hint_spotted_a_friend" );
							WRITE_BYTE( 1 );
						MESSAGE_END();
					}
				}
			}

			iHealth = pEntity->pev->health / pEntity->pev->max_health * 100;
			iIndex = ENTINDEX( edict() );
			flDelay = gpGlobals->time + 2.0;
		}
		else
		{
			if ( (unsigned int) ( pEntity->pev->team - 1 ) <= 1 )
			{
				if ( m_flDisplayHistory & HINT_SPOTTED_ENEMY )
				{
					m_flDisplayHistory |= HINT_SPOTTED_ENEMY;
					HintMessage( "#Hint_spotted_an_enemy", FALSE );
				}
			}

			return;
		}
	}

	m_flStatusBarDisappearDelay = flDelay;
	m_hLastIDTarget = pEntity;

	if ( iIndex != -1 && ( m_izSBarState[SBAR_ID_TARGETNAME] != iIndex || m_izSBarState[SBAR_ID_TARGETHEALTH] != iHealth ) )
	{
		int iSend = iHealth >= 0 ? iHealth : 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgStatusValue, NULL, pev );
			WRITE_BYTE( iSend );
		MESSAGE_END();

		m_izSBarState[SBAR_ID_TARGETHEALTH] = iSend;
		m_izSBarState[SBAR_ID_TARGETNAME] = iIndex;
	}
}

#define CLIMB_SHAKE_FREQUENCY 22  // how many frames in between screen shakes when climbing
#define MAX_CLIMB_SPEED 200		  // fastest vertical climbing speed possible
#define CLIMB_SPEED_DEC 15		  // climbing deceleration rate
#define CLIMB_PUNCH_X -7		  // how far to 'punch' client X axis when climbing
#define CLIMB_PUNCH_Z 7			  // how far to 'punch' client Z axis when climbing

// DoD PreThink - PORTED from restore (dod.so). See restore/ NOTES.

void CBasePlayer::PreThink( void )
{
	int buttonsChanged = ( m_afButtonLast ^ pev->button );  // These buttons have changed this frame

	if ( m_iJoiningState == READYTOGO )
	{
		m_iJoiningState = JOINED;
		StartObserver( pev->origin, pev->angles );
	}

	m_hintMessageQueue.Update( *this );

	// Debounced button codes for pressed/released
	// UP viewing: ----------------------------------------------------------------------------------------------------
	m_afButtonPressed = buttonsChanged & pev->button;  // The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & ( ~pev->button );  // The ones not down are "released"

	Stamina();

	if ( gpGlobals->time > m_flIdleTimer && IsAlive() )
		SetAnimation( PLAYER_IDLE );

	g_pGameRules->PlayerThink( this );

	if ( g_fGameOver )
		return;  // intermission or finale

	UTIL_MakeVectors( pev->v_angle );  // is this still used?

	if ( m_flNextAttack <= 0 && m_pActiveItem )
		m_pActiveItem->ItemPreFrame();

	WaterMove();

	if ( g_pGameRules && g_pGameRules->FAllowFlashlight() )
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;

	UpdateClientData();

	if ( pev->deadflag == DEAD_DEAD && gpGlobals->time > m_fDeadTime + 1.0 )
	{
		if ( pev->iuser2 == ENTINDEX( edict() ) )
		{
			SpawnClientSideCorpse();
			Observer_FindNextPlayer( false );
		}
	}

	if ( pev->iuser1 > 0 && ( m_afPhysicsFlags & PFLAG_OBSERVER ) )
	{
		Observer_HandleButtons();
		Observer_CheckTarget();
		Observer_CheckProperties();
		pev->impulse = 0;
		return;
	}

	if ( pev->deadflag > DEAD_NO )
	{
		if ( m_iJoiningState == JOINED )
			PlayerDeathThink();

		return;
	}

	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
		pev->flags |= FL_ONTRAIN;
	else
		pev->flags &= ~FL_ONTRAIN;

	// Train speed control
	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
	{
		CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );
		float vel;

		if ( !pTrain )
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -38 ), ignore_monsters, ENT( pev ), &trainTrace );

			// HACKHACK - Just look for the func_tracktrain classname
			if ( trainTrace.flFraction != 1.0 && trainTrace.pHit )
				pTrain = CBaseEntity::Instance( trainTrace.pHit );

			if ( !pTrain || !( pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE ) || !pTrain->OnControls( pev ) )
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
		}
		else if ( !FBitSet( pev->flags, FL_ONGROUND ) || FBitSet( pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL ) ||
				  ( pev->button & ( IN_MOVELEFT | IN_MOVERIGHT ) ) )
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;

		if ( m_afButtonPressed & IN_FORWARD )
		{
			vel = 1;
			pTrain->Use( this, this, USE_SET, (float) vel );
		}
		else if ( m_afButtonPressed & IN_BACK )
		{
			vel = -1;
			pTrain->Use( this, this, USE_SET, (float) vel );
		}

		if ( vel )
		{
			m_iTrain = TrainSpeed( pTrain->pev->speed, pTrain->pev->impulse );
			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		}
	}
	else if ( m_iTrain & TRAIN_ACTIVE )
	{
		m_iTrain = TRAIN_NEW | TRAIN_OFF;  // turn off train
	}

	if ( pev->button & IN_JUMP )
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}

	// If trying to duck, already ducked, or in the process of ducking
	if ( ( pev->button & IN_DUCK ) || ( ( pev->flags & FL_DUCKING ) && ( pev->deadflag || !STANCE_IS_PRONE( pev->iuser3 ) ) ) ||
		 ( m_afPhysicsFlags & PFLAG_DUCKING ) )
		Duck();

	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		m_flFallVelocity = -pev->velocity.z;
	}

	m_hEnemy = NULL;

	if ( m_afPhysicsFlags & PFLAG_ONBARNACLE )
	{
		pev->velocity = g_vecZero;
	}
}


/* Time based Damage works as follows: 
	1) There are several types of timebased damage:

		#define DMG_PARALYZE		(1 << 14)	// slows affected creature down
		#define DMG_NERVEGAS		(1 << 15)	// nerve toxins, very bad
		#define DMG_POISON			(1 << 16)	// blood poisioning
		#define DMG_RADIATION		(1 << 17)	// radiation exposure
		#define DMG_DROWNRECOVER	(1 << 18)	// drown recovery
		#define DMG_ACID			(1 << 19)	// toxic chemicals or acid burns
		#define DMG_SLOWBURN		(1 << 20)	// in an oven
		#define DMG_SLOWFREEZE		(1 << 21)	// in a subzero freezer

	2) A new hit inflicting tbd restarts the tbd counter - each monster has an 8bit counter,
		per damage type. The counter is decremented every second, so the maximum time 
		an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
		of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
		is determined by the type of tdb.  
			Paralyze		- 1/2 movement rate, 30 second duration.
			Nervegas		- 5 points per second, 16 second duration = 80 points max dose.
			Poison			- 2 points per second, 25 second duration = 50 points max dose.
			Radiation		- 1 point per second, 50 second duration = 50 points max dose.
			Drown			- 5 points per second, 2 second duration.
			Acid/Chemical	- 5 points per second, 10 second duration = 50 points max.
			Burn			- 10 points per second, 2 second duration.
			Freeze			- 3 points per second, 10 second duration = 30 points max.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

		Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
							- recharged by suit recharger
		Air In Lungs		- drowning damage is done to air in lungs first, then to body
							- recharged by poking head out of water
							- 10 seconds if swiming fast
		Air In SCUBA		- drowning damage is done to air in tanks first, then to body
							- 2 minutes in tanks. Need new tank once empty.
		Radiation Syringe	- Each syringe full provides protection vs one radiation dosage
		Antitoxin Syringe	- Each syringe full provides protection vs one poisoning (nervegas or poison).
		Health kit			- Immediate stop to acid/chemical, fire or freeze damage.
		Radiation Shower	- Immediate stop to radiation damage, acid/chemical or fire damage.
		
	
*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

//#define PARALYZE_DURATION	30		// number of 2 second intervals to take damage
//#define PARALYZE_DAMAGE		0.0		// damage to take each 2 second interval

//#define NERVEGAS_DURATION	16
//#define NERVEGAS_DAMAGE		5.0

//#define POISON_DURATION		25
//#define POISON_DAMAGE		2.0

//#define RADIATION_DURATION	50
//#define RADIATION_DAMAGE	1.0

//#define ACID_DURATION		10
//#define ACID_DAMAGE			5.0

//#define SLOWBURN_DURATION	2
//#define SLOWBURN_DAMAGE		1.0

//#define SLOWFREEZE_DURATION	1.0
//#define SLOWFREEZE_DAMAGE	3.0

/* */


/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation. 
Some functions are automatic, some require power. 
The player gets the suit shortly after getting off the train in C1A0 and it stays
with him for the entire game.

Protection

	Heat/Cold
		When the player enters a hot/cold area, the heating/cooling indicator on the suit 
		will come on and the battery will drain while the player stays in the area. 
		After the battery is dead, the player starts to take damage. 
		This feature is built into the suit and is automatically engaged.
	Radiation Syringe
		This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
	Anti-Toxin Syringe
		This will cure the player from being poisoned. Single use item.
	Health
		Small (1st aid kits, food, etc.)
		Large (boxes on walls)
	Armor
		The armor works using energy to create a protective field that deflects a
		percentage of damage projectile and explosive attacks. After the armor has been deployed,
		it will attempt to recharge itself to full capacity with the energy reserves from the battery.
		It takes the armor N seconds to fully charge. 

Notification (via the HUD)

x	Health
x	Ammo  
x	Automatic Health Care
		Notifies the player when automatic healing has been engaged. 
x	Geiger counter
		Classic Geiger counter sound and status bar at top of HUD 
		alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x	Poison
	Armor
		Displays the current level of armor. 

Augmentation 

	Reanimation (w/adrenaline)
		Causes the player to come back to life after he has been dead for 3 seconds. 
		Will not work if player was gibbed. Single use.
	Long Jump
		Used by hitting the ??? key(s). Caused the player to further than normal.
	SCUBA	
		Used automatically after picked up and after player enters the water. 
		Works for N seconds. Single use.	
	
Things powered by the battery

	Armor		
		Uses N watts for every M units of damage.
	Heat/Cool	
		Uses N watts for every second in hot/cold area.
	Long Jump	
		Uses N watts for every jump.
	Alien Cloak	
		Uses N watts for each use. Each use lasts M seconds.
	Alien Shield	
		Augments armor. Reduces Armor drain by one half
 
*/

// if in range of radiation source, ping geiger counter

#define GEIGERDELAY 0.25


/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME 3.5
#define SUITFIRSTUPDATETIME 0.1


// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

// DoD: CBasePlayer::SetSuitUpdate has no symbol in dod.so (suit system removed).
// Kept as an empty stub because items.cpp still calls it during the restore.
void CBasePlayer::SetSuitUpdate( const char *name, int fgroup, int iNoRepeatTime )
{
}

/*
================
CheckPowerups

Check for turning off powerups

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
================
*/
static void CheckPowerups( entvars_t *pev )
{
	if ( pev->health <= 0 )
		return;

	pev->modelindex = g_ulModelIndexPlayer;	 // don't use eyes
}

//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================

void CBasePlayer ::UpdatePlayerSound( void )
{
	int iBodyVolume = 0;
	int iVolume;
	CSound *pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
	if ( !pSound )
	{
		ALERT( at_console, "Client lost reserved sound!\n" );
		return;
	}
	pSound->m_iType = bits_SOUND_NONE;
	if ( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		iBodyVolume = (int) pev->velocity.Length();
		if ( iBodyVolume > 512 )
			iBodyVolume = 512;
	}
	if ( pev->button & IN_JUMP )
		iBodyVolume += 100;
	if ( m_iWeaponVolume > iBodyVolume )
	{
		m_iTargetVolume = m_iWeaponVolume;
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		m_iTargetVolume = iBodyVolume;
	}
	m_iWeaponVolume = (int) ( (float) m_iWeaponVolume - gpGlobals->frametime * 250.0f );
	iVolume = pSound->m_iVolume;
	if ( m_iTargetVolume > iVolume )
		iVolume = m_iTargetVolume;
	else if ( m_iTargetVolume < iVolume )
	{
		iVolume = (int) ( (float) iVolume - 250.0f * gpGlobals->frametime );
		if ( iVolume < m_iTargetVolume )
			iVolume = 0;
	}
	if ( gpGlobals->time > m_flStopExtraSoundTime )
		m_iExtraSoundTypes = 0;
	pSound->m_vecOrigin = pev->origin;
	pSound->m_iVolume = iVolume;
	pSound->m_iType |= ( bits_SOUND_PLAYER | m_iExtraSoundTypes );
}


// DoD PostThink - PORTED from restore (dod.so). See restore/ NOTES.

void CBasePlayer::PostThink()
{
	if ( g_fGameOver )
		goto pt_end;  // intermission or finale

	if ( !IsAlive() )
		goto pt_end;

	// Handle Tank controlling
	if ( m_pTank )
	{
		// if they've moved too far from the gun,  or selected a weapon, unuse the gun
		if ( !m_pTank->OnControls( pev ) || pev->weaponmodel )
		{
			m_pTank->Use( this, this, USE_OFF, 0 );
			m_pTank = NULL;
		}
	}

	// do weapon stuff
	ItemPostFrame();

	// check to see if player landed hard enough to make a sound
	if ( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		if ( pev->health > 0 && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD )
		{
			if ( m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED )
			{
				// after this point, we start doing damage
				float flFallDamage = g_pGameRules->FlPlayerFallDamage( this );

				if ( flFallDamage > pev->health )
				{
					// splat
					EMIT_SOUND_DYN( edict(), CHAN_ITEM, "common/bodysplat.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
				}

				if ( flFallDamage > 0 )
				{
					TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), flFallDamage, DMG_FALL );
					pev->punchangle.x = 0;
				}
			}

			// a hard landing takes the player out of the scope
			if ( m_flFallVelocity > 250 && m_iFOV > 0 )
			{
				for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
				{
					CBasePlayerItem *pItem = m_rgpPlayerItems[i];

					while ( pItem )
					{
						if ( CBasePlayerItem::ItemInfoArray[pItem->m_iId].iFlags & ITEM_FLAG_RIFLE )
							break;

						pItem = pItem->m_pNext;
					}

					if ( pItem )
					{
						m_iFOV = ( (CBasePlayerWeapon *) pItem )->ZoomOut();
						break;
					}
				}
			}

			if ( IsAlive() )
			{
				if ( pev->deadflag || !STANCE_IS_PRONE( pev->iuser3 ) )
				{
					if ( m_IdealActivity != DOD_PRONE_GETUP || m_fSequenceFinished )
						SetAnimation( PLAYER_WALK );
				}
				else if ( m_IdealActivity != DOD_PRONE_GETDOWN || m_fSequenceFinished )
				{
					SetAnimation( PLAYER_PRONE2 );
				}
			}
		}

		if ( FBitSet( pev->flags, FL_ONGROUND ) )
		{
			if ( m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer() )
			{
				CSoundEnt::InsertSound( bits_SOUND_PLAYER, pev->origin, m_flFallVelocity, 0.2 );
			}

			m_flFallVelocity = 0;
		}
	}

	// a prone player that ends up deep in the water gets up
	if ( pev->waterlevel > 2 && !pev->deadflag && pev->iuser3 == STANCE_PRONE )
	{
		pev->iuser3 = STANCE_STANDING;
		m_ianimupdate = 1;
		SetAnimation( PLAYER_PRONE3 );
		m_inextprone = gpGlobals->time + 1.0;
	}

	// running drains the stamina
	if ( ( pev->button & IN_RUN ) && ( pev->button & ( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT ) ) )
	{
		if ( gpGlobals->time > m_nextRunThink && pev->velocity.Length() > 100 )
		{
			pev->fuser4 = clamp( pev->fuser4 - 30, 0, 100 );
			m_nextRunThink = gpGlobals->time + 1.0;
		}
	}
	else
	{
		m_nextRunThink = gpGlobals->time - 0.1;
	}

	// select the proper animation for the player character
	if ( IsAlive() )
	{
		if ( pev->velocity.x != 0 || pev->velocity.y != 0 )
		{
			if ( FBitSet( pev->flags, FL_ONGROUND ) )
			{
				if ( !pev->deadflag && STANCE_IS_PRONE( pev->iuser3 ) )
				{
					pev->angles.x = 0;

					if ( m_IdealActivity != DOD_PRONE_GETDOWN || m_fSequenceFinished )
						SetAnimation( PLAYER_PRONE2 );
				}
				else
				{
					pev->angles.x = clamp( pev->angles.x, -30, 30 );

					if ( m_IdealActivity != DOD_PRONE_GETUP || m_fSequenceFinished )
						SetAnimation( PLAYER_WALK );
				}
			}
			else if ( pev->waterlevel > 1 )
			{
				if ( !pev->deadflag && STANCE_IS_PRONE( pev->iuser3 ) )
				{
					pev->button &= ~IN_JUMP;
					pev->button &= ~IN_DUCK;
					pev->flags &= ~FL_DUCKING;
					m_afPhysicsFlags &= ~PFLAG_DUCKING;
				}

				SetAnimation( PLAYER_WALK );
			}
		}
		else if ( !pev->deadflag && STANCE_IS_PRONE( pev->iuser3 ) )
		{
			if ( m_fSequenceFinished )
				SetAnimation( PLAYER_PRONE2 );

			pev->angles.x = clamp( pev->angles.x, -15, 15 );
		}
		else
		{
			if ( m_IdealActivity == ACT_RANGE_ATTACK1 )
			{
				if ( m_fSequenceFinished && m_bBazookaDeployed )
					SetAnimation( PLAYER_IDLE );
			}
			else if ( m_IdealActivity != DOD_PRONE_GETUP || m_fSequenceFinished )
			{
				SetAnimation( PLAYER_IDLE );
			}

			pev->angles.x = clamp( pev->angles.x, -30, 30 );
		}
	}

	CheckPlayerSpeed();

	StudioFrameAdvance();

	if ( 0 < pev->health )
		pev->modelindex = g_ulModelIndexPlayer;

	UpdatePlayerSound();

pt_end:
#if defined( CLIENT_WEAPONS )
	// Decay timers on weapons
	// go through all of the weapons and make a list of the ones to pack
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[i];

		while ( pPlayerItem )
		{
			CBasePlayerWeapon *gun = (CBasePlayerWeapon *) pPlayerItem->GetWeaponPtr();

			if ( gun && gun->UseDecrement() )
			{
				gun->m_flNextPrimaryAttack = max( gun->m_flNextPrimaryAttack - gpGlobals->frametime, -1.0f );
				gun->m_flNextSecondaryAttack = max( gun->m_flNextSecondaryAttack - gpGlobals->frametime, -0.001f );

				if ( gun->m_flTimeWeaponIdle != 1000 )
				{
					gun->m_flTimeWeaponIdle = max( gun->m_flTimeWeaponIdle - gpGlobals->frametime, -0.001f );
				}

				if ( gun->pev->fuser1 != 1000 )
				{
					gun->pev->fuser1 = max( gun->pev->fuser1 - gpGlobals->frametime, -0.001f );
				}
			}

			pPlayerItem = pPlayerItem->m_pNext;
		}
	}

	m_flNextAttack -= gpGlobals->frametime;

	if ( m_flNextAttack < -0.001 )
		m_flNextAttack = -0.001;
#endif

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;
	m_iGaitsequence = pev->gaitsequence;

	StudioProcessGait();
}


// checks if the spot is clear of players
BOOL IsSpawnPointValid( CBaseEntity *pPlayer, CBaseEntity *pSpot )
{
	CBaseEntity *ent = NULL;
	CBaseDMStart *pStart = (CBaseDMStart *) pSpot;

	if ( pStart->GetState( pPlayer ) != STATE_ON )
		return FALSE;

	if ( !pStart->IsMasteredOn() )
		return FALSE;

	if ( pSpot->pev->fuser1 > gpGlobals->time )
		return FALSE;

	while ( ( ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 96 ) ) != NULL )
	{
		// if ent is a client, don't spawn on 'em
		if ( ent->IsPlayer() && pPlayer != ent )
			return FALSE;
	}

	return TRUE;
}

DLL_GLOBAL CBaseEntity *g_pLastSpawn;
inline int FNullEnt( CBaseEntity *ent )
{
	return ( ent == NULL ) || FNullEnt( ent->edict() );
}

/*
============
EntSelectSpawnPoint

Returns the entity to spawn at

USES AND SETS GLOBAL g_pLastSpawn
============
*/

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

edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer, BOOL bJustChecking )
{
	CBaseEntity *pSpot = NULL;
	CBaseEntity *pFirstSpot;
	const char *pszClassname;

	if ( g_pLastSpawn && !FNullEnt( g_pLastSpawn->edict() ) )
		pSpot = g_pLastSpawn;

	if ( pPlayer->pev->team == TEAM_ALLIES || pPlayer->pev->team == TEAM_AXIS )
	{
		if ( pPlayer->pev->team == TEAM_ALLIES )
			pszClassname = "info_player_allies";
		else
			pszClassname = "info_player_axis";

		// pick the first valid spot of the team after the last one used
		pFirstSpot = UTIL_FindEntityByClassname( pSpot, pszClassname );
		pSpot = pFirstSpot;

		while ( 1 )
		{
			if ( pSpot && !FNullEnt( pSpot->edict() ) && IsSpawnPointValid( pPlayer, pSpot ) )
				break;

			pSpot = UTIL_FindEntityByClassname( pSpot, pszClassname );

			if ( pSpot == pFirstSpot )
				return NULL;
		}
	}
	else
	{
		pSpot = UTIL_FindEntityByClassname( NULL, "info_player_observer" );
	}

	return ReturnSpot( pSpot, (CBasePlayer *) pPlayer, bJustChecking );
}


// DoD Spawn - PORTED from restore (dod.so). See restore/ NOTES.

void CBasePlayer::Spawn( void )
{
	if ( pev->iuser1 )
		return;

	m_iGaitsequence = 0;
	m_flGaitframe = 0;
	m_flGaityaw = 0;
	m_flGaitMovement = 0;
	m_fHandleSignalsTime = 0;
	m_fGunDropTime = 0;

	if ( m_iJoiningState != JUSTJOINED )
	{
		if ( !g_pGameRules->GetPlayerSpawnSpot( this ) )
		{
			m_iJoiningState = JOINED;

			UTIL_EdictScreenFade( edict(), Vector( 0, 0, 0 ), 1.0f, 15.0f, 0, 5 );

			pev->deadflag = DEAD_RESPAWNABLE;
			pev->effects |= EF_NODRAW;

			m_bIsObserver = 1;

			SetThink( &CBasePlayer::SpawnThink );
			pev->nextthink = gpGlobals->time + 1.0;

			StartObserver( pev->origin, pev->angles );

			ClientPrint( pev, HUD_PRINTCENTER, "#game_no_spawns" );
			return;
		}
	}
	else
	{
		g_pGameRules->GetInitialSpawnSpot( this );
	}

	m_prevgaitorigin = pev->origin;

	m_hintMessageQueue.Reset();
	m_flDisplayHistory &= ~HINT_CAREFUL_AROUND_TEAMMATES;

	MAKE_STRING_CLASS( "player", pev );

	pev->health = 100;
	pev->armorvalue = 0;
	pev->takedamage = DAMAGE_AIM;
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_WALK;
	pev->max_health = pev->health;
	pev->flags &= 0x100000;
	pev->flags |= FL_CLIENT;
	pev->air_finished = gpGlobals->time + 12;
	pev->dmg = 2;  // initial water damage
	pev->effects = 0;
	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;
	pev->friction = 1.0;
	pev->gravity = 1.0;
	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	m_afPhysicsFlags = 0;

	if ( m_flSndRoomtype != 0 )
	{
		m_pentSndLast = NULL;
		m_flSndRange = 0;
		m_flSndRoomtype = 0;

		MESSAGE_BEGIN( MSG_ONE, SVC_ROOMTYPE, NULL, pev );  // roomtype 0 is dry
			WRITE_SHORT( 0 );
		MESSAGE_END();
	}

	pev->iuser1 = 0;
	pev->iuser2 = 0;
	pev->iuser3 = STANCE_STANDING;
	pev->vuser1.x = SANDBAG_NONE;
	pev->vuser1.y = 0;

	m_bIsObserver = 0;
	m_hObserverTarget = NULL;

	m_ianimupdate = 0;
	m_fSpeedFactor = 1.0;
	m_iRespawnFrames = 0;
	m_flLastTalkTime = 0;
	m_inextprone = 0;
	m_nextRunThink = gpGlobals->time - 0.1;

	m_flIdleTimer = RANDOM_LONG( 20, 30 ) + gpGlobals->time;
	m_bSlowedByHit = false;
	m_flUnslowTime = gpGlobals->time;
	m_hLastIDTarget = NULL;

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );

	m_iFOV = 0;  // init field of view.
	m_iClientFOV = 0;  // init client field of view.
	m_bloodColor = 247;
	m_flNextDecalTime = 0;  // let this player decal as soon as he spawns.

	m_flFieldOfView = 0.5;  // some monsters use this to determine whether or not the player is looking at them.

	m_flNextAttack = UTIL_WeaponTimeBase();
	StartSneaking();

	m_flFallVelocity = 0;

	g_pGameRules->SetDefaultPlayerTeam( this );

	SET_MODEL( ENT( pev ), "models/player.mdl" );
	g_ulModelIndexPlayer = pev->modelindex;
	pev->sequence = LookupActivity( ACT_IDLE );

	UTIL_SetSize( pev, Vector( -16, -16, -36 ), Vector( 16, 16, 36 ) );

	pev->view_ofs = Vector( 0, 0, 22 );  // view height
	Precache();
	m_HackedGunPos = Vector( 0, 32, 0 );

	if ( m_iPlayerSound == SOUNDLIST_EMPTY )
	{
		ALERT( at_console, "Couldn't alloc player sound slot!\n" );
	}

	m_pLastItem = NULL;
	m_pClientActiveItem = NULL;

	m_iClientHealth = -1;
	m_fInitHUD = TRUE;
	m_iClientHideHUD = -1;  // force this to be recalculated
	m_fWeapon = FALSE;
	m_flNextMapMarkerTime = gpGlobals->time;

	memset( m_rgAmmo, 0, sizeof( m_rgAmmo ) );
	memset( m_rgAmmoLast, 0, sizeof( m_rgAmmoLast ) );

	if ( !g_pGameRules->IsMultiplayer() || !m_bIsObserver )
		g_pGameRules->PlayerSpawn( this );

	pev->fuser4 = 100;
	m_nMGAmmoCount = 1;
	m_nGenericAmmoCount = 1;

	m_iClientFOV = 0;
	m_iFOV = 0;
	pev->fov = 0;

	UTIL_ScreenFade( this, Vector( 0, 0, 0 ), 0.001, 0, 0, 0 );

	HasObject = 0;
	m_pObject = NULL;

	MESSAGE_BEGIN( MSG_ONE, gmsgResetSens, NULL, pev );
	MESSAGE_END();

	m_iCapAreaIconIndex = -1;
	m_iObjectAreaIndex = -1;
	m_iCapAreaIndex = -1;
}



void CBasePlayer ::Precache( void )
{
	if ( WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet )
	{
		if ( !WorldGraph.FSetGraphPointers() )
			ALERT( at_console, "**Graph pointers were not set!\n" );
		else
			ALERT( at_console, "**Graph Pointers Set!\n" );
	}
	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;
	m_iTrain = TRAIN_NEW;
	LinkUserMessages();
	m_iUpdateTime = 5;
	if ( gInitHUD )
		m_fInitHUD = TRUE;
}


int CBasePlayer::Save( CSave &save )
{
	if ( !CBaseMonster::Save( save ) )
		return 0;

	return save.WriteFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE( m_playerSaveData ) );
}

//
// Marks everything as new so the player will resend this to the hud.
//

int CBasePlayer::Restore( CRestore &restore )
{
	if ( !CBaseMonster::Restore( restore ) )
		return 0;

	int status = restore.ReadFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE( m_playerSaveData ) );

	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *) gpGlobals->pSaveData;
	// landmark isn't present.
	if ( !pSaveData->fUseLandmark )
	{
		ALERT( at_console, "No Landmark:%s\n", pSaveData->szLandmarkName );

		// default to normal spawn
		edict_t *pentSpawnSpot = EntSelectSpawnPoint( this, false );
		pev->origin = VARS( pentSpawnSpot )->origin + Vector( 0, 0, 1 );
		pev->angles = VARS( pentSpawnSpot )->angles;
	}
	pev->v_angle.z = 0;	 // Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = TRUE;  // turn this way immediately

	// Copied from spawn() for now
	m_bloodColor = BLOOD_COLOR_RED;

	g_ulModelIndexPlayer = pev->modelindex;

	if ( FBitSet( pev->flags, FL_DUCKING ) )
	{
		// Use the crouch HACK
		//FixPlayerCrouchStuck( edict() );
		// Don't need to do this with new player prediction code.
		UTIL_SetSize( pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
	}
	else
	{
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );
	}

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );


	//Resync ammo data so you can reload - Solokiller
	TabulateAmmo();

#if defined( CLIENT_WEAPONS )
	// HACK:	This variable is saved/restored in CBaseMonster as a time variable, but we're using it
	//			as just a counter.  Ideally, this needs its own variable that's saved as a plain float.
	//			Barring that, we clear it out here instead of using the incorrect restored time value.
	m_flNextAttack = UTIL_WeaponTimeBase();
#endif

	return status;
}

void CBasePlayer::SelectNextItem( int iItem )
{
	CBasePlayerItem *pItem;

	pItem = m_rgpPlayerItems[iItem];

	if ( !pItem )
		return;

	if ( pItem == m_pActiveItem )
	{
		// select the next one in the chain
		pItem = m_pActiveItem->m_pNext;
		if ( !pItem )
		{
			return;
		}

		CBasePlayerItem *pLast;
		pLast = pItem;
		while ( pLast->m_pNext )
			pLast = pLast->m_pNext;

		// relink chain
		pLast->m_pNext = m_pActiveItem;
		m_pActiveItem->m_pNext = NULL;
		m_rgpPlayerItems[iItem] = pItem;
	}


	// FIX, this needs to queue them up and delay
	if ( m_pActiveItem )
	{
		m_pActiveItem->Holster();
	}

	m_pActiveItem = pItem;

	if ( m_pActiveItem )
	{
		m_pActiveItem->Deploy();
		m_pActiveItem->UpdateItemInfo();
	}
}

void CBasePlayer::SelectItem( const char *pstr )
{
	if ( !pstr )
		return;

	CBasePlayerItem *pItem = NULL;

	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if ( m_rgpPlayerItems[i] )
		{
			pItem = m_rgpPlayerItems[i];

			while ( pItem )
			{
				if ( FClassnameIs( pItem->pev, pstr ) )
					break;
				pItem = pItem->m_pNext;
			}
		}

		if ( pItem )
			break;
	}

	if ( !pItem )
		return;

	if ( pItem == m_pActiveItem )
		return;


	// FIX, this needs to queue them up and delay
	if ( m_pActiveItem )
		m_pActiveItem->Holster();

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if ( m_pActiveItem )
	{
		m_pActiveItem->Deploy();
		m_pActiveItem->UpdateItemInfo();
	}
}

void CBasePlayer::SelectLastItem( void )
{
	if ( !m_pLastItem )
	{
		return;
	}

	if ( m_pActiveItem && !m_pActiveItem->CanHolster() )
	{
		return;
	}


	// FIX, this needs to queue them up and delay
	if ( m_pActiveItem )
		m_pActiveItem->Holster();

	CBasePlayerItem *pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;
	m_pActiveItem->Deploy();
	m_pActiveItem->UpdateItemInfo();
}

//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
BOOL CBasePlayer::HasWeapons( void )
{
	int i;

	for ( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if ( m_rgpPlayerItems[i] )
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CBasePlayer::SelectPrevItem( int iItem ) {}

const char *CBasePlayer::TeamID( void )
{
	if ( pev == NULL )	// Not fully connected yet
		return "";

	// return their team name
	return m_szTeamName;
}

//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
  public:
	void Spawn( entvars_t *pevOwner );
	void Think( void );

	virtual int ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn( entvars_t *pevOwner )
{
	pev->origin = pevOwner->origin + Vector( 0, 0, 32 );
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT( pevOwner );
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM );
}

void CSprayCan::Think( void )
{
	TraceResult tr;
	int playernum;
	int nFrames;
	CBasePlayer *pPlayer;

	pPlayer = (CBasePlayer *) GET_PRIVATE( pev->owner );

	if ( pPlayer )
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX( pev->owner );

	// ALERT(at_console, "Spray by player %i, %i of %i\n", playernum, (int)(pev->frame + 1), nFrames);

	UTIL_MakeVectors( pev->angles );
	UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr );

	// No customization present.
	if ( nFrames == -1 )
	{
		UTIL_DecalTrace( &tr, DECAL_LAMBDA6 );
		UTIL_Remove( this );
	}
	else
	{
		UTIL_PlayerDecalTrace( &tr, playernum, pev->frame, TRUE );
		// Just painted last custom frame.
		if ( pev->frame++ >= ( nFrames - 1 ) )
			UTIL_Remove( this );
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

class CBloodSplat : public CBaseEntity
{
  public:
	void Spawn( entvars_t *pevOwner );
	void Spray( void );
};

void CBloodSplat::Spawn( entvars_t *pevOwner )
{
	pev->origin = pevOwner->origin + Vector( 0, 0, 32 );
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT( pevOwner );

	SetThink( &CBloodSplat::Spray );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBloodSplat::Spray( void )
{
	TraceResult tr;

	if ( g_Language != LANGUAGE_GERMAN )
	{
		UTIL_MakeVectors( pev->angles );
		UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr );

		UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
	}
	SetThink( &CBloodSplat::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

//==============================================


edict_t *CBasePlayer ::GiveNamedItem( const char *pszName )
{
	edict_t *pent = CREATE_NAMED_ENTITY( MAKE_STRING( pszName ) );
	if ( FNullEnt( pent ) )
	{
		ALERT( at_console, "NULL Ent in GiveNamedItem!\n" );
		return NULL;
	}
	VARS( pent )->origin = pev->origin;
	pent->v.spawnflags |= SF_NORESPAWN;
	DispatchSpawn( pent );
	DispatchTouch( pent, ENT( pev ) );
	return pent;
}


CBaseEntity *FindEntityForward( CBaseEntity *pMe )
{
	TraceResult tr;

	UTIL_MakeVectors( pMe->pev->v_angle );
	UTIL_TraceLine( pMe->pev->origin + pMe->pev->view_ofs,
					pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 8192, dont_ignore_monsters,
					pMe->edict(), &tr );
	if ( tr.flFraction != 1.0 && !FNullEnt( tr.pHit ) )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
		return pHit;
	}
	return NULL;
}


BOOL CBasePlayer ::FlashlightIsOn( void )
{
	return FBitSet( pev->effects, EF_DIMLIGHT );
}



void CBasePlayer ::FlashlightTurnOn( void )
{
	if ( g_pGameRules->FAllowFlashlight() )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM );
		SetBits( pev->effects, EF_DIMLIGHT );
	}
}



void CBasePlayer ::FlashlightTurnOff( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM );
	ClearBits( pev->effects, EF_DIMLIGHT );
}


/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
void CBasePlayer ::ForceClientDllUpdate( void )
{
	m_iClientHealth = -1;
	m_fWeapon = FALSE;		// Force weapon send
	m_fKnownItem = FALSE;	// Force weaponinit messages.
	m_fInitHUD = TRUE;		// Force HUD gmsgResetHUD message

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();

	if ( m_pActiveItem )
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *) m_pActiveItem;
		int iClip = pWeapon->m_iClip;

		if ( iClip == WEAPON_NOCLIP )
			iClip = AmmoInventory( GetAmmoIndex( CBasePlayerItem::ItemInfoArray[pWeapon->m_iId].pszAmmo1 ) );

		MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( pWeapon->m_iId );
			WRITE_BYTE( iClip );
		MESSAGE_END();
	}

	if ( g_pGameRules )
		( (CDoDTeamPlay *) g_pGameRules )->SendObjectiveStatus( this );

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgPTeam, NULL, pev );
				WRITE_BYTE( ENTINDEX( pPlayer->edict() ) );
				WRITE_BYTE( pPlayer->pev->team );
			MESSAGE_END();
		}
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgInitHUD, NULL, pev );
	MESSAGE_END();
}

/*
============
ImpulseCommands
============
*/
void CBasePlayer::ImpulseCommands()
{
	TraceResult tr;

	// Handle use events
	PlayerUse();

	switch ( pev->impulse )
	{
	case 201:  // paint decal

		if ( gpGlobals->time < m_flNextDecalTime )
		{
			// too early!
			break;
		}

		UTIL_MakeVectors( pev->v_angle );
		UTIL_TraceLine( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128,
						ignore_monsters, ENT( pev ), &tr );

		if ( tr.flFraction != 1.0 )
		{  // line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time + cvar_decalfrequency.value;
			CSprayCan *pCan = GetClassPtr( (CSprayCan *) NULL );
			pCan->Spawn( pev );
		}

		break;

	case 204:
		ForceClientDllUpdate();
		break;

	case 100:
		// temporary flashlight for level designers
		if ( FlashlightIsOn() )
		{
			FlashlightTurnOff();
		}
		else
		{
			FlashlightTurnOn();
		}

		break;

	default:
		break;
	}

	pev->impulse = 0;
}

//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands( int iImpulse )
{
#if !defined( HLDEMO_BUILD )
	if ( CVAR_GET_FLOAT( "sv_cheats" ) == 0.0 )
	{
		return;
	}

	CBaseEntity *pEntity;
	TraceResult tr;

	switch ( iImpulse )
	{
	case 76:
	{
		if ( !giPrecacheGrunt )
		{
			giPrecacheGrunt = 1;
			ALERT( at_console, "You must now restart to use Grunt-o-matic.\n" );
		}
		else
		{
			UTIL_MakeVectors( Vector( 0, pev->v_angle.y, 0 ) );
			Create( "monster_human_grunt", pev->origin + gpGlobals->v_forward * 128, pev->angles );
		}
		break;
	}

	case 101:
		gEvilImpulse101 = TRUE;
		GiveNamedItem( "item_suit" );
		GiveNamedItem( "item_battery" );
		GiveNamedItem( "weapon_crowbar" );
		GiveNamedItem( "weapon_9mmhandgun" );
		GiveNamedItem( "ammo_9mmclip" );
		GiveNamedItem( "weapon_shotgun" );
		GiveNamedItem( "ammo_buckshot" );
		GiveNamedItem( "weapon_9mmAR" );
		GiveNamedItem( "ammo_9mmAR" );
		GiveNamedItem( "ammo_ARgrenades" );
		GiveNamedItem( "weapon_handgrenade" );
		GiveNamedItem( "weapon_tripmine" );
#ifndef OEM_BUILD
		GiveNamedItem( "weapon_357" );
		GiveNamedItem( "ammo_357" );
		GiveNamedItem( "weapon_crossbow" );
		GiveNamedItem( "ammo_crossbow" );
		GiveNamedItem( "weapon_egon" );
		GiveNamedItem( "weapon_gauss" );
		GiveNamedItem( "ammo_gaussclip" );
		GiveNamedItem( "weapon_rpg" );
		GiveNamedItem( "ammo_rpgclip" );
		GiveNamedItem( "weapon_satchel" );
		GiveNamedItem( "weapon_snark" );
		GiveNamedItem( "weapon_hornetgun" );
#endif
		gEvilImpulse101 = FALSE;
		break;

	case 102:
		// Gibbage!!!
		CGib::SpawnRandomGibs( pev, 1, 1 );
		break;

	case 103:
		// What the hell are you doing?
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer();
			if ( pMonster )
				pMonster->ReportAIState();
		}
		break;

	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			ALERT( at_console, "Classname: %s", STRING( pEntity->pev->classname ) );

			if ( !FStringNull( pEntity->pev->targetname ) )
			{
				ALERT( at_console, " - Targetname: %s\n", STRING( pEntity->pev->targetname ) );
			}
			else
			{
				ALERT( at_console, " - TargetName: No Targetname\n" );
			}

			ALERT( at_console, "Model: %s\n", STRING( pEntity->pev->model ) );
			if ( pEntity->pev->globalname )
				ALERT( at_console, "Globalname: %s\n", STRING( pEntity->pev->globalname ) );
		}
		break;

	case 107:
	{
		TraceResult tr;

		edict_t *pWorld = g_engfuncs.pfnPEntityOfEntIndex( 0 );

		Vector start = pev->origin + pev->view_ofs;
		Vector end = start + gpGlobals->v_forward * 1024;
		UTIL_TraceLine( start, end, ignore_monsters, edict(), &tr );
		if ( tr.pHit )
			pWorld = tr.pHit;
		const char *pTextureName = TRACE_TEXTURE( pWorld, start, end );
		if ( pTextureName )
			ALERT( at_console, "Texture: %s\n", pTextureName );
	}
	break;
	case 195:  // show shortest paths for entire level to nearest node
	{
		Create( "node_viewer_fly", pev->origin, pev->angles );
	}
	break;
	case 196:  // show shortest paths for entire level to nearest node
	{
		Create( "node_viewer_large", pev->origin, pev->angles );
	}
	break;
	case 197:  // show shortest paths for entire level to nearest node
	{
		Create( "node_viewer_human", pev->origin, pev->angles );
	}
	break;
	case 199:  // show nearest node and all connections
	{
		ALERT( at_console, "%d\n", WorldGraph.FindNearestNode( pev->origin, bits_NODE_GROUP_REALM ) );
		WorldGraph.ShowNodeConnections( WorldGraph.FindNearestNode( pev->origin, bits_NODE_GROUP_REALM ) );
	}
	break;
	case 202:  // Random blood splatter
		UTIL_MakeVectors( pev->v_angle );
		UTIL_TraceLine( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128,
						ignore_monsters, ENT( pev ), &tr );

		if ( tr.flFraction != 1.0 )
		{  // line hit something, so paint a decal
			CBloodSplat *pBlood = GetClassPtr( (CBloodSplat *) NULL );
			pBlood->Spawn( pev );
		}
		break;
	case 203:  // remove creature.
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			if ( pEntity->pev->takedamage )
				pEntity->SetThink( &CBaseEntity::SUB_Remove );
		}
		break;
	}
#endif	// HLDEMO_BUILD
}

//
// Add a weapon to the player (Item == Weapon == Selectable Object)
//
int CBasePlayer::AddPlayerItem( CBasePlayerItem *pItem )
{
	CBasePlayerItem *pInsert;

	pInsert = m_rgpPlayerItems[pItem->iItemSlot()];

	while ( pInsert )
	{
		if ( FClassnameIs( pInsert->pev, STRING( pItem->pev->classname ) ) )
		{
			if ( pItem->AddDuplicate( pInsert ) )
			{
				g_pGameRules->PlayerGotWeapon( this, pItem );
				pItem->CheckRespawn();

				// ugly hack to update clip w/o an update clip message
				pInsert->UpdateItemInfo();
				if ( m_pActiveItem )
					m_pActiveItem->UpdateItemInfo();

				pItem->Kill();
			}
			else if ( gEvilImpulse101 )
			{
				// FIXME: remove anyway for deathmatch testing
				pItem->Kill();
			}
			return FALSE;
		}
		pInsert = pInsert->m_pNext;
	}

	if ( pItem->AddToPlayer( this ) )
	{
		g_pGameRules->PlayerGotWeapon( this, pItem );
		pItem->CheckRespawn();

		pItem->m_pNext = m_rgpPlayerItems[pItem->iItemSlot()];
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem;

		// should we switch to this item?
		if ( g_pGameRules->FShouldSwitchWeapon( this, pItem ) )
		{
			SwitchWeapon( pItem, FALSE );
		}

		return TRUE;
	}
	else if ( gEvilImpulse101 )
	{
		// FIXME: remove anyway for deathmatch testing
		pItem->Kill();
	}
	return FALSE;
}

int CBasePlayer::RemovePlayerItem( CBasePlayerItem *pItem )
{
	if ( m_pActiveItem == pItem )
	{
		pItem->Holster();
		pItem->pev->nextthink = 0;	// crowbar may be trying to swing again, etc.
		pItem->SetThink( NULL );
		m_pActiveItem = NULL;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	if ( m_pLastItem == pItem )
		m_pLastItem = NULL;

	CBasePlayerItem *pPrev = m_rgpPlayerItems[pItem->iItemSlot()];

	if ( pPrev == pItem )
	{
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem->m_pNext;
		return TRUE;
	}
	else
	{
		while ( pPrev && pPrev->m_pNext != pItem )
		{
			pPrev = pPrev->m_pNext;
		}
		if ( pPrev )
		{
			pPrev->m_pNext = pItem->m_pNext;
			return TRUE;
		}
	}
	return FALSE;
}

//
// Returns the unique ID for the ammo, or -1 if error
//
int CBasePlayer ::GiveAmmo( int iCount, const char *szName, int iMax )
{
	if ( !szName )
	{
		// no ammo.
		return -1;
	}

	if ( !g_pGameRules->CanHaveAmmo( this, szName, iMax ) )
	{
		// game rules say I can't have any more of this ammo type.
		return -1;
	}

	int i = GetAmmoIndex( szName );

	if ( i < 0 || i >= MAX_AMMO_SLOTS )
		return -1;

	int iAdd = min( iCount, iMax - m_rgAmmo[i] );
	if ( iAdd > 0 )
	{
		m_rgAmmo[i] += iAdd;

		TabulateAmmo();
	}

	return i;
}

/*
============
ItemPreFrame

Called every frame by the player PreThink
============
*/
void CBasePlayer::ItemPreFrame()
{
#if defined( CLIENT_WEAPONS )
	if ( m_flNextAttack > 0 )
#else
	if ( gpGlobals->time < m_flNextAttack )
#endif
	{
		return;
	}

	if ( !m_pActiveItem )
		return;

	m_pActiveItem->ItemPreFrame();
}

/*
============
ItemPostFrame

Called every frame by the player PostThink
============
*/
void CBasePlayer::ItemPostFrame()
{
	// check if the player is using a tank
	if ( !m_pTank && m_flNextAttack <= 0 )
	{
		ImpulseCommands();

		if ( m_pActiveItem )
		{
			UTIL_MakeVectors( pev->v_angle );
			m_pActiveItem->ItemPostFrame();
		}
	}
}

int CBasePlayer::AmmoInventory( int iAmmoIndex )
{
	if ( iAmmoIndex == -1 )
	{
		return -1;
	}

	return m_rgAmmo[iAmmoIndex];
}

int CBasePlayer::GetAmmoIndex( const char *psz )
{
	int i;

	if ( !psz )
		return -1;

	for ( i = 1; i < MAX_AMMO_SLOTS; i++ )
	{
		if ( !CBasePlayerItem::AmmoInfoArray[i].pszName )
			continue;

		if ( stricmp( psz, CBasePlayerItem::AmmoInfoArray[i].pszName ) == 0 )
			return i;
	}

	return -1;
}

// Called from UpdateClientData
// makes sure the client has all the necessary ammo info,  if values have changed
void CBasePlayer::SendAmmoUpdate( void )
{
	for ( int i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		if ( m_rgAmmo[i] != m_rgAmmoLast[i] )
		{
			m_rgAmmoLast[i] = m_rgAmmo[i];

			if ( m_rgAmmo[i] > 254 )
			{
				// does not fit into a byte
				MESSAGE_BEGIN( MSG_ONE, gmsgAmmoShort, NULL, pev );
				WRITE_BYTE( i );
				WRITE_SHORT( m_rgAmmo[i] );
				MESSAGE_END();
			}
			else
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgAmmoX, NULL, pev );
				WRITE_BYTE( i );
				WRITE_BYTE( max( m_rgAmmo[i], 0 ) );
				MESSAGE_END();
			}
		}
	}
}

/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/

void CBasePlayer ::UpdateClientData( void )
{
	if ( m_fInitHUD )
	{
		m_fInitHUD = FALSE;
		gInitHUD = FALSE;

		m_signals.Update();

		MESSAGE_BEGIN( MSG_ONE, gmsgResetHUD, NULL, pev );
		MESSAGE_END();

		if ( !m_fGameHUDInitialized )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgInitHUD, NULL, pev );
			MESSAGE_END();

			g_pGameRules->InitHUD( this );
			m_fGameHUDInitialized = TRUE;

			if ( g_pGameRules->IsMultiplayer() )
			{
				FireTargets( "game_playerjoin", this, this, USE_TOGGLE, 0 );
			}

			m_iObserverLastMode = OBS_ROAMING;

			( (CDoDTeamPlay *) g_pGameRules )->SendObjectiveStatus( this );
		}

		FireTargets( "game_playerspawn", this, this, USE_TOGGLE, 0 );

		if ( pev->team == TEAM_ALLIES )
		{
			if ( !pev->iuser1 && !pev->iuser2 )
				FireTargets( "dod_alliespawn", this, this, USE_TOGGLE, 0 );
		}
		else if ( pev->team == TEAM_AXIS && !pev->iuser1 && !pev->iuser2 )
		{
			FireTargets( "dod_axisspawn", this, this, USE_TOGGLE, 0 );
		}

		InitStatusBar();

		( (CDoDTeamPlay *) g_pGameRules )->SendClientGamerules( this );

		int iSleeve = 2;

		if ( pev->team == TEAM_ALLIES )
			iSleeve = (unsigned int) ( pev->playerclass - 21 ) <= 5;

		MESSAGE_BEGIN( MSG_ONE, SVC_WEAPONANIM, NULL, pev );
			WRITE_BYTE( pev->weaponanim );
			WRITE_BYTE( iSleeve );
		MESSAGE_END();
	}

	if ( m_iHideHUD != m_iClientHideHUD )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgHideWeapon, NULL, pev );
			WRITE_BYTE( m_iHideHUD );
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if ( m_iFOV != m_iClientFOV )
	{
		pev->fov = m_iFOV;

		MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
			WRITE_BYTE( m_iFOV );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_SPEC, gmsgHLTV );
			WRITE_BYTE( ENTINDEX( edict() ) );
			WRITE_BYTE( m_iFOV );
		MESSAGE_END();
	}

	// HACKHACK -- send the message to display the game title
	if ( pev->health != m_iClientHealth )
	{
		int iHealth = clamp( (int) pev->health, 0, 255 );  // make sure that no negative health values are sent

		// send "health" update message
		if ( pev->health > 0 && pev->health <= 1 )
			iHealth = 1;

		MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
			WRITE_BYTE( iHealth );
		MESSAGE_END();

		m_iClientHealth = pev->health;
	}

	if ( pev->dmg_take || pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType )
	{
		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED | DMG_BLAST;
	}

	if ( !m_fKnownItem )
	{
		m_fKnownItem = TRUE;

		// Send ALL the weapon info now
		for ( int i = 0; i < MAX_WEAPONS; i++ )
		{
			ItemInfo &II = CBasePlayerItem::ItemInfoArray[i];

			if ( !II.iId )
				continue;

			MESSAGE_BEGIN( MSG_ONE, gmsgWeaponList, NULL, pev );
				WRITE_BYTE( GetAmmoIndex( II.pszAmmo1 ) );
				WRITE_BYTE( II.iMaxAmmo1 );
				WRITE_BYTE( GetAmmoIndex( II.pszAmmo2 ) );
				WRITE_BYTE( II.iMaxAmmo2 );
				WRITE_BYTE( II.iSlot );
				WRITE_BYTE( II.iPosition );
				WRITE_SHORT( II.iId );
				WRITE_BYTE( II.iFlags );
				WRITE_BYTE( II.iMaxClip );
			MESSAGE_END();
		}
	}

	SendAmmoUpdate();

	// Update all the items
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if ( m_rgpPlayerItems[i] )  // each item updates it's successors
			m_rgpPlayerItems[i]->UpdateClientData( this );
	}

	// Cache and client weapon change
	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;

	// Update Status Bar
	if ( m_flNextSBarUpdateTime < gpGlobals->time )
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.5;
	}

	if ( !( m_flDisplayHistory & HINT_OUT_OF_AMMO ) && m_pActiveItem &&
		 !( CBasePlayerItem::ItemInfoArray[m_pActiveItem->m_iId].iFlags & ITEM_FLAG_EXHAUSTIBLE ) )
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *) m_pActiveItem;

		if ( pWeapon->m_iPrimaryAmmoType != -1 && !m_rgAmmo[pWeapon->m_iPrimaryAmmoType] && !pWeapon->m_iClip )
		{
			m_flDisplayHistory |= HINT_OUT_OF_AMMO;
			HintMessage( "#Hint_out_of_ammo", 0 );
		}
	}

	if ( gpGlobals->time > m_fHandleSignalsTime )
	{
		m_fHandleSignalsTime = gpGlobals->time + 0.5;
		HandleSignals();
	}
}


void CBasePlayer ::SetPrefsFromUserinfo( char *infobuffer )
{
	const char *pszKeyVal;

	pszKeyVal = g_engfuncs.pfnInfoKeyValue( infobuffer, "_ah" );
	if ( pszKeyVal[0] == '\0' )
		m_bShowHints = TRUE;
	else
		m_bShowHints = ( atoi( pszKeyVal ) != 0 );

	pszKeyVal = g_engfuncs.pfnInfoKeyValue( infobuffer, "cl_autoreload" );
	if ( pszKeyVal[0] == '\0' )
		m_bAutoReload = TRUE;
	else
		m_bAutoReload = ( atoi( pszKeyVal ) != 0 );

	pszKeyVal = g_engfuncs.pfnInfoKeyValue( infobuffer, "_cl_minimap" );
	if ( pszKeyVal[0] == '\0' )
		m_iMinimap = 0;
	else
		m_iMinimap = atoi( pszKeyVal );
}

//=========================================================
// FBecomeProne - Overridden for the player to set the proper
// physics flags when a barnacle grabs player.
//=========================================================
BOOL CBasePlayer ::FBecomeProne( void )
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	return TRUE;
}

//=========================================================
// BarnacleVictimBitten - bad name for a function that is called
// by Barnacle victims when the barnacle pulls their head
// into its mouth. For the player, just die.
//=========================================================

//=========================================================
// BarnacleVictimReleased - overridden for player who has
// physics flags concerns.
//=========================================================

//=========================================================
// Illumination
// return player light level plus virtual muzzle flash
//=========================================================

void CBasePlayer ::EnableControl( BOOL fControl )
{
	if ( !fControl )
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;
}

#define DOT_1DEGREE 0.9998476951564
#define DOT_2DEGREE 0.9993908270191
#define DOT_3DEGREE 0.9986295347546
#define DOT_4DEGREE 0.9975640502598
#define DOT_5DEGREE 0.9961946980917
#define DOT_6DEGREE 0.9945218953683
#define DOT_7DEGREE 0.9925461516413
#define DOT_8DEGREE 0.9902680687416
#define DOT_9DEGREE 0.9876883405951
#define DOT_10DEGREE 0.9848077530122
#define DOT_15DEGREE 0.9659258262891
#define DOT_20DEGREE 0.9396926207859
#define DOT_25DEGREE 0.9063077870367

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================



/*
=============
SetCustomDecalFrames

  UNDONE:  Determine real frame limit, 8 is a placeholder.
  Note:  -1 means no custom frames present.
=============
*/
void CBasePlayer ::SetCustomDecalFrames( int nFrames )
{
	if ( nFrames > 0 && nFrames < 8 )
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

/*
=============
GetCustomDecalFrames

  Returns the # of custom frames this player's custom clan logo contains.
=============
*/
int CBasePlayer ::GetCustomDecalFrames( void )
{
	return m_nCustomSprayFrames;
}

//=========================================================
// DropPlayerItem - drop the named item, or if no name,
// the active item.
//=========================================================


// DoD: 2-arg overload declared in player.h but never defined (link gap in dodsrc too).
// bRemove is reserved; behavior matches the single-arg drop.

void CBasePlayer::DropPlayerItem( char *pszItemName, bool bForceDrop )
{
	if ( !pszItemName[0] )
	{
		// no name means drop the active item
		pszItemName = NULL;
	}
	else if ( !strcmp( pszItemName, "weapon_handgrenade_ex" ) || !strcmp( pszItemName, "weapon_stickgrenade_ex" ) )
	{
		return;
	}

	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		CBasePlayerItem *pWeapon = m_rgpPlayerItems[i];

		while ( pWeapon )
		{
			if ( pszItemName )
			{
				if ( !strcmp( pszItemName, STRING( pWeapon->pev->classname ) ) )
					break;
			}
			else
			{
				if ( pWeapon == m_pActiveItem )
					break;
			}

			pWeapon = pWeapon->m_pNext;
		}

		if ( !pWeapon )
			continue;

		if ( !pWeapon->CanDrop() && !bForceDrop )
		{
			if ( pWeapon->Classify() == CLASS_ROCKET || pWeapon->Classify() == CLASS_MACHINEGUN )
				HintMessage( "#game_cannot_drop_while", 0 );
			else
				HintMessage( "#game_cannot_drop", 0 );

			return;
		}

		pWeapon->Holster();

		g_pGameRules->GetNextBestWeapon( this, pWeapon );

		UTIL_MakeVectors( pev->angles );

		Vector vecOrigin = pev->origin + gpGlobals->v_forward * 10;

		CWeaponBox *pWeaponBox = (CWeaponBox *) CBaseEntity::Create( "weaponbox", vecOrigin, pev->angles, edict() );

		pWeaponBox->pev->angles.x = 0;
		pWeaponBox->pev->angles.z = 0;
		pWeaponBox->SetThink( &CWeaponBox::ActivateThink );
		pWeaponBox->m_bTouch = FALSE;
		pWeaponBox->pev->nextthink = gpGlobals->time + 1;

		( (CBasePlayerWeapon *) pWeapon )->m_flNextPrimaryAttack = 0;
		( (CBasePlayerWeapon *) pWeapon )->m_flNextSecondaryAttack = 0;
		( (CBasePlayerWeapon *) pWeapon )->m_flTimeWeaponIdle = 0;

		if ( !pWeaponBox->PackWeapon( pWeapon ) )
		{
			UTIL_Remove( pWeaponBox );
			return;
		}

		ClearHasWeapon( pWeapon->m_iId );

		pWeaponBox->pev->velocity = gpGlobals->v_forward * 200 + gpGlobals->v_right * RANDOM_FLOAT( -80, 80 );

		const char *pszAmmo = CBasePlayerItem::ItemInfoArray[pWeapon->m_iId].pszAmmo1;

		if ( CBasePlayerItem::ItemInfoArray[pWeapon->m_iId].iFlags & ITEM_FLAG_EXHAUSTIBLE )
		{
			if ( pszAmmo )
			{
				int iAmmoIndex = GetAmmoIndex( pszAmmo );

				if ( iAmmoIndex != -1 )
				{
					pWeaponBox->PackAmmo( MAKE_STRING( pszAmmo ), m_rgAmmo[iAmmoIndex] > 0 );
					m_rgAmmo[iAmmoIndex] = 0;
				}
			}
		}
		else if ( pszAmmo )
		{
			int iAmmoIndex = GetAmmoIndex( pszAmmo );

			if ( iAmmoIndex != -1 )
			{
				pWeaponBox->PackAmmo( MAKE_STRING( pszAmmo ), m_rgAmmo[iAmmoIndex] );
				m_rgAmmo[iAmmoIndex] = 0;
			}
		}

		int iId = pWeapon->m_iId;
		int iModel = iId;

		if ( iId == WEAPON_M1CARBINE )
		{
			if ( g_pGameRules && ( (CDoDTeamPlay *) g_pGameRules )->IsTeamPara( pev->team ) )
				iModel = WEAPON_FOLDINGCARBINE;
		}
		else if ( iId == WEAPON_FG42 )
		{
			if ( ( (CBasePlayerWeapon *) pWeapon )->m_iWeaponState & 1 )
				iModel = WEAPON_SCOPED_FG42;
		}
		else if ( iId == WEAPON_ENFIELD )
		{
			if ( ( (CBasePlayerWeapon *) pWeapon )->m_iWeaponState & 1 )
				iModel = WEAPON_SCOPEDENFIELD;
		}

		SET_MODEL( pWeaponBox->edict(), P_WpnInfo[iModel].wmodel );

		return;
	}
}


//=========================================================
// HasPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasPlayerItem( CBasePlayerItem *pCheckItem )
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while ( pItem )
	{
		if ( FClassnameIs( pItem->pev, STRING( pCheckItem->pev->classname ) ) )
		{
			return TRUE;
		}
		pItem = pItem->m_pNext;
	}

	return FALSE;
}

//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasNamedPlayerItem( const char *pszItemName )
{
	CBasePlayerItem *pItem;
	int i;

	for ( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		pItem = m_rgpPlayerItems[i];

		while ( pItem )
		{
			if ( !strcmp( pszItemName, STRING( pItem->pev->classname ) ) )
			{
				return TRUE;
			}
			pItem = pItem->m_pNext;
		}
	}

	return FALSE;
}

//=========================================================
// HasPlayerItemFromID
// Just compare IDs, rather than classnames
//=========================================================

//=========================================================
//
//=========================================================

BOOL CBasePlayer ::SwitchWeapon( CBasePlayerItem *pWeapon, BOOL bJustSpawned )
{
	if ( !pWeapon->CanDeploy() )
		return FALSE;
	if ( m_pActiveItem )
		m_pActiveItem->Holster();
	m_pActiveItem = pWeapon;
	pWeapon->Deploy();
	return TRUE;
}


//=========================================================
// Dead HEV suit prop
//=========================================================
class CDeadHEV : public CBaseMonster
{
  public:
	void Spawn( void );
	int Classify( void ) { return CLASS_HUMAN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );

	int m_iPose;  // which sequence to display	-- temporary, don't need to save
 	static const char *m_szPoses[4];
};

const char *CDeadHEV::m_szPoses[] = { "deadback", "deadsitting", "deadstomach", "deadtable" };

void CDeadHEV::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_hevsuit_dead, CDeadHEV );

//=========================================================
// ********** DeadHEV SPAWN **********
//=========================================================
void CDeadHEV ::Spawn( void )
{
	PRECACHE_MODEL( "models/player.mdl" );
	SET_MODEL( ENT( pev ), "models/player.mdl" );

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	pev->body = 1;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if ( pev->sequence == -1 )
	{
		ALERT( at_console, "Dead hevsuit with bad pose\n" );
		pev->sequence = 0;
		pev->effects = EF_BRIGHTFIELD;
	}

	// Corpses have less health
	pev->health = 8;

	MonsterInitDead();
}

class CStripWeapons : public CPointEntity
{
  public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

  private:
};

LINK_ENTITY_TO_CLASS( player_weaponstrip, CStripWeapons );

void CStripWeapons ::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = NULL;

	if ( pActivator && pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *) pActivator;
	}
	else if ( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = (CBasePlayer *) CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	}

	if ( pPlayer )
		pPlayer->RemoveAllItems( FALSE );
}

class CRevertSaved : public CPointEntity
{
  public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT MessageThink( void );
	void EXPORT LoadThink( void );
	void KeyValue( KeyValueData *pkvd );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	inline float Duration( void ) { return pev->dmg_take; }
	inline float HoldTime( void ) { return pev->dmg_save; }
	inline float MessageTime( void ) { return m_messageTime; }
	inline float LoadTime( void ) { return m_loadTime; }

	inline void SetDuration( float duration ) { pev->dmg_take = duration; }
	inline void SetHoldTime( float hold ) { pev->dmg_save = hold; }
	inline void SetMessageTime( float time ) { m_messageTime = time; }
	inline void SetLoadTime( float time ) { m_loadTime = time; }

  private:
	float m_messageTime;
	float m_loadTime;
};

LINK_ENTITY_TO_CLASS( player_loadsaved, CRevertSaved );

TYPEDESCRIPTION CRevertSaved::m_SaveData[] = {
	DEFINE_FIELD( CRevertSaved, m_messageTime,
				  FIELD_FLOAT ),  // These are not actual times, but durations, so save as floats
	DEFINE_FIELD( CRevertSaved, m_loadTime, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CRevertSaved, CPointEntity );

void CRevertSaved ::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "duration" ) )
	{
		SetDuration( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "holdtime" ) )
	{
		SetHoldTime( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "messagetime" ) )
	{
		SetMessageTime( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "loadtime" ) )
	{
		SetLoadTime( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CRevertSaved ::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	UTIL_ScreenFadeAll( pev->rendercolor, Duration(), HoldTime(), pev->renderamt, FFADE_OUT );
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink( &CRevertSaved::MessageThink );
}

void CRevertSaved ::MessageThink( void )
{
	UTIL_ShowMessageAll( STRING( pev->message ) );
	float nextThink = LoadTime() - MessageTime();
	if ( nextThink > 0 )
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink( &CRevertSaved::LoadThink );
	}
	else
		LoadThink();
}

void CRevertSaved ::LoadThink( void )
{
	if ( !gpGlobals->deathmatch )
	{
		SERVER_COMMAND( "reload\n" );
	}
}

//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission : public CPointEntity
{
	void Spawn( void );
	void Think( void );
};

void CInfoIntermission::Spawn( void )
{
	UTIL_SetOrigin( pev, pev->origin );
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2;  // let targets spawn!
}

void CInfoIntermission::Think( void )
{
	CBaseEntity *pTarget;

	// find my target
	pTarget = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );

	if ( pTarget )
	{
		pev->v_angle = UTIL_VecToAngles( ( pTarget->pev->origin - pev->origin ).Normalize() );
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS( info_intermission, CInfoIntermission );

// DoD-only (player.cpp:5171 Stamina, 6513 CheckPlayerSpeed, 6587 SetSpeed) - PORTED
void CBasePlayer::Stamina( void )
{
	if ( !IsAlive() )
		return;

	if ( !( m_flDisplayHistory & HINT_LOW_STAMINA ) && pev->fuser4 < 25.0f )
	{
		if ( m_bShowHints )
			HintMessage( "#Hint_low_stamina", 0 );
		m_flDisplayHistory |= HINT_LOW_STAMINA;
	}

	if ( m_fNextStamina > gpGlobals->time )
		return;

	float speed2d = sqrt( pev->velocity.x * pev->velocity.x + pev->velocity.y * pev->velocity.y );
	float delta = 6.0f;	// idle recovery
	if ( speed2d > 0.0f )
	{
		delta = 1.0f;  // moving recovery
		if ( ( pev->button & IN_RUN ) && ( pev->flags & FL_ONGROUND ) )
			delta = 5.0f;  // sprinting recovery
	}
	pev->fuser4 += delta;
	if ( pev->fuser4 < 0.0f )
		pev->fuser4 = 0.0f;
	else if ( pev->fuser4 > 100.0f )
		pev->fuser4 = 100.0f;
	m_fNextStamina = gpGlobals->time + 0.1f;
}
bool CBasePlayer::CheckPlayerSpeed( void )
{
	if ( m_bSlowedByHit && m_flUnslowTime <= gpGlobals->time )
		m_bSlowedByHit = false;

	BOOL bBazookaDeployed = FALSE;
	if ( m_bBazookaDeployed && m_pActiveItem && m_pActiveItem->Classify() == CLASS_ROCKET )
		bBazookaDeployed = TRUE;

	int speed = 350;
	if ( IsAlive() )
	{
		speed = 1;
		if ( !( (CDoDTeamPlay *) g_pGameRules )->IsRoundFrozen() )
		{
			if ( pev->vuser1.x != SANDBAG_DEPLOYED )
			{
				int iUser3 = pev->iuser3;
				speed = 1;
				if ( iUser3 != 2 && iUser3 != 3 )
				{
					if ( pev->deadflag == DEAD_NO && (unsigned) ( iUser3 - 1 ) < 2 )
					{
						speed = 30 + ( ( m_iFOV < 1 ) ? 20 : 0 );
					}
					else
					{
						speed = 42;
						if ( m_iFOV < 1 )
						{
							speed = 50;
							if ( !bBazookaDeployed )
							{
								float stamina = pev->fuser4;
								float f;
								if ( stamina < 10.0f )
									f = 0.3f;
								else if ( stamina < 25.0f )
									f = 0.4f;
								else if ( stamina < 37.0f )
									f = 0.55f;
								else if ( stamina < 50.0f )
									f = 0.75f;
								else
									f = 1.0f;
								if ( m_bSlowedByHit )
									f = 0.2f;
								speed = (int) ( (float) m_iClassSpeed * f );
							}
						}
					}
				}
			}
		}
	}
	if ( m_iPlayerSpeed != speed )
	{
		ALERT( at_aiconsole, "Changing max speed to %d\n", speed );
		g_engfuncs.pfnSetClientMaxspeed( ENT( pev ), (float) speed );
		m_iPlayerSpeed = speed;
		return true;
	}
	return false;
}
bool CBasePlayer::SetSpeed( int speed )
{
	ALERT( at_aiconsole, "Changing max speed to %d\n", speed );
	g_engfuncs.pfnSetClientMaxspeed( ENT( pev ), (float) speed );
	m_iPlayerSpeed = speed;
	return true;
}

// DoD hint system - RESTORED from dod.so player.cpp 6308-6331.
bool CBasePlayer::HintMessage( const char *pMessage, BOOL bOverride )
{
	char tmp[512];

	if ( !m_bShowHints )
		return true;

	MESSAGE_BEGIN( MSG_ONE, gmsgHudText, NULL, pev );

	if ( strlen( pMessage ) > 499 )
	{
		strncpy( tmp, pMessage, 499 );
		tmp[499] = 0;
		pMessage = tmp;
	}

	WRITE_STRING( pMessage );
	WRITE_BYTE( 1 );
	MESSAGE_END();

	return true;
}

void CBasePlayer::GoProne( void )
{
	pev->iuser3 = STANCE_PRONE;
	m_flNextAttack = 0.1f;
	SetAnimation( PLAYER_PRONE1 );
	m_inextprone = gpGlobals->time + 1.0f;
	if ( m_iFOV < 1 )
		return;
	for ( int i = 0; i < 6; i++ )
	{
		for ( CBasePlayerItem *pItem = m_rgpPlayerItems[i]; pItem; pItem = pItem->m_pNext )
		{
			if ( CBasePlayerItem::ItemInfoArray[pItem->m_iId].iFlags & ITEM_FLAG_PRIMARY )
			{
				m_iFOV = ( (CBasePlayerWeapon *)pItem )->GetFOV();
				return;
			}
		}
	}
}

void CBasePlayer::MortarDeploy( void )
{
	if ( pev->deadflag == DEAD_NO && ( STANCE_IS_PRONE( pev->iuser3 ) ) )
	{
		pev->iuser3 = STANCE_STANDING;
		m_ianimupdate = 1;
		m_flNextAttack = 0.1f;
		SetAnimation( PLAYER_PRONE3 );
		m_inextprone = gpGlobals->time + 1.0f;
		if ( m_iFOV > 0 )
		{
			for ( int i = 0; i < 6; i++ )
			{
				BOOL fFound = FALSE;
				for ( CBasePlayerItem *pItem = m_rgpPlayerItems[i]; pItem; pItem = pItem->m_pNext )
				{
					if ( CBasePlayerItem::ItemInfoArray[pItem->m_iId].iFlags & ITEM_FLAG_PRIMARY )
					{
						m_iFOV = ( (CBasePlayerWeapon *)pItem )->GetFOV();
						fFound = TRUE;
						break;
					}
				}
				if ( fFound )
					break;
			}
		}
	}
	pev->iuser3 = STANCE_MORTAR;
	CheckPlayerSpeed();
}

extern int iVoiceToHandSignal[29];	// defined in dod_shared.cpp

void CBasePlayer::VoiceCom( int command )
{
	if ( pev->iuser1 != 0 || pev->iuser2 != 0 )
		return;
	if ( g_fGameOver )
		return;
	if ( pev->deadflag != DEAD_NO )
		return;
	if ( m_flNextVoice > gpGlobals->time )
		return;
	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;
	if ( pev->team == TEAM_ALLIES && pRules->IsAlliesBrit() )
	{
		unsigned short evt = pRules->usVoice();
		PLAYBACK_EVENT_FULL( 0, ENT( pev ), evt, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, command, 1, 0, 0 );
	}
	else
	{
		unsigned short evt = pRules->gerVoice();
		PLAYBACK_EVENT_FULL( 0, ENT( pev ), evt, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, command, 0, 0, 0 );
	}
	m_handSignal = iVoiceToHandSignal[command];
	SetAnimation( PLAYER_SIGNAL );
	m_flNextVoice = gpGlobals->time + 1.0f;
}

void CBasePlayer::PushPlayer( void )
{
	TraceResult tr;
	Vector vecSrc, vecEnd;
	UTIL_MakeVectors( pev->v_angle );
	vecSrc = GetGunPosition();
	vecEnd = vecSrc + gpGlobals->v_forward * 32.0f;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
	if ( tr.flFraction >= 1.0f )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, 3, ENT( pev ), &tr );
		if ( tr.flFraction >= 1.0f )
			return;
	}
	CBaseEntity *pEnt = CBaseEntity::Instance( tr.pHit );
	if ( pEnt && pEnt->Classify() == CLASS_PLAYER )
	{
		pEnt->pev->velocity.x = pev->velocity.x * 1.2f;
		pEnt->pev->velocity.y = pev->velocity.y * 1.2f;
		pEnt->pev->velocity.z = pev->velocity.z * 1.2f;
	}
}


// ============================================================================
// DoD-only CBasePlayer methods ported from dod.so (player.cpp). LINK addrs are
// in the .text comments; Ghidra addr = link + 0x10000.
// ============================================================================

const char *g_PlayerAnimationNames[34] = {
	"", "stand_%s_aim", "sprint_%s_aim", "crouch_%s_aim", "prone_%s_aim", "bipod_%s_aim", "sandbag_%s_aim",
	"stand_%s_zoom", "crouch_%s_zoom", "prone_%s_zoom", "stand_%s_shoot", "sprint_%s_shoot", "crouch_%s_shoot",
	"prone_%s_shoot", "bipod_%s_shoot", "sandbag_%s_shoot", "stand_%s_stab", "crouch_%s_stab", "prone_%s_stab",
	"prone_forward", "deep_idle", "stand_%s_reload", "crouch_%s_reload", "prone_%s_reload", "bipod_%s_reload",
	"sandbag_%s_reload", "stand_%s_roll", "crouch_%s_roll", "stand_%s_zoomload", "crouch_%s_zoomload",
	"prone_%s_zoomload", "stand_%s_swing", "crouch_%s_swing", "prone_%s_swing",
};

const char *sIdleAnimations[5] = { "dod_idle1", "dod_idle2", "dod_idle3", "dod_idle4", "dod_idle5" };
const char *g_PlayerAnimationList[88] = {
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "", "", "", "die_gutshot", "die_backwards", "die_forwards", "",
	"", "", "", "", "", "", "", "",
	"", "", "swim_idle", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "", "die_headshot", "die_gutshot", "die_gutshot", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "die_crouch_head", "die_crouch_backwards", "die_prone1", "die_backward_explode", "die_forward_explode", "die_simple", "die_buttstock",
};

bool CBasePlayer::IsInMGDeploy()
{
	return pev->iuser3 == STANCE_PRONE_DEPLOYED || pev->vuser1.x == SANDBAG_DEPLOYED;
}

void CBasePlayer::UpdateScope( int weapon_id )
{
	if ( pev->vuser1.z != 0.0f )
		return;
	MESSAGE_BEGIN( MSG_ONE, gmsgScope, NULL, pev );
	WRITE_BYTE( weapon_id );
	MESSAGE_END();
}

void CBasePlayer::AddFrags( int num, BOOL bSendMsgToAll )
{
	if ( !g_pGameRules->IsClanMatch() || g_pGameRules->IsClanMatchActive() )
	{
		pev->frags += num;

		if ( bSendMsgToAll )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgFrags );
			WRITE_BYTE( ENTINDEX( edict() ) );
			WRITE_SHORT( (int) pev->frags );
			MESSAGE_END();
		}
	}
}

void CBasePlayer::AddObjScore( int num, BOOL bSendMsgToAll )
{
	if ( !g_pGameRules->IsClanMatch() || g_pGameRules->IsClanMatchActive() )
	{
		m_iObjScore += num;

		if ( bSendMsgToAll )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgObjScore );
			WRITE_BYTE( ENTINDEX( edict() ) );
			WRITE_SHORT( m_iObjScore );
			MESSAGE_END();
		}
	}
}

void CBasePlayer::DelayedSpawn( void )
{
	SetThink( &CBasePlayer::SpawnThink );
	pev->nextthink = gpGlobals->time + 1.0;
}

void CBasePlayer::SpawnThink( void )
{
	edict_t *pSpawn = EntSelectSpawnPoint( this, 1 );
	if ( pSpawn )
	{
		StopObserver();
		RemoveAllItems( 1 );
		Spawn();
		return;
	}
	pev->nextthink = gpGlobals->time + 1.0;
}

bool CBasePlayer::PlayerHasExplodingGrenades()
{
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		for ( CBasePlayerItem *pItem = m_rgpPlayerItems[i]; pItem; pItem = pItem->m_pNext )
		{
			if ( pItem->Classify() == CLASS_GRENADE_EX && m_rgAmmo[pItem->PrimaryAmmoIndex()] > 0 )
				return true;
		}
	}
	return false;
}

void CBasePlayer::SetClientAreaIcon( int areaIndex, bool state )
{
	if ( areaIndex > 0x80 )
		return;
	MESSAGE_BEGIN( MSG_ONE, gmsgClientAreas, NULL, pev );
	WRITE_BYTE( areaIndex );
	WRITE_BYTE( state );
	MESSAGE_END();
}

void CBasePlayer::AddObjectIcon( char *szIconName )
{
	if ( !szIconName )
		return;
	MESSAGE_BEGIN( MSG_ONE, gmsgObject, NULL, pev );
	WRITE_STRING( szIconName );
	MESSAGE_END();
}

void CBasePlayer::RemoveObjectIcon()
{
	MESSAGE_BEGIN( MSG_ONE, gmsgObject, NULL, pev );
	WRITE_STRING( "" );
	MESSAGE_END();
}

void CBasePlayer::GetObject( CObject *pObject )
{
	if ( HasObject || !IsAlive() )
		return;
	HasObject = 1;
	m_pObject = pObject;
	if ( pObject->m_szObjectHIcon )
		AddObjectIcon( pObject->m_szObjectHIcon );
	pObject->SendPickupMsg( this );
	const char *pszTeam = ( pev->team == TEAM_ALLIES ) ? "Allies" : "Axis";
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"dod_object\"\n", STRING( pev->netname ),
					GETPLAYERUSERID( edict() ), GETPLAYERAUTHID( edict() ), pszTeam );
	EMIT_SOUND_DYN( edict(), CHAN_ITEM, "items/ammopickup.wav", 1.0, 0.8, 0, 100 );
	int entindex = ENTINDEX( edict() );
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );
		if ( plr && plr->pev->team == pev->team )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgPStatus, NULL, plr->pev );
			WRITE_BYTE( entindex );
			WRITE_BYTE( PlayerStatus() );
			MESSAGE_END();
		}
	}
}

CBasePlayerWeapon *CBasePlayer::GetPrimaryWeapon()
{
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
		for ( CBasePlayerItem *pItem = m_rgpPlayerItems[i]; pItem; pItem = pItem->m_pNext )
			if ( CBasePlayerItem::ItemInfoArray[pItem->m_iId].iFlags & ITEM_FLAG_PRIMARY )
				return (CBasePlayerWeapon *) pItem;
	return NULL;
}

int CBasePlayer::PlayerStatus()
{
	if ( HasObject && pev->deadflag == DEAD_NO )
		return 5;
	if ( pev->iuser1 >= 1 )
		return 1;
	return IsAlive() ? 0 : 1;
}

void CBasePlayer::HandleSignals()
{
	int changed = m_signals.Update();
	int sig = m_signals.GetState();

	if ( changed & SIGNAL_SANDBAG )
	{
		if ( !( sig & SIGNAL_SANDBAG ) )
			pev->vuser1.x = SANDBAG_NONE;
		else if ( pev->vuser1.x != SANDBAG_DEPLOYED )
			pev->vuser1.x = SANDBAG_ZONE;
	}
	if ( ( changed & SIGNAL_CAPTUREAREA ) && !( sig & SIGNAL_CAPTUREAREA ) )
	{
		if ( m_iCapAreaIconIndex < 0x81 )
			SetClientAreaIcon( m_iCapAreaIconIndex, false );
		m_iCapAreaIndex = -1;
		m_iCapAreaIconIndex = -1;
	}
	if ( ( changed & SIGNAL_OBJECTAREA ) && !( sig & SIGNAL_OBJECTAREA ) )
	{
		if ( m_iObjectAreaIndex < 0x81 )
			SetClientAreaIcon( m_iObjectAreaIndex, false );
		m_iObjectAreaIndex = -1;
	}
}

bool CBasePlayer::IsProneDeployed() { return pev->iuser3 == STANCE_PRONE_DEPLOYED; }
bool CBasePlayer::IsInSandbagZone() { return pev->vuser1.x == 1.0f; }
bool CBasePlayer::IsSandbagDeployed() { return pev->vuser1.x == SANDBAG_DEPLOYED; }
bool CBasePlayer::IsProne() { return pev->deadflag == DEAD_NO && STANCE_IS_PRONE( pev->iuser3 ); }
bool CBasePlayer::IsInMortarDeploy() { return pev->iuser3 == 3; }
void CBasePlayer::SetProneState( int state ) { pev->iuser3 = state; }
void CBasePlayer::SetSandbagState( int state ) { pev->vuser1.x = (float) state; }
bool CBasePlayer::IsDucking() { return ( pev->flags & FL_DUCKING ) != 0; }
bool CBasePlayer::IsParatrooper()
{
	return g_pGameRules != NULL && ( (CDoDTeamPlay *) g_pGameRules )->IsTeamPara( pev->team );
}
bool CBasePlayer::IsBritish() { return (unsigned) ( pev->playerclass - 0x15 ) < 6; }
void CBasePlayer::MortarFireCommand( byte command ) { }
void CBasePlayer::MortarUnDeploy()
{
	pev->iuser3 = 0;
	SetAnimation( PLAYER_WALK );
	CheckPlayerSpeed();
}
void CBasePlayer::UnProne()
{
	pev->iuser3 = STANCE_STANDING;
	m_ianimupdate = 1;
	m_flNextAttack = 0.1f;
	SetAnimation( PLAYER_PRONE3 );
	m_inextprone = gpGlobals->time + 1.0f;
	if ( m_iFOV < 1 )
		return;
	CBasePlayerWeapon *pWpn = GetPrimaryWeapon();
	if ( pWpn )
		m_iFOV = pWpn->GetFOV();
}
Activity CBasePlayer::SetIdealActivity( Activity i_RequestedActivity )
{
	Activity cur = m_IdealActivity;
	switch ( cur )
	{
	case ACT_SWIM:
	case ACT_HOVER:
	case ACT_HOP:
	case ACT_LEAP:
	case DOD_CROUCH_BACKWARD:
	case DOD_SIMPLE:
	case DOD_PRONE1:
	case ACT_DIESIMPLE:
	case DOD_CROUCH_HEADSHOT:
	case ACT_DIE_HEADSHOT:
	case DOD_EXPLOSION_FORWARD:
	case DOD_EXPLOSION_BACK:
	case ACT_DIE_CHESTSHOT:
	case ACT_DIE_GUTSHOT:
	case ACT_DIEFORWARD:
	case ACT_DIEBACKWARD:
	case DOD_BUTTSTOCK:
		return cur;
	default:
		return i_RequestedActivity;
	}
}
void CBasePlayer::ForceZoomOut()
{
	if ( m_iFOV < 1 )
		return;
	CBasePlayerWeapon *pWpn = GetPrimaryWeapon();
	if ( pWpn )
		m_iFOV = pWpn->GetFOV();
}
bool CBasePlayer::ShouldGunLower()
{
	if ( !m_pActiveItem )
		return true;
	int wpnClass = m_pActiveItem->Classify();
	if ( wpnClass == CLASS_SCOPED_RIFLE )
	{
		if ( m_iFOV > 0 )
			return false;
	}
	else if ( wpnClass < CLASS_SCOPED_RIFLE )
	{
		if ( wpnClass != CLASS_PISTOL && wpnClass != CLASS_RIFLE )
			return true;
	}
	else if ( wpnClass != CLASS_UNUSED_128 && wpnClass != CLASS_MACHINEGUN && wpnClass != CLASS_SMG )
	{
		return true;
	}
	return m_fGunDropTime <= gpGlobals->time;
}
void CBasePlayer::SetHasWeapon( int weaponId )
{
	if ( weaponId > 0x1f )
		m_iWeapons2 |= 1 << ( ( weaponId - 0x20 ) & 0x1f );
	else
		pev->weapons |= 1 << ( weaponId & 0x1f );
}
void CBasePlayer::ClearHasWeapon( int weaponId )
{
	if ( weaponId > 0x1f )
		m_iWeapons2 &= ~( 1 << ( ( weaponId - 0x20 ) & 0x1f ) );
	else
		pev->weapons &= ~( 1 << ( weaponId & 0x1f ) );
}
bool CBasePlayer::HasWeapons( int weaponId )
{
	if ( weaponId > 0x1f )
		return ( m_iWeapons2 & ( 1 << ( ( weaponId - 0x20 ) & 0x1f ) ) ) != 0;
	return ( pev->weapons & ( 1 << ( weaponId & 0x1f ) ) ) != 0;
}
void CBasePlayer::RemoveAllWeaponFlags()
{
	pev->weapons = 0;
	m_iWeapons2 = 0;
}
void CBasePlayer::ServerSpawnCheck()
{
	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;

	// the paratroopers can join a running round only while the para window is open
	if ( !pRules->IsTeamInfinite( pev->team ) && pRules->RoundState() == ROUNDSTATE_RUNNING && !pRules->m_RoundParaCanJoin )
	{
		pev->nextthink = gpGlobals->time + 1.0;
		return;
	}

	if ( pev->iuser1 )
	{
		// no team or class chosen yet
		if ( !pev->team || m_iNextClass == -1 )
		{
			pev->nextthink = gpGlobals->time + 1.0;
			return;
		}

		if ( pRules->RoundState() == ROUNDSTATE_RUNNING )
		{
			pRules->SendWaveStatus( this, 0 );
			RemoveAllItems( TRUE );

			SetThink( &CBasePlayer::SpawnThink );
			pev->nextthink = gpGlobals->time + 1.0;
			return;
		}

		// the round is not running, wait for it
		pev->nextthink = gpGlobals->time + 1.0;
		return;
	}

	if ( m_iJoiningState == JUSTJOINED )
		pev->nextthink = gpGlobals->time + 1.0;
}
void CBasePlayer::ClientDisconnect()
{
	m_iNumTKs = 0;
	pev->team = 0;
	m_iNextClass = -1;
	pev->playerclass = 0;
	m_bIsObserver = 0;
}
BOOL CBasePlayer::DropItemByClass( int weaponclass )
{
	BOOL bDropped = FALSE;
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		CBasePlayerItem *pItem = m_rgpPlayerItems[i];
		while ( pItem )
		{
			if ( weaponclass & pItem->Classify() )
			{
				char wpnname[64];
				strcpy( wpnname, STRING( pItem->pev->classname ) );
				DropPlayerItem( wpnname, true );
				bDropped = TRUE;
			}
			pItem = pItem->m_pNext;
		}
	}
	return bDropped;
}
void CBasePlayer::DropGenericAmmo()
{
	if ( pev->iuser1 != 0 || pev->iuser2 != 0 || pev->health <= 0.0f || m_nGenericAmmoCount <= 0 )
		return;
	const char *szName = "ammo_generic_german";
	if ( pev->team != TEAM_AXIS && (unsigned) ( pev->playerclass - 0x15 ) >= 6 )
		szName = "ammo_generic_american";
	else if ( pev->team != TEAM_AXIS )
		szName = "ammo_generic_british";
	CBaseEntity *pAmmo = CBaseEntity::Create( szName, pev->origin, pev->angles, edict() );
	pAmmo->pev->owner = edict();
	UTIL_MakeVectors( pev->angles );
	pAmmo->pev->angles.x = 0;
	pAmmo->pev->angles.z = 0;
	pAmmo->pev->velocity = gpGlobals->v_forward * 200.0f;
	pAmmo->SetThink( &CBaseEntity::SUB_Remove );
	pAmmo->pev->nextthink = gpGlobals->time + 30.0f;
	pAmmo->pev->spawnflags |= SF_NORESPAWN;
	m_nGenericAmmoCount--;
}
void CBasePlayer::ShowMenu( int slots, int time, char *szText, int bitsvalid, int menu )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, pev );
	WRITE_SHORT( slots );
	WRITE_CHAR( time );
	WRITE_BYTE( 0 );
	WRITE_STRING( szText );
	MESSAGE_END();
}
void CBasePlayer::ClearVoiceMenu()
{
	lastMenuSelected = -1;
	MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, pev );
	WRITE_SHORT( 0 );
	WRITE_CHAR( 0 );
	WRITE_BYTE( 0 );
	MESSAGE_END();
}
void CBasePlayer::VoteMenu( int kickplayer, int menu )
{
	lastMenuSelected = menu;
	MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, pev );
	WRITE_SHORT( 0x3ff );
	WRITE_CHAR( 0x7f );
	WRITE_BYTE( 0 );
	WRITE_STRING( "" );
	MESSAGE_END();
}
void CBasePlayer::VoiceMenu( int menuNum )
{
	char szText[32];

	if ( pev->iuser1 || pev->iuser2 )
		return;

	lastMenuSelected = menuNum;

	if ( menuNum == 1 )
	{
		if ( pev->team != TEAM_ALLIES )
			strcpy( szText, "#Menu_GerVoice1" );
		else if ( ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesBrit() )
			strcpy( szText, "#Menu_BritVoice1" );
		else
			strcpy( szText, "#Menu_AmerVoice1" );
	}
	else if ( menuNum == 2 )
	{
		if ( pev->team != TEAM_ALLIES )
			strcpy( szText, "#Menu_GerVoice2" );
		else if ( ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesBrit() )
			strcpy( szText, "#Menu_BritVoice2" );
		else
			strcpy( szText, "#Menu_AmerVoice2" );
	}
	else
	{
		if ( pev->team != TEAM_ALLIES )
			strcpy( szText, "#Menu_GerVoice3" );
		else if ( ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesBrit() )
			strcpy( szText, "#Menu_BritVoice3" );
		else
			strcpy( szText, "#Menu_AmerVoice3" );
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, pev );
		WRITE_SHORT( 1023 );
		WRITE_CHAR( 127 );
		WRITE_BYTE( 0 );
		WRITE_STRING( szText );
	MESSAGE_END();
}
void CBasePlayer::HandSignal( int signal )
{
	if ( m_flNextHand > gpGlobals->time )
		return;
	m_handSignal = signal;
	SetAnimation( PLAYER_SIGNAL );
	m_flNextHand = gpGlobals->time + 1.0f;
	if ( (unsigned) signal < 0x1a )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer->pev->team == pev->team )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgHandSignal, NULL, pPlayer->pev );
				WRITE_BYTE( ENTINDEX( edict() ) );
				WRITE_BYTE( signal );
				MESSAGE_END();
			}
		}
	}
}
void CBasePlayer::SpawnClientSideCorpse()
{
	char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer( edict() );
	char *pModel = g_engfuncs.pfnInfoKeyValue( infobuffer, "model" );
	MESSAGE_BEGIN( MSG_ALL, gmsgSendCorpse );
	WRITE_STRING( pModel );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_ANGLE( pev->angles.x );
	WRITE_ANGLE( pev->angles.y );
	WRITE_ANGLE( pev->angles.z );
	WRITE_BYTE( pev->sequence );
	WRITE_SHORT( pev->body );
	WRITE_BYTE( pev->team );
	MESSAGE_END();
}
void CBasePlayer::NextMapMarker()
{
	int m = m_iMapMarker + 1;
	m_iMapMarker = ( m < 0xf ) ? m : 0;
	MESSAGE_BEGIN( MSG_ONE, gmsgCurMarker, NULL, pev );
	WRITE_BYTE( m_iMapMarker );
	MESSAGE_END();
}
void CBasePlayer::PrevMapMarker()
{
	int m = m_iMapMarker - 1;
	m_iMapMarker = ( m >= 0 ) ? m : 0xe;
	MESSAGE_BEGIN( MSG_ONE, gmsgCurMarker, NULL, pev );
	WRITE_BYTE( m_iMapMarker );
	MESSAGE_END();
}
void CBasePlayer::SetMapMarker( int marker )
{
	if ( (unsigned) marker < 0xf )
		m_iMapMarker = marker;
}
void CBasePlayer::FireMapMarker()
{
	if ( !IsAlive() || pev->iuser1 != 0 || pev->iuser2 != 0 || g_fGameOver || m_flNextMapMarkerTime > gpGlobals->time )
		return;

	// the oldest marker of the player goes away when the limit is reached
	CBaseEntity *pMarker = UTIL_FindEntityByClassname( NULL, "dod_mapmarker" );
	float flMaxMarkers = 1;

	if ( pcvar_mp_nummapmarkers )
	{
		flMaxMarkers = 1;

		if ( pcvar_mp_nummapmarkers->value >= 1 )
			flMaxMarkers = pcvar_mp_nummapmarkers->value;
	}

	if ( pMarker )
	{
		int iCount = 1;
		CBaseEntity *pOldest = NULL;
		float flOldestCreatedTime = 1000000000;

		do
		{
			if ( pMarker->pev->owner == edict() )
			{
				iCount++;

				if ( flOldestCreatedTime > pMarker->pev->fuser1 )
				{
					flOldestCreatedTime = pMarker->pev->fuser1;
					pOldest = pMarker;
				}
			}

			pMarker = UTIL_FindEntityByClassname( pMarker, "dod_mapmarker" );
		}
		while ( pMarker );

		if ( iCount > flMaxMarkers && pOldest )
			UTIL_Remove( pOldest );
	}

	UTIL_MakeVectors( pev->v_angle );

	Vector vecOrg = GetGunPosition();
	Vector vecEnd = vecOrg + gpGlobals->v_forward * 4096;
	TraceResult tr;

	UTIL_TraceLine( vecOrg, vecEnd, ignore_monsters, edict(), &tr );

	if ( tr.flFraction < 1.0 )
		CMapMarker::CreateMapMarker( tr.vecEndPos, Vector( 0, 0, 0 ), this, m_iMapMarker );

	m_flNextMapMarkerTime = gpGlobals->time + 1.0;
}
void CBasePlayer::PopHelmet( Vector *vecdir )
{
	int helmetgroup = GetBodygroup( 1 );
	BOOL bIsPara = FALSE;
	int iHelmet;

	if ( g_pGameRules )
		bIsPara = ( (CDoDTeamPlay *) g_pGameRules )->IsTeamPara( pev->team );

	if ( pev->team == TEAM_ALLIES )
	{
		if ( bIsPara )
		{
			if ( helmetgroup > 1 )
				return;

			SetBodygroup( 1, g_ParaClassInfo[pev->playerclass].hairgroup );
			iHelmet = ( helmetgroup == 1 ) + 4;
		}
		else if ( (unsigned int) ( pev->playerclass - 21 ) > 5 )
		{
			if ( helmetgroup > 2 )
				return;

			SetBodygroup( 1, g_ClassInfo[pev->playerclass].hairgroup );
			iHelmet = 0;
		}
		else
		{
			if ( helmetgroup > 1 )
				return;

			SetBodygroup( 1, g_ClassInfo[pev->playerclass].hairgroup );
			iHelmet = 3;
		}
	}
	else if ( pev->team == TEAM_AXIS )
	{
		if ( bIsPara )
		{
			if ( helmetgroup > 0 )
				return;

			SetBodygroup( 1, g_ParaClassInfo[pev->playerclass].hairgroup );
			iHelmet = 6;
		}
		else
		{
			if ( helmetgroup > 1 )
				return;

			SetBodygroup( 1, g_ClassInfo[pev->playerclass].hairgroup );
			iHelmet = ( helmetgroup == 1 ) + 1;
		}
	}
	else
	{
		return;
	}

	Vector origin, angles;

	GetAttachment( 1, origin, angles );

	vecdir->x = vecdir->x + pev->velocity.x;
	vecdir->y = vecdir->y + pev->velocity.y;
	vecdir->z = vecdir->z + pev->velocity.z;

	PLAYBACK_EVENT_FULL( 0, NULL, g_pGameRules->PopHelmet(), 0.0f, origin, *vecdir, pev->angles.x, pev->angles.y,
						 iHelmet, 0, 0, 0 );

	UTIL_SendDirectorMessage( this, NULL, 5 | DRC_FLAG_SLOWMOTION );
}
void CBasePlayer::DropObject()
{
	if ( !HasObject || !m_pObject )
		return;

	CObject *pObject = m_pObject;

	UTIL_MakeVectors( pev->angles );

	pObject->pev->origin = pev->origin + gpGlobals->v_forward * 20;
	pObject->pev->velocity = pev->velocity + gpGlobals->v_forward * 400;

	pObject->Drop();

	HasObject = 0;
	m_pObject = NULL;

	pObject->SendDroppedMsg( this );

	if ( pObject->pev->noise2 )
		EMIT_SOUND_DYN( edict(), CHAN_STATIC, STRING( pObject->pev->noise2 ), 1.0, ATTN_NORM, 0, PITCH_NORM );

	MESSAGE_BEGIN( MSG_ONE, gmsgObject, NULL, pev );
		WRITE_STRING( "" );
	MESSAGE_END();

	pObject->m_hOwner = NULL;
	pObject->m_bCarried = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && pPlayer->pev->team == pev->team )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgPStatus, NULL, pPlayer->pev );
				WRITE_BYTE( ENTINDEX( edict() ) );
				WRITE_BYTE( PlayerStatus() );
			MESSAGE_END();
		}
	}
}
void CBasePlayer::StudioPlayerBlend( int *pBlend, float *pPitch )
{
	// calc up/down pointing
	*pBlend = (int)( -( 3.0f * *pPitch ) );

	if ( *pBlend < -45 )
	{
		*pPitch = *pPitch - 15;
		*pBlend = 0;
	}
	else if ( *pBlend > 45 )
	{
		*pPitch = *pPitch + 15;
		*pBlend = 255;
	}
	else
	{
		*pBlend = (int)( ( *pBlend + 45.0 ) * 255.0 / 90.0 );
		*pPitch = 0;
	}
}
void CBasePlayer::CalculatePitchBlend()
{
	int iBlend;
	float temp = pev->angles.x;
	StudioPlayerBlend( &iBlend, &temp );
	pev->blending[1] = iBlend;
	m_flPitch = temp;
}
void CBasePlayer::CalculateYawBlend()
{
	float dt;
	float flYaw;
	float maxyaw = 255.0f;
	float blend_yaw;

	StudioEstimateGait();

	// calc side to side turning
	flYaw = pev->angles.y - m_flGaityaw;
	flYaw = fmod( flYaw, 360.0f );

	if ( flYaw < -180 )
		flYaw = flYaw + 360;
	else if ( flYaw > 180 )
		flYaw = flYaw - 360;

	if ( m_flGaitMovement != 0.0 )
	{
		if ( flYaw > 120 )
		{
			m_flGaityaw = m_flGaityaw - 180;
			m_flGaitMovement = -m_flGaitMovement;
			flYaw = flYaw - 180;
		}
		else if ( flYaw < -120 )
		{
			m_flGaityaw = m_flGaityaw + 180;
			m_flGaitMovement = -m_flGaitMovement;
			flYaw = flYaw + 180;
		}
	}

	// adjust torso
	blend_yaw = ( flYaw / 90 ) * 128.0 + 127.0;
	blend_yaw = clamp( blend_yaw, 0.0f, maxyaw );
	blend_yaw = 255.0 - blend_yaw;

	pev->blending[0] = (int) blend_yaw;
	m_flYaw = blend_yaw;
}
void CBasePlayer::StudioEstimateGait()
{
	if ( !IsAlive() )
		return;
	float dt = gpGlobals->frametime;
	if ( dt < 0.0f )
	{
		m_flGaitMovement = 0;
		return;
	}
	if ( dt > 1.0f )
		dt = 1.0f;
	if ( dt == 0.0f )
	{
		m_flGaitMovement = 0;
		return;
	}
	Vector delta = pev->origin - m_prevgaitorigin;
	m_prevgaitorigin = pev->origin;
	m_flGaitMovement = delta.Length();
	if ( m_flGaitMovement / dt < 1.0f )
		m_flGaitMovement = 0;
	if ( ( pev->deadflag == DEAD_NO && STANCE_IS_PRONE( pev->iuser3 ) ) || pev->vuser1.x == SANDBAG_DEPLOYED )
	{
		m_flGaityaw = pev->angles.y;
	}
	else if ( delta.x != 0.0f || delta.y != 0.0f )
	{
		m_flGaityaw = atan2( delta.y, delta.x ) * 180.0f / 3.14159265f;
	}
}
void CBasePlayer::StudioProcessGait()
{
	CalculateYawBlend();
	CalculatePitchBlend();
	m_flGaitframe += gpGlobals->frametime * pev->framerate * m_flGaitMovement;
	if ( m_flGaitframe > 256.0f )
		m_flGaitframe = 0.0f;
}
float CBasePlayer::GetStamina() { return pev->fuser4; }
void CBasePlayer::SetStamina( float s ) { pev->fuser4 = s; }
void CBasePlayer::UTIL_EdictScreenFade( edict_t *pEdict, const Vector &color, float fadeTime, float fadeHold,
										int alpha, int flags )
{
	ScreenFade fade;

	UTIL_EdictScreenFadeBuild( fade, color, fadeTime, fadeHold, alpha, flags );
	UTIL_EdictScreenFadeWrite( fade, pEdict );
}

void UTIL_DodBloodPuff( Vector &origin )
{
	MESSAGE_BEGIN( MSG_PVS, gmsgBloodPuff, origin );
		WRITE_COORD( origin.x );
		WRITE_COORD( origin.y );
		WRITE_COORD( origin.z );
	MESSAGE_END();
}
// static helpers (inlined in binary; declared in player.h)
BOOL CBasePlayer::IsProne( CBasePlayer *pPlayer )
{
	return pPlayer && pPlayer->IsProne();
}
BOOL CBasePlayer::IsSandbagDeployed( CBasePlayer *pPlayer )
{
	return pPlayer && pPlayer->IsSandbagDeployed();
}

void UTIL_EdictScreenFadeBuild( ScreenFade &fade, const Vector &color, float fadeTime, float fadeHold, int alpha,
								int flags )
{
	fade.duration = FixedUnsigned16( fadeTime, 1 << 12 );  // 4.12 fixed
	fade.holdTime = FixedUnsigned16( fadeHold, 1 << 12 );  // 4.12 fixed
	fade.r = (int) color.x;
	fade.g = (int) color.y;
	fade.b = (int) color.z;
	fade.a = alpha;
	fade.fadeFlags = flags;
}

void UTIL_EdictScreenFadeWrite( const ScreenFade &fade, edict_t *pEntity )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgFade, NULL, pEntity );
	WRITE_SHORT( fade.duration );
	WRITE_SHORT( fade.holdTime );
	WRITE_SHORT( fade.fadeFlags );
	WRITE_BYTE( fade.r );
	WRITE_BYTE( fade.g );
	WRITE_BYTE( fade.b );
	WRITE_BYTE( fade.a );
	MESSAGE_END();
}

// No standalone symbols in dod.so (inlined at call sites). Used by objectives/maprules.
BOOL CBasePlayer::HasObjectOfGroup( const char *grp )
{
	return ( m_pObject != NULL && grp != NULL && FStrEq( m_pObject->sz_Group, grp ) ) ? TRUE : FALSE;
}

BOOL CBasePlayer::IsCarryingObject( void )
{
	return m_pObject != NULL;
}

float GetPlayerGaitYaw( const edict_t *pEdict )
{
	if ( pEdict )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pEdict->v.pContainingEntity );

		if ( pPlayer )
			return pPlayer->m_flGaityaw;
	}

	return 0;
}

float GetPlayerPitch( const edict_t *pEdict )
{
	if ( pEdict )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pEdict->v.pContainingEntity );

		if ( pPlayer )
			return pPlayer->m_flPitch;
	}

	return 0;
}

float GetPlayerYaw( const edict_t *pEdict )
{
	if ( pEdict )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pEdict->v.pContainingEntity );

		if ( pPlayer )
			return pPlayer->m_flYaw;
	}

	return 0;
}

float GetPlayerPitchBlend( const edict_t *pEdict )
{
	if ( pEdict )
		return pEdict->v.blending[1];

	return 0;
}

int GetPlayerGaitsequence( const edict_t *pEdict )
{
	if ( pEdict )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pEdict->v.pContainingEntity );

		if ( pPlayer )
			return pPlayer->m_iGaitsequence;
	}

	return 1;
}

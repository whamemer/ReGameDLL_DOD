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

//	-------------------------------------------
//
//	maprules.cpp
//
//	This module contains entities for implementing/changing game
//	rules dynamically within each map (.BSP)
//
//	-------------------------------------------

#include "extdll.h"
#include "util.h"
#include "eiface.h"
#include "gamerules.h"
#include "maprules.h"
#include "cbase.h"
#include "player.h"
extern DLL_GLOBAL BOOL g_fGameOver;
extern int gmsgStartProgressBar;
extern int gmsgStartProgressBarF;
extern int gmsgSetObjOwner;
extern int gmsgCapMsg;
extern int gmsgInitObjDisplay;
extern int gmsgTimeLeft;
extern int gmsgPStatus;
extern int gmsgProgBarUpdate;
extern int gmsgCancelProgressBar;
extern int gmsgTimerStatus;
extern cvar_t *mp_clan_match;
extern cvar_t *mp_clan_scoring;
extern cvar_t *mp_clan_scoring_delay;
const char *PlayerLogLine( CBaseEntity *pEntity );

class CRuleEntity : public CBaseEntity
{
  public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void SetMaster( int iszMaster ) { m_iszMaster = iszMaster; }

  protected:
	BOOL CanFireForActivator( CBaseEntity *pActivator );

  private:
	string_t m_iszMaster;
};

TYPEDESCRIPTION CRuleEntity::m_SaveData[] = {
	DEFINE_FIELD( CRuleEntity, m_iszMaster, FIELD_STRING ),
};

// .text:0010c910 CRuleEntity::Restore (file:maprules.cpp:57)
IMPLEMENT_SAVERESTORE( CRuleEntity, CBaseEntity );

void CRuleEntity::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
}

void CRuleEntity::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		SetMaster( ALLOC_STRING( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

BOOL CRuleEntity::CanFireForActivator( CBaseEntity *pActivator )
{
	if ( m_iszMaster )
	{
		if ( UTIL_IsMasterTriggered( m_iszMaster, pActivator ) )
			return TRUE;
		else
			return FALSE;
	}

	return TRUE;
}

//
// CRulePointEntity -- base class for all rule "point" entities (not brushes)
//
class CRulePointEntity : public CRuleEntity
{
  public:
	void Spawn( void );
	int m_iEnabled;
};

void CRulePointEntity::Spawn( void )
{
	CRuleEntity::Spawn();
	pev->frame = 0;
	pev->model = 0;
}

//
// CRuleBrushEntity -- base class for all rule "brush" entities (not brushes)
// Default behavior is to set up like a trigger, invisible, but keep the model for volume testing
//
class CRuleBrushEntity : public CRuleEntity
{
  public:
	void Spawn( void );

  private:
};

void CRuleBrushEntity::Spawn( void )
{
	SET_MODEL( edict(), STRING( pev->model ) );
	CRuleEntity::Spawn();
}

// CGameScore / game_score	-- award points to player / team
//	Points +/- total
//	Flag: Allow negative scores					SF_SCORE_NEGATIVE
//	Flag: Award points to team in teamplay		SF_SCORE_TEAM

#define SF_SCORE_NEGATIVE 0x0001
#define SF_SCORE_TEAM 0x0002

class CGameScore : public CRulePointEntity
{
  public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	inline int Points( void ) { return pev->frags; }
	inline BOOL AllowNegativeScore( void ) { return pev->spawnflags & SF_SCORE_NEGATIVE; }
	inline BOOL AwardToTeam( void ) { return pev->spawnflags & SF_SCORE_TEAM; }

	inline void SetPoints( int points ) { pev->frags = points; }

  private:
};

LINK_ENTITY_TO_CLASS( game_score, CGameScore );

void CGameScore::Spawn( void )
{
	CRulePointEntity::Spawn();

	RoundStore();
}

void CGameScore::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "points" ) )
	{
		SetPoints( atoi( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CRulePointEntity::KeyValue( pkvd );
}

void CGameScore::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	// Only players can use this
	if ( pActivator->IsPlayer() )
	{
		if ( AwardToTeam() )
		{
			pActivator->AddPointsToTeam( Points(), AllowNegativeScore() );
		}
		else
		{
			pActivator->AddPoints( Points(), AllowNegativeScore() );
		}
	}
}

// CGameEnd / game_end	-- Ends the game in MP

class CGameEnd : public CRulePointEntity
{
  public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

  private:
};

LINK_ENTITY_TO_CLASS( game_end, CGameEnd );

void CGameEnd::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	g_pGameRules->EndMultiplayerGame();
}

//
// CGameText / game_text	-- NON-Localized HUD Message (use env_message to display a titles.txt message)
//	Flag: All players					SF_ENVTEXT_ALLPLAYERS
//

#define SF_ENVTEXT_ALLPLAYERS 0x0001

class CGameText : public CRulePointEntity
{
  public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	inline BOOL MessageToAll( void ) { return ( pev->spawnflags & SF_ENVTEXT_ALLPLAYERS ); }
	inline void MessageSet( const char *pMessage ) { pev->message = ALLOC_STRING( pMessage ); }
	inline const char *MessageGet( void ) { return STRING( pev->message ); }

  private:
	hudtextparms_t m_textParms;
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( game_text, CGameText );

// Save parms as a block.  Will break save/restore if the structure changes, but this entity didn't ship with Half-Life, so
// it can't impact saved Half-Life games.
TYPEDESCRIPTION CGameText::m_SaveData[] = {
	DEFINE_ARRAY( CGameText, m_textParms, FIELD_CHARACTER, sizeof( hudtextparms_t ) ),
};

// .text:0010cc20 CGameText::Restore (file:maprules.cpp:277)
IMPLEMENT_SAVERESTORE( CGameText, CRulePointEntity );

void CGameText::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "channel" ) )
	{
		m_textParms.channel = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "x" ) )
	{
		m_textParms.x = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "y" ) )
	{
		m_textParms.y = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "effect" ) )
	{
		m_textParms.effect = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "color" ) )
	{
		int color[4];
		UTIL_StringToIntArray( color, 4, pkvd->szValue );
		m_textParms.r1 = color[0];
		m_textParms.g1 = color[1];
		m_textParms.b1 = color[2];
		m_textParms.a1 = color[3];
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "color2" ) )
	{
		int color[4];
		UTIL_StringToIntArray( color, 4, pkvd->szValue );
		m_textParms.r2 = color[0];
		m_textParms.g2 = color[1];
		m_textParms.b2 = color[2];
		m_textParms.a2 = color[3];
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "fadein" ) )
	{
		m_textParms.fadeinTime = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "fadeout" ) )
	{
		m_textParms.fadeoutTime = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "holdtime" ) )
	{
		m_textParms.holdTime = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "fxtime" ) )
	{
		m_textParms.fxTime = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CRulePointEntity::KeyValue( pkvd );
}

void CGameText::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	if ( MessageToAll() )
	{
		UTIL_HudMessageAll( m_textParms, MessageGet() );
	}
	else
	{
		if ( pActivator->IsNetClient() )
		{
			UTIL_HudMessage( pActivator, m_textParms, MessageGet() );
		}
	}
}

//
// CGameTeamMaster / game_team_master	-- "Masters" like multisource, but based on the team of the activator
// Only allows mastered entity to fire if the team matches my team
//
// team index (pulled from server team list "mp_teamlist"
// Flag: Remove on Fire
// Flag: Any team until set?		-- Any team can use this until the team is set (otherwise no teams can use it)
//

#define SF_TEAMMASTER_FIREONCE 0x0001
#define SF_TEAMMASTER_ANYTEAM 0x0002

class CGameTeamMaster : public CRulePointEntity
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int ObjectCaps( void ) { return CRulePointEntity::ObjectCaps() | FCAP_MASTER; }

	BOOL IsTriggered( CBaseEntity *pActivator );
	const char *TeamID( void );
	inline BOOL RemoveOnFire( void ) { return ( pev->spawnflags & SF_TEAMMASTER_FIREONCE ) ? TRUE : FALSE; }
	inline BOOL AnyTeam( void ) { return ( pev->spawnflags & SF_TEAMMASTER_ANYTEAM ) ? TRUE : FALSE; }

  private:
	BOOL TeamMatch( CBaseEntity *pActivator );

	int m_teamIndex;
	USE_TYPE triggerType;
	void Spawn( void );
	void RoundRespawn( void );
	int m_rrteamIndex;
};

LINK_ENTITY_TO_CLASS( game_team_master, CGameTeamMaster );

void CGameTeamMaster::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "teamindex" ) )
	{
		m_teamIndex = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "triggerstate" ) )
	{
		int type = atoi( pkvd->szValue );
		switch ( type )
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CRulePointEntity::KeyValue( pkvd );
}

void CGameTeamMaster::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	if ( useType == USE_SET )
	{
		if ( value < 0 )
		{
			m_teamIndex = -1;
		}
		else
		{
			m_teamIndex = g_pGameRules->GetTeamIndex( pActivator->TeamID() );
		}
		return;
	}

	if ( TeamMatch( pActivator ) )
	{
		SUB_UseTargets( pActivator, triggerType, value );
		if ( RemoveOnFire() )
			UTIL_Remove( this );
	}
}

BOOL CGameTeamMaster::IsTriggered( CBaseEntity *pActivator )
{
	return TeamMatch( pActivator );
}

const char *CGameTeamMaster::TeamID( void )
{
	if ( m_teamIndex < 0 )	// Currently set to "no team"
		return "";

	return g_pGameRules->GetIndexedTeamName( m_teamIndex );	 // UNDONE: Fill this in with the team from the "teamlist"
}

BOOL CGameTeamMaster::TeamMatch( CBaseEntity *pActivator )
{
	if ( m_teamIndex < 0 && AnyTeam() )
		return TRUE;

	if ( !pActivator )
		return FALSE;

	return UTIL_TeamsMatch( pActivator->TeamID(), TeamID() );
}

//
// CGameTeamSet / game_team_set	-- Changes the team of the entity it targets to the activator's team
// Flag: Fire once
// Flag: Clear team				-- Sets the team to "NONE" instead of activator

#define SF_TEAMSET_FIREONCE 0x0001
#define SF_TEAMSET_CLEARTEAM 0x0002

class CGameTeamSet : public CRulePointEntity
{
  public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline BOOL RemoveOnFire( void ) { return ( pev->spawnflags & SF_TEAMSET_FIREONCE ) ? TRUE : FALSE; }
	inline BOOL ShouldClearTeam( void ) { return ( pev->spawnflags & SF_TEAMSET_CLEARTEAM ) ? TRUE : FALSE; }

  private:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( game_team_set, CGameTeamSet );

void CGameTeamSet::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	if ( ShouldClearTeam() )
	{
		SUB_UseTargets( pActivator, USE_SET, -1 );
	}
	else
	{
		SUB_UseTargets( pActivator, USE_SET, 0 );
	}

	if ( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}

//
// CGamePlayerZone / game_player_zone -- players in the zone fire my target when I'm fired
//
// Needs master?
class CGamePlayerZone : public CRuleBrushEntity
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

  private:
	string_t m_iszInTarget;
	string_t m_iszOutTarget;
	string_t m_iszInCount;
	string_t m_iszOutCount;
};

LINK_ENTITY_TO_CLASS( game_zone_player, CGamePlayerZone );
TYPEDESCRIPTION CGamePlayerZone::m_SaveData[] = {
	DEFINE_FIELD( CGamePlayerZone, m_iszInTarget, FIELD_STRING ),
	DEFINE_FIELD( CGamePlayerZone, m_iszOutTarget, FIELD_STRING ),
	DEFINE_FIELD( CGamePlayerZone, m_iszInCount, FIELD_STRING ),
	DEFINE_FIELD( CGamePlayerZone, m_iszOutCount, FIELD_STRING ),
};

// .text:0010cb80 CGamePlayerZone::Restore (file:maprules.cpp:670)
IMPLEMENT_SAVERESTORE( CGamePlayerZone, CRuleBrushEntity );

void CGamePlayerZone::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "intarget" ) )
	{
		m_iszInTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "outtarget" ) )
	{
		m_iszOutTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "incount" ) )
	{
		m_iszInCount = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "outcount" ) )
	{
		m_iszOutCount = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CRuleBrushEntity::KeyValue( pkvd );
}

void CGamePlayerZone::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int playersInCount = 0;
	int playersOutCount = 0;

	if ( !CanFireForActivator( pActivator ) )
		return;

	CBaseEntity *pPlayer = NULL;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			TraceResult trace;
			int hullNumber;

			hullNumber = human_hull;
			if ( pPlayer->pev->flags & FL_DUCKING )
			{
				hullNumber = head_hull;
			}

			UTIL_TraceModel( pPlayer->pev->origin, pPlayer->pev->origin, hullNumber, edict(), &trace );

			if ( trace.fStartSolid )
			{
				playersInCount++;
				if ( m_iszInTarget )
				{
					FireTargets( STRING( m_iszInTarget ), pPlayer, pActivator, useType, value );
				}
			}
			else
			{
				playersOutCount++;
				if ( m_iszOutTarget )
				{
					FireTargets( STRING( m_iszOutTarget ), pPlayer, pActivator, useType, value );
				}
			}
		}
	}

	if ( m_iszInCount )
	{
		FireTargets( STRING( m_iszInCount ), pActivator, this, USE_SET, playersInCount );
	}

	if ( m_iszOutCount )
	{
		FireTargets( STRING( m_iszOutCount ), pActivator, this, USE_SET, playersOutCount );
	}
}

//
// CGamePlayerHurt / game_player_hurt	-- Damages the player who fires it
// Flag: Fire once

#define SF_PKILL_FIREONCE 0x0001
class CGamePlayerHurt : public CRulePointEntity
{
  public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline BOOL RemoveOnFire( void ) { return ( pev->spawnflags & SF_PKILL_FIREONCE ) ? TRUE : FALSE; }

  private:
};

LINK_ENTITY_TO_CLASS( game_player_hurt, CGamePlayerHurt );

void CGamePlayerHurt::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	if ( pActivator->IsPlayer() )
	{
		if ( pev->dmg < 0 )
			pActivator->TakeHealth( -pev->dmg, DMG_GENERIC );
		else
			pActivator->TakeDamage( pev, pev, pev->dmg, DMG_GENERIC );
	}

	SUB_UseTargets( pActivator, useType, value );

	if ( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}

//
// CGameCounter / game_counter	-- Counts events and fires target
// Flag: Fire once
// Flag: Reset on Fire

#define SF_GAMECOUNT_FIREONCE 0x0001
#define SF_GAMECOUNT_RESET 0x0002

class CGameCounter : public CRulePointEntity
{
  public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline BOOL RemoveOnFire( void ) { return ( pev->spawnflags & SF_GAMECOUNT_FIREONCE ) ? TRUE : FALSE; }
	inline BOOL ResetOnFire( void ) { return ( pev->spawnflags & SF_GAMECOUNT_RESET ) ? TRUE : FALSE; }

	inline void CountUp( void ) { pev->frags++; }
	inline void CountDown( void ) { pev->frags--; }
	inline void ResetCount( void ) { pev->frags = pev->dmg; }
	inline int CountValue( void ) { return pev->frags; }
	inline int LimitValue( void ) { return pev->health; }

	inline BOOL HitLimit( void ) { return CountValue() == LimitValue(); }

  private:
	inline void SetCountValue( int value ) { pev->frags = value; }
	inline void SetInitialValue( int value ) { pev->dmg = value; }
	void RoundRespawn( void );
};

LINK_ENTITY_TO_CLASS( game_counter, CGameCounter );

void CGameCounter::Spawn( void )
{
	// Save off the initial count
	SetInitialValue( CountValue() );
	CRulePointEntity::Spawn();

	RoundStore();
}

void CGameCounter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	switch ( useType )
	{
	case USE_ON:
	case USE_TOGGLE:
		CountUp();
		break;

	case USE_OFF:
		CountDown();
		break;

	case USE_SET:
		SetCountValue( (int) value );
		break;
	}

	if ( HitLimit() )
	{
		SUB_UseTargets( pActivator, USE_TOGGLE, 0 );
		if ( RemoveOnFire() )
		{
			UTIL_Remove( this );
		}

		if ( ResetOnFire() )
		{
			ResetCount();
		}
	}
}

//
// CGameCounterSet / game_counter_set	-- Sets the counter's value
// Flag: Fire once

#define SF_GAMECOUNTSET_FIREONCE 0x0001

class CGameCounterSet : public CRulePointEntity
{
  public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline BOOL RemoveOnFire( void ) { return ( pev->spawnflags & SF_GAMECOUNTSET_FIREONCE ) ? TRUE : FALSE; }

  private:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( game_counter_set, CGameCounterSet );

void CGameCounterSet::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	SUB_UseTargets( pActivator, USE_SET, pev->frags );

	if ( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}

//
// CGamePlayerEquip / game_playerequip	-- Sets the default player equipment
// Flag: USE Only

#define SF_PLAYEREQUIP_USEONLY 0x0001
#define MAX_EQUIP 32

class CGamePlayerEquip : public CRulePointEntity
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Touch( CBaseEntity *pOther );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	inline BOOL UseOnly( void ) { return ( pev->spawnflags & SF_PLAYEREQUIP_USEONLY ) ? TRUE : FALSE; }

  private:
	void EquipPlayer( CBaseEntity *pPlayer );

	string_t m_weaponNames[MAX_EQUIP];
	int m_weaponCount[MAX_EQUIP];
};

LINK_ENTITY_TO_CLASS( game_player_equip, CGamePlayerEquip );

void CGamePlayerEquip::KeyValue( KeyValueData *pkvd )
{
	CRulePointEntity::KeyValue( pkvd );

	if ( !pkvd->fHandled )
	{
		for ( int i = 0; i < MAX_EQUIP; i++ )
		{
			if ( !m_weaponNames[i] )
			{
				char tmp[128];

				UTIL_StripToken( pkvd->szKeyName, tmp, sizeof( tmp ) );

				m_weaponNames[i] = ALLOC_STRING( tmp );
				m_weaponCount[i] = atoi( pkvd->szValue );
				m_weaponCount[i] = max( 1, m_weaponCount[i] );
				pkvd->fHandled = TRUE;
				break;
			}
		}
	}
}

void CGamePlayerEquip::Touch( CBaseEntity *pOther )
{
	if ( !CanFireForActivator( pOther ) )
		return;

	if ( UseOnly() )
		return;

	EquipPlayer( pOther );
}

void CGamePlayerEquip::EquipPlayer( CBaseEntity *pEntity )
{
	CBasePlayer *pPlayer = NULL;

	if ( pEntity->IsPlayer() )
	{
		pPlayer = (CBasePlayer *) pEntity;
	}

	if ( !pPlayer )
		return;

	for ( int i = 0; i < MAX_EQUIP; i++ )
	{
		if ( !m_weaponNames[i] )
			break;
		for ( int j = 0; j < m_weaponCount[i]; j++ )
		{
			pPlayer->GiveNamedItem( STRING( m_weaponNames[i] ) );
		}
	}
}

void CGamePlayerEquip::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	EquipPlayer( pActivator );
}

//
// CGamePlayerTeam / game_player_team	-- Changes the team of the player who fired it
// Flag: Fire once
// Flag: Kill Player
// Flag: Gib Player

#define SF_PTEAM_FIREONCE 0x0001
#define SF_PTEAM_KILL 0x0002
#define SF_PTEAM_GIB 0x0004

class CGamePlayerTeam : public CRulePointEntity
{
  public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

  private:
	inline BOOL RemoveOnFire( void ) { return ( pev->spawnflags & SF_PTEAM_FIREONCE ) ? TRUE : FALSE; }
	inline BOOL ShouldKillPlayer( void ) { return ( pev->spawnflags & SF_PTEAM_KILL ) ? TRUE : FALSE; }
	inline BOOL ShouldGibPlayer( void ) { return ( pev->spawnflags & SF_PTEAM_GIB ) ? TRUE : FALSE; }

	const char *TargetTeamName( const char *pszTargetName );
};

LINK_ENTITY_TO_CLASS( game_player_team, CGamePlayerTeam );

const char *CGamePlayerTeam::TargetTeamName( const char *pszTargetName )
{
	CBaseEntity *pTeamEntity = NULL;

	while ( ( pTeamEntity = UTIL_FindEntityByTargetname( pTeamEntity, pszTargetName ) ) != NULL )
	{
		if ( FClassnameIs( pTeamEntity->pev, "game_team_master" ) )
			return pTeamEntity->TeamID();
	}

	return NULL;
}

void CGamePlayerTeam::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	if ( pActivator->IsPlayer() )
	{
		const char *pszTargetTeam = TargetTeamName( STRING( pev->target ) );
		if ( pszTargetTeam )
		{
			CBasePlayer *pPlayer = (CBasePlayer *) pActivator;
			g_pGameRules->ChangePlayerTeam( pPlayer, pszTargetTeam, ShouldKillPlayer(), ShouldGibPlayer() );
		}
	}

	if ( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}

// DOD capture_area - original file triggers.cpp - source restore/objectives/capture_area.cpp
// capture_area.cpp -- dod_capture_area (CAreaCapture)
// Restored from dod.so. Ghidra addr = link addr + 0x10000.
#include "objective.h"

// KeyValue fields (rest => CBaseTrigger::KeyValue):
// area_allies_cancap / area_axis_cancap (int>0) | area_allies_numcap /
// area_axis_numcap (min 1) | area_allies_startcap / area_axis_startcap
// area_allies_endcap / area_axis_endcap | area_allies_breakcap /
// area_axis_breakcap | master => trigger master | area_hud_sprite
// area_time_to_cap => m_nCapTime | area_cap_point => sz_CapPointName
// area_object_group => sz_RequiredObjectGroup, sets m_bRequiresObject.
// .text:00153970 KeyValue (file:triggers.cpp:5500 approx, maprules 58 group)

// .text:00157040 Spawn (file:triggers.cpp:5455 area): InitTrigger, touch =
// AreaTouch, think 0.1 s, m_iAreaIndex = m_nOwningTeam = -1,
// m_nCapturingTeam = -1, m_bCapturing = 0, m_fTimeRemaining = 0.
// .text:00157920 RoundRespawn: same reset; BreakCapture when capturing.
// .text:0014fc90 Precache: hud sprite model.
// .text:00150b00 area_SetIndex: store; broadcast ClientAreas(index,-1,icon)
// when index <= 0x80. .text:0015af00 area_SendStatus: same, unicast.
// .text:0015ad10 SetOwner(team): BreakCapture when capturing, set owner.

// .text:00153dc0 AreaTouch (file:triggers.cpp:5498)
// Master gate, RoundState==1, game live. Lazily resolves m_pPoint by target
// and mirrors owner. Players only (IsPlayer+IsAlive): sets HUD area slot,
// area icon + hint, requires-object group check. Records presence via
// player area field = m_iAreaIndex.
void CAreaCapture::AreaTouch( CBaseEntity *pOther )
{
	if ( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;
	if ( g_pGameRules && g_pGameRules->RoundState() != 1 )
		return;
	if ( g_fGameOver )
		return;
	if ( !m_pPoint )
	{
		m_pPoint = (CControlPoint *) UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		if ( m_pPoint )
			m_nOwningTeam = m_pPoint->m_iTeam;
	}
	else
		m_nOwningTeam = m_pPoint->m_iTeam;
	if ( !pOther->IsPlayer() || !pOther->IsAlive() )
		return;
	CBasePlayer *pPlayer = (CBasePlayer *) pOther;
	int team = pOther->pev->team;
	BOOL can = ( team == 1 ) ? m_bAlliesCanCap : ( team == 2 ? m_bAxisCanCap : FALSE );
	if ( ( pPlayer->m_flDisplayHistory & 0x40 ) == 0 && team != m_nOwningTeam )
	{
		if ( !can )
		{
			pPlayer->m_iCapAreaIconIndex = m_iAreaIndex;
			goto presentState;
		}
		pPlayer->m_flDisplayHistory |= 0x40;
		pPlayer->HintMessageOnce( "#Hint_touched_area_capture" );
		pPlayer->m_iCapAreaIconIndex = m_iAreaIndex;
	}
	else
	{
		pPlayer->m_iCapAreaIconIndex = m_iAreaIndex;
		if ( !can )
			goto presentState;
	}
	pPlayer->SetClientAreaIcon( m_iAreaIndex, true );
presentState:
	pPlayer->m_signals.m_flSignal |= 2;
	if ( m_bRequiresObject && can )
	{
		if ( !pPlayer->HasObject || !pPlayer->m_pObject )
		{
			if ( pPlayer->m_flDisplayHistory & 0x8000 )
				return;
			if ( m_nOwningTeam == pPlayer->pev->team )
				return;
			pPlayer->m_flDisplayHistory |= 0x8000;
			pPlayer->HintMessageOnce( "#Hint_area_requires_object" );
			return;
		}
		if ( strcmp( sz_RequiredObjectGroup, pPlayer->m_pObject->sz_Group ) != 0 )
			return;
	}
	pPlayer->m_iCapAreaIndex = m_iAreaIndex;
}

// .text:0015ab60 GetNumRequiredPlayers (file:triggers.cpp:5584): outputs
// m_nAlliesNumCap / m_nAxisNumCap clamped to min 1.
// .text:0015ac80 GetNumAlive(team) (file:triggers.cpp:5860): alive players.
// .text:0015ac30 CanUseCatchup() (file:triggers.cpp:5616): para rules
// (AlliesInfinite && not AxisInfinite) && spawnflags & 1.

// .text:0015b050 StartCapture(team, capmode) (file:triggers.cpp:5953)
// Fire start targets, m_nCapturingTeam = team, m_bCapturing = 1,
// m_fTimeRemaining = m_nCapTime. HUD: StartProgressBar(index, team, captime)
// or StartProgressBarF(index, team, captime * required / players) in
// catchup mode. Skipped when point hidden (spawnflags & 1). Director msg 12.
void CAreaCapture::StartCapture( int team, int capmode )
{
	if ( team == 1 )
		FireTargets( sz_AlliesStart, NULL, this, USE_TOGGLE, 0.0 );
	else if ( team == 2 )
		FireTargets( sz_AxisStart, NULL, this, USE_TOGGLE, 0.0 );
	m_nCapturingTeam = team;
	m_iCapMode = capmode;
	m_bCapturing = 1;
	m_fTimeRemaining = (float) m_nCapTime;
	if ( m_pPoint && !( m_pPoint->pev->spawnflags & 1 ) )
	{
		if ( capmode == 1 )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgStartProgressBarF );
			WRITE_BYTE( m_pPoint->m_iIndex );
			WRITE_BYTE( team );
			WRITE_COORD( (float) m_nCapTime * ( (float) m_iCappingRequired / (float) m_iCappingPlayers ) );
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgStartProgressBar );
			WRITE_BYTE( m_pPoint->m_iIndex );
			WRITE_BYTE( team );
			WRITE_SHORT( m_nCapTime );
			MESSAGE_END();
		}
	}
	UTIL_SendDirectorMessage( this, NULL, 12 );
}

// .text:0015b240 EndCapture(team) (file:triggers.cpp:5998)
// Fire end targets + team logs. Every qualifying player in area is logged;
// non-first cappers get AddObjScore(m_pPoint->m_iCapPoints). Consumes a
// carried required object (sound, icon clear, PStatus refresh). Then
// m_bCapturing = 0, m_nOwningTeam = team, point SetOwner / reset model,
// director msg 12.
void CAreaCapture::EndCapture( int team )
{
	if ( team == 1 )
	{
		FireTargets( sz_AlliesCap, NULL, this, USE_TOGGLE, 0.0 );
		if ( !m_pPoint )
			UTIL_LogPrintf( "Team \"Allies\" triggered a \"dod_capture_area\"\n" );
		else
			UTIL_LogPrintf( "Team \"Allies\" triggered a \"dod_capture_area\" - \"%s\"\n", STRING( m_pPoint->pev->netname ) );
	}
	else if ( team == 2 )
	{
		FireTargets( sz_AxisCap, NULL, this, USE_TOGGLE, 0.0 );
		if ( !m_pPoint )
			UTIL_LogPrintf( "Team \"Axis\" triggered a \"dod_capture_area\"\n" );
		else
			UTIL_LogPrintf( "Team \"Axis\" triggered a \"dod_capture_area\" - \"%s\"\n", STRING( m_pPoint->pev->netname ) );
	}
	m_iCappingRequired = 0;
	m_iCappingPlayers = 0;

	CBasePlayer *pFirstCapper = NULL;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if ( !plr )
			continue;
		if ( ( plr->m_signals.m_flState & 2 ) == 0 || plr->m_iCapAreaIndex != m_iAreaIndex )
			continue;
		const char *teamName = ( plr->pev->team == 1 ) ? "Allies" : "Axis";
		if ( !m_pPoint )
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"dod_capture_area\"\n", STRING( plr->pev->netname ),
				GETPLAYERUSERID( plr->edict() ), GETPLAYERAUTHID( plr->edict() ), teamName );
		else
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"dod_capture_area\" - \"%s\"\n", STRING( plr->pev->netname ),
				GETPLAYERUSERID( plr->edict() ), GETPLAYERAUTHID( plr->edict() ), teamName, STRING( m_pPoint->pev->netname ) );

		if ( pFirstCapper && plr != pFirstCapper && m_pPoint )
			plr->AddObjScore( m_pPoint->m_iCapPoints, TRUE );
		pFirstCapper = plr;

		if ( m_bRequiresObject && plr->HasObject && plr->m_pObject )
		{
			CObject *pObj = plr->m_pObject;
			if ( pObj->pev->noise1 )
				EMIT_SOUND_DYN( plr->edict(), CHAN_STATIC, STRING( pObj->pev->noise1 ), 1.0, 0.0, 0, 100 );
			plr->HasObject = FALSE;
			plr->m_pObject = NULL;
			plr->RemoveObjectIcon();
			int idx = ENTINDEX( plr->edict() );
			for ( int j = 1; j <= gpGlobals->maxClients; j++ )
			{
				CBaseEntity *pMate = UTIL_PlayerByIndex( j );
				if ( pMate && pMate->pev->team == plr->pev->team )
				{
					MESSAGE_BEGIN( MSG_ONE, gmsgPStatus, NULL, pMate->pev );
					WRITE_BYTE( idx );
					WRITE_BYTE( plr->PlayerStatus() );
					MESSAGE_END();
				}
			}
		}
	}
	m_bCapturing = 0;
	m_fTimeRemaining = 0.0;
	m_nOwningTeam = team;
	if ( m_pPoint )
	{
		if ( !m_pPoint->m_bActive )
		{
			SET_MODEL( m_pPoint->edict(), m_pPoint->sz_ResetModel );
			m_pPoint->SetBodygroup( 0, m_pPoint->m_iResetModelBodygroup );
		}
		else
			m_pPoint->SetOwner( team, TRUE, pFirstCapper );
	}
	UTIL_SendDirectorMessage( this, NULL, 12 );
}

// .text:001511b0 / .text:0015bfa0 BreakCapture(team) (file:triggers.cpp:6127)
// Clears progress, fires break relays for capmode relays, prints
// #game_capture_broken_allie / _axis, cancels progress bar HUD, director 12.

// .text:0015b770 Think() (file:triggers.cpp:5632), every 0.1 s.
// Census: count allies/axis with matching area slot; carrier without the
// required object group is ejected (slot = -1). On change: m_nNumAllies /
// m_nNumAxis update + SendNumPlayers broadcast.
// Idle: nobody present => return. Single team present + canCap + not owner +
// count >= required => StartCapture(team, 0). Catchup path (CanUseCatchup):
// alive count == present count => StartCapture(team, 1).
// Capturing capmode 0: time -= 0.1; contest/empty/requirement lost =>
// BreakCapture; expiry => EndCapture. Mode 1: time -= (players/required)*0.1
// with ProgBarUpdate resend on count change.
void CAreaCapture::Think()
{
	if ( g_fGameOver )
		return;
	pev->nextthink = gpGlobals->time + 0.1;	 // _DAT_0018ee74

	int numAllies = 0;
	int numAxis = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if ( !plr )
			continue;
		if ( ( plr->m_signals.m_flState & 2 ) == 0 || plr->m_iCapAreaIndex != m_iAreaIndex )
			continue;
		if ( m_bRequiresObject )
		{
			if ( !plr->HasObject || !plr->m_pObject ||
				 strcmp( sz_RequiredObjectGroup, plr->m_pObject->sz_Group ) != 0 )
			{
				plr->m_iCapAreaIndex = -1;
				continue;
			}
		}
		if ( plr->pev->team == 1 )
			numAllies++;
		else if ( plr->pev->team == 2 )
			numAxis++;
	}

	if ( m_nNumAllies != numAllies || m_nNumAxis != numAxis )
	{
		m_nNumAllies = numAllies;
		m_nNumAxis = numAxis;
		SendNumPlayers( NULL );
	}

	int requiredAllies = 1;
	int requiredAxis = 1;
	GetNumRequiredPlayers( &requiredAllies, &requiredAxis );

	if ( !m_bCapturing )
	{
		if ( numAllies < 1 && numAxis < 1 )
			return;
		if ( numAllies > 0 && numAxis < 1 )
		{
			if ( m_bAlliesCanCap && m_nOwningTeam != 1 && requiredAllies <= numAllies )
			{
				m_iCappingRequired = requiredAllies;
				m_iCappingPlayers = numAllies;
				StartCapture( 1, 0 );
				return;
			}
		}
		else if ( numAxis > 0 && numAllies < 1 && m_bAxisCanCap && m_nOwningTeam != 2 && requiredAxis <= numAxis )
		{
			m_iCappingRequired = requiredAxis;
			m_iCappingPlayers = numAxis;
			StartCapture( 2, 0 );
			return;
		}

		if ( !( pev->spawnflags & 1 ) )
			return;
		if ( !CanUseCatchup() )
			return;
		if ( numAllies > 0 && numAxis < 1 && m_bAlliesCanCap && m_nOwningTeam != 1 )
		{
			int n = GetNumAlive( 1 );
			if ( n == numAllies )
			{
				m_iCappingRequired = requiredAllies;
				m_iCappingPlayers = n;
				StartCapture( 1, 1 );
				return;
			}
		}
		if ( numAxis < 1 || numAllies > 0 )
			return;
		if ( !m_bAxisCanCap || m_nOwningTeam == 2 )
			return;
		int n = GetNumAlive( 2 );
		if ( n != numAxis )
			return;
		m_iCappingRequired = requiredAxis;
		m_iCappingPlayers = n;
		StartCapture( 2, 1 );
		return;
	}

	if ( m_iCapMode == 1 )
	{
		int players = ( m_nCapturingTeam == 1 ) ? numAllies : numAxis;
		int required = ( m_nCapturingTeam == 1 ) ? requiredAllies : requiredAxis;
		if ( (float)required <= (float)players )
			m_iCapMode = 0;
		float t = m_fTimeRemaining - ( (float)players / (float)required ) * 0.1f;
		m_fTimeRemaining = t;
		if ( ( numAllies != 0 || numAxis != 0 ) && ( numAxis < 1 || numAllies < 1 ) )
		{
			if ( t <= 0.0f )
			{
				EndCapture( m_nCapturingTeam );
				return;
			}
			if ( (float)players == (float)m_iCappingPlayers )
				return;
			m_iCappingPlayers = players;
			if ( !m_pPoint || ( m_pPoint->pev->spawnflags & 1 ) )
				return;
			MESSAGE_BEGIN( MSG_ALL, gmsgProgBarUpdate );
				WRITE_BYTE( m_pPoint->m_iIndex );
				WRITE_COORD( (float)m_iCappingRequired / (float)players * (float)m_nCapTime );
			MESSAGE_END();
			return;
		}
		BreakCapture( m_nCapturingTeam );
		return;
	}

	float t = m_fTimeRemaining - 0.1f;
	m_fTimeRemaining = t;
	if ( numAxis > 0 && numAllies > 0 )
	{
		BreakCapture( m_nCapturingTeam );
		return;
	}
	if ( numAllies == 0 && numAxis == 0 )
	{
		BreakCapture( m_nCapturingTeam );
		return;
	}
	if ( m_nCapturingTeam == 1 )
	{
		if ( requiredAllies <= numAllies )
			goto timeCheck;
		if ( ( pev->spawnflags & 1 ) && CanUseCatchup() && GetNumAlive( 1 ) == numAllies )
		{
			m_iCapMode = 1;
			goto timeCheck;
		}
		if ( m_bCapturing )
		{
			BreakCapture( m_nCapturingTeam );
			t = m_fTimeRemaining;
		}
	}
	else
	{
		if ( m_nCapturingTeam != 2 || requiredAxis <= numAxis )
			goto timeCheck;
		if ( ( pev->spawnflags & 1 ) && CanUseCatchup() && GetNumAlive( 2 ) == numAxis )
		{
			m_iCapMode = 1;
			goto timeCheck;
		}
		if ( m_bCapturing )
			BreakCapture( m_nCapturingTeam );
	}
	t = m_fTimeRemaining;
timeCheck:
	if ( t > 0.0f )
		return;
	EndCapture( m_nCapturingTeam );
}

// .text:0015ad40 SendNumPlayers (file:triggers.cpp:5886): PlayersInArea msg
// (point index, capturing team, player count, required) to all / one player.
// .text:0015afb0 ReSendSwipeInfo (file:triggers.cpp:5934): ProgBarUpdate
// resend with required/players scaled captime.
// DOD control_point - original file triggers.cpp - source restore/objectives/control_point.cpp
// control_point.cpp -- dod_control_point + dod_control_point_master
// Restored from dod.so. Ghidra addr = link addr + 0x10000.
#include "objective.h"

// KeyValue fields, dod_control_point (all handled, rest => CBaseEntity):
// point_name => pev->netname | point_pointvalue => m_iPointValue
// point_team_points => m_iTeamPoints | point_points_for_cap => m_iCapPoints
// point_reset_time => m_iResetTime | point_default_owner => m_iDefaultOwner (0..2)
// point_axis_capsound => pev->noise2 | point_allies_capsound => pev->noise1
// point_resetsound => pev->noise | point_allies_model / point_axis_model /
//   point_reset_model (+ _bodygroup ints, default 0)
// point_allies_target / point_axis_target / point_reset_target
// point_can_allies_touch => m_bAlliesCantTouch (NOTE inverted: >0 blocks)
// point_can_axis_touch => m_bAxisCantTouch (same inversion)
// visible => m_bPointVisible | point_win_string => sz_WinString
// point_index => m_iPointIndex = value - 1 | point_group => m_szGroup
// master => m_sMaster | point_hud_icon_allies/axis/neutral
// point_allies_win_string / point_axis_win_string

// .text:00159f10 CControlPoint::Spawn() (file:triggers.cpp:3968)
void CControlPoint::Spawn()
{
	Precache();
	pev->solid = SOLID_BBOX;  // 1
	pev->movetype = MOVETYPE_NONE;
	pev->sequence = 0;
	pev->framerate = 1.0;
	m_fCapDelayTime = 1.0;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	pev->movedir = g_vecZero;
	UTIL_MakeVectors( pev->angles );
	SetTouch( NULL );
	if ( m_bActive )
	{
		SetOwner( m_iDefaultOwner, FALSE, NULL );
	}
	else
	{
		SET_MODEL( edict(), sz_ResetModel );
		SetBodygroup( 0, m_iResetModelBodygroup );
	}
	RoundStore();
}

// .text:00155bf0 CControlPoint::KeyValue(KeyValueData *) (file:triggers.cpp:4367)
// See table in header comment above. point_index stored 0-based.

// .text:0014ee60 CControlPoint::Precache() (file:triggers.cpp:4315)
void CControlPoint::Precache()
{
	if ( pev->noise )
		PRECACHE_SOUND( STRING( pev->noise ) );
	if ( pev->noise1 )
		PRECACHE_SOUND( STRING( pev->noise1 ) );
	if ( pev->noise2 )
		PRECACHE_SOUND( STRING( pev->noise2 ) );
	PRECACHE_MODEL( sz_AlliesModel );
	PRECACHE_MODEL( sz_AxisModel );
	PRECACHE_MODEL( sz_ResetModel );
}

// .text:0015a0c0 CControlPoint::Touch(CBaseEntity *pOther) (file:triggers.cpp:3994)
// Instant steal on touch. Guards: RoundState()==1, m_bActive,
// gpGlobals->time >= m_fNextCapTime, IsPlayer + IsAlive checks,
// per-team can-touch block, no-op when toucher team == owner.
void CControlPoint::Touch( CBaseEntity *pOther )
{
	if ( g_pGameRules->RoundState() != 1 )
		return;
	if ( !m_bActive )
		return;
	if ( gpGlobals->time < m_fNextCapTime )
		return;
	if ( !pOther->IsPlayer() || !pOther->IsAlive() )
		return;
	int team = pOther->pev->team;
	if ( team == 1 && m_bAlliesCantTouch )
		return;
	if ( team == 2 && m_bAxisCantTouch )
		return;
	if ( team != m_iTeam )
	{
		if ( !m_bActive )
		{
			SET_MODEL( edict(), sz_ResetModel );
			SetBodygroup( 0, m_iResetModelBodygroup );
		}
		else
		{
			SetOwner( team, TRUE, (CBasePlayer *) pOther );
		}
		// one-time hint per player (bit 0x80 in m_flDisplayHistory)
		if ( ( ( (CBasePlayer *) pOther )->m_flDisplayHistory & 0x80 ) == 0 )
		{
			( (CBasePlayer *) pOther )->m_flDisplayHistory |= 0x80;
			( (CBasePlayer *) pOther )->HintMessageOnce( "#Hint_touched_control_point" );
		}
		UTIL_LogPrintf( "%s triggered a dod_control_point - %s", PlayerLogLine( pOther ), STRING( pev->netname ) );
		m_fNextCapTime = gpGlobals->time + m_fCapDelayTime;
	}
}

// .text:00159a60 CControlPoint::SetOwner(int, BOOL, CBasePlayer *) (file:triggers.cpp:4206)
// Guards: not freeze/restart, RoundState 0/1, not game over.
// Effects: m_iTeam, team model + bodygroup, cap sound, team target,
// AddObjScore(player, m_iCapPoints), team score update, SetObjOwner msg,
// cap string, m_fNextCapTime = time + m_fCapDelayTime.
void CControlPoint::SetOwner( int owner, BOOL bMakeSound, CBasePlayer *pPlayer )
{
	if ( g_pGameRules->IsFreezePeriod() || g_pGameRules->IsRoundRestarting() )
		return;
	int st = g_pGameRules->RoundState();
	if ( st != 1 && st != 0 )
		return;
	if ( g_fGameOver )
		return;
	m_iTeam = owner;
	if ( owner == 1 )
	{
		if ( bMakeSound && pev->noise1 )
			EMIT_SOUND_DYN( edict(), CHAN_ITEM, STRING( pev->noise1 ), 1.0, 0, 0, 100 );
		SET_MODEL( edict(), sz_AlliesModel );
		SetBodygroup( 0, m_iAlliesModelBodygroup );
		FireTargets( sz_AlliesTarget, this, this, USE_TOGGLE, 0.0 );
	}
	else if ( owner == 2 )
	{
		if ( bMakeSound && pev->noise2 )
			EMIT_SOUND_DYN( edict(), CHAN_ITEM, STRING( pev->noise2 ), 1.0, 0, 0, 100 );
		SET_MODEL( edict(), sz_AxisModel );
		SetBodygroup( 0, m_iAxisModelBodygroup );
		FireTargets( sz_AxisTarget, this, this, USE_TOGGLE, 0.0 );
	}
	else
	{
		if ( bMakeSound && pev->noise )
			EMIT_SOUND_DYN( edict(), CHAN_ITEM, STRING( pev->noise ), 1.0, 0, 0, 100 );
		SET_MODEL( edict(), sz_ResetModel );
		SetBodygroup( 0, m_iResetModelBodygroup );
		FireTargets( sz_ResetTarget, this, this, USE_TOGGLE, 0.0 );
	}
	if ( bMakeSound && g_pGameRules )
	{
		if ( pPlayer )
			pPlayer->AddObjScore( m_iCapPoints, TRUE );
		if ( owner == 1 || owner == 2 )
			g_pGameRules->RecheckScores();
	}
	if ( !( pev->spawnflags & 1 ) )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgSetObjOwner );
		WRITE_BYTE( m_iIndex );
		WRITE_BYTE( owner );
		WRITE_BYTE( 0 );
		MESSAGE_END();
		UTIL_SendDirectorMessage( this, NULL, 0x0b );
	}
	if ( bMakeSound && owner != 0 )
		SendCapString( owner, pPlayer );
	m_fNextCapTime = gpGlobals->time + m_fCapDelayTime;
}

// .text:00159980 CControlPoint::SendCapString(int, CBasePlayer *) (file:triggers.cpp:4049)
void CControlPoint::SendCapString( int team, CBasePlayer *pPlayer )
{
	if ( sz_WinString[0] == 0 )
		return;
	MESSAGE_BEGIN( MSG_ALL, gmsgCapMsg );
	WRITE_BYTE( pPlayer ? ENTINDEX( pPlayer->edict() ) : 0 );
	WRITE_STRING( STRING( pev->netname ) );
	WRITE_BYTE( team );
	MESSAGE_END();
}

// .text:0015a4e0 CControlPoint::Reset() (file:triggers.cpp:4175)
// .text:0015a050 CControlPoint::RoundRespawn() (file:triggers.cpp:4196)
// Active: SetOwner(m_iDefaultOwner, FALSE, NULL) (verified by disasm:
// EDX=[this+0x184], ECX=EAX=0, JMP SetOwner). Inactive: reset model.
void CControlPoint::Reset()
{
	if ( m_bActive )
		SetOwner( m_iDefaultOwner, FALSE, NULL );
	else
	{
		SET_MODEL( edict(), sz_ResetModel );
		SetBodygroup( 0, m_iResetModelBodygroup );
	}
}

// .text:0015a050 CControlPoint::RoundRespawn() (file:triggers.cpp:4196)
void CControlPoint::RoundRespawn()
{
	if ( m_bActive )
		SetOwner( m_iDefaultOwner, FALSE, NULL );
	else
	{
		SET_MODEL( edict(), sz_ResetModel );
		SetBodygroup( 0, m_iResetModelBodygroup );
	}
}

// .text:0015a550 CControlPoint::SetIndex(int) (file:triggers.cpp:4183)
void CControlPoint::SetIndex( int i_index )
{
	if ( g_pGameRules && m_iAlliesIcon == 1 && g_pGameRules->IsAlliesBrit() )
		m_iAlliesIcon = 0x1b;  // 27, brit HUD icon
	m_iIndex = i_index;
}

// .text:0015a640 CControlPoint::SetActive(bool) (file:triggers.cpp:4536)
void CControlPoint::SetActive( bool active )
{
	m_bActive = active;
	if ( !active )
		pev->effects |= EF_NODRAW;	// 0x80
	else
		pev->effects &= ~EF_NODRAW;
}

// .text:0015a610 GetOwner / .text:0015a620 GetGroup / .text:0015a630 PointValue
// trivial field readers (triggers.cpp:4295/4300/4310).

// .text:001597d0 IsThisIndexUnique(CControlPoint *) (file:triggers.cpp:3925)
BOOL IsThisIndexUnique( CControlPoint *pThis )
{
	CControlPoint *p = (CControlPoint *) UTIL_FindEntityByClassname( NULL, "dod_control_point" );
	while ( p )
	{
		if ( p->m_iPointIndex == pThis->m_iPointIndex && p != pThis )
			return FALSE;
		p = (CControlPoint *) UTIL_FindEntityByClassname( p, "dod_control_point" );
	}
	return TRUE;
}
// DOD control_master - original file triggers.cpp - source restore/objectives/control_master.cpp
// control_master.cpp -- dod_control_point_master
// Restored from dod.so. Ghidra addr = link addr + 0x10000.
#include "objective.h"

// KeyValue fields, dod_control_point_master:
// point_give_delay_time => m_iGivePointsDelay | allies_capture_target,
// axis_capture_target | main => m_iMainObjective | master => m_sMaster
// cpm_group => m_szGroup (matched against point_group). Rest unhandled.
// .text:00152640 KeyValue (file:triggers.cpp:4613)

// .text:0014f3d0 CControlPointMaster::Spawn() (file:triggers.cpp:4571)
void CControlPointMaster::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;  // 0x80
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	SetTouch( NULL );
	SetThink( &CControlPointMaster::CPMThink );
	FoundPoints = 0;
	m_bActive = 0;
	pev->nextthink = gpGlobals->time + 2.0;				   // _DAT_00192a44
	m_flNextSpectatorUpdateTime = gpGlobals->time + 10.0;  // _DAT_0018fde8
	RoundStore();
}

// .text:0014fc10 CControlPointMaster::RoundRespawn() (file:triggers.cpp:4595)
// Clan scoring active: next give = now + mp_clan_scoring_delay (default 30.0).
// Else: now + m_iGivePointsDelay.
void CControlPointMaster::RoundRespawn()
{
	if ( mp_clan_match && mp_clan_match->value > 0.0 && mp_clan_scoring && mp_clan_scoring->value > 0.0 )
	{
		float d = 30.0;
		if ( mp_clan_scoring_delay )
			d = mp_clan_scoring_delay->value;
		m_fGivePointsTime = gpGlobals->time + d;
		return;
	}
	m_fGivePointsTime = (float) m_iGivePointsDelay + gpGlobals->time;
}

// .text:0015a700 Reset (file:triggers.cpp:4650): m_bTeamHasWon = false.
// .text:0015a810 GetPointOwner(int) (file:triggers.cpp:5089): team of slot,
// 0 when slot empty/out of range.
// .text:0015a9c0 IsMasteredOn() (file:triggers.cpp:5137): true when no master
// or UTIL_IsMasterTriggered.
// .text:0015a9f0 TeamOwnsAllPoints() (file:triggers.cpp:5158): common owner of
// all slots, 0 when mixed.

// .text:00151880 BecomeActive() (file:triggers.cpp:5044)
// Activates every discovered point (m_bActive, clear EF_NODRAW) and broadcasts
// InitObjDisplay to all.
void CControlPointMaster::BecomeActive()
{
	for ( int i = 0; i < m_iNumPoints; i++ )
	{
		ControlPointArray[i]->m_bActive = true;
		ControlPointArray[i]->pev->effects &= ~EF_NODRAW;
	}
	SendObjectiveStatus( NULL );
	m_bActive = 1;
}

// .text:0015a7d0 BecomeInactive() (file:triggers.cpp:5079)
void CControlPointMaster::BecomeInactive()
{
	for ( int i = 0; i < m_iNumPoints; i++ )
	{
		ControlPointArray[i]->m_bActive = false;
		ControlPointArray[i]->pev->effects |= EF_NODRAW;
	}
	m_bActive = 0;
}

// .text:0015a840 SendObjectiveStatus(CBasePlayer *) (file:triggers.cpp:5105)
// Per player or broadcast InitObjDisplay: numPoints, then per point
// edict index, m_iIndex, team, visible bit, 3 HUD icons, origin x/y.
void CControlPointMaster::SendObjectiveStatus( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		MESSAGE_BEGIN( MSG_ALL, gmsgInitObjDisplay );
	else
		MESSAGE_BEGIN( MSG_ONE, gmsgInitObjDisplay, NULL, pPlayer->edict() );
	WRITE_BYTE( m_iNumPoints );
	for ( int i = 0; i < m_iNumPoints; i++ )
	{
		CControlPoint *p = ControlPointArray[i];
		WRITE_SHORT( ENTINDEX( p->edict() ) );
		WRITE_BYTE( p->m_iIndex );
		WRITE_BYTE( ( i < 20 && p ) ? p->m_iTeam : 0 );
		WRITE_BYTE( ( p->pev->spawnflags & 1 ) ^ 1 );
		WRITE_BYTE( p->m_iNeutralIcon );
		WRITE_BYTE( p->m_iAlliesIcon );
		WRITE_BYTE( p->m_iAxisIcon );
		WRITE_COORD( p->pev->origin.x );
		WRITE_COORD( p->pev->origin.y );
	}
	MESSAGE_END();
}

// .text:00154930 CPMThink() (file:triggers.cpp:4688), think every 0.5 s.
// Stages:
// 1. Master gate: m_sMaster via UTIL_IsMasterTriggered. While off, points
//    forced inactive + hidden. When on and not active, BecomeActive.
// 2. Discovery (once): collect dod_control_point with matching cpm_group,
//    max 20. Indexing: point_index values when unique, else distance order
//    from info_player_allies. Brit fixup of allies icon 1 -> 27.
//    Broadcasts InitObjDisplay. Error to console when zero points.
// 3. Idle states: RoundState 5 (or inactive / 3 / 4) => reschedule 0.5 s.
// 4. Periodic score (RoundState==1, game live, past m_fGivePointsTime):
//    sum m_iPointValue per owner team, require a live player of that team,
//    AddToTeamScores(team, sum). Clan variant uses mp_clan_scoring_values_*
//    digit strings per slot. Reschedule by delay (clan delay or give delay).
// 5. Win check: all slots share owner != 0 => FireTargets(allies/axis target),
//    m_bTeamHasWon = true.
// 6. Spectator refresh every 20.0 s: InitObjDisplay to MSG_SPEC + TimeLeft.
extern int GetClanScoringValues( int team, int cp );
extern int get_maptime_left( void );

void CControlPointMaster::CPMThink()
{
	// ---- stage 1: master gate / activation ----
	if ( !m_sMaster )
	{
		if ( !m_bActive )
			BecomeActive();
	}
	else if ( !FoundPoints )
	{
		m_bMasteredOn = UTIL_IsMasterTriggered( m_sMaster, this ) ? 1 : 0;
		if ( !m_bMasteredOn )
		{
			for ( int i = 0; i < m_iNumPoints; i++ )
			{
				ControlPointArray[i]->m_bActive = false;
				ControlPointArray[i]->pev->effects |= EF_NODRAW;
			}
			m_bActive = 0;
		}
		else if ( !m_bActive )
			BecomeActive();
	}
	else
	{
		BOOL on = UTIL_IsMasterTriggered( m_sMaster, this ) ? 1 : 0;
		if ( m_bMasteredOn != on )
		{
			m_bMasteredOn = on;
			if ( !on )
			{
				for ( int i = 0; i < m_iNumPoints; i++ )
				{
					ControlPointArray[i]->m_bActive = false;
					ControlPointArray[i]->pev->effects |= EF_NODRAW;
				}
				m_bActive = 0;
			}
			else if ( !m_bActive )
				BecomeActive();
		}
	}

	// ---- stage 2: one-time discovery ----
	if ( !FoundPoints )
	{
		CControlPoint *pPoint = (CControlPoint *) UTIL_FindEntityByClassname( NULL, "dod_control_point" );
		if ( !pPoint )
		{
			ALERT( at_aiconsole, "Error! No control points found in map!\n" );
			return;
		}
		BOOL bUnique = TRUE;
		int num = 0;
		do
		{
			if ( strcmp( m_szGroup, pPoint->m_szGroup ) == 0 )
			{
				if ( pPoint->m_iPointIndex < 0 )
					bUnique = FALSE;
				ControlPointArray[num] = pPoint;
				for ( int i = num - 1; i >= 0; i-- )
				{
					if ( ControlPointArray[i]->m_iPointIndex == pPoint->m_iPointIndex )
						bUnique = FALSE;
				}
				if ( g_pGameRules && pPoint->m_iAlliesIcon == 1 &&
					 ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesBrit() )
					pPoint->m_iAlliesIcon = 0x1b;
				pPoint->m_iIndex = pPoint->m_iPointIndex;
				pPoint->m_bActive = ( m_bActive != 0 );
				if ( pPoint->m_bActive )
					pPoint->pev->effects &= ~EF_NODRAW;
				else
					pPoint->pev->effects |= EF_NODRAW;
				num++;
			}
			pPoint = (CControlPoint *) UTIL_FindEntityByClassname( pPoint, "dod_control_point" );
		} while ( num < 20 && pPoint );

		if ( !bUnique )
		{
			CBaseEntity *pSpawn = UTIL_FindEntityByClassname( NULL, "info_player_allies" );
			if ( !pSpawn )
			{
				if ( num == 0 )
				{
					ALERT( at_aiconsole, "Error! No control points found in map!\n" );
					return;
				}
				for ( int i = 0; i < num; i++ )
				{
					CControlPoint *p = ControlPointArray[i];
					if ( p )
					{
						if ( g_pGameRules && p->m_iAlliesIcon == 1 &&
							 ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesBrit() )
							p->m_iAlliesIcon = 0x1b;
						p->m_iIndex = i;
					}
				}
			}
			else
			{
				Vector spawnpos = pSpawn->pev->origin;
				int handled[20];
				for ( int i = 0; i < num; i++ )
					handled[i] = 0;
				for ( int slot = 0; slot < num; slot++ )
				{
					float leastdist = 99999.0f;
					int selected = 0;
					for ( int i = 0; i < num; i++ )
					{
						if ( handled[i] )
							continue;
						float dx = spawnpos.x - ControlPointArray[i]->pev->origin.x;
						float dy = spawnpos.y - ControlPointArray[i]->pev->origin.y;
						float dz = spawnpos.z - ControlPointArray[i]->pev->origin.z;
						float dist = sqrt( dx * dx + dy * dy + dz * dz );
						if ( dist < leastdist )
						{
							leastdist = dist;
							selected = i;
						}
					}
					CControlPoint *p = ControlPointArray[selected];
					if ( g_pGameRules && p->m_iAlliesIcon == 1 &&
						 ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesBrit() )
						p->m_iAlliesIcon = 0x1b;
					p->m_iIndex = slot;
					handled[selected] = 1;
				}
			}
		}
		else if ( num == 0 )
		{
			ALERT( at_aiconsole, "Error! No control points found in map!\n" );
			return;
		}

		m_iNumPoints = num;
		FoundPoints = 1;
		SendObjectiveStatus( NULL );
	}

	// ---- stage 3: idle round states ----
	int iRoundState = g_pGameRules->RoundState();
	if ( iRoundState == 5 || !m_bActive || iRoundState == 4 || iRoundState == 3 )
	{
		pev->nextthink = gpGlobals->time + 0.5;	 // _DAT_0018eca4
		return;
	}

	// ---- stage 4: periodic scoring ----
	CDoDTeamPlay *pRules = (CDoDTeamPlay *) g_pGameRules;
	BOOL bClanMatch = pRules->IsClanMatch() && pRules->IsTeamplay();
	BOOL bClanScoring = ( mp_clan_scoring && mp_clan_scoring->value > 0.0 );

	int numPoints = m_iNumPoints;
	int alliesPoints = 0;
	int axisPoints = 0;
	if ( gpGlobals->time > m_fGivePointsTime )
	{
		if ( bClanMatch && bClanScoring )
		{
			for ( int i = 0; i < numPoints; i++ )
			{
				int team = ControlPointArray[i]->m_iTeam;
				if ( team == 1 )
					alliesPoints += GetClanScoringValues( 1, i );
				else if ( team == 2 )
					axisPoints += GetClanScoringValues( 2, i );
			}
		}
		else
		{
			for ( int i = 0; i < numPoints; i++ )
			{
				if ( ControlPointArray[i]->m_iTeam == 1 )
					alliesPoints += ControlPointArray[i]->m_iPointValue;
				else if ( ControlPointArray[i]->m_iTeam == 2 )
					axisPoints += ControlPointArray[i]->m_iPointValue;
			}
		}
		BOOL bAxisPoints = ( axisPoints > 0 );

		BOOL bFoundAllies = FALSE;
		BOOL bFoundAxis = FALSE;
		for ( int i = 1; i <= gpGlobals->maxClients && !( bFoundAllies && bFoundAxis ); i++ )
		{
			CBaseEntity *pPlr = UTIL_PlayerByIndex( i );
			if ( !pPlr )
				continue;
			if ( pPlr->pev->team == 1 )
				bFoundAllies = TRUE;
			else if ( pPlr->pev->team == 2 )
				bFoundAxis = TRUE;
		}

		if ( bFoundAllies && alliesPoints > 0 && g_pGameRules->RoundState() == 1 && !g_fGameOver )
		{
			if ( bClanMatch )
			{
				if ( alliesPoints == 1 )
					UTIL_ClientPrintAll( 3, "#game_score_allie_point", 0, 0, 0, 0 );
				else
					UTIL_ClientPrintAll( 3, "#game_score_allie_points", UTIL_dtos1( alliesPoints ), 0, 0, 0 );
			}
			pRules->AddToTeamScores( 1, alliesPoints );
		}
		if ( bAxisPoints && bFoundAxis && g_pGameRules->RoundState() == 1 && !g_fGameOver )
		{
			if ( bClanMatch )
			{
				if ( axisPoints == 1 )
					UTIL_ClientPrintAll( 3, "#game_score_axis_point", 0, 0, 0, 0 );
				else
					UTIL_ClientPrintAll( 3, "#game_score_axis_points", UTIL_dtos1( axisPoints ), 0, 0, 0 );
			}
			pRules->AddToTeamScores( 2, axisPoints );
		}

		if ( bClanMatch && bClanScoring )
		{
			float d = 30.0;	 // _DAT_0018fdec
			if ( mp_clan_scoring_delay )
				d = mp_clan_scoring_delay->value;
			m_fGivePointsTime = gpGlobals->time + d;
		}
		else
			m_fGivePointsTime = (float) m_iGivePointsDelay + gpGlobals->time;
	}

	// ---- stage 5: full-hold win check ----
	int team = ControlPointArray[0]->m_iTeam;
	BOOL allSame = ( numPoints < 2 );
	if ( !allSame && ControlPointArray[1]->m_iTeam == team )
	{
		allSame = TRUE;
		for ( int i = 1; i < numPoints; i++ )
		{
			if ( ControlPointArray[i]->m_iTeam != team )
			{
				allSame = FALSE;
				break;
			}
		}
	}
	if ( allSame && team > 0 )
	{
		if ( team == 1 )
		{
			FireTargets( sz_AlliesTarget, NULL, this, USE_TOGGLE, 0.0 );
			m_bTeamHasWon = true;
		}
		else if ( team == 2 )
		{
			FireTargets( sz_AxisTarget, NULL, this, USE_TOGGLE, 0.0 );
			m_bTeamHasWon = true;
		}
	}

	// ---- stage 6: spectator refresh every 20 s ----
	if ( m_flNextSpectatorUpdateTime < gpGlobals->time )
	{
		MESSAGE_BEGIN( MSG_SPEC, gmsgInitObjDisplay );
		WRITE_BYTE( m_iNumPoints );
		for ( int i = 0; i < m_iNumPoints; i++ )
		{
			CControlPoint *p = ControlPointArray[i];
			WRITE_SHORT( ENTINDEX( p->edict() ) );
			WRITE_BYTE( p->m_iIndex );
			WRITE_BYTE( ( i < 20 && p ) ? p->m_iTeam : 0 );
			WRITE_BYTE( ( p->pev->spawnflags & 1 ) ^ 1 );
			WRITE_BYTE( p->m_iNeutralIcon );
			WRITE_BYTE( p->m_iAlliesIcon );
			WRITE_BYTE( p->m_iAxisIcon );
			WRITE_COORD( p->pev->origin.x );
			WRITE_COORD( p->pev->origin.y );
		}
		MESSAGE_END();
		MESSAGE_BEGIN( MSG_SPEC, gmsgTimeLeft );
		WRITE_SHORT( get_maptime_left() );
		MESSAGE_END();
		m_flNextSpectatorUpdateTime = gpGlobals->time + 20.0;  // _DAT_00190294
	}
	pev->nextthink = gpGlobals->time + 0.5;	 // _DAT_0018eca4
}
// DOD timers_relays triggers parts - original files triggers.cpp plus mapmarker plus dod_gamerules notes - source restore/objectives/timers_relays.cpp
// timers_relays.cpp -- dod_round_timer, dod_point_relay, dod_location,
// dod_mapmarker, info_doddetect, para timer. Restored from dod.so.
#include "objective.h"

// ---- dod_round_timer (CDodRoundTimer, triggers.cpp) ----
// KeyValue: round_timer_length (int seconds) => m_fTimerLength,
// master => m_sMaster. Rest unhandled.
// .text:00151b50 KeyValue (file:triggers.cpp:6508)
// .text:0014ef60 Spawn (file:triggers.cpp:6372): solid none, timer off,
// think 0.1 s.
// Think cadence 0.1 s (.text:001559f0, file:triggers.cpp:6380): counts down
// m_fRoundTime; HUD TimerStatus(on, remaining) at most every 30.0 s and only
// when spawnflags & 1 + master allows. At zero: timer off, zero status,
// FireTargets(pev target). Only while m_bTimer.
// .text:00154800 Use (file:triggers.cpp:6461): toggles m_bTimer with the same
// gated status send (remaining on start, 0.0 on stop).
// .text:0015c0b0 StartTimer (file:triggers.cpp:6412): on + status.
// .text:0015c160 StopTimer (file:triggers.cpp:6422): off + zero status.
// .text:0015c1e0 ResetTimer (file:triggers.cpp:6440): off, update 30.0,
// m_fRoundTime = m_fTimerLength + status.
// .text:0015c290 / .text:0015c320 SendStatus (file:triggers.cpp:6470/6489):
// broadcast / unicast TimerStatus, same gates.

// ---- dod_point_relay (CDodPointRelay, triggers.cpp) ----
// KeyValue: dod_relay_team => m_nTeam (default -1), else unhandled.
// .text:00151c10 (file:triggers.cpp:6348)
// .text:0014ef30 Spawn (file:triggers.cpp:6288): solid none, Use = DodRelayUse.
// .text:0015a2c0 DodRelayUse (file:triggers.cpp:6296): finds point by target.
// USE_START (and point visible): StartProgressBar(index, m_nTeam, value).
// USE_BREAK (and visible): CancelProgressBar. Else: SetOwner path when the
// point is active: activator player + useType/value forwarded as
// (owner, sound, player) args; inactive point + no player activator =>
// reset model + bodygroup.
void CDodPointRelay::DodRelayUse( CBaseEntity *pA, CBaseEntity *pC, USE_TYPE u, float v )
{
	CControlPoint *p = (CControlPoint *) UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
	if ( !p )
		return;
	if ( u == USE_START )
	{
		if ( p->pev->spawnflags & 1 )
			return;
		MESSAGE_BEGIN( MSG_ALL, gmsgStartProgressBar );
		WRITE_BYTE( p->m_iIndex );
		WRITE_BYTE( m_nTeam );
		WRITE_SHORT( (int) v );
		MESSAGE_END();
		return;
	}
	if ( u == USE_BREAK )
	{
		if ( p->pev->spawnflags & 1 )
			return;
		MESSAGE_BEGIN( MSG_ALL, gmsgCancelProgressBar );
		WRITE_BYTE( p->m_iIndex );
		WRITE_BYTE( m_nTeam );
		MESSAGE_END();
		return;
	}
	if ( !p->m_bActive )
	{
		SET_MODEL( p->edict(), p->sz_ResetModel );
		p->SetBodygroup( 0, p->m_iResetModelBodygroup );
		return;
	}
	int owner = 0;
	CBasePlayer *pPlayer = NULL;
	if ( pA && pA->IsPlayer() )
	{
		owner = pA->pev->team;
		pPlayer = (CBasePlayer *) pA;
	}
	p->SetOwner( owner, TRUE, pPlayer );
}

// .text:0014ef30 CDodPointRelay::Spawn (file:triggers.cpp:6288)
void CDodPointRelay::Spawn()
{
	pev->solid = SOLID_NOT;
	SetUse( &CDodPointRelay::DodRelayUse );
}

// .text:00151c10 CDodPointRelay::KeyValue (file:triggers.cpp:6348)
void CDodPointRelay::KeyValue( KeyValueData *pkvd )
{
	m_nTeam = -1;
	if ( FStrEq( pkvd->szKeyName, "dod_relay_team" ) )
	{
		m_nTeam = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

// .text:0014f4e0 CLocation::Spawn (file:triggers.cpp:3872)
void CLocation::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	SetTouch( NULL );
	pev->movedir = g_vecZero;
	RoundStore();
}

// .text:0014f4a0 CLocation::RoundRespawn (file:triggers.cpp:3888)
void CLocation::RoundRespawn()
{
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	SetTouch( NULL );
}

// .text:00151d50 CLocation::KeyValue (file:triggers.cpp:3895)
void CLocation::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "location_name" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

// .text:0014ef60 CDodRoundTimer::Spawn (file:triggers.cpp:6372)
void CDodRoundTimer::Spawn()
{
	m_bTimer = false;
	pev->solid = SOLID_NOT;
	m_fLastThinkTime = 0.0;
	pev->nextthink = gpGlobals->time + 0.1;	 // _DAT_0018ee74
}

// .text:00151b50 CDodRoundTimer::KeyValue (file:triggers.cpp:6508)
void CDodRoundTimer::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "round_timer_length" ) )
	{
		m_fTimerLength = (float) atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

// .text:00154800 CDodRoundTimer::Use (file:triggers.cpp:6461)
void CDodRoundTimer::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !m_bTimer )
	{
		if ( UTIL_IsMasterTriggered( m_sMaster, this ) )
		{
			m_bTimer = true;
			if ( ( pev->spawnflags & 1 ) && UTIL_IsMasterTriggered( m_sMaster, this ) && m_bTimer )
			{
				MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
				WRITE_BYTE( m_bTimer );
				WRITE_COORD( m_fRoundTime );
				MESSAGE_END();
			}
		}
	}
	else
	{
		m_bTimer = false;
		if ( ( pev->spawnflags & 1 ) && UTIL_IsMasterTriggered( m_sMaster, this ) )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
			WRITE_BYTE( m_bTimer );
			WRITE_COORD( 0.0 );
			MESSAGE_END();
		}
	}
}

// .text:001559f0 CDodRoundTimer::Think (file:triggers.cpp:6380)
void CDodRoundTimer::Think()
{
	pev->nextthink = gpGlobals->time + 0.1;	 // _DAT_0018ee74
	if ( !m_bTimer )
		return;
	m_fRoundTime -= 0.1f;
	m_fUpdateTimer -= 0.1f;
	if ( m_fUpdateTimer <= 0.0f )
	{
		if ( ( pev->spawnflags & 1 ) && UTIL_IsMasterTriggered( m_sMaster, this ) && m_bTimer )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
			WRITE_BYTE( m_bTimer );
			WRITE_COORD( m_fRoundTime );
			MESSAGE_END();
		}
		m_fUpdateTimer = 30.0;
	}
	if ( m_fRoundTime <= 0.0f )
	{
		m_bTimer = false;
		if ( ( pev->spawnflags & 1 ) && UTIL_IsMasterTriggered( m_sMaster, this ) )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
			WRITE_BYTE( m_bTimer );
			WRITE_COORD( 0.0 );
			MESSAGE_END();
		}
		if ( pev->target )
			FireTargets( STRING( pev->target ), this, NULL, USE_TOGGLE, 0.0 );
	}
}

// ---- dod_location (CLocation, triggers.cpp) ----
// KeyValue: location_name => pev->netname, master => m_sMaster.
// .text:00151d50 (file:triggers.cpp:3895)
// .text:0014f4e0 Spawn (file:triggers.cpp:3872): solid/movetype none,
// zero size, no touch. .text:0014f4a0 RoundRespawn (file:triggers.cpp:3888):
// zero size, no touch. Pure position + name marker.

// ---- dod_mapmarker (CMapMarker, mapmarker.cpp, 4 funcs) ----
// .text:0016ff50 CreateMapMarker (file:mapmarker.cpp:20): edict alloc,
// classname dod_mapmarker, icon => pev->body, owner/team from creator,
// angles (0,90,0), fuser1 = now, static counter++.
// .text:0016fdb0 Spawn (file:mapmarker.cpp:47): solid none, markernull
// sprite, zero size, rehash classname, Think = Die at now + stay time
// (max(mp_markerstaytime, 2.0)). .text:0016fda0 Die (file:mapmarker.cpp:70):
// UpdateOnRemove + FL_KILLME + clear targetname.
// 4th func: LINK factory thunk. Player side (Fire/Next/Prev/SetMapMarker,
// player.cpp:6390-6484) belongs to the player agent; HUD icon flows through
// pev->body.

// ---- info_doddetect (CDoDDetect, dod_gamerules.cpp) ----
// Ctor defaults (.text:000ee020, file:dod_gamerules.cpp:6387): infinite
// lives both true, paras false, brit false, timer off, elim points 5/5,
// timer-expire points 0, respawn factors 1.0, timer team 0.
// .text:000ea0a0 Spawn: hash classname, solid none, DetectGameRules().
// .text:000ee0b0 IsMasteredOn: no master or master triggered.
// KeyValue (.text:000e5420, file:dod_gamerules.cpp:6403) table:
// detect_axis_infinite / allies_infinite_lifes / axis_infinite_lifes (==1) /
// detect_allies_infinite (==1) => infinite flags; detect_axis_paras /
// detect_allies_paras (>0); detect_allies_country (==1 => brit);
// detect_points_allieseliminated / axiseliminated (int);
// detect_target_allieseliminated / axiseliminated (string);
// detect_use_timer (>0); detect_target_timerexpired (string);
// detect_timer_team (int); detect_points_timerexpired (int);
// detect_allies_respawnfactor / detect_axis_respawnfactor (float);
// master => m_sMaster. Rest unhandled.

// ---- para round timer (CDodParaRoundTimer, dod_gamerules.cpp) ----
// Code-created (no map LINK). .text:000e3b90 Spawn: Think = TimerThink,
// 0.1 s, timers cleared, update 30.0. .text:000eaa80 TimerThink: frame-delta
// countdown; round not active => stop + status + Think NULL; 30 s status
// refresh; expiry => CDoDTeamPlay::TimerExpired + stop + status.
// .text:000ee150 Start / .text:000ee1f0 Stop / .text:000ee270 Reset
// (240.0 s, on, update 30.0) / .text:000ee310 / .text:000ee380 SendStatus.

// ---- gamerules hook ----
// .text:000e8e90 CDoDTeamPlay::SendObjectiveStatus (dod_gamerules.cpp:5080):
// RoundState to player, first mastered dod_control_point_master =>
// SendObjectiveStatus, then per-player area_SendStatus for dod_capture_area,
// dod_object_goal, func_breakable, plus dod_round_timer SendStatus and
// particle_shooter updates.

// ============================================================================
// Additional capture member bodies ported from dod.so (Ghidra = link + 0x10000)
// ============================================================================

// .text:0014fc90 CAreaCapture::Precache (file:triggers.cpp:5447)
void CAreaCapture::Precache()
{
	if ( sz_HudIcon[0] )
		PRECACHE_MODEL( sz_HudIcon );
}

// .text:00157040 CAreaCapture::Spawn (file:triggers.cpp:5418)
void CAreaCapture::Spawn()
{
	CBaseTrigger::InitTrigger();
	Precache();
	m_iAreaIndex = -1;
	pev->sequence = 0;
	pev->framerate = 1.0;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SetThink( NULL );
	SetTouch( &CAreaCapture::AreaTouch );
	m_bCapturing = 0;
	m_nCapturingTeam = -1;
	m_nOwningTeam = -1;
	m_fTimeRemaining = 0.0;
	pev->nextthink = gpGlobals->time + 0.1;
	RoundStore();
}

// .text:00157920 CAreaCapture::RoundRespawn (file:triggers.cpp:6153)
void CAreaCapture::RoundRespawn()
{
	CBaseTrigger::InitTrigger();
	m_iAreaIndex = -1;
	pev->sequence = 0;
	pev->framerate = 1.0;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SetTouch( &CAreaCapture::AreaTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	if ( m_bCapturing )
		BreakCapture( m_nCapturingTeam );
	m_nCapturingTeam = -1;
	m_bCapturing = 0;
	m_nOwningTeam = -1;
	m_fTimeRemaining = 0.0;
}

// .text:0015ad10 CAreaCapture::SetOwner (file:triggers.cpp:5877)
void CAreaCapture::SetOwner( int team )
{
	if ( m_bCapturing )
		BreakCapture( m_nCapturingTeam );
	m_nOwningTeam = team;
}

// .text:00153970 CAreaCapture::KeyValue (file:triggers.cpp:6180)
void CAreaCapture::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "area_allies_cancap" ) )
	{
		m_bAlliesCanCap = ( atoi( pkvd->szValue ) > 0 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_axis_cancap" ) )
	{
		m_bAxisCanCap = ( atoi( pkvd->szValue ) > 0 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_allies_numcap" ) )
	{
		int v = atoi( pkvd->szValue );
		m_nAlliesNumCap = ( v > 0 ) ? v : 1;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_axis_numcap" ) )
	{
		int v = atoi( pkvd->szValue );
		m_nAxisNumCap = ( v > 0 ) ? v : 1;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_allies_startcap" ) )
	{
		strcpy( sz_AlliesStart, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_axis_startcap" ) )
	{
		strcpy( sz_AxisStart, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_allies_endcap" ) )
	{
		strcpy( sz_AlliesCap, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_axis_endcap" ) )
	{
		strcpy( sz_AxisCap, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_allies_breakcap" ) )
	{
		strcpy( sz_AlliesBreak, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_axis_breakcap" ) )
	{
		strcpy( sz_AxisBreak, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_hud_sprite" ) )
	{
		strcpy( sz_HudIcon, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_time_to_cap" ) )
	{
		m_nCapTime = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_cap_point" ) )
	{
		strcpy( sz_CapPointName, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_object_group" ) )
	{
		strcpy( sz_RequiredObjectGroup, pkvd->szValue );
		m_bRequiresObject = ( sz_RequiredObjectGroup[0] != 0 );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

// .text:0014fbf0 CObjectCapture::Precache (file:triggers.cpp:3820)
void CObjectCapture::Precache()
{
	if ( sz_HudIcon[0] )
		PRECACHE_MODEL( sz_HudIcon );
}

// .text:00157260 CObjectCapture::Spawn (file:triggers.cpp:3646)
void CObjectCapture::Spawn()
{
	UTIL_SetOrigin( pev, pev->origin );
	CBaseTrigger::InitTrigger();
	RoundStore();
	m_bCapped = false;
}

// .text:00157230 CObjectCapture::RoundRespawn (file:triggers.cpp:3656)
void CObjectCapture::RoundRespawn()
{
	UTIL_SetOrigin( pev, pev->origin );
	CBaseTrigger::InitTrigger();
	m_bCapped = false;
}

// .text:0015d5d0 CObjectCapture::KeyValue (file:triggers.cpp:3609)
void CObjectCapture::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "capobj_group" ) )
	{
		strcpy( sz_Group, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "capobj_donetarget" ) )
	{
		strcpy( sz_DoneTarget, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "capobj_hud_sprite" ) )
	{
		strcpy( sz_HudIcon, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

// .text:00155bf0 CControlPoint::KeyValue (file:triggers.cpp:4367)
void CControlPoint::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "point_name" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_pointvalue" ) )
	{
		m_iPointValue = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_team_points" ) )
	{
		m_iTeamPoints = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_points_for_cap" ) )
	{
		m_iCapPoints = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_reset_time" ) )
	{
		m_iResetTime = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_default_owner" ) )
	{
		int v = atoi( pkvd->szValue );
		m_iDefaultOwner = ( v < 3 ) ? v : 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_axis_capsound" ) )
	{
		if ( pkvd->szValue )
			pev->noise2 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_allies_capsound" ) )
	{
		if ( pkvd->szValue )
			pev->noise1 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_resetsound" ) )
	{
		if ( pkvd->szValue )
			pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_allies_model" ) )
	{
		strcpy( sz_AlliesModel, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_axis_model" ) )
	{
		strcpy( sz_AxisModel, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_reset_model" ) )
	{
		strcpy( sz_ResetModel, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_allies_model_bodygroup" ) )
	{
		m_iAlliesModelBodygroup = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_axis_model_bodygroup" ) )
	{
		m_iAxisModelBodygroup = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_reset_model_bodygroup" ) )
	{
		m_iResetModelBodygroup = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_allies_target" ) )
	{
		strcpy( sz_AlliesTarget, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_axis_target" ) )
	{
		strcpy( sz_AxisTarget, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_reset_target" ) )
	{
		strcpy( sz_ResetTarget, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_can_allies_touch" ) )
	{
		if ( atoi( pkvd->szValue ) > 0 )
			m_bAlliesCantTouch = true;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_can_axis_touch" ) )
	{
		if ( atoi( pkvd->szValue ) > 0 )
			m_bAxisCantTouch = true;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "visible" ) )
	{
		m_bPointVisible = atoi( pkvd->szValue );
	}
	else if ( FStrEq( pkvd->szKeyName, "point_win_string" ) )
	{
		strcpy( sz_WinString, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_index" ) )
	{
		m_iPointIndex = atoi( pkvd->szValue ) - 1;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_group" ) )
	{
		strcpy( m_szGroup, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_hud_icon_allies" ) )
	{
		m_iAlliesIcon = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_hud_icon_axis" ) )
	{
		m_iAxisIcon = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_hud_icon_neutral" ) )
	{
		m_iNeutralIcon = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_allies_win_string" ) )
	{
		m_sAlliesCapString = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_axis_win_string" ) )
	{
		m_sAxisCapString = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

// .text:00152640 CControlPointMaster::KeyValue (file:triggers.cpp:4613)
void CControlPointMaster::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "point_give_delay_time" ) )
	{
		m_iGivePointsDelay = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "allies_capture_target" ) )
	{
		strcpy( sz_AlliesTarget, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "axis_capture_target" ) )
	{
		strcpy( sz_AxisTarget, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "main" ) )
	{
		m_iMainObjective = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "cpm_group" ) )
	{
		strcpy( m_szGroup, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

// .text:0015a9c0 CControlPointMaster::IsMasteredOn (file:triggers.cpp:5137)
bool CControlPointMaster::IsMasteredOn()
{
	if ( m_sMaster )
		return UTIL_IsMasterTriggered( m_sMaster, this ) != 0;
	return true;
}

// .text:0014a6e0 CControlPointMaster::CControlPointMaster (file:triggers.cpp:4565)
CControlPointMaster::CControlPointMaster()
{
	m_szGroup[0] = 0;
}

// Capture entity factory registrations (string -> class, verified via binary xrefs)
LINK_ENTITY_TO_CLASS( dod_control_point, CControlPoint );
LINK_ENTITY_TO_CLASS( dod_control_point_master, CControlPointMaster );
LINK_ENTITY_TO_CLASS( dod_capture_area, CAreaCapture );
LINK_ENTITY_TO_CLASS( dod_object_goal, CObjectCapture );
LINK_ENTITY_TO_CLASS( dod_round_timer, CDodRoundTimer );
LINK_ENTITY_TO_CLASS( dod_point_relay, CDodPointRelay );
LINK_ENTITY_TO_CLASS( dod_location, CLocation );

// .text:0015a700 CControlPointMaster::Reset (file:maprules.cpp)
void CControlPointMaster::Reset( void )
{
	m_bTeamHasWon = false;
}

// .text:0010c6b0 CGameText::Spawn
void CGameText::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	pev->frame = 0;
	pev->model = 0;
	m_iEnabled = 1;
	RoundStore();
}
// .text:0010c710 CGameTeamMaster::Spawn
void CGameTeamMaster::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	pev->frame = 0;
	pev->model = 0;
	m_iEnabled = 1;
	m_rrteamIndex = m_teamIndex;
	RoundStore();
}
// .text:0010c770 CGameTeamMaster::RoundRespawn
void CGameTeamMaster::RoundRespawn( void )
{
	m_iEnabled = 1;
	m_teamIndex = m_rrteamIndex;
}
// .text:0010c790 CGameTeamSet::Spawn
void CGameTeamSet::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	pev->frame = 0;
	pev->model = 0;
	m_iEnabled = 1;
	RoundStore();
}
// .text:0010c860 CGameCounter::RoundRespawn
void CGameCounter::RoundRespawn( void )
{
	pev->dmg = (float)(int)pev->frags;
}
// .text:0010c880 CGameCounterSet::Spawn
void CGameCounterSet::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	pev->frame = 0;
	pev->model = 0;
	m_iEnabled = 1;
	RoundStore();
}

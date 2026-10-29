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

#include "objective.h"

// KeyValue fields (rest => CBaseTrigger::KeyValue):
// area_allies_cancap / area_axis_cancap (int>0) | area_allies_numcap /
// area_axis_numcap (min 1) | area_allies_startcap / area_axis_startcap
// area_allies_endcap / area_axis_endcap | area_allies_breakcap /
// area_axis_breakcap | master => trigger master | area_hud_sprite
// area_time_to_cap => m_nCapTime | area_cap_point => sz_CapPointName
// area_object_group => sz_RequiredObjectGroup, sets m_bRequiresObject.


// m_nAlliesNumCap / m_nAxisNumCap clamped to min 1.
// (AlliesInfinite && not AxisInfinite) && spawnflags & 1.


// Clears progress, fires break relays for capmode relays, prints
// #game_capture_broken_allie / _axis, cancels progress bar HUD, director 12.


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


// See table in header comment above. point_index stored 0-based.


#include "objective.h"

#include "objective.h"

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
extern cvar_t *pcvar_mp_clan_match;
extern cvar_t *pcvar_mp_clan_scoring;
extern cvar_t *pcvar_mp_clan_scoring_delay;

// KeyValue fields, dod_control_point_master:
// point_give_delay_time => m_iGivePointsDelay | allies_capture_target,
// axis_capture_target | main => m_iMainObjective | master => m_sMaster
// cpm_group => m_szGroup (matched against point_group). Rest unhandled.


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
	if ( !pActivator )
	{
		ALERT( at_console, "game_score entity triggered with a null pActivator\n" );
		return;
	}

	if ( !CanFireForActivator( pActivator ) )
		return;

	// Only players can use this
	if ( pActivator->IsPlayer() )
	{
		if ( AwardToTeam() )
			pActivator->AddPointsToTeam( Points(), AllowNegativeScore() );
		else
			( (CBasePlayer *) pActivator )->AddObjScore( Points(), TRUE );
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
#define SF_ENVTEXT_ONLY_ONCE 0x0002
#define SF_ENVTEXT_ALLIES 0x0004
#define SF_ENVTEXT_AXIS 0x0008
#define SF_ENVTEXT_ALLIES_ACTIVATOR 0x0010
#define SF_ENVTEXT_AXIS_ACTIVATOR 0x0020

class CGameText : public CRulePointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	inline BOOL MessageToAllies( void ) { return ( pev->spawnflags & SF_ENVTEXT_ALLIES ); }
	inline BOOL MessageToAxis( void ) { return ( pev->spawnflags & SF_ENVTEXT_AXIS ); }
	inline BOOL MessageToAll( void ) { return ( pev->spawnflags & SF_ENVTEXT_ALLPLAYERS ); }
	inline void MessageSet( const char *pMessage ) { pev->message = ALLOC_STRING( pMessage ); }
	inline const char *MessageGet( void ) { return STRING( pev->message ); }

private:
	int m_iEnabled;
	hudtextparms_t m_textParms;
	void Spawn( void );
	void RoundRespawn( void );
};

void CGameText::RoundRespawn( void )
{
	m_iEnabled = 1;
}

LINK_ENTITY_TO_CLASS( game_text, CGameText );


// ---- gamerules hook ----
// RoundState to player, first mastered dod_control_point_master =>
// SendObjectiveStatus, then per-player area_SendStatus for dod_capture_area,
// dod_object_goal, func_breakable, plus dod_round_timer SendStatus and
// particle_shooter updates.

// ============================================================================
// ============================================================================



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

// Save parms as a block.  Will break save/restore if the structure changes, but this entity didn't ship with Half-Life, so
// it can't impact saved Half-Life games.
TYPEDESCRIPTION CGameText::m_SaveData[] = {
	DEFINE_ARRAY( CGameText, m_textParms, FIELD_CHARACTER, sizeof( hudtextparms_t ) ),
};

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
	if ( !m_iEnabled )
		return;

	if ( !CanFireForActivator( pActivator ) )
		return;

	if ( pev->spawnflags & SF_ENVTEXT_ALLIES_ACTIVATOR )
	{
		if ( pActivator && pActivator->IsPlayer() && pActivator->pev->team == TEAM_ALLIES )
			UTIL_HudMessage( pActivator, m_textParms, MessageGet() );
	}
	else if ( pev->spawnflags & SF_ENVTEXT_AXIS_ACTIVATOR )
	{
		if ( pActivator && pActivator->IsPlayer() && pActivator->pev->team == TEAM_AXIS )
			UTIL_HudMessage( pActivator, m_textParms, MessageGet() );
	}
	else if ( MessageToAxis() )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer && pPlayer->IsPlayer() && pPlayer->pev->team == TEAM_AXIS )
				UTIL_HudMessage( pPlayer, m_textParms, MessageGet() );
		}
	}
	else if ( MessageToAllies() )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer && pPlayer->IsPlayer() && pPlayer->pev->team == TEAM_ALLIES )
				UTIL_HudMessage( pPlayer, m_textParms, MessageGet() );
		}
	}
	else if ( MessageToAll() )
	{
		UTIL_HudMessageAll( m_textParms, MessageGet() );
	}
	else if ( pActivator && pActivator->IsNetClient() )
	{
		UTIL_HudMessage( pActivator, m_textParms, MessageGet() );
	}

	if ( pev->spawnflags & SF_ENVTEXT_ONLY_ONCE )
		m_iEnabled = 0;
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

	int m_iEnabled;
	int m_teamIndex;
	int m_rrteamIndex;
	USE_TYPE triggerType;
	void Spawn( void );
	void RoundRespawn( void );
};

LINK_ENTITY_TO_CLASS( game_team_master, CGameTeamMaster );

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
void CGameTeamMaster::RoundRespawn( void )
{
	m_iEnabled = 1;
	m_teamIndex = m_rrteamIndex;
}

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
	if ( !m_iEnabled )
		return;

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
	}
	else if ( TeamMatch( pActivator ) )
	{
		SUB_UseTargets( pActivator, triggerType, value );

		if ( RemoveOnFire() )
			m_iEnabled = 0;
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
	int m_iEnabled;
	void Spawn( void );
	void RoundRespawn( void );
};

void CGameTeamSet::RoundRespawn( void )
{
	m_iEnabled = 1;
}

LINK_ENTITY_TO_CLASS( game_team_set, CGameTeamSet );

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

void CGameTeamSet::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !m_iEnabled )
		return;

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
		m_iEnabled = 0;
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
	int m_iEnabled;
	void Spawn( void );
	void RoundRespawn( void );
};

void CGamePlayerHurt::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	pev->frame = 0;
	pev->model = 0;
	m_iEnabled = 1;
	RoundStore();
}
void CGamePlayerHurt::RoundRespawn( void )
{
	m_iEnabled = 1;
}

LINK_ENTITY_TO_CLASS( game_player_hurt, CGamePlayerHurt );

void CGamePlayerHurt::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !m_iEnabled )
		return;

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
		m_iEnabled = 0;
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

void CGameCounter::RoundRespawn( void )
{
	pev->dmg = (float)(int)pev->frags;
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

		if ( ResetOnFire() )
			ResetCount();
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
	int m_iEnabled;
	void Spawn( void );
	void RoundRespawn( void );
};

void CGameCounterSet::RoundRespawn( void )
{
	m_iEnabled = 1;
}

LINK_ENTITY_TO_CLASS( game_counter_set, CGameCounterSet );

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

void CGameCounterSet::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !m_iEnabled )
		return;

	if ( !CanFireForActivator( pActivator ) )
		return;

	SUB_UseTargets( pActivator, USE_SET, pev->frags );

	if ( RemoveOnFire() )
		m_iEnabled = 0;
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

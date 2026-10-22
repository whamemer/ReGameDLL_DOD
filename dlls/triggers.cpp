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

===== triggers.cpp ========================================================

  spawn and use functions for editor-placed triggers              

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "saverestore.h"
#include "trains.h"	 // trigger_camera has train functionality
#include "gamerules.h"
#include "objective.h"
#include "dod_gamerules.h"

#define SF_TRIGGER_PUSH_START_OFF 2			//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_TARGETONCE 1		// Only fire hurt target once
#define SF_TRIGGER_HURT_START_OFF 2			//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_NO_CLIENTS 8		//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_CLIENTONLYFIRE 16	// trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32	// only clients may touch this trigger.

extern DLL_GLOBAL BOOL g_fGameOver;
extern int gmsgCameraView;

extern void SetMovedir( entvars_t *pev );
extern Vector VecBModelOrigin( entvars_t *pevBModel );

class CFrictionModifier : public CBaseEntity
{
  public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT ChangeFriction( CBaseEntity *pOther );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	virtual int ObjectCaps( void ) { return CBaseEntity ::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static TYPEDESCRIPTION m_SaveData[];

	float m_frictionFraction;  // Sorry, couldn't resist this name :)
};

LINK_ENTITY_TO_CLASS( func_friction, CFrictionModifier );

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION CFrictionModifier::m_SaveData[] = {
	DEFINE_FIELD( CFrictionModifier, m_frictionFraction, FIELD_FLOAT ),
};

// .text:00150070 CFrictionModifier::Restore (file:triggers.cpp:128)
IMPLEMENT_SAVERESTORE( CFrictionModifier, CBaseEntity );

// Modify an entity's friction
void CFrictionModifier ::Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );	// set size and link into world
	pev->movetype = MOVETYPE_NONE;
	SetTouch( &CFrictionModifier::ChangeFriction );
}

// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier ::ChangeFriction( CBaseEntity *pOther )
{
	if ( pOther->pev->movetype != MOVETYPE_BOUNCEMISSILE && pOther->pev->movetype != MOVETYPE_BOUNCE )
		pOther->pev->friction = m_frictionFraction;
}

// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier ::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "modifier" ) )
	{
		m_frictionFraction = atof( pkvd->szValue ) / 100.0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

// This trigger will fire when the level spawns (or respawns if not fire once)
// It will check a global state before firing.  It supports delay and killtargets

#define SF_AUTO_FIREONCE 0x0001

class CAutoTrigger : public CBaseDelay
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
	void Think( void );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

  private:
	int m_globalstate;
	USE_TYPE triggerType;
	int m_iEnabled;
	bool b_isRoundRestart;
	void RoundRespawn( void );
};
LINK_ENTITY_TO_CLASS( trigger_auto, CAutoTrigger );

TYPEDESCRIPTION CAutoTrigger::m_SaveData[] = {
	DEFINE_FIELD( CAutoTrigger, m_globalstate, FIELD_STRING ),
	DEFINE_FIELD( CAutoTrigger, triggerType, FIELD_INTEGER ),
};

// .text:00150010 CAutoTrigger::Restore (file:triggers.cpp:198)
IMPLEMENT_SAVERESTORE( CAutoTrigger, CBaseDelay );

void CAutoTrigger::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "globalstate" ) )
	{
		m_globalstate = ALLOC_STRING( pkvd->szValue );
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
		CBaseDelay::KeyValue( pkvd );
}

void CAutoTrigger::Spawn( void )
{
	Precache();
}

void CAutoTrigger::Precache( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
}

void CAutoTrigger::Think( void )
{
	if ( !m_globalstate || gGlobalState.EntityGetState( m_globalstate ) == GLOBAL_ON )
	{
		SUB_UseTargets( this, triggerType, 0 );
		if ( pev->spawnflags & SF_AUTO_FIREONCE )
			UTIL_Remove( this );
	}
}

#define SF_RELAY_FIREONCE 0x0001

class CTriggerRelay : public CBaseDelay
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

  private:
	USE_TYPE triggerType;
};
LINK_ENTITY_TO_CLASS( trigger_relay, CTriggerRelay );

TYPEDESCRIPTION CTriggerRelay::m_SaveData[] = {
	DEFINE_FIELD( CTriggerRelay, triggerType, FIELD_INTEGER ),
};

// .text:0014ffb0 CTriggerRelay::Restore (file:triggers.cpp:318)
IMPLEMENT_SAVERESTORE( CTriggerRelay, CBaseDelay );

void CTriggerRelay::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "triggerstate" ) )
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
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerRelay::Spawn( void ) {}

void CTriggerRelay::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SUB_UseTargets( this, triggerType, 0 );
	if ( pev->spawnflags & SF_RELAY_FIREONCE )
		UTIL_Remove( this );
}

//**********************************************************
// The Multimanager Entity - when fired, will fire up to 16 targets
// at specified times.
// FLAG:		THREAD (create clones when triggered)
// FLAG:		CLONE (this is a clone for a threaded execution)

#define SF_MULTIMAN_CLONE 0x80000000
#define SF_MULTIMAN_THREAD 0x00000001

class CMultiManager : public CBaseToggle
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void EXPORT ManagerThink( void );
	void EXPORT ManagerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

#if _DEBUG
	void EXPORT ManagerReport( void );
#endif

	BOOL HasTarget( string_t targetname );
	virtual STATE GetState();

	int ObjectCaps( void ) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	STATE m_iState;
	int m_cTargets;							   // the total number of targets in this manager's fire list.
	int m_index;							   // Current target
	float m_startTime;						   // Time we started firing
	int m_iTargetName[MAX_MULTI_TARGETS];	   // list if indexes into global string array
	float m_flTargetDelay[MAX_MULTI_TARGETS];  // delay (in seconds) from time of manager fire to target fire
  private:
	inline BOOL IsClone( void ) { return ( pev->spawnflags & SF_MULTIMAN_CLONE ) ? TRUE : FALSE; }
	inline BOOL ShouldClone( void )
	{
		if ( IsClone() )
			return FALSE;

		return ( pev->spawnflags & SF_MULTIMAN_THREAD ) ? TRUE : FALSE;
	}

	CMultiManager *Clone( void );

	int m_rscTargets;
	int m_rsindex;
	float m_rsstartTime;
	float m_rsflWait;
	float m_rsflMaxWait;
	string_t m_rssMaster;
	int m_rsiMode;
	int m_rsiTargetName[MAX_MULTI_TARGETS];
	float m_rsflTargetDelay[MAX_MULTI_TARGETS];
	float m_flWait;
	float m_flMaxWait;
	string_t m_sMaster;
	int m_iMode;
	USE_TYPE m_triggerType;
	void RoundRespawn( void );
};
LINK_ENTITY_TO_CLASS( multi_manager, CMultiManager );

// Global Savedata for multi_manager
TYPEDESCRIPTION CMultiManager::m_SaveData[] = {
	DEFINE_FIELD( CMultiManager, m_cTargets, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_index, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_startTime, FIELD_TIME ),
	DEFINE_ARRAY( CMultiManager, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS ),
	DEFINE_ARRAY( CMultiManager, m_flTargetDelay, FIELD_FLOAT, MAX_MULTI_TARGETS ),
};

// .text:0014ff50 CMultiManager::Restore (file:triggers.cpp:511)
IMPLEMENT_SAVERESTORE( CMultiManager, CBaseToggle );

void CMultiManager ::KeyValue( KeyValueData *pkvd )
{
	// UNDONE: Maybe this should do something like this:
	//CBaseToggle::KeyValue( pkvd );
	// if ( !pkvd->fHandled )
	// ... etc.

	if ( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else  // add this field to the target list
	{
		// this assumes that additional fields are targetnames and their values are delay values.
		if ( m_cTargets < MAX_MULTI_TARGETS )
		{
			char tmp[128];

			UTIL_StripToken( pkvd->szKeyName, tmp, sizeof( tmp ) );
			m_iTargetName[m_cTargets] = ALLOC_STRING( tmp );
			m_flTargetDelay[m_cTargets] = atof( pkvd->szValue );
			m_cTargets++;
			pkvd->fHandled = TRUE;
		}
	}
}

void CMultiManager ::Spawn( void )
{
	pev->solid = SOLID_NOT;
	SetUse( &CMultiManager::ManagerUse );
	SetThink( &CMultiManager::ManagerThink );

	// Sort targets
	// Quick and dirty bubble sort
	int swapped = 1;

	while ( swapped )
	{
		swapped = 0;
		for ( int i = 1; i < m_cTargets; i++ )
		{
			if ( m_flTargetDelay[i] < m_flTargetDelay[i - 1] )
			{
				// Swap out of order elements
				int name = m_iTargetName[i];
				float delay = m_flTargetDelay[i];
				m_iTargetName[i] = m_iTargetName[i - 1];
				m_flTargetDelay[i] = m_flTargetDelay[i - 1];
				m_iTargetName[i - 1] = name;
				m_flTargetDelay[i - 1] = delay;
				swapped = 1;
			}
		}
	}
}

BOOL CMultiManager::HasTarget( string_t targetname )
{
	for ( int i = 0; i < m_cTargets; i++ )
		if ( FStrEq( STRING( targetname ), STRING( m_iTargetName[i] ) ) )
			return TRUE;

	return FALSE;
}

// Designers were using this to fire targets that may or may not exist --
// so I changed it to use the standard target fire code, made it a little simpler.
void CMultiManager ::ManagerThink( void )
{
	float time;

	time = gpGlobals->time - m_startTime;
	while ( m_index < m_cTargets && m_flTargetDelay[m_index] <= time )
	{
		FireTargets( STRING( m_iTargetName[m_index] ), m_hActivator, this, USE_TOGGLE, 0 );
		m_index++;
	}

	if ( m_index >= m_cTargets )  // have we fired all targets?
	{
		SetThink( NULL );
		if ( IsClone() )
		{
			UTIL_Remove( this );
			return;
		}
		SetUse( &CMultiManager::ManagerUse );  // allow manager re-use
	}
	else
		pev->nextthink = m_startTime + m_flTargetDelay[m_index];
}

CMultiManager *CMultiManager::Clone( void )
{
	CMultiManager *pMulti = GetClassPtr( (CMultiManager *) NULL );

	edict_t *pEdict = pMulti->pev->pContainingEntity;
	memcpy( pMulti->pev, pev, sizeof( *pev ) );
	pMulti->pev->pContainingEntity = pEdict;

	pMulti->pev->spawnflags |= SF_MULTIMAN_CLONE;
	pMulti->m_cTargets = m_cTargets;
	memcpy( pMulti->m_iTargetName, m_iTargetName, sizeof( m_iTargetName ) );
	memcpy( pMulti->m_flTargetDelay, m_flTargetDelay, sizeof( m_flTargetDelay ) );

	return pMulti;
}

// The USE function builds the time table and starts the entity thinking.
void CMultiManager ::ManagerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// In multiplayer games, clone the MM and execute in the clone (like a thread)
	// to allow multiple players to trigger the same multimanager
	if ( ShouldClone() )
	{
		CMultiManager *pClone = Clone();
		pClone->ManagerUse( pActivator, pCaller, useType, value );
		return;
	}

	m_hActivator = pActivator;
	m_index = 0;
	m_startTime = gpGlobals->time;

	SetUse( NULL );	 // disable use until all targets have fired

	SetThink( &CMultiManager::ManagerThink );
	pev->nextthink = gpGlobals->time;
}

#if _DEBUG
void CMultiManager ::ManagerReport( void )
{
	int cIndex;

	for ( cIndex = 0; cIndex < m_cTargets; cIndex++ )
	{
		ALERT( at_console, "%s %f\n", STRING( m_iTargetName[cIndex] ), m_flTargetDelay[cIndex] );
	}
}
#endif

//***********************************************************

//
// Render parameters trigger
//
// This entity will copy its render parameters (renderfx, rendermode, rendercolor, renderamt)
// to its targets when triggered.
//

// Flags to indicate masking off various render parameters that are normally copied to the targets
#define SF_RENDER_MASKFX ( 1 << 0 )
#define SF_RENDER_MASKAMT ( 1 << 1 )
#define SF_RENDER_MASKMODE ( 1 << 2 )
#define SF_RENDER_MASKCOLOR ( 1 << 3 )

class CRenderFxManager : public CBaseEntity
{
  public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( env_render, CRenderFxManager );

void CRenderFxManager ::Spawn( void )
{
	pev->solid = SOLID_NOT;
}

void CRenderFxManager ::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !FStringNull( pev->target ) )
	{
		edict_t *pentTarget = NULL;
		while ( 1 )
		{
			pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING( pev->target ) );
			if ( FNullEnt( pentTarget ) )
				break;

			entvars_t *pevTarget = VARS( pentTarget );
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKFX ) )
				pevTarget->renderfx = pev->renderfx;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKAMT ) )
				pevTarget->renderamt = pev->renderamt;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKMODE ) )
				pevTarget->rendermode = pev->rendermode;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKCOLOR ) )
				pevTarget->rendercolor = pev->rendercolor;
		}
	}
}

// ODR: CBaseTrigger canonical in objective.h


LINK_ENTITY_TO_CLASS( trigger, CBaseTrigger );

/*
================
InitTrigger
================
*/
void CBaseTrigger::InitTrigger()
{
	// trigger angles are used for one-way touches.  An angle of 0 is assumed
	// to mean no restrictions, so use a yaw of 360 instead.
	if ( pev->angles != g_vecZero )
		SetMovedir( pev );
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );	// set size and link into world
	if ( CVAR_GET_FLOAT( "showtriggers" ) == 0 )
		SetBits( pev->effects, EF_NODRAW );
}

//
// Cache user-entity-field values until spawn is called.
//

void CBaseTrigger ::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "damage" ) )
	{
		pev->dmg = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "count" ) )
	{
		m_cTriggersLeft = (int) atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "damagetype" ) )
	{
		m_bitsDamageInflict = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

class CTriggerHurt : public CBaseTrigger
{
  public:
	void Spawn( void );
	void EXPORT RadiationThink( void );
	virtual void RoundRespawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_hurt, CTriggerHurt );

//
// trigger_monsterjump
//
class CTriggerMonsterJump : public CBaseTrigger
{
  public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void Think( void );
};

LINK_ENTITY_TO_CLASS( trigger_monsterjump, CTriggerMonsterJump );

void CTriggerMonsterJump ::Spawn( void )
{
	SetMovedir( pev );

	InitTrigger();

	pev->nextthink = 0;
	pev->speed = 200;
	m_flHeight = 150;

	if ( !FStringNull( pev->targetname ) )
	{  // if targetted, spawn turned off
		pev->solid = SOLID_NOT;
		UTIL_SetOrigin( pev, pev->origin );	 // Unlink from trigger list
		SetUse( &CTriggerMonsterJump::ToggleUse );
	}
}

void CTriggerMonsterJump ::Think( void )
{
	pev->solid = SOLID_NOT;				 // kill the trigger for now !!!UNDONE
	UTIL_SetOrigin( pev, pev->origin );	 // Unlink from trigger list
	SetThink( NULL );
}

void CTriggerMonsterJump ::Touch( CBaseEntity *pOther )
{
	entvars_t *pevOther = pOther->pev;

	if ( !FBitSet( pevOther->flags, FL_MONSTER ) )
	{  // touched by a non-monster.
		return;
	}

	pevOther->origin.z += 1;

	if ( FBitSet( pevOther->flags, FL_ONGROUND ) )
	{  // clear the onground so physics don't bitch
		pevOther->flags &= ~FL_ONGROUND;
	}

	// toss the monster!
	pevOther->velocity = pev->movedir * pev->speed;
	pevOther->velocity.z += m_flHeight;
	pev->nextthink = gpGlobals->time;
}

//=====================================
//
// trigger_cdaudio - starts/stops cd audio tracks
//
class CTriggerCDAudio : public CBaseTrigger
{
  public:
	void Spawn( void );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void PlayTrack( void );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_cdaudio, CTriggerCDAudio );

//
// Changes tracks or stops CD when player touches
//
// !!!HACK - overloaded HEALTH to avoid adding new field
void CTriggerCDAudio ::Touch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
	{  // only clients may trigger these events
		return;
	}

	PlayTrack();
}

void CTriggerCDAudio ::Spawn( void )
{
	InitTrigger();
}

void CTriggerCDAudio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	PlayTrack();
}

void PlayCDTrack( int iTrack )
{
	edict_t *pClient;

	// manually find the single player.
	pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );

	// Can't play if the client is not connected!
	if ( !pClient )
		return;

	if ( iTrack < -1 || iTrack > 30 )
	{
		ALERT( at_console, "TriggerCDAudio - Track %d out of range\n" );
		return;
	}

	if ( iTrack == -1 )
	{
		CLIENT_COMMAND( pClient, "cd stop\n" );
	}
	else
	{
		char string[64];

		sprintf( string, "cd play %3d\n", iTrack );
		CLIENT_COMMAND( pClient, string );
	}
}

// only plays for ONE client, so only use in single play!
void CTriggerCDAudio ::PlayTrack( void )
{
	PlayCDTrack( (int) pev->health );

	SetTouch( NULL );
	UTIL_Remove( this );
}

// This plays a CD track when fired or when the player enters it's radius
class CTargetCDAudio : public CPointEntity
{
  public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Think( void );
	void Play( void );
};

LINK_ENTITY_TO_CLASS( target_cdaudio, CTargetCDAudio );

void CTargetCDAudio ::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		pev->scale = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTargetCDAudio ::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if ( pev->scale > 0 )
		pev->nextthink = gpGlobals->time + 1.0;
}

void CTargetCDAudio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Play();
}

// only plays for ONE client, so only use in single play!
void CTargetCDAudio::Think( void )
{
	edict_t *pClient;

	// manually find the single player.
	pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );

	// Can't play if the client is not connected!
	if ( !pClient )
		return;

	pev->nextthink = gpGlobals->time + 0.5;

	if ( ( pClient->v.origin - pev->origin ).Length() <= pev->scale )
		Play();
}

void CTargetCDAudio::Play( void )
{
	PlayCDTrack( (int) pev->health );
	UTIL_Remove( this );
}

//=====================================

//
// trigger_hurt - hurts anything that touches it. if the trigger has a targetname, firing it will toggle state
//
//int gfToggleState = 0; // used to determine when all radiation trigger hurts have called 'RadiationThink'

void CTriggerHurt ::Spawn( void )
{
	InitTrigger();
	SetTouch( &CTriggerHurt::HurtTouch );

	if ( !FStringNull( pev->targetname ) )
	{
		SetUse( &CTriggerHurt::ToggleUse );
	}
	else
	{
		SetUse( NULL );
	}

	if ( m_bitsDamageInflict & DMG_RADIATION )
	{
		SetThink( &CTriggerHurt::RadiationThink );
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.0, 0.5 );
	}

	if ( FBitSet( pev->spawnflags,
				  SF_TRIGGER_HURT_START_OFF ) )	 // if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	UTIL_SetOrigin( pev, pev->origin );	 // Link into the list
}

// .text:001576e0 CTriggerHurt::RoundRespawn (file:triggers.cpp:1792)
// Same as Spawn minus the final UTIL_SetOrigin link.
void CTriggerHurt ::RoundRespawn( void )
{
	InitTrigger();
	SetTouch( &CTriggerHurt::HurtTouch );

	if ( !FStringNull( pev->targetname ) )
		SetUse( &CTriggerHurt::ToggleUse );
	else
		SetUse( NULL );

	if ( m_bitsDamageInflict & DMG_RADIATION )
	{
		SetThink( &CTriggerHurt::RadiationThink );
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.0, 0.5 );
	}

	if ( FBitSet( pev->spawnflags, SF_TRIGGER_HURT_START_OFF ) )
		pev->solid = SOLID_NOT;
}

// trigger hurt that causes radiation will do a radius
// check and set the player's geiger counter level
// according to distance from center of trigger

void CTriggerHurt ::RadiationThink( void )
{
	// DoD's CTriggerHurt::RadiationThink has no geiger/range code in the
	// binary (link 0x13e9e0): it only centers the trigger, does a PVS
	// client lookup (result unused), restores origin/view_ofs and
	// re-thinks in 0.25 s.
	Vector origin;
	Vector view_ofs;

	// set origin to center of trigger so that this check works
	origin = pev->origin;
	view_ofs = pev->view_ofs;

	pev->origin = ( pev->absmin + pev->absmax ) * 0.5;
	pev->view_ofs = pev->view_ofs * 0.0;

	FIND_CLIENT_IN_PVS( edict() );

	pev->origin = origin;
	pev->view_ofs = view_ofs;

	pev->nextthink = gpGlobals->time + 0.25;
}

//
// ToggleUse - If this is the USE function for a trigger, its state will toggle every time it's fired
//
void CBaseTrigger ::ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( pev->solid == SOLID_NOT )
	{  // if the trigger is off, turn it on
		pev->solid = SOLID_TRIGGER;

		// Force retouch
		gpGlobals->force_retouch++;
	}
	else
	{  // turn the trigger off
		pev->solid = SOLID_NOT;
	}
	UTIL_SetOrigin( pev, pev->origin );
}

// When touched, a hurt trigger does DMG points of damage each half-second
void CBaseTrigger ::HurtTouch( CBaseEntity *pOther )
{
	float fldmg;

	if ( !pOther->pev->takedamage )
		return;

	if ( ( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH ) && !pOther->IsPlayer() )
	{
		// this trigger is only allowed to touch clients, and this ain't a client.
		return;
	}

	if ( ( pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS ) && pOther->IsPlayer() )
		return;

	// HACKHACK -- In multiplayer, players touch this based on packet receipt.
	// So the players who send packets later aren't always hurt.  Keep track of
	// how much time has passed and whether or not you've touched that player
	if ( g_pGameRules->IsMultiplayer() )
	{
		if ( pev->dmgtime > gpGlobals->time )
		{
			if ( gpGlobals->time != pev->pain_finished )
			{  // too early to hurt again, and not same frame with a different entity
				if ( pOther->IsPlayer() )
				{
					int playerMask = 1 << ( pOther->entindex() - 1 );

					// If I've already touched this player (this time), then bail out
					if ( pev->impulse & playerMask )
						return;

					// Mark this player as touched
					// BUGBUG - There can be only 32 players!
					pev->impulse |= playerMask;
				}
				else
				{
					return;
				}
			}
		}
		else
		{
			// New clock, "un-touch" all players
			pev->impulse = 0;
			if ( pOther->IsPlayer() )
			{
				int playerMask = 1 << ( pOther->entindex() - 1 );

				// Mark this player as touched
				// BUGBUG - There can be only 32 players!
				pev->impulse |= playerMask;
			}
		}
	}
	else  // Original code -- single player
	{
		if ( pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished )
		{  // too early to hurt again, and not same frame with a different entity
			return;
		}
	}

	// If this is time_based damage (poison, radiation), override the pev->dmg with a
	// default for the given damage type.  Monsters only take time-based damage
	// while touching the trigger.  Player continues taking damage for a while after
	// leaving the trigger

	fldmg = pev->dmg * 0.5;	 // 0.5 seconds worth of damage, pev->dmg is damage/second

	// JAY: Cut this because it wasn't fully realized.  Damage is simpler now.
#if 0
	switch (m_bitsDamageInflict)
	{
	default: break;
	case DMG_POISON:		fldmg = POISON_DAMAGE/4; break;
	case DMG_NERVEGAS:		fldmg = NERVEGAS_DAMAGE/4; break;
	case DMG_RADIATION:		fldmg = RADIATION_DAMAGE/4; break;
	case DMG_PARALYZE:		fldmg = PARALYZE_DAMAGE/4; break; // UNDONE: cut this? should slow movement to 50%
	case DMG_ACID:			fldmg = ACID_DAMAGE/4; break;
	case DMG_SLOWBURN:		fldmg = SLOWBURN_DAMAGE/4; break;
	case DMG_SLOWFREEZE:	fldmg = SLOWFREEZE_DAMAGE/4; break;
	}
#endif

	if ( fldmg < 0 )
	{
		BOOL bApplyHeal = TRUE;

		if ( g_pGameRules->IsMultiplayer() && pOther->IsPlayer() )
		{
			bApplyHeal = pOther->pev->deadflag == DEAD_NO;
		}

		if ( bApplyHeal )
		{
			pOther->TakeHealth( -fldmg, m_bitsDamageInflict );
		}
	}
	else
		pOther->TakeDamage( pev, pev, fldmg, m_bitsDamageInflict );

	// Store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;

	// Apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5;  // half second delay until this trigger can hurt toucher again

	if ( pev->target )
	{
		// trigger has a target it wants to fire.
		if ( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE )
		{
			// if the toucher isn't a client, don't fire the target!
			if ( !pOther->IsPlayer() )
			{
				return;
			}
		}

		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		if ( pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE )
			pev->target = 0;
	}
}

/*QUAKED trigger_multiple (.5 .5 .5) ? notouch
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "health" is set, the trigger must be killed to activate each time.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
If notouch is set, the trigger is only fired by other entities, not by touching.
NOTOUCH has been obsoleted by trigger_relay!
sounds
1)      secret
2)      beep beep
3)      large switch
4)
NEW
if a trigger has a NETNAME, that NETNAME will become the TARGET of the triggered object.
*/
class CTriggerMultiple : public CBaseTrigger
{
  public:
	void Spawn( void );
	virtual void RoundRespawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_multiple, CTriggerMultiple );

void CTriggerMultiple ::Spawn( void )
{
	if ( m_flWait == 0 )
		m_flWait = 0.2;

	InitTrigger();

	ASSERTSZ( pev->health == 0, "trigger_multiple with health" );
	//	UTIL_SetOrigin(pev, pev->origin);
	//	SET_MODEL( ENT(pev), STRING(pev->model) );
	//	if (pev->health > 0)
	//		{
	//		if (FBitSet(pev->spawnflags, SPAWNFLAG_NOTOUCH))
	//			ALERT(at_error, "trigger_multiple spawn: health and notouch don't make sense");
	//		pev->max_health = pev->health;
	//UNDONE: where to get pfnDie from?
	//		pev->pfnDie = multi_killed;
	//		pev->takedamage = DAMAGE_YES;
	//		pev->solid = SOLID_BBOX;
	//		UTIL_SetOrigin(pev, pev->origin);  // make sure it links into the world
	//		}
	//	else
	{
		SetTouch( &CTriggerMultiple::MultiTouch );
	}
}

// .text:00157620 CTriggerMultiple::RoundRespawn (file:triggers.cpp:2062)
void CTriggerMultiple ::RoundRespawn( void )
{
	if ( m_flWait == 0 )
		m_flWait = 0.2;

	InitTrigger();
	SetTouch( &CTriggerMultiple::MultiTouch );

	if ( pev->spawnflags & 0x200 )  // DoD: re-arm countdown triggers on round restart
		pev->nextthink = gpGlobals->time;
}

/*QUAKED trigger_once (.5 .5 .5) ? notouch
Variable sized trigger. Triggers once, then removes itself.  You must set the key "target" to the name of another object in the level that has a matching
"targetname".  If "health" is set, the trigger must be killed to activate.
If notouch is set, the trigger is only fired by other entities, not by touching.
if "killtarget" is set, any objects that have a matching "target" will be removed when the trigger is fired.
if "angle" is set, the trigger will only fire when someone is facing the direction of the angle.  Use "360" for an angle of 0.
sounds
1)      secret
2)      beep beep
3)      large switch
4)
*/
class CTriggerOnce : public CTriggerMultiple
{
  public:
	void Spawn( void );
	virtual void RoundRespawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_once, CTriggerOnce );
void CTriggerOnce::Spawn( void )
{
	m_flWait = -1;

	CTriggerMultiple ::Spawn();
}

// .text:0014eb00 CTriggerOnce::RoundRespawn (file:triggers.cpp:2106)
void CTriggerOnce::RoundRespawn( void )
{
	if ( pev->spawnflags & 0x200 )  // DoD: re-arm once-triggers on round restart
		SetTouch( &CBaseTrigger::MultiTouch );
}

void CBaseTrigger ::MultiTouch( CBaseEntity *pOther )
{
	entvars_t *pevToucher;

	pevToucher = pOther->pev;

	// Only touch clients, monsters, or pushables (depending on flags)
 	if ( ( ( pevToucher->flags & FL_CLIENT ) && !( pev->spawnflags & SF_TRIGGER_NOCLIENTS ) ) ||
 		 ( ( pevToucher->flags & FL_MONSTER ) && ( pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS ) ) ||
 		 ( ( pev->spawnflags & SF_TRIGGER_PUSHABLES ) && FClassnameIs( pevToucher, "func_pushable" ) ) )
	{

#if 0
		// if the trigger has an angles field, check player's facing direction
		if (pev->movedir != g_vecZero)
		{
			UTIL_MakeVectors( pevToucher->angles );
			if ( DotProduct( gpGlobals->v_forward, pev->movedir ) < 0 )
				return;         // not facing the right way
		}
#endif

		ActivateMultiTrigger( pOther );
	}
}

//
// the trigger was just touched/killed/used
// self.enemy should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
//
void CBaseTrigger ::ActivateMultiTrigger( CBaseEntity *pActivator )
{
	if ( pev->nextthink > gpGlobals->time )
		return;	 // still waiting for reset time

	if ( !UTIL_IsMasterTriggered( m_sMaster, pActivator ) )
		return;

	if ( FClassnameIs( pev, "trigger_secret" ) )
	{
		if ( pev->enemy == NULL || !FClassnameIs( pev->enemy, "player" ) )
			return;
		gpGlobals->found_secrets++;
	}

	if ( !FStringNull( pev->noise ) )
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, (char *) STRING( pev->noise ), 1, ATTN_NORM );

	// don't trigger again until reset
	// pev->takedamage = DAMAGE_NO;

	m_hActivator = pActivator;
	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 );

	if ( pev->message && pActivator->IsPlayer() )
	{
		UTIL_ShowMessage( STRING( pev->message ), pActivator );
		//		CLIENT_PRINTF( ENT( pActivator->pev ), print_center, STRING(pev->message) );
	}

	if ( m_flWait > 0 )
	{
		SetThink( &CBaseTrigger::MultiWaitOver );
		pev->nextthink = gpGlobals->time + m_flWait;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouch( NULL );
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink( &CBaseTrigger::SUB_Remove );
	}
}

// the wait time has passed, so set back up for another activation
void CBaseTrigger ::MultiWaitOver( void )
{
	//	if (pev->max_health)
	//		{
	//		pev->health		= pev->max_health;
	//		pev->takedamage	= DAMAGE_YES;
	//		pev->solid		= SOLID_BBOX;
	//		}
	SetThink( NULL );
}

// ========================= COUNTING TRIGGER =====================================

//
// GLOBALS ASSUMED SET:  g_eoActivator
//
void CBaseTrigger::CounterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_cTriggersLeft--;
	m_hActivator = pActivator;

	if ( m_cTriggersLeft < 0 )
		return;

	BOOL fTellActivator = ( m_hActivator != 0 ) && FClassnameIs( m_hActivator->pev, "player" ) &&
						  !FBitSet( pev->spawnflags, SPAWNFLAG_NOMESSAGE );
	if ( m_cTriggersLeft != 0 )
	{
		if ( fTellActivator )
		{
			// UNDONE: I don't think we want these Quakesque messages
			switch ( m_cTriggersLeft )
			{
			case 1:
				ALERT( at_console, "Only 1 more to go..." );
				break;
			case 2:
				ALERT( at_console, "Only 2 more to go..." );
				break;
			case 3:
				ALERT( at_console, "Only 3 more to go..." );
				break;
			default:
				ALERT( at_console, "There are more to go..." );
				break;
			}
		}
		return;
	}

	// !!!UNDONE: I don't think we want these Quakesque messages
	if ( fTellActivator )
		ALERT( at_console, "Sequence completed!" );

	ActivateMultiTrigger( m_hActivator );
}

/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.
If nomessage is not set, it will print "1 more.. " etc when triggered and
"sequence complete" when finished.  After the counter has been triggered "cTriggersLeft"
times (default 2), it will fire all of it's targets and remove itself.
*/
class CTriggerCounter : public CBaseTrigger
{
  public:
	void Spawn( void );
};
LINK_ENTITY_TO_CLASS( trigger_counter, CTriggerCounter );

void CTriggerCounter ::Spawn( void )
{
	// By making the flWait be -1, this counter-trigger will disappear after it's activated
	// (but of course it needs cTriggersLeft "uses" before that happens).
	m_flWait = -1;

	if ( m_cTriggersLeft == 0 )
		m_cTriggersLeft = 2;
	SetUse( &CTriggerCounter::CounterUse );
}

// ====================== TRIGGER_CHANGELEVEL ================================

class CTriggerVolume : public CPointEntity	// Derive from point entity so this doesn't move across levels
{
  public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_transition, CTriggerVolume );

// Define space that travels across a level transition
void CTriggerVolume ::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );	// set size and link into world
	pev->model = NULL;
	pev->modelindex = 0;
}

// Fires a target after level transition and then dies
class CFireAndDie : public CBaseDelay
{
  public:
	void Spawn( void );
	void Precache( void );
	void Think( void );
	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() | FCAP_FORCE_TRANSITION; }	 // Always go across transitions
};
LINK_ENTITY_TO_CLASS( fireanddie, CFireAndDie );

void CFireAndDie::Spawn( void )
{
	pev->classname = MAKE_STRING( "fireanddie" );
	// Don't call Precache() - it should be called on restore
}

void CFireAndDie::Precache( void )
{
	// This gets called on restore
	pev->nextthink = gpGlobals->time + m_flDelay;
}

void CFireAndDie::Think( void )
{
	SUB_UseTargets( this, USE_TOGGLE, 0 );
	UTIL_Remove( this );
}

#define SF_CHANGELEVEL_USEONLY 0x0002
class CChangeLevel : public CBaseTrigger
{
  public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT UseChangeLevel( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TriggerChangeLevel( void );
	void EXPORT ExecuteChangeLevel( void );
	void EXPORT TouchChangeLevel( CBaseEntity *pOther );
	void ChangeLevelNow( CBaseEntity *pActivator );

	static edict_t *FindLandmark( const char *pLandmarkName );
	static int ChangeList( LEVELLIST *pLevelList, int maxList );
	static int AddTransitionToList( LEVELLIST *pLevelList, int listCount, const char *pMapName,
									const char *pLandmarkName, edict_t *pentLandmark );
	static int InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	char m_szMapName[cchMapNameMost];		// trigger_changelevel only:  next map
	char m_szLandmarkName[cchMapNameMost];	// trigger_changelevel only:  landmark on next map
	int m_changeTarget;
	float m_changeTargetDelay;
};
LINK_ENTITY_TO_CLASS( trigger_changelevel, CChangeLevel );

// Global Savedata for changelevel trigger
TYPEDESCRIPTION CChangeLevel::m_SaveData[] = {
	DEFINE_ARRAY( CChangeLevel, m_szMapName, FIELD_CHARACTER, cchMapNameMost ),
	DEFINE_ARRAY( CChangeLevel, m_szLandmarkName, FIELD_CHARACTER, cchMapNameMost ),
	DEFINE_FIELD( CChangeLevel, m_changeTarget, FIELD_STRING ),
	DEFINE_FIELD( CChangeLevel, m_changeTargetDelay, FIELD_FLOAT ),
};

// .text:0014fe30 CChangeLevel::Restore (file:triggers.cpp:2412)
IMPLEMENT_SAVERESTORE( CChangeLevel, CBaseTrigger );

//
// Cache user-entity-field values until spawn is called.
//

void CChangeLevel ::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "map" ) )
	{
		if ( strlen( pkvd->szValue ) >= cchMapNameMost )
			ALERT( at_error, "Map name '%s' too long (32 chars)\n", pkvd->szValue );
		strcpy( m_szMapName, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "landmark" ) )
	{
		if ( strlen( pkvd->szValue ) >= cchMapNameMost )
			ALERT( at_error, "Landmark name '%s' too long (32 chars)\n", pkvd->szValue );
		strcpy( m_szLandmarkName, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "changetarget" ) )
	{
		m_changeTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "changedelay" ) )
	{
		m_changeTargetDelay = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

/*QUAKED trigger_changelevel (0.5 0.5 0.5) ? NO_INTERMISSION
When the player touches this, he gets sent to the map listed in the "map" variable.  Unless the NO_INTERMISSION flag is set, the view will go to the info_intermission spot and display stats.
*/

void CChangeLevel ::Spawn( void )
{
	if ( FStrEq( m_szMapName, "" ) )
		ALERT( at_console, "a trigger_changelevel doesn't have a map" );

	if ( FStrEq( m_szLandmarkName, "" ) )
		ALERT( at_console, "trigger_changelevel to %s doesn't have a landmark", m_szMapName );

	if ( !FStringNull( pev->targetname ) )
	{
		SetUse( &CChangeLevel::UseChangeLevel );
	}
	InitTrigger();
	if ( !( pev->spawnflags & SF_CHANGELEVEL_USEONLY ) )
		SetTouch( &CChangeLevel::TouchChangeLevel );
	//	ALERT( at_console, "TRANSITION: %s (%s)\n", m_szMapName, m_szLandmarkName );
}

void CChangeLevel ::ExecuteChangeLevel( void )
{
	MESSAGE_BEGIN( MSG_ALL, SVC_CDTRACK );
	WRITE_BYTE( 3 );
	WRITE_BYTE( 3 );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, SVC_INTERMISSION );
	MESSAGE_END();
}

FILE_GLOBAL char st_szNextMap[cchMapNameMost];
FILE_GLOBAL char st_szNextSpot[cchMapNameMost];

edict_t *CChangeLevel ::FindLandmark( const char *pLandmarkName )
{
	edict_t *pentLandmark;

	pentLandmark = FIND_ENTITY_BY_STRING( NULL, "targetname", pLandmarkName );
	while ( !FNullEnt( pentLandmark ) )
	{
		// Found the landmark
		if ( FClassnameIs( pentLandmark, "info_landmark" ) )
			return pentLandmark;
		else
			pentLandmark = FIND_ENTITY_BY_STRING( pentLandmark, "targetname", pLandmarkName );
	}
	ALERT( at_error, "Can't find landmark %s\n", pLandmarkName );
	return NULL;
}

//=========================================================
// CChangeLevel :: Use - allows level transitions to be
// triggered by buttons, etc.
//
//=========================================================
void CChangeLevel ::UseChangeLevel( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	ChangeLevelNow( pActivator );
}

void CChangeLevel ::ChangeLevelNow( CBaseEntity *pActivator )
{
	edict_t *pentLandmark;
	LEVELLIST levels[16];

	ASSERT( !FStrEq( m_szMapName, "" ) );

	// Don't work in deathmatch
	if ( g_pGameRules->IsDeathmatch() )
		return;

	// Some people are firing these multiple times in a frame, disable
	if ( gpGlobals->time == pev->dmgtime )
		return;

	pev->dmgtime = gpGlobals->time;

	CBaseEntity *pPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	if ( !InTransitionVolume( pPlayer, m_szLandmarkName ) )
	{
		ALERT( at_aiconsole, "Player isn't in the transition volume %s, aborting\n", m_szLandmarkName );
		return;
	}

	// Create an entity to fire the changetarget
	if ( m_changeTarget )
	{
		CFireAndDie *pFireAndDie = GetClassPtr( (CFireAndDie *) NULL );
		if ( pFireAndDie )
		{
			// Set target and delay
			pFireAndDie->pev->target = m_changeTarget;
			pFireAndDie->m_flDelay = m_changeTargetDelay;
			pFireAndDie->pev->origin = pPlayer->pev->origin;
			// Call spawn
			DispatchSpawn( pFireAndDie->edict() );
		}
	}
	// This object will get removed in the call to CHANGE_LEVEL, copy the params into "safe" memory
	strcpy( st_szNextMap, m_szMapName );

	m_hActivator = pActivator;
	SUB_UseTargets( pActivator, USE_TOGGLE, 0 );
	st_szNextSpot[0] = 0;  // Init landmark to NULL

	// look for a landmark entity
	pentLandmark = FindLandmark( m_szLandmarkName );
	if ( !FNullEnt( pentLandmark ) )
	{
		strcpy( st_szNextSpot, m_szLandmarkName );
		gpGlobals->vecLandmarkOffset = VARS( pentLandmark )->origin;
	}
	//	ALERT( at_console, "Level touches %d levels\n", ChangeList( levels, 16 ) );
	ALERT( at_console, "CHANGE LEVEL: %s %s\n", st_szNextMap, st_szNextSpot );
	CHANGE_LEVEL( st_szNextMap, st_szNextSpot );
}

//
// GLOBALS ASSUMED SET:  st_szNextMap
//
void CChangeLevel ::TouchChangeLevel( CBaseEntity *pOther )
{
	if ( !FClassnameIs( pOther->pev, "player" ) )
		return;

	ChangeLevelNow( pOther );
}

// Add a transition to the list, but ignore duplicates
// (a designer may have placed multiple trigger_changelevels with the same landmark)
int CChangeLevel::AddTransitionToList( LEVELLIST *pLevelList, int listCount, const char *pMapName,
									   const char *pLandmarkName, edict_t *pentLandmark )
{
	int i;

	if ( !pLevelList || !pMapName || !pLandmarkName || !pentLandmark )
		return 0;

	for ( i = 0; i < listCount; i++ )
	{
		if ( pLevelList[i].pentLandmark == pentLandmark && strcmp( pLevelList[i].mapName, pMapName ) == 0 )
			return 0;
	}
	strcpy( pLevelList[listCount].mapName, pMapName );
	strcpy( pLevelList[listCount].landmarkName, pLandmarkName );
	pLevelList[listCount].pentLandmark = pentLandmark;
	pLevelList[listCount].vecLandmarkOrigin = VARS( pentLandmark )->origin;

	return 1;
}

int BuildChangeList( LEVELLIST *pLevelList, int maxList )
{
	return CChangeLevel::ChangeList( pLevelList, maxList );
}

int CChangeLevel::InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName )
{
	edict_t *pentVolume;

	if ( pEntity->ObjectCaps() & FCAP_FORCE_TRANSITION )
		return 1;

	// If you're following another entity, follow it through the transition (weapons follow the player)
	if ( pEntity->pev->movetype == MOVETYPE_FOLLOW )
	{
		if ( pEntity->pev->aiment != NULL )
			pEntity = CBaseEntity::Instance( pEntity->pev->aiment );
	}

	int inVolume = 1;  // Unless we find a trigger_transition, everything is in the volume

	pentVolume = FIND_ENTITY_BY_TARGETNAME( NULL, pVolumeName );
	while ( !FNullEnt( pentVolume ) )
	{
		CBaseEntity *pVolume = CBaseEntity::Instance( pentVolume );

		if ( pVolume && FClassnameIs( pVolume->pev, "trigger_transition" ) )
		{
			if ( pVolume->Intersects( pEntity ) )  // It touches one, it's in the volume
				return 1;
			else
				inVolume =
					0;	// Found a trigger_transition, but I don't intersect it -- if I don't find another, don't go!
		}
		pentVolume = FIND_ENTITY_BY_TARGETNAME( pentVolume, pVolumeName );
	}

	return inVolume;
}

// We can only ever move 512 entities across a transition
#define MAX_ENTITY 512

// This has grown into a complicated beast
// Can we make this more elegant?
// This builds the list of all transitions on this level and which entities are in their PVS's and can / should
// be moved across.
int CChangeLevel::ChangeList( LEVELLIST *pLevelList, int maxList )
{
	edict_t *pentChangelevel, *pentLandmark;
	int i, count;

	count = 0;

	// Find all of the possible level changes on this BSP
	pentChangelevel = FIND_ENTITY_BY_STRING( NULL, "classname", "trigger_changelevel" );
	if ( FNullEnt( pentChangelevel ) )
		return 0;
	while ( !FNullEnt( pentChangelevel ) )
	{
		CChangeLevel *pTrigger;

		pTrigger = GetClassPtr( (CChangeLevel *) VARS( pentChangelevel ) );
		if ( pTrigger )
		{
			// Find the corresponding landmark
			pentLandmark = FindLandmark( pTrigger->m_szLandmarkName );
			if ( pentLandmark )
			{
				// Build a list of unique transitions
				if ( AddTransitionToList( pLevelList, count, pTrigger->m_szMapName, pTrigger->m_szLandmarkName,
										  pentLandmark ) )
				{
					count++;
					if ( count >= maxList )	 // FULL!!
						break;
				}
			}
		}
		pentChangelevel = FIND_ENTITY_BY_STRING( pentChangelevel, "classname", "trigger_changelevel" );
	}

	if ( gpGlobals->pSaveData && ( (SAVERESTOREDATA *) gpGlobals->pSaveData )->pTable )
	{
		CSave saveHelper( (SAVERESTOREDATA *) gpGlobals->pSaveData );

		for ( i = 0; i < count; i++ )
		{
			int j, entityCount = 0;
			CBaseEntity *pEntList[MAX_ENTITY];
			int entityFlags[MAX_ENTITY];

			// Follow the linked list of entities in the PVS of the transition landmark
			edict_t *pent = UTIL_EntitiesInPVS( pLevelList[i].pentLandmark );

			// Build a list of valid entities in this linked list (we're going to use pent->v.chain again)
			while ( !FNullEnt( pent ) )
			{
				CBaseEntity *pEntity = CBaseEntity::Instance( pent );
				if ( pEntity )
				{
					//					ALERT( at_console, "Trying %s\n", STRING(pEntity->pev->classname) );
					int caps = pEntity->ObjectCaps();
					if ( !( caps & FCAP_DONT_SAVE ) )
					{
						int flags = 0;

						// If this entity can be moved or is global, mark it
						if ( caps & FCAP_ACROSS_TRANSITION )
							flags |= FENTTABLE_MOVEABLE;
						if ( pEntity->pev->globalname && !pEntity->IsDormant() )
							flags |= FENTTABLE_GLOBAL;
						if ( flags )
						{
							pEntList[entityCount] = pEntity;
							entityFlags[entityCount] = flags;
							entityCount++;
							if ( entityCount > MAX_ENTITY )
								ALERT( at_error, "Too many entities across a transition!" );
						}
						//						else
						//							ALERT( at_console, "Failed %s\n", STRING(pEntity->pev->classname) );
					}
					//					else
					//						ALERT( at_console, "DON'T SAVE %s\n", STRING(pEntity->pev->classname) );
				}
				pent = pent->v.chain;
			}

			for ( j = 0; j < entityCount; j++ )
			{
				// Check to make sure the entity isn't screened out by a trigger_transition
				if ( entityFlags[j] && InTransitionVolume( pEntList[j], pLevelList[i].landmarkName ) )
				{
					// Mark entity table with 1<<i
					int index = saveHelper.EntityIndex( pEntList[j] );
					// Flag it with the level number
					saveHelper.EntityFlagsSet( index, entityFlags[j] | ( 1 << i ) );
				}
				//				else
				//					ALERT( at_console, "Screened out %s\n", STRING(pEntList[j]->pev->classname) );
			}
		}
	}

	return count;
}

/*
go to the next level for deathmatch
only called if a time or frag limit has expired
*/
void NextLevel( void )
{
	edict_t *pent;
	CChangeLevel *pChange;

	// find a trigger_changelevel
	pent = FIND_ENTITY_BY_CLASSNAME( NULL, "trigger_changelevel" );

	// go back to start if no trigger_changelevel
	if ( FNullEnt( pent ) )
	{
		gpGlobals->mapname = ALLOC_STRING( "start" );
		pChange = GetClassPtr( (CChangeLevel *) NULL );
		strcpy( pChange->m_szMapName, "start" );
	}
	else
		pChange = GetClassPtr( (CChangeLevel *) VARS( pent ) );

	strcpy( st_szNextMap, pChange->m_szMapName );
	g_fGameOver = TRUE;

	if ( pChange->pev->nextthink < gpGlobals->time )
	{
		pChange->SetThink( &CChangeLevel::ExecuteChangeLevel );
		pChange->pev->nextthink = gpGlobals->time + 0.1;
	}
}

// ============================== LADDER =======================================

class CLadder : public CBaseTrigger
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
};
LINK_ENTITY_TO_CLASS( func_ladder, CLadder );

void CLadder ::KeyValue( KeyValueData *pkvd )
{
	CBaseTrigger::KeyValue( pkvd );
}

//=========================================================
// func_ladder - makes an area vertically negotiable
//=========================================================
void CLadder ::Precache( void )
{
	// Do all of this in here because we need to 'convert' old saved games
	pev->solid = SOLID_NOT;
	pev->skin = CONTENTS_LADDER;
	if ( CVAR_GET_FLOAT( "showtriggers" ) == 0 )
	{
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 0;
	}
	pev->effects &= ~EF_NODRAW;
}

void CLadder ::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), STRING( pev->model ) );	// set size and link into world
	pev->movetype = MOVETYPE_PUSH;
}

// ========================== A TRIGGER THAT PUSHES YOU ===============================

class CTriggerPush : public CBaseTrigger
{
  public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Touch( CBaseEntity *pOther );
	void RoundRespawn( void );
	int m_iEnabled;
};
LINK_ENTITY_TO_CLASS( trigger_push, CTriggerPush );

void CTriggerPush ::KeyValue( KeyValueData *pkvd )
{
	CBaseTrigger::KeyValue( pkvd );
}

/*QUAKED trigger_push (.5 .5 .5) ? TRIG_PUSH_ONCE
Pushes the player
*/

void CTriggerPush ::Spawn()
{
	if ( pev->angles == g_vecZero )
		pev->angles.y = 360;
	InitTrigger();

	if ( pev->speed == 0 )
		pev->speed = 100;

	if ( FBitSet( pev->spawnflags,
				  SF_TRIGGER_PUSH_START_OFF ) )	 // if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	SetUse( &CTriggerPush::ToggleUse );

	UTIL_SetOrigin( pev, pev->origin );	 // Link into the list
}

void CTriggerPush ::Touch( CBaseEntity *pOther )
{
	entvars_t *pevToucher = pOther->pev;

	// UNDONE: Is there a better way than health to detect things that have physics? (clients/monsters)
	switch ( pevToucher->movetype )
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
	case MOVETYPE_FOLLOW:
		return;
	}

	if ( pevToucher->solid != SOLID_NOT && pevToucher->solid != SOLID_BSP )
	{
		// Instant trigger, just transfer velocity and remove
		if ( FBitSet( pev->spawnflags, SF_TRIG_PUSH_ONCE ) )
		{
			pevToucher->velocity = pevToucher->velocity + ( pev->speed * pev->movedir );
			if ( pevToucher->velocity.z > 0 )
				pevToucher->flags &= ~FL_ONGROUND;
			UTIL_Remove( this );
		}
		else
		{  // Push field, transfer to base velocity
			Vector vecPush = ( pev->speed * pev->movedir );
			if ( pevToucher->flags & FL_BASEVELOCITY )
				vecPush = vecPush + pevToucher->basevelocity;

			pevToucher->basevelocity = vecPush;

			pevToucher->flags |= FL_BASEVELOCITY;
			//			ALERT( at_console, "Vel %f, base %f\n", pevToucher->velocity.z, pevToucher->basevelocity.z );
		}
	}
}

//======================================
// teleport trigger
//
//

void CBaseTrigger ::TeleportTouch( CBaseEntity *pOther )
{
	entvars_t *pevToucher = pOther->pev;
	edict_t *pentTarget = NULL;

	// Only teleport monsters or clients
	if ( !FBitSet( pevToucher->flags, FL_CLIENT | FL_MONSTER ) )
		return;

	if ( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;

	if ( !( pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS ) )
	{  // no monsters allowed!
		if ( FBitSet( pevToucher->flags, FL_MONSTER ) )
		{
			return;
		}
	}

	if ( ( pev->spawnflags & SF_TRIGGER_NOCLIENTS ) )
	{  // no clients allowed
		if ( pOther->IsPlayer() )
		{
			return;
		}
	}

	pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING( pev->target ) );
	if ( FNullEnt( pentTarget ) )
		return;

	Vector tmp = VARS( pentTarget )->origin;

	if ( pOther->IsPlayer() )
	{
		tmp.z -= pOther->pev->mins
					 .z;  // make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}

	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;

	UTIL_SetOrigin( pevToucher, tmp );

	pevToucher->angles = pentTarget->v.angles;

	if ( pOther->IsPlayer() )
	{
		pevToucher->v_angle = pentTarget->v.angles;
	}

	pevToucher->fixangle = TRUE;
	pevToucher->velocity = pevToucher->basevelocity = g_vecZero;
}

class CTriggerTeleport : public CBaseTrigger
{
  public:
	void Spawn( void );
	virtual void RoundRespawn( void );
};
LINK_ENTITY_TO_CLASS( trigger_teleport, CTriggerTeleport );

void CTriggerTeleport ::Spawn( void )
{
	InitTrigger();

	SetTouch( &CTriggerTeleport::TeleportTouch );
}

// .text:001573b0 CTriggerTeleport::RoundRespawn (file:triggers.cpp:3042)
void CTriggerTeleport ::RoundRespawn( void )
{
	InitTrigger();
	SetTouch( &CTriggerTeleport::TeleportTouch );
}

LINK_ENTITY_TO_CLASS( info_teleport_destination, CPointEntity );

class CTriggerSave : public CBaseTrigger
{
  public:
	void Spawn( void );
	void EXPORT SaveTouch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( trigger_autosave, CTriggerSave );

void CTriggerSave::Spawn( void )
{
	if ( g_pGameRules->IsDeathmatch() )
	{
		REMOVE_ENTITY( ENT( pev ) );
		return;
	}

	InitTrigger();
	SetTouch( &CTriggerSave::SaveTouch );
}

void CTriggerSave::SaveTouch( CBaseEntity *pOther )
{
	if ( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;

	// Only save on clients
	if ( !pOther->IsPlayer() )
		return;

	SetTouch( NULL );
	UTIL_Remove( this );
	SERVER_COMMAND( "autosave\n" );
}

#define SF_ENDSECTION_USEONLY 0x0001

class CTriggerEndSection : public CBaseTrigger
{
  public:
	void Spawn( void );
	void EXPORT EndSectionTouch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT EndSectionUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};
LINK_ENTITY_TO_CLASS( trigger_endsection, CTriggerEndSection );

void CTriggerEndSection::EndSectionUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Only save on clients
	if ( pActivator && !pActivator->IsNetClient() )
		return;

	SetUse( NULL );

	if ( pev->message )
	{
		g_engfuncs.pfnEndSection( STRING( pev->message ) );
	}
	UTIL_Remove( this );
}

void CTriggerEndSection::Spawn( void )
{
	if ( g_pGameRules->IsDeathmatch() )
	{
		REMOVE_ENTITY( ENT( pev ) );
		return;
	}

	InitTrigger();

	SetUse( &CTriggerEndSection::EndSectionUse );
	// If it is a "use only" trigger, then don't set the touch function.
	if ( !( pev->spawnflags & SF_ENDSECTION_USEONLY ) )
		SetTouch( &CTriggerEndSection::EndSectionTouch );
}

void CTriggerEndSection::EndSectionTouch( CBaseEntity *pOther )
{
	// Only save on clients
	if ( !pOther->IsNetClient() )
		return;

	SetTouch( NULL );

	if ( pev->message )
	{
		g_engfuncs.pfnEndSection( STRING( pev->message ) );
	}
	UTIL_Remove( this );
}

void CTriggerEndSection ::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "section" ) )
	{
		//		m_iszSectionName = ALLOC_STRING( pkvd->szValue );
		// Store this in message so we don't have to write save/restore for this ent
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

class CTriggerGravity : public CBaseTrigger
{
  public:
	void Spawn( void );
	void EXPORT GravityTouch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( trigger_gravity, CTriggerGravity );

void CTriggerGravity::Spawn( void )
{
	InitTrigger();
	SetTouch( &CTriggerGravity::GravityTouch );
}

void CTriggerGravity::GravityTouch( CBaseEntity *pOther )
{
	// Only save on clients
	if ( !pOther->IsPlayer() )
		return;

	pOther->pev->gravity = pev->gravity;
}

// this is a really bad idea.
class CTriggerChangeTarget : public CBaseDelay
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

  private:
	int m_iszNewTarget;
};
LINK_ENTITY_TO_CLASS( trigger_changetarget, CTriggerChangeTarget );

TYPEDESCRIPTION CTriggerChangeTarget::m_SaveData[] = {
	DEFINE_FIELD( CTriggerChangeTarget, m_iszNewTarget, FIELD_STRING ),
};

// .text:0014fd70 CTriggerChangeTarget::Restore (file:triggers.cpp:3266)
IMPLEMENT_SAVERESTORE( CTriggerChangeTarget, CBaseDelay );

void CTriggerChangeTarget::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "m_iszNewTarget" ) )
	{
		m_iszNewTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerChangeTarget::Spawn( void ) {}

void CTriggerChangeTarget::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = UTIL_FindEntityByString( NULL, "targetname", STRING( pev->target ) );

	if ( pTarget )
	{
		pTarget->pev->target = m_iszNewTarget;
		CBaseMonster *pMonster = pTarget->MyMonsterPointer();
		if ( pMonster )
		{
			pMonster->m_pGoalEnt = NULL;
		}
	}
}

#define SF_CAMERA_PLAYER_POSITION 1
#define SF_CAMERA_PLAYER_TARGET 2
#define SF_CAMERA_PLAYER_TAKECONTROL 4

class CTriggerCamera : public CBaseDelay
{
  public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT FollowTarget( void );
	void Move( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	virtual int ObjectCaps( void ) { return CBaseEntity ::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;
	CBaseEntity *m_pentPath;
	int m_sPath;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	int m_state;
};
LINK_ENTITY_TO_CLASS( trigger_camera, CTriggerCamera );

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION CTriggerCamera::m_SaveData[] = {
	DEFINE_FIELD( CTriggerCamera, m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCamera, m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCamera, m_pentPath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTriggerCamera, m_sPath, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCamera, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_flReturnTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerCamera, m_flStopTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerCamera, m_moveDistance, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_targetSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_initialSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_acceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_deceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_state, FIELD_INTEGER ),
};

// .text:0014fd10 CTriggerCamera::Restore (file:triggers.cpp:3355)
IMPLEMENT_SAVERESTORE( CTriggerCamera, CBaseDelay );

void CTriggerCamera::Spawn( void )
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;	 // Remove model & collisions
	pev->renderamt = 0;		 // The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;

	m_initialSpeed = pev->speed;
	if ( m_acceleration == 0 )
		m_acceleration = 500;
	if ( m_deceleration == 0 )
		m_deceleration = 500;
}

void CTriggerCamera ::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "moveto" ) )
	{
		m_sPath = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "acceleration" ) )
	{
		m_acceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "deceleration" ) )
	{
		m_deceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerCamera::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_state ) )
		return;

	// Toggle state
	m_state = !m_state;
	if ( m_state == 0 )
	{
		m_flReturnTime = gpGlobals->time;
		return;
	}
	if ( !pActivator || !pActivator->IsPlayer() )
	{
		pActivator = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	}

	m_hPlayer = pActivator;

	m_flReturnTime = gpGlobals->time + m_flWait;
	pev->speed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	if ( FBitSet( pev->spawnflags, SF_CAMERA_PLAYER_TARGET ) )
	{
		m_hTarget = m_hPlayer;
	}
	else
	{
		m_hTarget = GetNextTarget();
	}

	// Nothing to look at!
	if ( m_hTarget == NULL )
	{
		return;
	}

	if ( FBitSet( pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL ) )
	{
		( (CBasePlayer *) pActivator )->EnableControl( FALSE );
	}

	if ( m_sPath )
	{
		m_pentPath = Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_sPath ) ) );
	}
	else
	{
		m_pentPath = NULL;
	}

	m_flStopTime = gpGlobals->time;
	if ( m_pentPath )
	{
		if ( m_pentPath->pev->speed != 0 )
			m_targetSpeed = m_pentPath->pev->speed;

		m_flStopTime += m_pentPath->GetDelay();
	}

	// copy over player information
	if ( FBitSet( pev->spawnflags, SF_CAMERA_PLAYER_POSITION ) )
	{
		UTIL_SetOrigin( pev, pActivator->pev->origin + pActivator->pev->view_ofs );
		pev->angles.x = -pActivator->pev->angles.x;
		pev->angles.y = pActivator->pev->angles.y;
		pev->angles.z = 0;
		pev->velocity = pActivator->pev->velocity;
	}
	else
	{
		pev->velocity = Vector( 0, 0, 0 );
	}

	SET_VIEW( pActivator->edict(), edict() );

	SET_MODEL( ENT( pev ), STRING( pActivator->pev->model ) );

	// follow the player down
	SetThink( &CTriggerCamera::FollowTarget );
	pev->nextthink = gpGlobals->time;

	m_moveDistance = 0;
	Move();
}

void CTriggerCamera::FollowTarget()
{
	if ( m_hPlayer == NULL )
		return;

	if ( m_hTarget == NULL || m_flReturnTime < gpGlobals->time )
	{
		if ( m_hPlayer->IsAlive() )
		{
			SET_VIEW( m_hPlayer->edict(), m_hPlayer->edict() );
			( (CBasePlayer *) ( (CBaseEntity *) m_hPlayer ) )->EnableControl( TRUE );
		}
		SUB_UseTargets( this, USE_TOGGLE, 0 );
		pev->avelocity = Vector( 0, 0, 0 );
		m_state = 0;
		return;
	}

	Vector vecGoal = UTIL_VecToAngles( m_hTarget->pev->origin - pev->origin );
	vecGoal.x = -vecGoal.x;

	if ( pev->angles.y > 360 )
		pev->angles.y -= 360;

	if ( pev->angles.y < 0 )
		pev->angles.y += 360;

	float dx = vecGoal.x - pev->angles.x;
	float dy = vecGoal.y - pev->angles.y;

	if ( dx < -180 )
		dx += 360;
	if ( dx > 180 )
		dx = dx - 360;

	if ( dy < -180 )
		dy += 360;
	if ( dy > 180 )
		dy = dy - 360;

	pev->avelocity.x = dx * 40 * gpGlobals->frametime;
	pev->avelocity.y = dy * 40 * gpGlobals->frametime;

	if ( !( FBitSet( pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL ) ) )
	{
		pev->velocity = pev->velocity * 0.8;
		if ( pev->velocity.Length() < 10.0 )
			pev->velocity = g_vecZero;
	}

	pev->nextthink = gpGlobals->time;

	Move();
}

void CTriggerCamera::Move()
{
	// Not moving on a path, return
	if ( !m_pentPath )
		return;

	// Subtract movement from the previous frame
	m_moveDistance -= pev->speed * gpGlobals->frametime;

	// Have we moved enough to reach the target?
	if ( m_moveDistance <= 0 )
	{
		// Fire the passtarget if there is one
		if ( m_pentPath->pev->message )
		{
			FireTargets( STRING( m_pentPath->pev->message ), this, this, USE_TOGGLE, 0 );
			if ( FBitSet( m_pentPath->pev->spawnflags, SF_CORNER_FIREONCE ) )
				m_pentPath->pev->message = 0;
		}
		// Time to go to the next target
		m_pentPath = m_pentPath->GetNextTarget();

		// Set up next corner
		if ( !m_pentPath )
		{
			pev->velocity = g_vecZero;
		}
		else
		{
			if ( m_pentPath->pev->speed != 0 )
				m_targetSpeed = m_pentPath->pev->speed;

			Vector delta = m_pentPath->pev->origin - pev->origin;
			m_moveDistance = delta.Length();
			pev->movedir = delta.Normalize();
			m_flStopTime = gpGlobals->time + m_pentPath->GetDelay();
		}
	}

	if ( m_flStopTime > gpGlobals->time )
		pev->speed = UTIL_Approach( 0, pev->speed, m_deceleration * gpGlobals->frametime );
	else
		pev->speed = UTIL_Approach( m_targetSpeed, pev->speed, m_acceleration * gpGlobals->frametime );

	float fraction = 2 * gpGlobals->frametime;
	pev->velocity = ( ( pev->movedir * pev->speed ) * fraction ) + ( pev->velocity * ( 1 - fraction ) );
}
// DOD CSandBag plus CDoDCamera - source restore/triggers/dod_triggers.cpp - original file dod/dlls/triggers.cpp
// dod_triggers.cpp -- DoD-specific triggers from dod/dlls/triggers.cpp
// Restored from dod.so. Ghidra addr = link addr + 0x10000.
// file:line below is addr2line on the LINK address (use Ghidra addr - 0x10000).
//
// SCOPE (per work split):
//   PORTED here: dod_trigger_sandbag (CSandBag), dod_camera (CDoDCamera).
//   SDK-IDENTICAL (not ported, listed for completeness): every other
//     trigger_* / func_* / info_teleport_destination / multi_manager /
//     trigger_camera (SDK) / trigger_endsection / trigger_cdaudio etc.
//   NOT HERE (other agents): dod_capture_area / dod_control_point /
//     dod_control_point_master / dod_object / dod_object_goal /
//     dod_point_relay / dod_round_timer / dod_location / dod_mapmarker /
//     info_doddetect -> objectives agent (~/rev/restore/objectives/);
//     dod_preround -> gamerules agent; dod_stats -> stats (not ported).
//
// DoD entity -> class map (LINK_ENTITY_TO_CLASS, verified via string xrefs):
//   dod_trigger_sandbag -> CSandBag (this file)
//   dod_camera          -> CDoDCamera (this file)
//   ammo_generic_american / german / british -> GenericAmmo* (items.cpp)
//   dod_ammo -> CDodAmmoBox (weapons agent, see NOTES)
//
// KeyValue fields -- CSandBag (rest unhandled):
//   sandbag_range (int) -> m_nRange | master (string) -> m_sMaster
//
// KeyValue fields -- CDoDCamera (rest -> CBaseDelay::KeyValue):
//   wait (float) -> m_flWait | moveto (string) -> m_sPath |
//   acceleration (float) -> m_acceleration | deceleration (float) -> m_deceleration |
//   teambound (int) -> m_teamBound (3 = both teams) |
//   zoomlevel (int, clamped 0..90) -> m_iFOV (QUIRK: fHandled NOT set) |
//   overlay (int) -> m_iOverlay | subtitle (int) -> m_iSubtitle |
//   triggerstart (string) -> m_sTrigrStart | triggerend (string) -> m_sTrigrEnd |
//   cam_fade (int) -> m_iCamFade

#include "cbase.h"

class CSandBag : public CBaseTrigger
{
  public:
	CSandBag();
	void Spawn();
	void RoundRespawn();
	void KeyValue( KeyValueData *pkvd );
	void EXPORT SandBagTouch( CBaseEntity *pOther );
	BOOL IsMasteredOn();

	int m_nYaw;		// +0x200, from pev->angles.y at Spawn
	int m_nRange;	// +0x204, default 5 (ctor + LINK alloc init)
	int m_sMaster;	// +0x208, string_t
};

// .text:0015aa50 dod_trigger_sandbag (file:triggers.cpp:5210)
// Factory: GetClassPtr sets the vtable + pev (ctor inits m_nRange=5, m_sMaster=0).
LINK_ENTITY_TO_CLASS( dod_trigger_sandbag, CSandBag );

// .text:0015aad0 CSandBag::CSandBag (file:triggers.cpp:5212)
CSandBag::CSandBag()
{
	m_nRange = 5;
	m_sMaster = 0;
}

// .text:00157180 CSandBag::Spawn (file:triggers.cpp:5220)
void CSandBag::Spawn()
{
	m_nYaw = (int) pev->angles.y;
	if ( m_nYaw > 180 )
		m_nYaw -= 360;
	CBaseTrigger::InitTrigger();
	Precache();
	pev->sequence = 0;
	pev->framerate = 1.0;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SetTouch( &CSandBag::SandBagTouch );
}

// .text:00157120 CSandBag::RoundRespawn (file:triggers.cpp:5260)
void CSandBag::RoundRespawn()
{
	CBaseTrigger::InitTrigger();
	pev->sequence = 0;
	pev->framerate = 1.0;
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SetTouch( &CSandBag::SandBagTouch );
}

// .text:00151ca0 CSandBag::KeyValue (file:triggers.cpp:5244)
void CSandBag::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "sandbag_range" ) )
	{
		m_nRange = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		pkvd->fHandled = FALSE;
	}
}

// .text: CSandBag::IsMasteredOn inlined (no symbol; 305B = SandBagTouch 0x141070 size 0x131=305): return !m_sMaster || IsMasterTriggered exact
BOOL CSandBag::IsMasteredOn()
{
	return !m_sMaster || UTIL_IsMasterTriggered( m_sMaster, this );
}

// .text:00151070 CSandBag::SandBagTouch (file:triggers.cpp:5280)
// Deploy/bipod zone: sets bit0 @player+0x76c always, and bit 0x400 @player+0x7cc
// with hint on first touch. Facing window = m_nRange degrees (default 5).
// Active-item gate: m_pActiveItem->Classify() == 0x100 (LMG: MG34/MG42/BREN,
// weapons agents own exact table) OR player already deployed.
void CSandBag::SandBagTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() || !pOther->IsAlive() )
		return;
	if ( !IsMasteredOn() )
		return;
	int delta = abs( (int) pOther->pev->angles.y - m_nYaw );
	if ( delta > 180 )
		delta = 360 - delta;
	if ( CBasePlayer::IsProne( (CBasePlayer *) pOther ) )
	{
		if ( !CBasePlayer::IsSandbagDeployed( (CBasePlayer *) pOther ) )
			return;
	}
	else
	{
		if ( pOther->pev->button & IN_DUCK )
			return;
		if ( delta > m_nRange )
		{
			if ( !CBasePlayer::IsSandbagDeployed( (CBasePlayer *) pOther ) )
				return;
		}
		else
		{
			CBasePlayerItem *pActive = ( (CBasePlayer *) pOther )->m_pActiveItem;  // +0x46c
			if ( !pActive || pActive->Classify() != 0x100 )						   // 0x100 = LMG class
			{
				if ( !CBasePlayer::IsSandbagDeployed( (CBasePlayer *) pOther ) )
					return;
			}
		}
	}
	// player fields owned by player agent: dword @+0x76c bit 0, dword @+0x7cc bit 0x400
	CBasePlayer *pPlayer = (CBasePlayer *) pOther;
	*(int *) ( (char *) pPlayer + 0x76c ) |= 1;
	int *pFlags = (int *) ( (char *) pPlayer + 0x7cc );
	if ( !( *pFlags & 0x400 ) )
	{
		*pFlags |= 0x400;
		CBasePlayer::HintMessage( pPlayer, "#Hint_sandbag_area_touch", 0 );
	}
}

// ---- dod_camera (CDoDCamera) ----
// Cutscene camera for win/loose/draw/spawn scenes (see gamerules
// FireDODCamera: allies_win_cut, axis_loose_cut, ...). Spawn hides the model
// (null.mdl, MOVETYPE_NOCLIP, SOLID_NOT, rendermode TransTexture) and caches
// pev->angles/origin plus the mapper-supplied pev->speed; speed then follows
// path_corner targets. Entity size 0x2c8.
// iuser2 selects the scene slot:
//   1 axis spawn | 2 allie spawn | 3 axis win | 4 allie win |
//   5 axis loose | 6 allie loose | 7 axis draw | 8 allie draw

class CDoDCamera : public CBaseDelay
{
  public:
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	void Precache( void );
	static TYPEDESCRIPTION m_SaveData[];
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT FollowTarget();
	void Move();
	void SetCameraMode( const char *szTargetname, int nFade );
	float ReturnTimeLeft( void );
	int ObjectCaps( void ) { return 0; }

	EHANDLE m_hPlayer[33];	 // +0x160, indices 1..32 used (MAX clients)
	EHANDLE m_hTarget;		 // +0x268
	string_t m_sPath;		 // +0x26c, moveto target (first path_corner)
	int m_teamBound;		 // +0x270, 3 = both teams
	string_t m_sTrigrStart;	 // +0x274
	string_t m_sTrigrEnd;	 // +0x278
	int m_iOverlay;			 // +0x27c
	int m_iFOV;				 // +0x280, zoomlevel clamped 0..90
	int m_iSubtitle;		 // +0x284
	int m_iCamFade;			 // +0x288
	float m_flWait;			 // +0x28c
	float m_flReturnTime;	 // +0x290
	float m_flStopTime;		 // +0x294
	float m_moveDistance;	 // +0x298
	float m_targetSpeed;	 // +0x29c
	float m_initialSpeed;	 // +0x2a0
	float m_acceleration;	 // +0x2a4
	float m_deceleration;	 // +0x2a8
	int m_state;			 // +0x2ac
	CBaseEntity
		*m_pentPath;  // +0x2b0 binary-verified 0x14c3d0 alloc 0x2c8, offsets 0x26c..0x2ac KeyValue block verified
	Vector m_angles;  // +0x2b4 binary-verified Move origin-angles block 0x14c580
	Vector
		m_origin;  // +0x2c0 binary-verified Move origin-angles block 0x14c580; alloc 0x2c8 leaves 4 bytes tail verified
};

// .text:0015c3d0 dod_camera (file:triggers.cpp:6554)
// Factory: GetClassPtr sets the vtable + pev (size 0x2c8).
LINK_ENTITY_TO_CLASS( dod_camera, CDoDCamera );

// .text:0014efa0 CDoDCamera::Precache (file:triggers.cpp:6675)
void CDoDCamera::Precache( void )
{
	PRECACHE_MODEL( "models/mapmodels/null.mdl" );
}

// .text:0014efb0 CDoDCamera::Spawn (file:triggers.cpp:6577)
void CDoDCamera::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/mapmodels/null.mdl" );
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->renderamt = 0.0;
	pev->rendermode = 2;
	m_angles = pev->angles;	 // remember spawn angles/origin
	m_origin = pev->origin;
	for ( int i = 1; i <= 32; i++ )
		m_hPlayer[i] = NULL;
	m_initialSpeed = pev->speed;
	if ( m_acceleration == 0.0 )
		m_acceleration = 500.0;
	if ( m_deceleration == 0.0 )
		m_deceleration = 500.0;
}

// .text:00153180 CDoDCamera::KeyValue (file:triggers.cpp:6671)
void CDoDCamera::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "moveto" ) )
	{
		m_sPath = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "acceleration" ) )
	{
		m_acceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "deceleration" ) )
	{
		m_deceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "teambound" ) )
	{
		m_teamBound = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "zoomlevel" ) )
	{
		// QUIRK: clamps 0..90 but does NOT set fHandled (falls through unset).
		m_iFOV = atoi( pkvd->szValue );
		if ( m_iFOV > 90 )
			m_iFOV = 90;
		if ( m_iFOV < 0 )
			m_iFOV = 0;
	}
	else if ( FStrEq( pkvd->szKeyName, "overlay" ) )
	{
		m_iOverlay = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "subtitle" ) )
	{
		m_iSubtitle = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "triggerstart" ) )
	{
		m_sTrigrStart = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "triggerend" ) )
	{
		m_sTrigrEnd = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "cam_fade" ) )
	{
		m_iCamFade = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseDelay::KeyValue( pkvd );
	}
}

static BOOL SceneAlreadySeen( CBasePlayer *pPlayer, int slot );
static void MarkSceneSeen( CBasePlayer *pPlayer, int slot );
static const char *CutSceneName( int scene );

// .text:0015ced0 CDoDCamera::Use (file:triggers.cpp:6679)
// Binds players 1..32 (skips non-team, wrong-team unless teamBound==3,
// invisible/dead-check via iuser2 scene slot state), then on toggle-on:
// sets speed/target from moveto path, snaps to stored origin/angles,
// SetView + EnableControl(0) + CameraView msg + scene-seen flag per iuser2,
// fires triggerstart, think = FollowTarget.
// NaN gate: binary 0x14ced0 Use has no float compare in bind loop, decompiler NaN equality never true, port omits it verified.
void CDoDCamera::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_state ) )
		return;
	// scene-seen flags live on CBasePlayer (player agent owns offsets):
	// 1 i_seenAxisSpawnScene | 2 i_seenAllieSpawnScene | 3 i_seenAxisWinScene |
	// 4 i_seenAllieWinScene | 5 i_seenAxisLooseScene | 6 i_seenAllieLooseScene |
	// 7 i_seenAxisDrawScene | 8 i_seenAllieDrawScene
	BOOL bFound = FALSE;
	for ( int i = 1; i <= 32; i++ )
	{
		m_hPlayer[i] = UTIL_PlayerByIndex( i );
		if ( !m_hPlayer[i] )
			continue;
		CBaseEntity *pEnt = m_hPlayer[i];
		if ( !pEnt->IsPlayer() || ( m_teamBound != 3 && pEnt->pev->team != m_teamBound ) ||
			 SceneAlreadySeen( (CBasePlayer *) (CBaseEntity *) m_hPlayer[i], pev->iuser2 ) )
		{
			m_hPlayer[i] = NULL;
			continue;
		}
		bFound = TRUE;
	}
	if ( !bFound )
		return;
	m_state = !m_state;
	if ( m_state == 0 )
	{
		m_flReturnTime = gpGlobals->time;
		return;
	}
	m_flReturnTime = gpGlobals->time + m_flWait;
	pev->speed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;
	m_hTarget = GetNextTarget();
	if ( !m_hTarget )
		return;
	if ( !m_sPath )
	{
		m_pentPath = NULL;
		m_flStopTime = gpGlobals->time;
	}
	else
	{
		m_pentPath = UTIL_FindEntityByTargetname( NULL, STRING( m_sPath ) );
		m_flStopTime = gpGlobals->time;
		if ( m_pentPath )
		{
			if ( m_pentPath->pev->speed != 0 )
				m_targetSpeed = m_pentPath->pev->speed;
			m_flStopTime = gpGlobals->time + m_pentPath->GetDelay();  // vtable[0x1f]
		}
	}
	pev->velocity = g_vecZero;
	pev->angles = m_angles;
	pev->origin = m_origin;
	for ( int i = 1; i <= 32; i++ )
	{
		if ( !m_hPlayer[i] || m_hPlayer[i]->pev->team <= 0 )
			continue;
		CBasePlayer *pPlayer = (CBasePlayer *) (CBaseEntity *) m_hPlayer[i];
		pPlayer->pev->vuser1.z = 1.0;
		UTIL_ScreenFade( pPlayer, g_vecZero, 1.0, 15.0, 0, 5 );
		g_engfuncs.pfnSetView( pPlayer->pev->pContainingEntity, pev->pContainingEntity );
		pPlayer->EnableControl( 0 );
		MESSAGE_BEGIN( MSG_ONE, gmsgCameraView, NULL, pPlayer->pev );
		WRITE_STRING( STRING( pev->targetname ) );
		MESSAGE_END();
		MarkSceneSeen( pPlayer, pev->iuser2 );	// sets the i_seen* flag above
	}
	FireTargets( STRING( m_sTrigrStart ), this, this, USE_TOGGLE, 0 );
	// dword @g_pGameRules+8 inc (active-camera counter; gamerules agent owns it)
	( *(int *) ( (char *) g_pGameRules + 8 ) )++;
	SetThink( &CDoDCamera::FollowTarget );
	pev->nextthink = gpGlobals->time;
	m_moveDistance = 0;
	Move();
}

// .text:0015cb50 CDoDCamera::FollowTarget (file:triggers.cpp:6857)
// While time is before m_flReturnTime and m_hTarget lives: face the target
// (angle wrap + avelocity), damp velocity unless spawnflags&4, think again
// next frame via Move(). Otherwise: release all bound players (EnableControl
// + SetView self), fire triggerend + SUB_UseTargets, park at stored
// origin/angles, m_state = 0.
void CDoDCamera::FollowTarget()
{
	if ( m_hTarget && gpGlobals->time <= m_flReturnTime )
	{
		Vector vecGoal = m_hTarget->pev->origin - pev->origin;
		Vector vecAng;
		vecAng = UTIL_VecToAngles( vecGoal );
		vecAng.x = -vecAng.x;
		if ( pev->angles.y > 360.0 )
			pev->angles.y -= 360.0;
		if ( pev->angles.y < 0.0 )
			pev->angles.y += 360.0;
		float dx = vecAng.x - pev->angles.x;
		float dy = vecAng.y - pev->angles.y;
		if ( dx < -180.0 )
			dx += 360.0;
		if ( dx > 180.0 )
			dx -= 360.0;
		if ( dy < -180.0 )
			dy += 360.0;
		if ( dy > 180.0 )
			dy -= 360.0;
		pev->avelocity.x = dx * 40.0 * gpGlobals->frametime;
		pev->avelocity.y = dy * 40.0 * gpGlobals->frametime;
		if ( !( pev->spawnflags & 4 ) )
		{
			pev->velocity = pev->velocity * 0.8;
			if ( pev->velocity.Length() < 10.0 )
				pev->velocity = g_vecZero;
		}
		pev->nextthink = gpGlobals->time;
		Move();
		return;
	}
	// dword @g_pGameRules+8 dec (active-camera counter; gamerules agent owns it)
	( *(int *) ( (char *) g_pGameRules + 8 ) )--;
	for ( int i = 1; i <= 32; i++ )
	{
		if ( !m_hPlayer[i] )
			continue;
		CBaseEntity *pEnt = m_hPlayer[i];
		if ( pEnt->pev->team > 0 )
		{
			( (CBasePlayer *) (CBaseEntity *) m_hPlayer[i] )->EnableControl( 1 );
			g_engfuncs.pfnSetView( pEnt->pev->pContainingEntity, pEnt->pev->pContainingEntity );
			pEnt->pev->vuser1.z = 0;
			m_hPlayer[i] = NULL;
		}
	}
	FireTargets( STRING( m_sTrigrEnd ), this, this, USE_TOGGLE, 0 );
	SUB_UseTargets( this, USE_TOGGLE, 0 );
	pev->avelocity = g_vecZero;
	m_state = 0;
	pev->angles = m_angles;
	pev->origin = m_origin;
}

// file-local reconstruction of the inline iuser2 scene-slot gate in Use:
// slot -> CBasePlayer::i_seen* flag binary-verified 0x14ced0 Use scene gates 1..8, member names from DWARF.
static int SeenFlag( CBasePlayer *pPlayer, int slot )
{
	switch ( slot )
	{
	case 1:
		return pPlayer->i_seenAxisSpawnScene;
	case 2:
		return pPlayer->i_seenAllieSpawnScene;
	case 3:
		return pPlayer->i_seenAxisWinScene;
	case 4:
		return pPlayer->i_seenAllieWinScene;
	case 5:
		return pPlayer->i_seenAxisLooseScene;
	case 6:
		return pPlayer->i_seenAllieLooseScene;
	case 7:
		return pPlayer->i_seenAxisDrawScene;
	case 8:
		return pPlayer->i_seenAllieDrawScene;
	default:
		return 0;
	}
}
static BOOL SceneAlreadySeen( CBasePlayer *pPlayer, int slot )
{
	return SeenFlag( pPlayer, slot ) != 0;
}
static void MarkSceneSeen( CBasePlayer *pPlayer, int slot )
{
	switch ( slot )
	{
	case 1:
		pPlayer->i_seenAxisSpawnScene = 1;
		break;
	case 2:
		pPlayer->i_seenAllieSpawnScene = 1;
		break;
	case 3:
		pPlayer->i_seenAxisWinScene = 1;
		break;
	case 4:
		pPlayer->i_seenAllieWinScene = 1;
		break;
	case 5:
		pPlayer->i_seenAxisLooseScene = 1;
		break;
	case 6:
		pPlayer->i_seenAllieLooseScene = 1;
		break;
	case 7:
		pPlayer->i_seenAxisDrawScene = 1;
		break;
	case 8:
		pPlayer->i_seenAllieDrawScene = 1;
		break;
	default:
		break;
	}
}

// .text:0015c580 CDoDCamera::Move (file:triggers.cpp:6943)
// Advances m_moveDistance by pev->speed * frametime toward m_pentPath.
// On arrival: console log (plain for iuser2 < 1, cut-scene name for 1..8),
// fire path message target (cleared when spawnflags & 4), advance along
// GetNextTarget chain, re-seed speed/distance/dir. Speed approaches
// m_targetSpeed (accel) before m_flStopTime else 0 (decel); velocity blends
// with 2 * frametime smoothing. Cut-scene names by iuser2:
//   1 axis_spawn_cut | 2 allie_spawn_cut | 3 axis_win_cut | 4 allie_win_cut |
//   5 axis_loose_cut | 6 allie_loose_cut | 7 axis_draw_cut | 8 allie_draw_cut
void CDoDCamera::Move()
{
	CBaseEntity *pPath = m_pentPath;
	if ( !pPath )
	{
		ALERT( at_console, "**dod_camera** No Path_corner FOUND !!\n" );
		return;
	}
	m_moveDistance = m_moveDistance - pev->speed * gpGlobals->frametime;
	if ( m_moveDistance <= 0 )
	{
		if ( pev->iuser2 < 1 )
		{
			ALERT( at_console, "**dod_camera** Reached %s: %s\n", STRING( pPath->pev->classname ),
				   STRING( pPath->pev->targetname ) );
		}
		else
		{
			ALERT( at_console, "**dod_camera** type: %s  Reached %s: %s\n", CutSceneName( pev->iuser2 ),
				   STRING( pPath->pev->classname ), STRING( pPath->pev->targetname ) );
		}
		if ( pPath->pev->message )
		{
			FireTargets( STRING( pPath->pev->message ), this, this, USE_TOGGLE, 0 );
			if ( pPath->pev->spawnflags & 4 )
				pPath->pev->message = 0;
		}
		if ( !strcmp( STRING( pPath->pev->classname ), "path_corner" ) )
			// fade = dword @pev+0x164 = pev->frags (calibrated entvars map, see NOTES)
			SetCameraMode( STRING( pPath->pev->targetname ), (int) pPath->pev->frags );
		pPath = pPath->GetNextTarget();
		m_pentPath = pPath;
		if ( !pPath )
		{
			pev->velocity = g_vecZero;
		}
		else
		{
			if ( pPath->pev->speed != 0 )
				m_targetSpeed = pPath->pev->speed;
			Vector delta = pPath->pev->origin - pev->origin;
			float len = delta.Length();
			m_moveDistance = len;
			if ( len == 0 )
				pev->movedir = Vector( 0, 0, 1 );
			else
				pev->movedir = delta * ( 1.0 / len );
			// path_corner delay wait (vtable[0x1f] = GetDelay)
			m_flStopTime = gpGlobals->time + pPath->GetDelay();
		}
	}
	if ( m_flStopTime <= gpGlobals->time )
		pev->speed = UTIL_Approach( m_targetSpeed, pev->speed, m_acceleration * gpGlobals->frametime );
	else
		pev->speed = UTIL_Approach( 0, pev->speed, m_deceleration * gpGlobals->frametime );
	float blend = gpGlobals->frametime + gpGlobals->frametime;
	pev->velocity = pev->velocity * ( 1.0 - blend ) + pev->movedir * pev->speed * blend;
}

// file-local: iuser2 -> cut-scene target name for the Move() log line.
static const char *CutSceneName( int scene )
{
	switch ( scene )
	{
	case 1:
		return "axis_spawn_cut";
	case 2:
		return "allie_spawn_cut";
	case 3:
		return "axis_win_cut";
	case 4:
		return "allie_win_cut";
	case 5:
		return "axis_loose_cut";
	case 6:
		return "allie_loose_cut";
	case 7:
		return "axis_draw_cut";
	case 8:
		return "allie_draw_cut";
	default:
		return "";
	}
}

// .text:0015c450 CDoDCamera::SetCameraMode (file:triggers.cpp:7070)
// Sends gmsgCameraView(targetname) to every bound on-team player, with an
// optional screen fade. fade index: 1..5 -> (fadeTime, flags) =
// (0.6,1) (2.0,1) (2.0,0) (3.0,1) (4.0,0); anything else -> (2.0,5).
// holdtime 1.0, color black, alpha 0xff.
void CDoDCamera::SetCameraMode( const char *szTargetname, int nFade )
{
	for ( int i = 1; i <= 32; i++ )
	{
		if ( !m_hPlayer[i] )
			continue;
		CBaseEntity *pEnt = m_hPlayer[i];
		if ( pEnt->pev->team <= 0 )
			continue;
		if ( nFade != 0 )
		{
			float fadeTime;
			int flags;
			switch ( nFade )
			{
			case 1:
				fadeTime = 0.6;
				flags = 1;
				break;
			case 2:
				fadeTime = 2.0;
				flags = 1;
				break;
			case 3:
				fadeTime = 2.0;
				flags = 0;
				break;
			case 4:
				fadeTime = 3.0;
				flags = 1;
				break;
			case 5:
				fadeTime = 4.0;
				flags = 0;
				break;
			default:
				fadeTime = 2.0;
				flags = 5;
				break;
			}
			UTIL_ScreenFade( (CBaseEntity *) m_hPlayer[i], g_vecZero, fadeTime, 1.0, 0xff, flags );
		}
		MESSAGE_BEGIN( MSG_ONE, gmsgCameraView, NULL, pEnt->pev );
		WRITE_STRING( szTargetname );
		MESSAGE_END();
	}
}


// .text:0015a710 GetClanScoringValues (file:triggers.cpp:4654)
int GetClanScoringValues( int team, int cp )
{
	const char *szValues = NULL;
	if ( team == 1 )
		szValues = CVAR_GET_STRING( "mp_clan_scoring_values_allies" );
	else if ( team == 2 )
		szValues = CVAR_GET_STRING( "mp_clan_scoring_values_axis" );
	else
		return 0;
	if ( !szValues || (int)strlen( szValues ) - 1 < cp )
	{
		ALERT( at_console, "Tried to get a clan scoring value for a point not defined in mp_clan_scoring_values_x\n" );
		return 0;
	}
	return szValues[cp] - 30;
}

// .text:0015c440 CDoDCamera::ReturnTimeLeft (file:triggers.cpp:7065)
float CDoDCamera::ReturnTimeLeft( void )
{
	return m_flReturnTime;
}
extern int gmsgClientAreas;
extern int gmsgTimerStatus;
extern int gmsgPlayersInArea;
extern int gmsgProgBarUpdate;
extern int gmsgCancelProgressBar;

// ---- CDoDCamera SaveData (binary SaveData[13], mirrors CTriggerCamera 13 fields) ----
// .text:001400d0 CDoDCamera::Save (file:triggers.cpp:6560)
// .text:0014fcb0 CDoDCamera::Restore (file:triggers.cpp:6574)
TYPEDESCRIPTION CDoDCamera::m_SaveData[] = {
	DEFINE_FIELD( CDoDCamera, m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CDoDCamera, m_pentPath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CDoDCamera, m_sPath, FIELD_STRING ),
	DEFINE_FIELD( CDoDCamera, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CDoDCamera, m_flReturnTime, FIELD_TIME ),
	DEFINE_FIELD( CDoDCamera, m_flStopTime, FIELD_TIME ),
	DEFINE_FIELD( CDoDCamera, m_moveDistance, FIELD_FLOAT ),
	DEFINE_FIELD( CDoDCamera, m_targetSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CDoDCamera, m_initialSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CDoDCamera, m_acceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CDoDCamera, m_deceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CDoDCamera, m_state, FIELD_INTEGER ),
	DEFINE_FIELD( CDoDCamera, m_teamBound, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CDoDCamera, CBaseDelay );

// ODR: CControlPoint canonical in objective.h (DWARF layout)
// ODR: CControlPointMaster canonical in objective.h

// ODR: CAreaCapture canonical in objective.h

// ODR: CObjectCapture canonical in objective.h

// ODR: CDodRoundTimer canonical in objective.h


// CControlPoint (binary ctor + getters)
// .text:00159910 CControlPoint::CControlPoint (file:triggers.cpp:3949)
CControlPoint::CControlPoint()
{
	m_iPointIndex = -1;
	m_bPointVisible = 0;
	m_iAlliesIcon = 1;
	m_iAxisIcon = 2;
	m_iNeutralIcon = 0;
	m_iAlliesModelBodygroup = 0;
	m_bAlliesCantTouch = false;
	m_bAxisCantTouch = false;
	m_szGroup[0] = 0;
	m_iAxisModelBodygroup = 0;
	m_iResetModelBodygroup = 0;
}
// .text:0015a610 CControlPoint::GetOwner (file:triggers.cpp:4295)
int CControlPoint::GetOwner( void ) { return m_iTeam; }
// .text:0015a620 CControlPoint::GetGroup (file:triggers.cpp:4300)
char *CControlPoint::GetGroup( void ) { return m_szGroup; }
// .text:0015a630 CControlPoint::PointValue (file:triggers.cpp:4310)
int CControlPoint::PointValue( void ) { return m_iPointValue; }

// CControlPointMaster helpers
// .text:0015a810 CControlPointMaster::GetPointOwner (file:triggers.cpp:5089)
int CControlPointMaster::GetPointOwner( int point )
{
	int owner = 0;
	if ( point >= 0 && point < 20 && ControlPointArray[point] )
		owner = ( (CControlPoint *)ControlPointArray[point] )->m_iTeam;
	return owner;
}
// .text:0015a9f0 CControlPointMaster::TeamOwnsAllPoints (file:triggers.cpp:5158)
int CControlPointMaster::TeamOwnsAllPoints()
{
	if ( m_iNumPoints < 1 || !ControlPointArray[0] )
		return 0;
	int team = ( (CControlPoint *)ControlPointArray[0] )->m_iTeam;
	for ( int i = 1; i < m_iNumPoints; i++ )
	{
		if ( !ControlPointArray[i] || ( (CControlPoint *)ControlPointArray[i] )->m_iTeam != team )
			return 0;
	}
	return team;
}

// CAreaCapture helpers (decompile + DWARF field names)
// .text:00150b00 CAreaCapture::area_SetIndex (file:triggers.cpp:5455)
void CAreaCapture::area_SetIndex( int index )
{
	m_iAreaIndex = index;
	if ( index > 0x80 )
		return;
	MESSAGE_BEGIN( MSG_ALL, gmsgClientAreas );
		WRITE_BYTE( m_iAreaIndex );
		WRITE_BYTE( -1 );
		WRITE_STRING( sz_HudIcon );
	MESSAGE_END();
}
// .text:0015af00 CAreaCapture::area_SendStatus (file:triggers.cpp:5475)
void CAreaCapture::area_SendStatus( CBasePlayer *p )
{
	if ( !p )
		return;
	if ( m_iAreaIndex > 0x80 )
		return;
	MESSAGE_BEGIN( MSG_ONE, gmsgClientAreas, NULL, p->pev );
		WRITE_BYTE( m_iAreaIndex );
		WRITE_BYTE( -1 );
		WRITE_STRING( sz_HudIcon );
	MESSAGE_END();
	SendNumPlayers( p );
}
// .text:0015ab60 CAreaCapture::GetNumRequiredPlayers (file:triggers.cpp:5584)
void CAreaCapture::GetNumRequiredPlayers( int *reqAllies, int *reqAxis )
{
	*reqAllies = m_nAlliesNumCap;
	*reqAxis = m_nAxisNumCap;
	if ( *reqAllies < 1 )
		*reqAllies = 1;
	if ( *reqAxis < 1 )
		*reqAxis = 1;
}
// .text:0015ac30 CAreaCapture::CanUseCatchup (file:triggers.cpp:5616)
// ODR: CDoDTeamPlay canonical in dod_gamerules.h
bool CAreaCapture::CanUseCatchup()
{
	bool para = false;
	if ( g_pGameRules )
	{
		CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;
		if ( pRules->IsAlliesInfinite() && !pRules->IsAxisInfinite() )
			para = true;
	}
	return (bool)( para && ( pev->spawnflags & 1 ) );
}
// .text:0015ac80 CAreaCapture::GetNumAlive (file:triggers.cpp:5860)
int CAreaCapture::GetNumAlive( int team )
{
	int n = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( i );
		if ( pEnt && pEnt->IsPlayer() && pEnt->pev->team == team && pEnt->pev->deadflag == DEAD_NO )
			n++;
	}
	return n;
}
// .text:0015ad40 CAreaCapture::SendNumPlayers (file:triggers.cpp:5886)
void CAreaCapture::SendNumPlayers( CBasePlayer *pPlayer )
{
	if ( !m_pPoint )
		return;
	int team = 0;
	int num = 0;
	int req = -1;
	int allies = m_nNumAllies;
	int axis = m_nNumAxis;
	if ( allies > 0 && axis < 1 && m_nOwningTeam != 1 )
	{
		int ra = 1, rx = 1;
		GetNumRequiredPlayers( &ra, &rx );
		team = 1; num = allies; req = ra;
	}
	else if ( axis > 0 && allies < 1 && m_nOwningTeam != 2 )
	{
		int ra = 1, rx = 1;
		GetNumRequiredPlayers( &ra, &rx );
		team = 2; num = axis; req = rx;
	}
	else
	{
		int ra = 1, rx = 1;
		GetNumRequiredPlayers( &ra, &rx );
		team = 0; num = 0; req = -1;
	}
	if ( req != -1 && req < 2 )
		return;
	if ( !pPlayer )
		MESSAGE_BEGIN( MSG_ALL, gmsgPlayersInArea );
	else
		MESSAGE_BEGIN( MSG_ONE, gmsgPlayersInArea, NULL, pPlayer->pev );
		WRITE_BYTE( m_pPoint->m_iIndex );
		WRITE_BYTE( team );
		WRITE_BYTE( num );
		WRITE_BYTE( req );
	MESSAGE_END();
}
// .text:0015afb0 CAreaCapture::ReSendSwipeInfo (file:triggers.cpp:5934)
void CAreaCapture::ReSendSwipeInfo()
{
	if ( !m_pPoint )
		return;
	if ( m_pPoint->pev->spawnflags & 1 )
		return;
	int denom = m_iCappingPlayers ? m_iCappingPlayers : 1;
	float captime = (float)m_nCapTime * (float)m_iCappingRequired / (float)denom;
	MESSAGE_BEGIN( MSG_ALL, gmsgProgBarUpdate );
		WRITE_BYTE( m_pPoint->m_iIndex );
		WRITE_COORD( captime );
	MESSAGE_END();
}
// .text:001511b0 CAreaCapture::BreakCapture.part (file:triggers.cpp:6126)
// .text:0015bfa0 CAreaCapture::BreakCapture (file:triggers.cpp:6127)
void CAreaCapture::BreakCapture( int team )
{
	if ( !m_bCapturing )
		return;
	m_bCapturing = 0;
	m_fTimeRemaining = 0;
	if ( m_iCapMode == 1 )
		FireTargets( team == 1 ? (char *)sz_AlliesBreak : (char *)sz_AxisBreak, NULL, this, USE_TOGGLE, 0 );
	else
		FireTargets( team == 1 ? (char *)sz_AlliesStart : (char *)sz_AxisStart, NULL, this, USE_TOGGLE, 0 );
	if ( team == 1 )
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "#game_capture_broken_allie" );
	else
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "#game_capture_broken_axis" );
	if ( m_pPoint && !( m_pPoint->pev->spawnflags & 1 ) )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgCancelProgressBar );
			WRITE_BYTE( m_pPoint->m_iIndex );
			WRITE_BYTE( team );
		MESSAGE_END();
	}
	UTIL_SendDirectorMessage( this, NULL, 12 );
}

// CObjectCapture area (triggers.cpp:3849/3827)
// .text:00150b80 CObjectCapture::area_SetIndex (file:triggers.cpp:3849)
void CObjectCapture::area_SetIndex( int index )
{
	m_iAreaIndex = index;
	if ( index > 0x80 )
		return;
	MESSAGE_BEGIN( MSG_ALL, gmsgClientAreas );
		WRITE_BYTE( m_iAreaIndex );
		WRITE_BYTE( -1 );
		WRITE_STRING( sz_HudIcon );
	MESSAGE_END();
}
// .text:00150c00 CObjectCapture::area_SendStatus (file:triggers.cpp:3827)
void CObjectCapture::area_SendStatus( CBasePlayer *p )
{
	if ( !p || m_iAreaIndex > 0x80 )
		return;
	MESSAGE_BEGIN( MSG_ONE, gmsgClientAreas, NULL, p->pev );
		WRITE_BYTE( m_iAreaIndex );
		WRITE_BYTE( -1 );
		WRITE_STRING( sz_HudIcon );
	MESSAGE_END();
}

// CDodRoundTimer (triggers.cpp:6412/6422/6440/6470/6489)
// .text:0015c0b0 CDodRoundTimer::StartTimer (file:triggers.cpp:6412)
void CDodRoundTimer::StartTimer()
{
	m_bTimer = 1;
	if ( !( pev->spawnflags & 1 ) )
		return;
	if ( !UTIL_IsMasterTriggered( m_sMaster, this ) )
		return;
	if ( !m_bTimer )
		return;
	MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
		WRITE_BYTE( m_bTimer );
		WRITE_COORD( m_fRoundTime );
	MESSAGE_END();
}
// .text:0015c160 CDodRoundTimer::StopTimer (file:triggers.cpp:6422)
void CDodRoundTimer::StopTimer()
{
	m_bTimer = 0;
	if ( !( pev->spawnflags & 1 ) )
		return;
	if ( !UTIL_IsMasterTriggered( m_sMaster, this ) )
		return;
	MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
		WRITE_BYTE( m_bTimer );
		WRITE_COORD( 0.0 );
	MESSAGE_END();
}
// .text:0015c1e0 CDodRoundTimer::ResetTimer (file:triggers.cpp:6440)
void CDodRoundTimer::ResetTimer()
{
	m_bTimer = 0;
	m_fUpdateTimer = 30.0;
	m_fRoundTime = m_fTimerLength;
	if ( !( pev->spawnflags & 1 ) )
		return;
	if ( !UTIL_IsMasterTriggered( m_sMaster, this ) )
		return;
	MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
		WRITE_BYTE( m_bTimer );
		WRITE_COORD( m_fRoundTime );
	MESSAGE_END();
}
// .text:0015c290 CDodRoundTimer::SendStatus (file:triggers.cpp:6470)
void CDodRoundTimer::SendStatus()
{
	if ( !( pev->spawnflags & 1 ) )
		return;
	if ( !UTIL_IsMasterTriggered( m_sMaster, this ) )
		return;
	if ( !m_bTimer )
		return;
	MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
		WRITE_BYTE( m_bTimer );
		WRITE_COORD( m_fRoundTime );
	MESSAGE_END();
}
// .text:0015c320 CDodRoundTimer::SendStatus (file:triggers.cpp:6489)
void CDodRoundTimer::SendStatus( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return;
	if ( !( pev->spawnflags & 1 ) )
		return;
	if ( !UTIL_IsMasterTriggered( m_sMaster, this ) )
		return;
	if ( !m_bTimer )
		return;
	MESSAGE_BEGIN( MSG_ONE, gmsgTimerStatus, NULL, pPlayer->pev );
		WRITE_BYTE( m_bTimer );
		WRITE_COORD( m_fRoundTime );
	MESSAGE_END();
}

// GetState (triggers.cpp:451/940)
// .text:0015d540 CMultiManager::GetState (file:triggers.cpp:451)
STATE CMultiManager::GetState() { return m_iState; }

// ---- CStateWatcher (ptype: SaveData[3], 0x248 alloc, multi_watcher/watcher) ----
// .text:0014fef0 CStateWatcher::Restore (file:triggers.cpp:969)
// .text:0015d550 CStateWatcher::GetState (file:triggers.cpp:940)
// .text:00156b50 CStateWatcher::EvalLogic (file:triggers.cpp:1072)
// .text:00156a70 multi_watcher (file:triggers.cpp:958)
// .text:00156ae0 watcher (file:triggers.cpp:959)
class CStateWatcher : public CBaseToggle
{
public:
	void Spawn( void );
	void Think( void );
	void KeyValue( KeyValueData *pkvd );
	virtual STATE GetState();
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	BOOL EvalLogic();
	virtual void RoundRespawn( void );
	static TYPEDESCRIPTION m_SaveData[];
	int m_fLogic;
	int m_cTargets;
	int m_iTargetName[16];
};
TYPEDESCRIPTION CStateWatcher::m_SaveData[] = {
	DEFINE_FIELD( CStateWatcher, m_fLogic, FIELD_INTEGER ),
	DEFINE_FIELD( CStateWatcher, m_cTargets, FIELD_INTEGER ),
	DEFINE_ARRAY( CStateWatcher, m_iTargetName, FIELD_STRING, 16 ),
};
IMPLEMENT_SAVERESTORE( CStateWatcher, CBaseToggle );

// .text:0014e8d0 CStateWatcher::Spawn (file:triggers.cpp:1016)
void CStateWatcher::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->nextthink = gpGlobals->time + 0.5;
}

// .text:0014e900 CStateWatcher::RoundRespawn (file:triggers.cpp:1026)
void CStateWatcher::RoundRespawn( void )
{
	pev->nextthink = gpGlobals->time + 0.5;
}

// .text:00151f70 CStateWatcher::KeyValue (file:triggers.cpp:972)
void CStateWatcher::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "m_fLogic" ) )
	{
		m_fLogic = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iszWatch" ) )
	{
		if ( m_cTargets < 16 )
		{
			m_iTargetName[m_cTargets] = ALLOC_STRING( pkvd->szValue );
			m_cTargets++;
			pkvd->fHandled = TRUE;
		}
	}
	else if ( m_cTargets < 16 )
	{
		char tmp[128];
		UTIL_StripToken( pkvd->szKeyName, tmp, sizeof( tmp ) );
		m_iTargetName[m_cTargets] = ALLOC_STRING( tmp );
		m_cTargets++;
		pkvd->fHandled = TRUE;
	}
}

// .text:00156d20 CStateWatcher::Think (file:triggers.cpp:1031)
// Re-evaluates EvalLogic each 0.1s, latches result into spawnflag 0x200 and,
// on a state change, fires targets (USE_ON/USE_OFF/USE_TOGGLE gated by
// spawnflags 1=fire, 2=no-on, 4=no-off).
void CStateWatcher::Think( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	BOOL oldflag = ( pev->spawnflags & 0x200 );
	BOOL on = EvalLogic();

	if ( on )
		pev->spawnflags |= 0x200;
	else
		pev->spawnflags &= ~0x200;

	if ( ( pev->spawnflags & 0x200 ) != oldflag )
	{
		if ( oldflag == 0 )
		{
			if ( pev->spawnflags & 2 )
				return;
			if ( !( pev->spawnflags & 1 ) )
			{
				SUB_UseTargets( this, USE_ON, 0 );
				return;
			}
		}
		else
		{
			if ( pev->spawnflags & 4 )
				return;
			if ( !( pev->spawnflags & 1 ) )
			{
				SUB_UseTargets( this, USE_OFF, 0 );
				return;
			}
		}
		SUB_UseTargets( this, USE_TOGGLE, 0 );
	}
}

STATE CStateWatcher::GetState() { return (STATE)( ( pev->spawnflags >> 8 ) & 2 ); }
BOOL CStateWatcher::EvalLogic()
{
	int got = 0;
	if ( m_cTargets < 1 )
	{
		if ( m_fLogic == 3 || m_fLogic == 0 )
			return TRUE;
		if ( m_fLogic == 4 )
			return got;
		if ( m_fLogic == 5 )
			return ( got == 0 );
		return FALSE;
	}
	for ( int i = 0; i < m_cTargets; i++ )
	{
		CBaseEntity *pEnt = UTIL_FindEntityByTargetname( NULL, STRING( m_iTargetName[i] ) );
		if ( !pEnt )
			continue;
		int st = ( (int (*)( CBaseEntity * ))( ( (void ***)pEnt )[0][15] ) )( pEnt );
		int flags = pev->spawnflags;
		if ( st == 0 )
		{
			if ( !( flags & 0x10 ) )
			{
				if ( m_fLogic == 0 )
					return FALSE;
				if ( m_fLogic == 2 )
					return TRUE;
				continue;
			}
		}
		else if ( st == 1 || st == 3 )
		{
			if ( !( flags & 0x20 ) )
			{
				if ( m_fLogic == 0 )
					return FALSE;
				if ( m_fLogic == 2 )
					return TRUE;
				continue;
			}
		}
		else if ( st == 2 )
		{
			if ( !( flags & 8 ) )
			{
				if ( m_fLogic == 0 )
					return FALSE;
				if ( m_fLogic == 2 )
					return TRUE;
				continue;
			}
		}
		else
		{
			if ( !( flags & 0x80 ) )
			{
				if ( m_fLogic == 0 )
					return FALSE;
				if ( m_fLogic == 2 )
					return TRUE;
				continue;
			}
		}
		if ( m_fLogic == 3 )
			return FALSE;
		if ( m_fLogic == 1 )
			return TRUE;
		if ( m_fLogic == 4 )
		{
			if ( got )
				return FALSE;
			got = 1;
			continue;
		}
		if ( m_fLogic == 5 )
		{
			if ( got )
				return TRUE;
			got = 1;
			continue;
		}
	}
	if ( m_fLogic == 3 || m_fLogic == 0 )
		return TRUE;
	if ( m_fLogic == 4 )
		return got;
	if ( m_fLogic == 5 )
		return ( got == 0 );
	return FALSE;
}
// .text:00156a70 multi_watcher (file:triggers.cpp:958)
// .text:00156ae0 watcher (file:triggers.cpp:959)
LINK_ENTITY_TO_CLASS( multi_watcher, CStateWatcher );
LINK_ENTITY_TO_CLASS( watcher, CStateWatcher );

// ---- CEnvCustomize (ptype SaveData[9], 0x17c alloc) ----
// .text:0014fe90 CEnvCustomize::Restore (file:triggers.cpp:1282)
// .text:00156e80 env_customize (file:triggers.cpp:1267)
class CEnvCustomize : public CBaseEntity
{
public:
	void Spawn( void );
	void Think( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void );
	void KeyValue( KeyValueData *pkvd );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
	float m_flRadius;
	int m_iszModel;
	int m_iClass;
	int m_iPrisoner;
	int m_iPlayerAlly;
	int m_iVisible;
	int m_iSolid;
	int m_iProvoked;
	int m_voicePitch;
};
TYPEDESCRIPTION CEnvCustomize::m_SaveData[] = {
	DEFINE_FIELD( CEnvCustomize, m_flRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvCustomize, m_iszModel, FIELD_STRING ),
	DEFINE_FIELD( CEnvCustomize, m_iClass, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvCustomize, m_iPrisoner, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvCustomize, m_iPlayerAlly, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvCustomize, m_iVisible, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvCustomize, m_iSolid, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvCustomize, m_iProvoked, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvCustomize, m_voicePitch, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CEnvCustomize, CBaseEntity );

// .text:0014fb40 CEnvCustomize::Spawn (file:triggers.cpp:1336)
void CEnvCustomize::Spawn( void )
{
	pev->solid = SOLID_NOT;
	if ( m_iszModel )
		PRECACHE_MODEL( (char *) STRING( m_iszModel ) );
	if ( pev->targetname == 0 )
		pev->nextthink = gpGlobals->time + 0.1;
}

// .text:0015d570 CEnvCustomize::ObjectCaps (file:triggers.cpp:1248)
int CEnvCustomize::ObjectCaps( void )
{
	return 0;
}

// .text:00156e80 env_customize (file:triggers.cpp:1267)
// Factory: GetClassPtr sets the vtable + pev (size 0x17c).
LINK_ENTITY_TO_CLASS( env_customize, CEnvCustomize );

// ---- CTriggerSetPatrol (ptype SaveData[1], 0x164 alloc) ----
// .text:0014fdd0 CTriggerSetPatrol::Restore (file:triggers.cpp:3209)
// .text:00158c30 trigger_startpatrol (file:triggers.cpp:3202)
class CTriggerSetPatrol : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
	int m_iszPath;
};
TYPEDESCRIPTION CTriggerSetPatrol::m_SaveData[] = {
	DEFINE_FIELD( CTriggerSetPatrol, m_iszPath, FIELD_STRING ),
};
IMPLEMENT_SAVERESTORE( CTriggerSetPatrol, CBaseDelay );

// .text:0014edc0 CTriggerSetPatrol::Spawn (file:triggers.cpp:3224)
void CTriggerSetPatrol::Spawn( void ) {}

// .text:0015d5a0 CTriggerSetPatrol::ObjectCaps (file:triggers.cpp:3193)
int CTriggerSetPatrol::ObjectCaps( void )
{
	return 0;
}

// .text:00151e70 CTriggerSetPatrol::KeyValue (file:triggers.cpp:3212)
void CTriggerSetPatrol::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "m_iszPath" ) )
	{
		m_iszPath = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

// .text:00158c30 trigger_startpatrol (file:triggers.cpp:3202)
// Factory: GetClassPtr sets the vtable + pev (size 0x164).
LINK_ENTITY_TO_CLASS( trigger_startpatrol, CTriggerSetPatrol );

// .text:001434a0 CEnvCustomize::Use (file:triggers.cpp:1298)
void CEnvCustomize::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = UTIL_FindEntityGeneric( STRING( pev->target ), pev->origin, m_flRadius );
	if ( !pTarget )
		return;

	CBaseMonster *pMonster = pTarget->MyMonsterPointer();

	if ( !( pev->spawnflags & 1 ) && pMonster && pMonster->m_MonsterState == 8 )
		return;

	if ( m_iszModel )
	{
		Vector vecMins = pTarget->pev->mins;
		Vector vecMaxs = pTarget->pev->maxs;
		( *g_engfuncs.pfnSetModel )( ENT( pTarget->pev ), STRING( m_iszModel ) );
		UTIL_SetSize( pTarget->pev, vecMins, vecMaxs );
	}

	if ( pev->body != -1 )
		pTarget->pev->body = pev->body;

	if ( pev->skin == -2 )
		pTarget->pev->skin = ( pTarget->pev->skin == 0 );
	else if ( pev->skin != -1 && pev->skin != -99 )
		pTarget->pev->skin = pev->skin;
	else if ( pev->skin == -99 )
		pTarget->pev->skin = -1;

	if ( m_iVisible != 0 )
	{
		unsigned int u = pTarget->pev->effects;
		if ( !( u & 0x80 ) )
		{
			if ( m_iVisible != 2 )
				pTarget->pev->effects = u | 0x80;
		}
		else
		{
			if ( m_iVisible - 2 > 1 )
				pTarget->pev->effects = u | 0x80;
			else
				pTarget->pev->effects = u & ~0x80;
		}
	}

	if ( m_iSolid != 0 )
	{
		if ( pTarget->pev->solid != 0 )
		{
			if ( m_iSolid != 2 )
				pTarget->pev->solid = 0;
		}
		else
		{
			if ( m_iSolid - 2 > 1 )
				pTarget->pev->solid = 0;
			else
				pTarget->pev->solid = pMonster ? 3 : 4;
		}
	}

	if ( pMonster )
	{
		if ( m_iClass != 0 )
		{
			pMonster->m_iClass = m_iClass;
			if ( pMonster->m_hEnemy != NULL )
			{
				pMonster->m_hEnemy = NULL;
				pMonster->SetConditions( 0x10000 );
			}
		}

		if ( m_iPrisoner != 0 )
		{
			unsigned int u = pMonster->pev->spawnflags;
			if ( u & 0x10 )
			{
				if ( (unsigned)( m_iPrisoner - 2 ) <= 1 )
					pMonster->pev->spawnflags = u & ~0x10;
			}
			else if ( m_iPrisoner != 2 )
			{
				pMonster->pev->spawnflags = u | 0x10;
				if ( pMonster->m_hEnemy != NULL )
				{
					pMonster->m_hEnemy = NULL;
					pMonster->SetConditions( 0x10000 );
				}
			}
		}

		if ( m_iPlayerAlly != 0 )
		{
			unsigned int u = pMonster->pev->spawnflags;
			if ( u & 0x800 )
			{
				if ( (unsigned)( m_iPlayerAlly - 2 ) <= 1 )
					pMonster->pev->spawnflags = u & ~0x800;
			}
			else if ( m_iPlayerAlly != 2 )
			{
				pMonster->pev->spawnflags = u | 0x800;
			}
		}

		if ( m_iProvoked != 0 )
		{
			unsigned int u = pMonster->m_afMemory;
			if ( !( u & 1 ) )
			{
				if ( m_iProvoked != 2 )
					pMonster->m_afMemory = u | 1;
			}
			else if ( (unsigned)( m_iProvoked - 2 ) < 2 )
				pMonster->m_afMemory = u & ~1;
			else
				pMonster->m_afMemory = u | 1;
		}
	}

	if ( pev->spawnflags & 2 )
		UTIL_Remove( this );
}

// .text:0013e940 CEnvCustomize::Think (file:triggers.cpp)
void CEnvCustomize::Think( void )
{
	Use( this, this, USE_TOGGLE, 0 );
}

// .text:00142ef0 CEnvCustomize::KeyValue (file:triggers.cpp)
void CEnvCustomize::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "m_iVisible" ) )
	{
		m_iVisible = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iSolid" ) )
	{
		m_iSolid = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iszModel" ) )
	{
		m_iszModel = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_voicePitch" ) )
	{
		m_voicePitch = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iPlayerAlly" ) )
	{
		m_iPlayerAlly = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iPrisoner" ) )
	{
		m_iPrisoner = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iClass" ) )
	{
		m_iClass = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_flRadius" ) )
	{
		m_flRadius = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iProvoked" ) )
	{
		m_iProvoked = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

// .text:0014f570 CTriggerSetPatrol::Use (file:triggers.cpp:3218)
void CTriggerSetPatrol::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseMonster *pMonster = (CBaseMonster *)UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
	CBaseEntity *path = UTIL_FindEntityByTargetname( NULL, STRING( m_iszPath ) );

	if ( path && pMonster && !strncmp( STRING( pMonster->pev->classname ), "monster_", 8 ) )
		pMonster->StartPatrol( path );
}

// .text:0014fb00 CAutoTrigger::RoundRespawn (file:triggers.cpp)
void CAutoTrigger::RoundRespawn( void )
{
	m_iEnabled = 1;
	if ( pev->spawnflags & 0x200 )
	{
		pev->nextthink = gpGlobals->time + 0.1f;
		b_isRoundRestart = true;
	}
}

// .text:00157420 CTriggerPush::RoundRespawn (file:triggers.cpp)
void CTriggerPush::RoundRespawn( void )
{
	InitTrigger();
	SetUse( &CBaseTrigger::ToggleUse );
	m_iEnabled = 1;
}

// .text:0013e680 CMultiManager::RoundRespawn (file:triggers.cpp)
void CMultiManager::RoundRespawn( void )
{
	int i;
	int count = m_rscTargets;

	m_index = m_rsindex;
	m_cTargets = count;
	m_startTime = m_rsstartTime;
	m_flWait = m_rsflWait;
	m_flMaxWait = m_rsflMaxWait;
	m_sMaster = m_rssMaster;
	m_iMode = m_rsiMode;

	for ( i = 0; i < count; i++ )
	{
		m_iTargetName[i] = m_rsiTargetName[i];
		m_flTargetDelay[i] = m_rsflTargetDelay[i];
	}

	m_iState = STATE_OFF;
	SetUse( &CMultiManager::ManagerUse );
	SetThink( &CMultiManager::ManagerThink );
	if ( !( pev->spawnflags & 0x20000000 ) )
		m_triggerType = USE_TOGGLE;

	bool swapped;
	do
	{
		swapped = false;
		for ( i = 1; i < count; i++ )
		{
			if ( m_flTargetDelay[i] < m_flTargetDelay[i - 1] )
			{
				int name = m_iTargetName[i];
				m_iTargetName[i] = m_iTargetName[i - 1];
				m_iTargetName[i - 1] = name;
				float d = m_flTargetDelay[i];
				m_flTargetDelay[i] = m_flTargetDelay[i - 1];
				m_flTargetDelay[i - 1] = d;
				swapped = true;
			}
		}
	} while ( swapped );
}

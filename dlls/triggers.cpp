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
#include "talkmonster.h"

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

extern DLL_GLOBAL BOOL g_fGameOver;
extern int gmsgCameraView;
extern int gmsgStartProgressBar;
extern int gmsgStartProgressBarF;
extern int gmsgSetObjOwner;
extern int gmsgInitObjDisplay;
extern int gmsgTimeLeft;
extern cvar_t *pcvar_mp_clan_match;
extern cvar_t *pcvar_mp_clan_scoring;
extern cvar_t *pcvar_mp_clan_scoring_delay;
extern int get_maptime_left( void );

extern void SetMovedir( entvars_t *pev );
extern Vector VecBModelOrigin( entvars_t *pevBModel );

extern const char *GetStringForUseType( USE_TYPE useType );

static BOOL SceneAlreadySeen( CBasePlayer *pPlayer, int slot );
static void MarkSceneSeen( CBasePlayer *pPlayer, int slot );
static const char *CutSceneName( int scene );
extern int gmsgClientAreas;
extern int gmsgTimerStatus;
extern int gmsgPlayersInArea;
extern int gmsgProgBarUpdate;
extern int gmsgCancelProgressBar;

#define SF_TRIGGER_PUSH_START_OFF 2			//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_TARGETONCE 1		// Only fire hurt target once
#define SF_TRIGGER_HURT_START_OFF 2			//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_NO_CLIENTS 8		//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_CLIENTONLYFIRE 16	// trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32	// only clients may touch this trigger.
#define SF_TRIGGER_HURT_NO_ALLIES 64		// the Allies are not hurt
#define SF_TRIGGER_HURT_NO_AXIS 128		// the Axis are not hurt

#define SF_LADDER_SILENT 0x0001		// the climbing makes no step sounds, pm_shared reads it from iuser1
#define SF_DODCAMERA_NO_DAMPING 0x0004	// the camera does not slow down while it turns
#define SF_WATCHER_TOGGLE 0x0001	// the watcher fires USE_TOGGLE for both changes
#define SF_WATCHER_NO_ON 0x0002		// no targets when the logic turns on
#define SF_WATCHER_NO_OFF 0x0004	// no targets when the logic turns off
#define SF_WATCHER_STATE 0x0200		// set by the watcher itself: the last result of the logic
#define SF_CUSTOMIZE_DEAD 0x0001	// env_customize changes a dead monster too
#define SF_CUSTOMIZE_ONCE 0x0002	// env_customize removes itself after it was used

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
#define SF_AUTO_ROUNDRESTART 0x0200

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
	int m_iEnabled;
	int m_globalstate;
	USE_TYPE triggerType;
	bool b_isRoundRestart;
	void RoundRespawn( void );
};
LINK_ENTITY_TO_CLASS( trigger_auto, CAutoTrigger );

TYPEDESCRIPTION CAutoTrigger::m_SaveData[] = {
	DEFINE_FIELD( CAutoTrigger, m_globalstate, FIELD_STRING ),
	DEFINE_FIELD( CAutoTrigger, triggerType, FIELD_INTEGER ),
};

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
	m_iEnabled = 1;
	b_isRoundRestart = false;

	Precache();

	RoundStore();
}

void CAutoTrigger::Precache( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
}

void CAutoTrigger::Think( void )
{
	if ( m_iEnabled &&
		 ( !( pev->spawnflags & SF_AUTO_ROUNDRESTART ) || g_pGameRules->IsMultiplayer() != TRUE || b_isRoundRestart ) &&
		 ( !m_globalstate || gGlobalState.EntityGetState( m_globalstate ) == GLOBAL_ON ) )
	{
		SUB_UseTargets( this, triggerType, 0 );

		if ( pev->spawnflags & SF_AUTO_FIREONCE )
			m_iEnabled = 0;
	}
}

void CAutoTrigger::RoundRespawn( void )
{
	m_iEnabled = 1;
	if ( pev->spawnflags & SF_AUTO_ROUNDRESTART )
	{
		pev->nextthink = gpGlobals->time + 0.1f;
		b_isRoundRestart = true;
	}
}

#define SF_RELAY_FIREONCE 0x0001
#define SF_RELAY_DEBUG 0x0002
#define SF_RELAY_USETYPE 0x80000000

class CTriggerRelay : public CBaseDelay
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void RoundRespawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

  private:
	int m_iEnabled;
	USE_TYPE m_triggerType;
	string_t m_sMaster;
};

void CTriggerRelay::RoundRespawn( void )
{
	m_iEnabled = 1;
}

LINK_ENTITY_TO_CLASS( trigger_relay, CTriggerRelay );

TYPEDESCRIPTION CTriggerRelay::m_SaveData[] = {
	DEFINE_FIELD( CTriggerRelay, m_triggerType, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerRelay, m_sMaster, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CTriggerRelay, CBaseDelay );

void CTriggerRelay::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "triggerstate" ) )
	{
		int type = atoi( pkvd->szValue );
		switch ( type )
		{
		case 0:
			m_triggerType = USE_OFF;
			break;
		case 2:
			m_triggerType = USE_TOGGLE;
			break;
		case 4:
			m_triggerType = USE_KILL;
			break;
		case 5:
			pev->spawnflags |= SF_RELAY_USETYPE;
			break;
		default:
			m_triggerType = USE_ON;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerRelay::Spawn( void )
{
	m_iEnabled = 1;

	RoundStore();
}

void CTriggerRelay::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !m_iEnabled )
		return;

	if ( !UTIL_IsMasterTriggered( m_sMaster, pActivator ) )
	{
		if ( pev->spawnflags & SF_RELAY_DEBUG )
		{
			if ( pev->targetname )
				ALERT( at_console, "DEBUG: trigger_relay %s wasn't activated: locked by master %s\n", STRING( pev->targetname ),
					   STRING( m_sMaster ) );
			else
				ALERT( at_console, "DEBUG: unnamed trigger_relay wasn't activated: locked by master %s\n", STRING( m_sMaster ) );
		}

		return;
	}

	if ( pev->spawnflags & SF_RELAY_DEBUG )
	{
		if ( pev->targetname )
			ALERT( at_console, "DEBUG: trigger_relay %s was activated: ", STRING( pev->targetname ) );
		else
			ALERT( at_console, "DEBUG: unnamed trigger_relay was activated: " );
	}

	if ( pev->spawnflags & SF_RELAY_USETYPE )
	{
		// pass on the way the relay was used
		if ( pev->spawnflags & SF_RELAY_DEBUG )
		{
			if ( m_flDelay == 0 )
				ALERT( at_console, "sending %s now.\n", GetStringForUseType( useType ) );
			else
				ALERT( at_console, "will send %s in %f seconds.\n", GetStringForUseType( useType ), m_flDelay );
		}

		SUB_UseTargets( this, useType, value );
	}
	else
	{
		if ( pev->spawnflags & SF_RELAY_DEBUG )
		{
			if ( m_flDelay == 0 )
				ALERT( at_console, "sending %s now.\n", GetStringForUseType( m_triggerType ) );
			else
				ALERT( at_console, "will send %s in %f seconds.\n", GetStringForUseType( m_triggerType ), m_flDelay );
		}

		SUB_UseTargets( this, m_triggerType, 0 );
	}

	if ( pev->spawnflags & SF_RELAY_FIREONCE )
		m_iEnabled = 0;
}

//**********************************************************
// The Multimanager Entity - when fired, will fire up to 16 targets
// at specified times.
// FLAG:		THREAD (create clones when triggered)
// FLAG:		CLONE (this is a clone for a threaded execution)

#define SF_MULTIMAN_CLONE 0x80000000
#define SF_MULTIMAN_THREAD 0x00000001
#define SF_MULTIMAN_LOOP 0x00000002
#define SF_MULTIMAN_ONLYONCE 0x00000004
#define SF_MULTIMAN_TRIGGERTYPE 0x20000000
#define SF_MULTIMAN_USETYPE 0x40000000

class CMultiManager : public CBaseEntity
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void EXPORT ManagerThink( void );
	void EXPORT ManagerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual BOOL HasTarget( string_t targetname );

	int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	virtual STATE GetState();

	STATE m_iState;
	int m_cTargets;							   // the total number of targets in this manager's fire list.
	int m_index;							   // Current target
	float m_startTime;						   // Time we started firing
	int m_iTargetName[MAX_MULTI_TARGETS];	   // list if indexes into global string array
	float m_flTargetDelay[MAX_MULTI_TARGETS];  // delay (in seconds) from time of manager fire to target fire
	float m_flWait;
	float m_flMaxWait;
	string_t m_sMaster;
	int m_iMode;
	int m_rscTargets;
	int m_rsindex;
	float m_rsstartTime;
	int m_rsiTargetName[MAX_MULTI_TARGETS];
	float m_rsflTargetDelay[MAX_MULTI_TARGETS];
	float m_rsflWait;
	float m_rsflMaxWait;
	string_t m_rssMaster;
	int m_rsiMode;
	EHANDLE m_hActivator;
	USE_TYPE m_triggerType;

  private:
	inline BOOL IsClone( void ) { return ( pev->spawnflags & SF_MULTIMAN_CLONE ) ? TRUE : FALSE; }
	inline BOOL ShouldClone( void )
	{
		if ( IsClone() )
			return FALSE;

		return ( pev->spawnflags & SF_MULTIMAN_THREAD ) ? TRUE : FALSE;
	}

	CMultiManager *Clone( void );

	void RoundRespawn( void );
};

// GetState (triggers.cpp:451/940)
STATE CMultiManager::GetState() { return m_iState; }

LINK_ENTITY_TO_CLASS( multi_manager, CMultiManager );

// Global Savedata for multi_manager
TYPEDESCRIPTION CMultiManager::m_SaveData[] = {
	DEFINE_FIELD( CMultiManager, m_cTargets, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_index, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_iMode, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_startTime, FIELD_TIME ),
	DEFINE_FIELD( CMultiManager, m_triggerType, FIELD_INTEGER ),
	DEFINE_ARRAY( CMultiManager, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS ),
	DEFINE_ARRAY( CMultiManager, m_flTargetDelay, FIELD_FLOAT, MAX_MULTI_TARGETS ),
	DEFINE_FIELD( CMultiManager, m_sMaster, FIELD_STRING ),
	DEFINE_FIELD( CMultiManager, m_hActivator, FIELD_EHANDLE ),
	DEFINE_FIELD( CMultiManager, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CMultiManager, m_flMaxWait, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CMultiManager, CBaseEntity );

void CMultiManager ::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "maxwait" ) )
	{
		m_flMaxWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "mode" ) )
	{
		m_iMode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "triggerstate" ) )
	{
		int type = atoi( pkvd->szValue );

		switch ( type )
		{
		case 1:
			m_triggerType = USE_ON;
			break;
		case 2:
			m_triggerType = USE_OFF;
			break;
		case 3:
			m_triggerType = USE_KILL;
			break;
		case 4:
			pev->spawnflags |= SF_MULTIMAN_USETYPE;
			break;
		default:
			m_triggerType = USE_TOGGLE;
			break;
		}

		pev->spawnflags |= SF_MULTIMAN_TRIGGERTYPE;
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
		else
		{
			ALERT( at_error, "Too many targets on multi_manager %s\n", STRING( pev->targetname ) );
		}
	}
}

void CMultiManager ::Spawn( void )
{
	pev->solid = SOLID_NOT;
	m_iState = STATE_OFF;
	SetUse( &CMultiManager::ManagerUse );
	SetThink( &CMultiManager::ManagerThink );

	if ( !( pev->spawnflags & SF_MULTIMAN_TRIGGERTYPE ) )
		m_triggerType = USE_TOGGLE;

	// the state to go back to when the round restarts
	m_rsindex = m_index;
	m_rscTargets = m_cTargets;
	m_rsstartTime = m_startTime;
	m_rsflWait = m_flWait;
	m_rsflMaxWait = m_flMaxWait;
	m_rssMaster = m_sMaster;
	m_rsiMode = m_iMode;

	for ( int i = 0; i < MAX_MULTI_TARGETS; i++ )
	{
		m_rsiTargetName[i] = m_iTargetName[i];
		m_rsflTargetDelay[i] = m_flTargetDelay[i];
	}

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

	RoundStore();
}

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
	if ( !( pev->spawnflags & SF_MULTIMAN_TRIGGERTYPE ) )
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
	int i;

	if ( m_iMode == 0 )
	{
		// fire the targets one after the other at their delays
		time = gpGlobals->time - m_startTime;

		while ( m_index < m_cTargets )
		{
			if ( time < m_flTargetDelay[m_index] )
			{
				m_iState = STATE_ON;
				pev->nextthink = m_startTime + m_flTargetDelay[m_index];
				return;
			}

			FireTargets( STRING( m_iTargetName[m_index] ), m_hActivator, this, m_triggerType, 0 );
			m_index++;
		}

		if ( pev->spawnflags & SF_MULTIMAN_LOOP )
		{
			// all targets fired, start over
			m_index = 0;

			if ( m_flMaxWait == 0 )
			{
				if ( m_flWait == 0 )
				{
					m_startTime = gpGlobals->time;
					m_iState = STATE_ON;
				}
				else
				{
					m_startTime = m_flWait + gpGlobals->time;
					m_iState = STATE_TURN_ON;
				}
			}
			else
			{
				m_iState = STATE_TURN_ON;
				m_startTime = RANDOM_FLOAT( m_flWait, m_flMaxWait ) + gpGlobals->time;
			}

			pev->nextthink = m_startTime;
			return;
		}
	}
	else if ( m_iMode == 1 )
	{
		// fire one of the targets, the delays are the chances
		float flTotal = 0;

		for ( i = 0; i < m_cTargets; i++ )
			flTotal += m_flTargetDelay[i];

		if ( m_cTargets <= 0 || flTotal == 0 )
		{
			FireTargets( STRING( m_iTargetName[RANDOM_LONG( 0, m_cTargets - 1 )] ), m_hActivator, this, m_triggerType, 0 );
		}
		else
		{
			float flRand = RANDOM_FLOAT( 0, flTotal );
			float flSum = 0;

			for ( i = 0; i < m_cTargets; i++ )
			{
				flSum += m_flTargetDelay[i];

				if ( flSum >= flRand )
				{
					FireTargets( STRING( m_iTargetName[i] ), m_hActivator, this, m_triggerType, 0 );
					break;
				}
			}
		}
	}
	else
	{
		// every target is fired with its own chance in percent
		for ( i = 0; i < m_cTargets; i++ )
		{
			if ( m_flTargetDelay[i] >= RANDOM_LONG( 0, 100 ) )
				FireTargets( STRING( m_iTargetName[i] ), m_hActivator, this, m_triggerType, 0 );
		}
	}

	m_iState = STATE_OFF;
	SetThink( NULL );

	if ( !IsClone() && !( pev->spawnflags & SF_MULTIMAN_ONLYONCE ) )
	{
		SetUse( &CMultiManager::ManagerUse );  // allow manager re-use
		return;
	}

	UTIL_Remove( this );
}

CMultiManager *CMultiManager::Clone( void )
{
	CMultiManager *pMulti = GetClassPtr( (CMultiManager *) NULL );

	edict_t *pEdict = pMulti->pev->pContainingEntity;
	memcpy( pMulti->pev, pev, sizeof( *pev ) );
	pMulti->pev->pContainingEntity = pEdict;

	pMulti->pev->spawnflags |= SF_MULTIMAN_CLONE;
	pMulti->m_cTargets = m_cTargets;
	pMulti->m_triggerType = m_triggerType;
	pMulti->m_iMode = m_iMode;
	memcpy( pMulti->m_iTargetName, m_iTargetName, sizeof( m_iTargetName ) );
	memcpy( pMulti->m_flTargetDelay, m_flTargetDelay, sizeof( m_flTargetDelay ) );

	return pMulti;
}

// The USE function builds the time table and starts the entity thinking.
void CMultiManager ::ManagerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CMultiManager *pManager = this;

	while ( 1 )
	{
		BOOL bLoop = ( pManager->pev->spawnflags & SF_MULTIMAN_LOOP ) ? TRUE : FALSE;

		// a looping manager is stopped by the use
		if ( useType == USE_OFF ? bLoop : ( bLoop && pManager->m_iState != STATE_OFF ) )
		{
			if ( useType == USE_OFF ? pManager->m_iState == STATE_OFF : useType == USE_ON )
				return;

			pManager->m_iState = STATE_OFF;
			pManager->SetThink( NULL );
			return;
		}

		if ( !UTIL_IsMasterTriggered( pManager->m_sMaster, pActivator ) )
			return;

		// In multiplayer games, clone the MM and execute in the clone (like a thread)
		// to allow multiple players to trigger the same multimanager
		if ( !pManager->ShouldClone() )
			break;

		pManager = pManager->Clone();
	}

	pManager->m_hActivator = pActivator;
	pManager->m_index = 0;

	if ( pManager->m_flMaxWait == 0 )
	{
		if ( pManager->m_flWait == 0 )
		{
			pManager->m_startTime = gpGlobals->time;
			pManager->m_iState = STATE_ON;
		}
		else
		{
			pManager->m_startTime = pManager->m_flWait + gpGlobals->time;
			pManager->m_iState = STATE_TURN_ON;
		}
	}
	else
	{
		pManager->m_iState = STATE_TURN_ON;
		pManager->m_startTime = RANDOM_FLOAT( pManager->m_flWait, pManager->m_flMaxWait ) + gpGlobals->time;
	}

	// a negative delay of the first target starts the manager earlier
	if ( pManager->m_cTargets > 0 && pManager->m_flTargetDelay[0] < 0 )
		pManager->m_startTime = pManager->m_startTime + pManager->m_flTargetDelay[0];

	if ( pManager->pev->spawnflags & SF_MULTIMAN_USETYPE )
		pManager->m_triggerType = useType;

	if ( !( pManager->pev->spawnflags & SF_MULTIMAN_LOOP ) )
		pManager->SetUse( NULL );  // disable use until all targets have fired

	pManager->SetThink( &CMultiManager::ManagerThink );
	pManager->pev->nextthink = pManager->m_startTime;
}

// ---- CStateWatcher (ptype: SaveData[3], 0x248 alloc, multi_watcher/watcher) ----
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

STATE CStateWatcher::GetState() { return (STATE)( ( pev->spawnflags >> 8 ) & 2 ); }
LINK_ENTITY_TO_CLASS( multi_watcher, CStateWatcher );
LINK_ENTITY_TO_CLASS( watcher, CStateWatcher );

void CStateWatcher::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "m_fLogic" ) )
	{
		m_fLogic = (int) atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
		return;
	}

	if ( FStrEq( pkvd->szKeyName, "m_iszWatch" ) )
	{
		if ( m_cTargets < MAX_MULTI_TARGETS )
		{
			m_iTargetName[m_cTargets] = ALLOC_STRING( pkvd->szValue );
			m_cTargets++;
			pkvd->fHandled = TRUE;
			return;
		}
	}
	else if ( m_cTargets < MAX_MULTI_TARGETS )
	{
		char tmp[128];

		UTIL_StripToken( pkvd->szKeyName, tmp, sizeof( tmp ) );
		m_iTargetName[m_cTargets] = ALLOC_STRING( tmp );
		m_cTargets++;
		pkvd->fHandled = TRUE;
		return;
	}

	if ( pev->targetname )
		ALERT( at_console, "%s: Too many targets for %s %s (limit is %d)\n", pkvd->szKeyName, STRING( pev->classname ),
			   STRING( pev->targetname ), MAX_MULTI_TARGETS );
	else
		ALERT( at_console, "%s: Too many targets for unnamed %s (limit is %d)\n", pkvd->szKeyName,
			   STRING( pev->classname ), MAX_MULTI_TARGETS );
}

TYPEDESCRIPTION CStateWatcher::m_SaveData[] = {
	DEFINE_FIELD( CStateWatcher, m_fLogic, FIELD_INTEGER ),
	DEFINE_FIELD( CStateWatcher, m_cTargets, FIELD_INTEGER ),
	DEFINE_ARRAY( CStateWatcher, m_iTargetName, FIELD_STRING, 16 ),
};
IMPLEMENT_SAVERESTORE( CStateWatcher, CBaseToggle );

void CStateWatcher::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->nextthink = gpGlobals->time + 0.5;

	RoundStore();
}

void CStateWatcher::RoundRespawn( void )
{
	pev->nextthink = gpGlobals->time + 0.5;
}

// Re-evaluates EvalLogic each 0.1s, latches result into spawnflag 0x200 and,
// on a state change, fires targets (USE_ON/USE_OFF/USE_TOGGLE gated by
// spawnflags 1=fire, 2=no-on, 4=no-off).
void CStateWatcher::Think( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	BOOL oldflag = ( pev->spawnflags & SF_WATCHER_STATE );
	BOOL on = EvalLogic();

	if ( on )
		pev->spawnflags |= SF_WATCHER_STATE;
	else
		pev->spawnflags &= ~SF_WATCHER_STATE;

	if ( ( pev->spawnflags & SF_WATCHER_STATE ) != oldflag )
	{
		if ( oldflag == 0 )
		{
			if ( pev->spawnflags & SF_WATCHER_NO_ON )
				return;
			if ( !( pev->spawnflags & SF_WATCHER_TOGGLE ) )
			{
				SUB_UseTargets( this, USE_ON, 0 );
				return;
			}
		}
		else
		{
			if ( pev->spawnflags & SF_WATCHER_NO_OFF )
				return;
			if ( !( pev->spawnflags & SF_WATCHER_TOGGLE ) )
			{
				SUB_UseTargets( this, USE_OFF, 0 );
				return;
			}
		}
		SUB_UseTargets( this, USE_TOGGLE, 0 );
	}
}

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

	RoundStore();
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

// ---- CEnvCustomize (ptype SaveData[9], 0x17c alloc) ----
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

int CEnvCustomize::ObjectCaps( void )
{
	return 0;
}

// Factory: GetClassPtr sets the vtable + pev (size 0x17c).
LINK_ENTITY_TO_CLASS( env_customize, CEnvCustomize );

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

void CEnvCustomize::Spawn( void )
{
	pev->solid = SOLID_NOT;
	if ( m_iszModel )
		PRECACHE_MODEL( (char *) STRING( m_iszModel ) );
	if ( pev->targetname == 0 )
		pev->nextthink = gpGlobals->time + 0.1;
}

void CEnvCustomize::Think( void )
{
	Use( this, this, USE_TOGGLE, 0 );
}

void CEnvCustomize::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = UTIL_FindEntityGeneric( STRING( pev->target ), pev->origin, m_flRadius );
	if ( !pTarget )
		return;

	CBaseMonster *pMonster = pTarget->MyMonsterPointer();

	if ( !( pev->spawnflags & SF_CUSTOMIZE_DEAD ) && pMonster && pMonster->m_MonsterState == MONSTERSTATE_DEAD )
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
		if ( pTarget->pev->solid != SOLID_NOT )
		{
			if ( m_iSolid != 2 )
				pTarget->pev->solid = SOLID_NOT;
		}
		else
		{
			if ( m_iSolid - 2 > 1 )
				pTarget->pev->solid = SOLID_NOT;
			else
				pTarget->pev->solid = pMonster ? 3 : 4;
		}
	}

	if ( pMonster )
	{
		if ( m_voicePitch > 0 )
		{
			const char *pszClass = STRING( pTarget->pev->classname );
			if ( FStrEq( pszClass, "monster_barney" ) || FStrEq( pszClass, "monster_scientist" ) ||
				 FStrEq( pszClass, "monster_sitting_scientist" ) )
				( (CTalkMonster *) pTarget )->m_voicePitch = m_voicePitch;
		}

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

	if ( pev->spawnflags & SF_CUSTOMIZE_ONCE )
		UTIL_Remove( this );
}

class CBaseTrigger : public CBaseToggle
{
  public:
	void EXPORT TeleportTouch( CBaseEntity *pOther );
	virtual void KeyValue( KeyValueData *pkvd );
	void EXPORT MultiTouch( CBaseEntity *pOther );
	void EXPORT HurtTouch( CBaseEntity *pOther );
	void EXPORT CDAudioTouch( CBaseEntity *pOther );
	void ActivateMultiTrigger( CBaseEntity *pActivator );
	void EXPORT MultiWaitOver( void );
	void EXPORT CounterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void InitTrigger( void );

	virtual int ObjectCaps( void );
};

int CBaseTrigger::ObjectCaps( void )
{
	return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
}

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
	else if ( FStrEq( pkvd->szKeyName, "teamonly" ) )
	{
		m_team = atoi( pkvd->szValue );
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

	RoundStore();
}

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

	if ( !UTIL_IsMasterTriggered( m_sMaster, this ) || !pOther->pev->takedamage )
		return;

	if ( ( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH ) && !pOther->IsPlayer() )
	{
		// this trigger is only allowed to touch clients, and this ain't a client.
		return;
	}

	if ( ( pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS ) && pOther->IsPlayer() )
		return;

	// the team the trigger is closed for isn't hurt
	if ( pOther->pev->team == TEAM_ALLIES )
	{
		if ( pev->spawnflags & SF_TRIGGER_HURT_NO_ALLIES )
			return;
	}
	else if ( pOther->pev->team == TEAM_AXIS && ( pev->spawnflags & SF_TRIGGER_HURT_NO_AXIS ) )
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
		pOther->TakeHealth( -fldmg, m_bitsDamageInflict );
	else
		pOther->TakeDamage( pev, pev, fldmg, m_bitsDamageInflict );

	// Store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;

	// Apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5;  // half second delay until this trigger can hurt toucher again

	// trigger has a target it wants to fire, if the toucher is still alive
	// (and a client, when the trigger is flagged to fire only for clients).
	if ( pev->target && pOther->pev->health > 0 &&
		 ( !( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE ) || pOther->IsPlayer() ) )
	{
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

	SetTouch( &CTriggerMultiple::MultiTouch );

	RoundStore();
}

void CTriggerMultiple ::RoundRespawn( void )
{
	if ( m_flWait == 0 )
		m_flWait = 0.2;

	InitTrigger();
	SetTouch( &CTriggerMultiple::MultiTouch );

	if ( pev->spawnflags & SF_TRIGGER_ROUND )  // DoD: re-arm countdown triggers on round restart
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

void CTriggerOnce::RoundRespawn( void )
{
	if ( pev->spawnflags & SF_TRIGGER_ROUND )  // DoD: re-arm once-triggers on round restart
		SetTouch( &CBaseTrigger::MultiTouch );
}

void CBaseTrigger ::MultiTouch( CBaseEntity *pOther )
{
	entvars_t *pevToucher;

	pevToucher = pOther->pev;

	// the team the trigger is closed for can't fire it
	if ( FBitSet( pev->spawnflags, SF_TRIGGER_NO_ALLIES ) && pOther->IsPlayer() && pOther->pev->team == TEAM_ALLIES )
		return;

	if ( FBitSet( pev->spawnflags, SF_TRIGGER_NO_AXIS ) && pOther->IsPlayer() && pOther->pev->team == TEAM_AXIS )
		return;

	// teamonly: 1 for the Allies, 2 for the Axis
	if ( m_team == 1 )
	{
		if ( pevToucher->team == TEAM_AXIS )
			return;
	}
	else if ( m_team == 2 && pevToucher->team == TEAM_ALLIES )
		return;

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

	// a round trigger only works while the round is running
	if ( FBitSet( pev->spawnflags, SF_TRIGGER_ROUND ) && g_pGameRules->RoundState() != ROUNDSTATE_RUNNING )
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
		SetTouch( NULL );

		if ( !FBitSet( pev->spawnflags, SF_TRIGGER_ROUND ) )
		{
			// we can't just remove (self) here, because this is a touch function
			// called while C code is looping through area links...
			pev->nextthink = gpGlobals->time + 0.1;
			SetThink( &CBaseTrigger::SUB_Remove );
		}
		else
		{
			// a round trigger stays and comes back with the next round
			SetThink( NULL );
			pev->nextthink = 0;
		}
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
	MAKE_STRING_CLASS( "fireanddie", pev );
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

	pev->iuser1 = pev->spawnflags & SF_LADDER_SILENT;

	SET_MODEL( ENT( pev ), STRING( pev->model ) );	// set size and link into world
	pev->movetype = MOVETYPE_PUSH;

	RoundStore();
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

	m_iEnabled = 1;

	RoundStore();
}

void CTriggerPush::RoundRespawn( void )
{
	InitTrigger();
	SetUse( &CBaseTrigger::ToggleUse );
	m_iEnabled = 1;
}

void CTriggerPush ::Touch( CBaseEntity *pOther )
{
	if ( !m_iEnabled )
		return;

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
		// Instant trigger, just transfer velocity and switch off until the round restart
		if ( FBitSet( pev->spawnflags, SF_TRIG_PUSH_ONCE ) )
		{
			pevToucher->velocity = pevToucher->velocity + ( pev->speed * pev->movedir );
			if ( pevToucher->velocity.z > 0 )
				pevToucher->flags &= ~FL_ONGROUND;
			m_iEnabled = 0;
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

	// the team the trigger is closed for is not teleported
	if ( FBitSet( pev->spawnflags, SF_TRIGGER_NO_ALLIES ) && pOther->IsPlayer() && pOther->pev->team == TEAM_ALLIES )
		return;

	if ( FBitSet( pev->spawnflags, SF_TRIGGER_NO_AXIS ) && pOther->IsPlayer() && pOther->pev->team == TEAM_AXIS )
		return;

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

	RoundStore();
}

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

// ---- CTriggerSetPatrol (ptype SaveData[1], 0x164 alloc) ----
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

int CTriggerSetPatrol::ObjectCaps( void )
{
	return 0;
}

// Factory: GetClassPtr sets the vtable + pev (size 0x164).
LINK_ENTITY_TO_CLASS( trigger_startpatrol, CTriggerSetPatrol );

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

TYPEDESCRIPTION CTriggerSetPatrol::m_SaveData[] = {
	DEFINE_FIELD( CTriggerSetPatrol, m_iszPath, FIELD_STRING ),
};
IMPLEMENT_SAVERESTORE( CTriggerSetPatrol, CBaseDelay );

void CTriggerSetPatrol::Spawn( void ) {}

void CTriggerSetPatrol::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseMonster *pMonster = (CBaseMonster *)UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
	CBaseEntity *path = UTIL_FindEntityByTargetname( NULL, STRING( m_iszPath ) );

	if ( path && pMonster && !strncmp( STRING( pMonster->pev->classname ), "monster_", 8 ) )
		pMonster->StartPatrol( path );
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

class CObjectCapture : public CBaseTrigger
{
  public:
	bool m_bCapped;	// set when spawnflags & 2 (one-shot)
	char sz_Group[256];		 // capobj_group, must match carried object
	char sz_DoneTarget[256];	 // capobj_donetarget
	string_t m_sMaster;
	int m_iAreaIndex;
	char sz_HudIcon[256];	 // capobj_hud_sprite

  public:
	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void Touch( CBaseEntity *pOther );
	virtual void KeyValue( KeyValueData *pkvd );
	virtual void RoundRespawn( void );
	virtual void area_SendStatus( class CBasePlayer *p );
	virtual void area_SetIndex( int index );
};

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

LINK_ENTITY_TO_CLASS( dod_object_goal, CObjectCapture );

void CObjectCapture::Spawn()
{
	UTIL_SetOrigin( pev, pev->origin );
	CBaseTrigger::InitTrigger();
	RoundStore();
	m_bCapped = false;
}

void CObjectCapture::RoundRespawn()
{
	UTIL_SetOrigin( pev, pev->origin );
	CBaseTrigger::InitTrigger();
	m_bCapped = false;
}

// link 0x141310 size 0x56a Ghidra 0x151310
void CObjectCapture::Touch( CBaseEntity *pOther )
{
	if ( g_pGameRules->RoundState() != ROUNDSTATE_RUNNING )
		return;
	if ( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;
	if ( g_pGameRules != NULL && g_pGameRules->RoundState() != ROUNDSTATE_RUNNING )
		return;
	if ( !pOther->IsPlayer() || !pOther->IsAlive() )
		return;
	if ( m_bCapped )
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	pPlayer->m_signals.Signal( SIGNAL_OBJECTAREA );
	pPlayer->m_iObjectAreaIndex = m_iAreaIndex;
	pPlayer->SetClientAreaIcon( m_iAreaIndex, true );
	if ( !pPlayer->HasObject || !pPlayer->m_pObject )
	{
		if ( ( pPlayer->m_flDisplayHistory & HINT_AREA_REQUIRES_OBJECT ) == 0 )
		{
			pPlayer->m_flDisplayHistory |= HINT_AREA_REQUIRES_OBJECT;
			pPlayer->HintMessage( "#Hint_area_requires_object", 0 );
		}
		return;
	}
	CObject *pObj = pPlayer->m_pObject;
	if ( strcmp( sz_Group, pObj->sz_Group ) != 0 )
		return;
	CBaseEntity *pLast = NULL;
	int NumObjects = 0;
	CBaseEntity *pEnt = UTIL_FindEntityByClassname( NULL, "dod_object" );
	while ( pEnt )
	{
		if ( pEnt == (CBaseEntity *)pObj )
		{
			UTIL_SendDirectorMessage( this, pOther, 11 );
		}
		else if ( strcmp( ( (CObject *)pEnt )->sz_Group, pObj->sz_Group ) == 0 )
		{
			if ( ( pEnt->pev->effects & EF_NODRAW ) == 0 )
				NumObjects++;
		}
		pLast = pEnt;
		pEnt = UTIL_FindEntityByClassname( pEnt, "dod_object" );
	}
	pPlayer->HasObject = FALSE;
	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;
	if ( pRules->m_RoundDoesCount )
		pPlayer->AddObjScore( pObj->m_iCapPoints, TRUE );
	if ( NumObjects == 0 || ( pev->spawnflags & SF_OBJECTGOAL_ONE_IS_ENOUGH ) )
	{
		FireTargets( sz_DoneTarget, pOther, this, USE_ON, 0.0 );
		if ( pObj->sz_DoneTarget[0] )
			FireTargets( pObj->sz_DoneTarget, pOther, this, USE_ON, 0.0 );
		if ( pev->spawnflags & SF_OBJECTGOAL_ONCE )
			m_bCapped = true;
		int idx = ENTINDEX( pOther->edict() );
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pTeammate = UTIL_PlayerByIndex( i );
			if ( pTeammate && pTeammate->pev->team == pOther->pev->team )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgPStatus, NULL, pTeammate->pev );
				WRITE_BYTE( idx );
				WRITE_BYTE( pPlayer->PlayerStatus() );
				MESSAGE_END();
			}
		}
	}
	else
	{
		if ( NumObjects == 1 )
			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_left_object", UTIL_dtos1( 1 ), 0, 0, 0 );
		else
			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_left_plural_object", UTIL_dtos1( NumObjects ), 0, 0, 0 );
		if ( pev->target )
			FireTargets( STRING( pev->target ), pOther, this, USE_TOGGLE, 0.0 );
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlr = (CBasePlayer *)UTIL_PlayerByIndex( i );
			if ( pPlr && pPlr->pev->team == pOther->pev->team && pPlr != pPlayer && pRules->m_RoundDoesCount )
				pPlr->AddObjScore( pObj->m_iOtherPoints, TRUE );
		}
	}
	const char *teamName = ( pOther->pev->team == TEAM_ALLIES ) ? "Allies" : "Axis";
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"dod_object_goal\"\n", STRING( pOther->pev->netname ),
		GETPLAYERUSERID( pOther->edict() ), GETPLAYERAUTHID( pOther->edict() ), teamName );
	if ( pLast && pLast->pev->noise1 )
		EMIT_SOUND_DYN( pOther->edict(), CHAN_STATIC, STRING( ( (CObject *)pLast )->pev->noise1 ), 1.0, 0.0, 0, 100 );
	else if ( pObj->pev->noise1 )
		EMIT_SOUND_DYN( pOther->edict(), CHAN_STATIC, STRING( pObj->pev->noise1 ), 1.0, 0.0, 0, 100 );
	pPlayer->HasObject = FALSE;
	pPlayer->RemoveObjectIcon();
}

void CObjectCapture::Precache()
{
	if ( sz_HudIcon[0] )
		PRECACHE_MODEL( sz_HudIcon );
}

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

// CObjectCapture area (triggers.cpp:3849/3827)
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

LINK_ENTITY_TO_CLASS( dod_location, CLocation );

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

void CLocation::RoundRespawn()
{
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	SetTouch( NULL );
}

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

LINK_ENTITY_TO_CLASS( dod_control_point, CControlPoint );

// ---- CDoDCamera SaveData (binary SaveData[13], mirrors CTriggerCamera 13 fields) ----
TYPEDESCRIPTION CDoDCamera::m_SaveData[] = {
	DEFINE_FIELD( CDoDCamera, m_hPlayer, FIELD_EHANDLE ),
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
};
IMPLEMENT_SAVERESTORE( CDoDCamera, CBaseDelay );

// ODR: CControlPoint canonical in objective.h (DWARF layout)
// ODR: CControlPointMaster canonical in objective.h

// ODR: CAreaCapture canonical in objective.h

// ODR: CObjectCapture canonical in objective.h

// ODR: CDodRoundTimer canonical in objective.h


// CControlPoint (binary ctor + getters)
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

// Instant steal on touch. Guards: RoundState()==ROUNDSTATE_RUNNING, m_bActive,
// gpGlobals->time >= m_fNextCapTime, IsPlayer + IsAlive checks,
// per-team can-touch block, no-op when toucher team == owner.
void CControlPoint::Touch( CBaseEntity *pOther )
{
	if ( g_pGameRules->RoundState() == ROUNDSTATE_RUNNING && m_bActive && m_fNextCapTime <= gpGlobals->time && pOther->IsPlayer() &&
		 pOther->IsAlive() )
	{
		int team = pOther->pev->team;

		if ( team == TEAM_ALLIES )
		{
			if ( m_bAlliesCantTouch )
				return;
		}
		else if ( team == TEAM_AXIS && m_bAxisCantTouch )
		{
			return;
		}

		if ( team != m_iTeam )
		{
			if ( m_bActive )
			{
				SetOwner( team, TRUE, (CBasePlayer *) pOther );
			}
			else
			{
				SET_MODEL( edict(), sz_ResetModel );
				SetBodygroup( 0, m_iResetModelBodygroup );
			}

			CBasePlayer *pPlayer = (CBasePlayer *) pOther;

			if ( !( pPlayer->m_flDisplayHistory & HINT_TOUCHED_CONTROL_POINT ) )
			{
				pPlayer->m_flDisplayHistory |= HINT_TOUCHED_CONTROL_POINT;
				pPlayer->HintMessage( "#Hint_touched_control_point", 0 );
			}

			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"dod_control_point\" - \"%s\"\n", STRING( pOther->pev->netname ),
							GETPLAYERUSERID( pOther->edict() ), GETPLAYERAUTHID( pOther->edict() ),
							pOther->pev->team == TEAM_ALLIES ? "Allies" : "Axis", STRING( pev->netname ) );

			m_fNextCapTime = gpGlobals->time + m_fCapDelayTime;
		}
	}
}

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

void CControlPoint::SetIndex( int i_index )
{
	if ( g_pGameRules && m_iAlliesIcon == 1 && ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesBrit() )
		m_iAlliesIcon = 0x1b;  // 27, brit HUD icon
	m_iIndex = i_index;
}

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

// Guards: not freeze/restart, RoundState 0/1, not game over.
// Effects: m_iTeam, team model + bodygroup, cap sound, team target,
// AddObjScore(player, m_iCapPoints), team score update, SetObjOwner msg,
// cap string, m_fNextCapTime = time + m_fCapDelayTime.
void CControlPoint::SetOwner( int owner, BOOL bMakeSound, CBasePlayer *pPlayer )
{
	if ( !m_bActive )
	{
		SET_MODEL( edict(), sz_ResetModel );
		SetBodygroup( 0, m_iResetModelBodygroup );

		return;
	}

	if ( g_pGameRules->IsClanMatch() && !g_pGameRules->IsClanMatchActive() )
		return;

	if ( g_pGameRules->RoundState() != ROUNDSTATE_RUNNING && g_pGameRules->RoundState() != ROUNDSTATE_PREROUND )
		return;

	if ( g_fGameOver )
		return;

	m_iTeam = owner;

	if ( owner == TEAM_ALLIES )
	{
		if ( bMakeSound && pev->noise1 )
			EMIT_SOUND_DYN( edict(), CHAN_STATIC, STRING( pev->noise1 ), 1.0, ATTN_NONE, 0, PITCH_NORM );

		SET_MODEL( edict(), sz_AlliesModel );
		SetBodygroup( 0, m_iAlliesModelBodygroup );

		FireTargets( sz_AlliesTarget, this, this, USE_TOGGLE, 0.0 );
	}
	else if ( owner == TEAM_AXIS )
	{
		if ( bMakeSound && pev->noise2 )
			EMIT_SOUND_DYN( edict(), CHAN_STATIC, STRING( pev->noise2 ), 1.0, ATTN_NONE, 0, PITCH_NORM );

		SET_MODEL( edict(), sz_AxisModel );
		SetBodygroup( 0, m_iAxisModelBodygroup );

		FireTargets( sz_AxisTarget, this, this, USE_TOGGLE, 0.0 );
	}
	else if ( owner == TEAM_UNASSIGNED )
	{
		if ( bMakeSound && pev->noise )
			EMIT_SOUND_DYN( edict(), CHAN_STATIC, STRING( pev->noise ), 1.0, ATTN_NONE, 0, PITCH_NORM );

		SET_MODEL( edict(), sz_ResetModel );
		SetBodygroup( 0, m_iResetModelBodygroup );

		FireTargets( sz_ResetTarget, this, this, USE_TOGGLE, 0.0 );
	}

	// the points count only while the round counts
	if ( bMakeSound && g_pGameRules && ( (CDoDTeamPlay *)g_pGameRules )->m_RoundDoesCount == 1 )
	{
		if ( pPlayer )
			pPlayer->AddObjScore( m_iCapPoints, TRUE );

		if ( owner == TEAM_ALLIES || owner == TEAM_AXIS )
			g_pGameRules->AddToTeamScores( owner, m_iTeamPoints );
	}

	if ( !( pev->spawnflags & SF_CONTROLPOINT_HIDDEN ) )
	{
		if ( !gmsgSetObjOwner )
			return;

		MESSAGE_BEGIN( MSG_ALL, gmsgSetObjOwner );
			WRITE_BYTE( m_iIndex );
			WRITE_BYTE( owner );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		UTIL_SendDirectorMessage( this, NULL, 11 );
	}

	if ( bMakeSound && owner != TEAM_UNASSIGNED )
		SendCapString( owner, pPlayer );

	m_fNextCapTime = gpGlobals->time + m_fCapDelayTime;
}

int CControlPoint::GetOwner( void ) { return m_iTeam; }
char *CControlPoint::GetGroup( void ) { return m_szGroup; }
int CControlPoint::PointValue( void ) { return m_iPointValue; }

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

void CControlPoint::SetActive( bool active )
{
	m_bActive = active;
	if ( !active )
		pev->effects |= EF_NODRAW;	// 0x80
	else
		pev->effects &= ~EF_NODRAW;
}

LINK_ENTITY_TO_CLASS( dod_control_point_master, CControlPointMaster );

CControlPointMaster::CControlPointMaster()
{
	m_szGroup[0] = 0;
}

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

// Clan scoring active: next give = now + pcvar_mp_clan_scoring_delay (default 30.0).
// Else: now + m_iGivePointsDelay.
void CControlPointMaster::RoundRespawn()
{
	if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value > 0.0 && pcvar_mp_clan_scoring && pcvar_mp_clan_scoring->value > 0.0 )
	{
		float d = 30.0;
		if ( pcvar_mp_clan_scoring_delay )
			d = pcvar_mp_clan_scoring_delay->value;
		m_fGivePointsTime = gpGlobals->time + d;
		return;
	}
	m_fGivePointsTime = (float) m_iGivePointsDelay + gpGlobals->time;
}

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

void CControlPointMaster::Reset( void )
{
	m_bTeamHasWon = false;
}

int GetClanScoringValues( int team, int cp )
{
	const char *szValues;

	if ( team != TEAM_ALLIES )
	{
		if ( team != TEAM_AXIS )
			return 0;

		szValues = CVAR_GET_STRING( "mp_clan_scoring_values_axis" );
	}
	else
	{
		szValues = CVAR_GET_STRING( "mp_clan_scoring_values_allies" );
	}

	if ( (int) strlen( szValues ) - 1 < cp )
	{
		ALERT( at_console, "Tried to get a clan scoring value for a point not defined in mp_clan_scoring_values_x\n" );
		return 0;
	}

	return szValues[cp] - '0';
}

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
	BOOL bClanMatch = pRules->IsClanMatch() && pRules->IsClanMatchActive();
	BOOL bClanScoring = ( pcvar_mp_clan_scoring && pcvar_mp_clan_scoring->value > 0.0 );

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
				if ( team == TEAM_ALLIES )
					alliesPoints += GetClanScoringValues( 1, i );
				else if ( team == TEAM_AXIS )
					axisPoints += GetClanScoringValues( 2, i );
			}
		}
		else
		{
			for ( int i = 0; i < numPoints; i++ )
			{
				if ( ControlPointArray[i]->m_iTeam == TEAM_ALLIES )
					alliesPoints += ControlPointArray[i]->m_iPointValue;
				else if ( ControlPointArray[i]->m_iTeam == TEAM_AXIS )
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
			if ( pPlr->pev->team == TEAM_ALLIES )
				bFoundAllies = TRUE;
			else if ( pPlr->pev->team == TEAM_AXIS )
				bFoundAxis = TRUE;
		}

		if ( bFoundAllies && alliesPoints > 0 && g_pGameRules->RoundState() == ROUNDSTATE_RUNNING && !g_fGameOver )
		{
			if ( bClanMatch )
			{
				if ( alliesPoints == 1 )
					UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_score_allie_point", 0, 0, 0, 0 );
				else
					UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_score_allie_points", UTIL_dtos1( alliesPoints ), 0, 0, 0 );
			}
			pRules->AddToTeamScores( 1, alliesPoints );
		}
		if ( bAxisPoints && bFoundAxis && g_pGameRules->RoundState() == ROUNDSTATE_RUNNING && !g_fGameOver )
		{
			if ( bClanMatch )
			{
				if ( axisPoints == 1 )
					UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_score_axis_point", 0, 0, 0, 0 );
				else
					UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_score_axis_points", UTIL_dtos1( axisPoints ), 0, 0, 0 );
			}
			pRules->AddToTeamScores( 2, axisPoints );
		}

		if ( bClanMatch && bClanScoring )
		{
			float d = 30.0;	 // _DAT_0018fdec
			if ( pcvar_mp_clan_scoring_delay )
				d = pcvar_mp_clan_scoring_delay->value;
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
		if ( team == TEAM_ALLIES )
		{
			FireTargets( sz_AlliesTarget, NULL, this, USE_TOGGLE, 0.0 );
			m_bTeamHasWon = true;
		}
		else if ( team == TEAM_AXIS )
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
			WRITE_BYTE( ( p->pev->spawnflags & SF_CONTROLPOINT_HIDDEN ) ^ 1 );
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

// Activates every discovered point (m_bActive, clear EF_NODRAW) and broadcasts
// InitObjDisplay to all.
void CControlPointMaster::BecomeActive()
{
	if ( m_bActive )
		return;

	for ( int i = 0; i < m_iNumPoints; i++ )
	{
		ControlPointArray[i]->m_bActive = true;
		ControlPointArray[i]->pev->effects &= ~EF_NODRAW;
	}
	SendObjectiveStatus( NULL );
	m_bActive = 1;
}

void CControlPointMaster::BecomeInactive()
{
	for ( int i = 0; i < m_iNumPoints; i++ )
	{
		ControlPointArray[i]->m_bActive = false;
		ControlPointArray[i]->pev->effects |= EF_NODRAW;
	}
	m_bActive = 0;
}

// CControlPointMaster helpers
int CControlPointMaster::GetPointOwner( int point )
{
	int owner = 0;
	if ( point >= 0 && point < 20 && ControlPointArray[point] )
		owner = ( (CControlPoint *)ControlPointArray[point] )->m_iTeam;
	return owner;
}

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
		WRITE_BYTE( ( p->pev->spawnflags & SF_CONTROLPOINT_HIDDEN ) ^ 1 );
		WRITE_BYTE( p->m_iNeutralIcon );
		WRITE_BYTE( p->m_iAlliesIcon );
		WRITE_BYTE( p->m_iAxisIcon );
		WRITE_COORD( p->pev->origin.x );
		WRITE_COORD( p->pev->origin.y );
	}
	MESSAGE_END();
}

bool CControlPointMaster::IsMasteredOn()
{
	if ( m_sMaster )
		return UTIL_IsMasterTriggered( m_sMaster, this ) != 0;
	return true;
}

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

// .text: CSandBag::IsMasteredOn inlined (no symbol; 305B = SandBagTouch 0x141070 size 0x131=305): return !m_sMaster || IsMasterTriggered exact
BOOL CSandBag::IsMasteredOn()
{
	return !m_sMaster || UTIL_IsMasterTriggered( m_sMaster, this );
}

// Factory: GetClassPtr sets the vtable + pev (ctor inits m_nRange=5, m_sMaster=0).
LINK_ENTITY_TO_CLASS( dod_trigger_sandbag, CSandBag );

CSandBag::CSandBag()
{
	m_nRange = 5;
	m_sMaster = 0;
}

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

	RoundStore();
}

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

void CSandBag::RoundRespawn()
{
	CBaseTrigger::InitTrigger();
	pev->sequence = 0;
	pev->framerate = 1.0;
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SetTouch( &CSandBag::SandBagTouch );
}

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
		pPlayer->HintMessage( "#Hint_sandbag_area_touch", 0 );
	}
}

class CAreaCapture : public CBaseTrigger
{
  public:
	int m_iCapMode;	 // 0 timed, 1 catchup (para rules)
	int m_bCapturing;
	int m_nCapturingTeam;  // -1 idle
	int m_nOwningTeam;	   // mirrors point owner, -1 none
	int m_nCapTime;		  // area_time_to_cap (seconds)
	float m_fTimeRemaining;
	int m_nAlliesNumCap;  // area_allies_numcap, min 1
	int m_nAxisNumCap;	  // area_axis_numcap, min 1
	int m_nNumAllies;
	int m_nNumAxis;
	bool m_bAlliesCanCap;  // area_allies_cancap
	bool m_bAxisCanCap;	  // area_axis_cancap
	int m_iCappingRequired;
	int m_iCappingPlayers;
	char sz_AlliesCap[256];			  // area_allies_endcap
	char sz_AxisCap[256];			  // area_axis_endcap
	char sz_AlliesStart[256];		  // area_allies_startcap
	char sz_AxisStart[256];			  // area_axis_startcap
	char sz_AlliesBreak[256];		  // area_allies_breakcap
	char sz_AxisBreak[256];			  // area_axis_breakcap
	int m_iAreaIndex;  // HUD area slot, -1 until assigned
	char sz_HudIcon[256];			  // area_hud_sprite
	CControlPoint *m_pPoint;		  // area_cap_point target
	char sz_CapPointName[256];		  // area_cap_point
	char sz_RequiredObjectGroup[256];  // area_object_group
	bool m_bRequiresObject;

  public:
	virtual void Precache( void );
	virtual void RoundRespawn( void );
	virtual void area_SetIndex( int index );
	virtual void area_SendStatus( class CBasePlayer *p );
	int GetNumAlive( int team );
	bool CanUseCatchup();
	void GetNumRequiredPlayers( int *reqAllies, int *reqAxis );
	virtual void Spawn( void );
	void EXPORT AreaTouch( CBaseEntity *pOther );
	virtual void Think( void );
	virtual void KeyValue( KeyValueData *pkvd );
	void StartCapture( int team, int capmode );
	void ReSendSwipeInfo();
	void EndCapture( int team );
	void BreakCapture( int team );
	void SendNumPlayers( class CBasePlayer *p );
	void SetOwner( int team );
};

LINK_ENTITY_TO_CLASS( dod_capture_area, CAreaCapture );

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

void CAreaCapture::Precache()
{
	if ( sz_HudIcon[0] )
		PRECACHE_MODEL( sz_HudIcon );
}

// CAreaCapture helpers (decompile + DWARF field names)
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

// Master gate, RoundState==1, game live. Lazily resolves m_pPoint by target
// and mirrors owner. Players only (IsPlayer+IsAlive): sets HUD area slot,
// area icon + hint, requires-object group check. Records presence via
// player area field = m_iAreaIndex.
void CAreaCapture::AreaTouch( CBaseEntity *pOther )
{
	if ( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;
	if ( g_pGameRules && g_pGameRules->RoundState() != ROUNDSTATE_RUNNING )
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
	BOOL can = ( team == TEAM_ALLIES ) ? m_bAlliesCanCap : ( team == TEAM_AXIS ? m_bAxisCanCap : FALSE );
	if ( ( pPlayer->m_flDisplayHistory & HINT_TOUCHED_AREA_CAPTURE ) == 0 && team != m_nOwningTeam )
	{
		if ( !can )
		{
			pPlayer->m_iCapAreaIconIndex = m_iAreaIndex;
			goto presentState;
		}
		pPlayer->m_flDisplayHistory |= HINT_TOUCHED_AREA_CAPTURE;
		pPlayer->HintMessage( "#Hint_touched_area_capture", 0 );
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
	pPlayer->m_signals.Signal( SIGNAL_CAPTUREAREA );
	if ( m_bRequiresObject && can )
	{
		if ( !pPlayer->HasObject || !pPlayer->m_pObject )
		{
			if ( pPlayer->m_flDisplayHistory & HINT_AREA_REQUIRES_OBJECT )
				return;
			if ( m_nOwningTeam == pPlayer->pev->team )
				return;
			pPlayer->m_flDisplayHistory |= HINT_AREA_REQUIRES_OBJECT;
			pPlayer->HintMessage( "#Hint_area_requires_object", 0 );
			return;
		}
		if ( strcmp( sz_RequiredObjectGroup, pPlayer->m_pObject->sz_Group ) != 0 )
			return;
	}
	pPlayer->m_iCapAreaIndex = m_iAreaIndex;
}

void CAreaCapture::GetNumRequiredPlayers( int *reqAllies, int *reqAxis )
{
	int numAllies = 0;
	int numAxis = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer )
		{
			if ( pPlayer->pev->team == TEAM_ALLIES )
				numAllies++;
			else if ( pPlayer->pev->team == TEAM_AXIS )
				numAxis++;

			if ( numAllies > m_nAlliesNumCap && numAxis > m_nAxisNumCap )
				break;
		}
	}

	*reqAllies = m_nAlliesNumCap;
	*reqAxis = m_nAxisNumCap;

	if ( *reqAllies < 1 )
		*reqAllies = 1;
	if ( *reqAxis < 1 )
		*reqAxis = 1;
}
// ODR: CDoDTeamPlay canonical in dod_gamerules.h
bool CAreaCapture::CanUseCatchup()
{
	bool bCatchup = false;
	CDoDTeamPlay *pRules = (CDoDTeamPlay *) g_pGameRules;

	if ( pRules )
	{
		bCatchup = true;

		if ( pRules->IsAlliesInfinite() )
			bCatchup = !pRules->IsAxisInfinite();
	}

	return ( pev->spawnflags & SF_AREA_CATCHUP ) && bCatchup;
}

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
		if ( ( plr->m_signals.GetState() & SIGNAL_CAPTUREAREA ) == 0 || plr->m_iCapAreaIndex != m_iAreaIndex )
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
		if ( plr->pev->team == TEAM_ALLIES )
			numAllies++;
		else if ( plr->pev->team == TEAM_AXIS )
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
			if ( m_bAlliesCanCap && m_nOwningTeam != TEAM_ALLIES && requiredAllies <= numAllies )
			{
				m_iCappingRequired = requiredAllies;
				m_iCappingPlayers = numAllies;
				StartCapture( 1, 0 );
				return;
			}
		}
		else if ( numAxis > 0 && numAllies < 1 && m_bAxisCanCap && m_nOwningTeam != TEAM_AXIS && requiredAxis <= numAxis )
		{
			m_iCappingRequired = requiredAxis;
			m_iCappingPlayers = numAxis;
			StartCapture( 2, 0 );
			return;
		}

		if ( !( pev->spawnflags & SF_AREA_CATCHUP ) )
			return;
		if ( !CanUseCatchup() )
			return;
		if ( numAllies > 0 && numAxis < 1 && m_bAlliesCanCap && m_nOwningTeam != TEAM_ALLIES )
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
		if ( !m_bAxisCanCap || m_nOwningTeam == TEAM_AXIS )
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
		int players = ( m_nCapturingTeam == TEAM_ALLIES ) ? numAllies : numAxis;
		int required = ( m_nCapturingTeam == TEAM_ALLIES ) ? requiredAllies : requiredAxis;
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
			// the capturing team can be gone while the other one stands in the area, don't divide by nobody
			if ( !m_pPoint || ( m_pPoint->pev->spawnflags & SF_CONTROLPOINT_HIDDEN ) || players < 1 )
				return;
			MESSAGE_BEGIN( MSG_ALL, gmsgProgBarUpdate );
				WRITE_BYTE( m_pPoint->m_iIndex );
				WRITE_COORD( (float)m_nCapTime * (float)( m_iCappingRequired / players ) );
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
	if ( m_nCapturingTeam == TEAM_ALLIES )
	{
		if ( requiredAllies <= numAllies )
			goto timeCheck;
		if ( ( pev->spawnflags & SF_AREA_CATCHUP ) && CanUseCatchup() && GetNumAlive( 1 ) == numAllies )
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
		if ( m_nCapturingTeam != TEAM_AXIS || requiredAxis <= numAxis )
			goto timeCheck;
		if ( ( pev->spawnflags & SF_AREA_CATCHUP ) && CanUseCatchup() && GetNumAlive( 2 ) == numAxis )
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

void CAreaCapture::SetOwner( int team )
{
	if ( m_bCapturing )
		BreakCapture( m_nCapturingTeam );
	m_nOwningTeam = team;
}

void CAreaCapture::SendNumPlayers( CBasePlayer *pPlayer )
{
	if ( !m_pPoint )
		return;
	int team = 0;
	int num = 0;
	int req = -1;
	int allies = m_nNumAllies;
	int axis = m_nNumAxis;
	if ( allies > 0 && axis < 1 && m_nOwningTeam != TEAM_ALLIES )
	{
		int ra = 1, rx = 1;
		GetNumRequiredPlayers( &ra, &rx );
		team = 1; num = allies; req = ra;
	}
	else if ( axis > 0 && allies < 1 && m_nOwningTeam != TEAM_AXIS )
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
	if ( req != -1 && num > req )
		num = req;
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
void CAreaCapture::ReSendSwipeInfo()
{
	if ( !m_pPoint )
		return;
	if ( m_pPoint->pev->spawnflags & SF_CONTROLPOINT_HIDDEN )
		return;
	// nobody is capturing, nothing to divide by
	if ( m_iCappingPlayers < 1 )
		return;
	float captime = (float)m_nCapTime * (float)( m_iCappingRequired / m_iCappingPlayers );
	MESSAGE_BEGIN( MSG_ALL, gmsgProgBarUpdate );
		WRITE_BYTE( m_pPoint->m_iIndex );
		WRITE_COORD( captime );
	MESSAGE_END();
}

// Fire start targets, m_nCapturingTeam = team, m_bCapturing = 1,
// m_fTimeRemaining = m_nCapTime. HUD: StartProgressBar(index, team, captime)
// or StartProgressBarF(index, team, captime * required / players) in
// catchup mode. Skipped when point hidden (spawnflags & 1). Director msg 12.
void CAreaCapture::StartCapture( int team, int capmode )
{
	if ( team == TEAM_ALLIES )
		FireTargets( sz_AlliesStart, NULL, this, USE_TOGGLE, 0.0 );
	else if ( team == TEAM_AXIS )
		FireTargets( sz_AxisStart, NULL, this, USE_TOGGLE, 0.0 );
	m_nCapturingTeam = team;
	m_iCapMode = capmode;
	m_bCapturing = 1;
	m_fTimeRemaining = (float) m_nCapTime;
	if ( m_pPoint && !( m_pPoint->pev->spawnflags & SF_CONTROLPOINT_HIDDEN ) )
	{
		if ( capmode == 1 )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgStartProgressBarF );
			WRITE_BYTE( m_pPoint->m_iIndex );
			WRITE_BYTE( team );
			WRITE_COORD( (float) ( m_iCappingRequired / m_iCappingPlayers * m_nCapTime ) );
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

// Fire end targets + team logs. Every qualifying player in area is logged;
// non-first cappers get AddObjScore(m_pPoint->m_iCapPoints). Consumes a
// carried required object (sound, icon clear, PStatus refresh). Then
// m_bCapturing = 0, m_nOwningTeam = team, point SetOwner / reset model,
// director msg 12.
void CAreaCapture::EndCapture( int team )
{
	if ( team == TEAM_ALLIES )
	{
		FireTargets( sz_AlliesCap, NULL, this, USE_TOGGLE, 0.0 );
		if ( !m_pPoint )
			UTIL_LogPrintf( "Team \"Allies\" triggered a \"dod_capture_area\"\n" );
		else
			UTIL_LogPrintf( "Team \"Allies\" triggered a \"dod_capture_area\" - \"%s\"\n", STRING( m_pPoint->pev->netname ) );
	}
	else if ( team == TEAM_AXIS )
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
		if ( ( plr->m_signals.GetState() & SIGNAL_CAPTUREAREA ) == 0 || plr->m_iCapAreaIndex != m_iAreaIndex )
			continue;
		const char *teamName = ( plr->pev->team == TEAM_ALLIES ) ? "Allies" : "Axis";
		if ( !m_pPoint )
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"dod_capture_area\"\n", STRING( plr->pev->netname ),
				GETPLAYERUSERID( plr->edict() ), GETPLAYERAUTHID( plr->edict() ), teamName );
		else
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"dod_capture_area\" - \"%s\"\n", STRING( plr->pev->netname ),
				GETPLAYERUSERID( plr->edict() ), GETPLAYERAUTHID( plr->edict() ), teamName, STRING( m_pPoint->pev->netname ) );

		if ( pFirstCapper && plr != pFirstCapper && m_pPoint )
			plr->AddObjScore( m_pPoint->m_iCapPoints, TRUE );
		if ( !pFirstCapper )
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

void CAreaCapture::BreakCapture( int team )
{
	if ( !m_bCapturing )
		return;

	m_iCappingRequired = 0;
	m_iCappingPlayers = 0;

	if ( m_nCapturingTeam == TEAM_ALLIES )
		FireTargets( sz_AlliesBreak, NULL, this, USE_BREAK, 0 );
	else if ( m_nCapturingTeam == TEAM_AXIS )
		FireTargets( sz_AxisBreak, NULL, this, USE_BREAK, 0 );

	if ( team == TEAM_ALLIES )
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "#game_capture_broken_allie" );
	else
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "#game_capture_broken_axis" );

	m_bCapturing = 0;

	if ( m_pPoint )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgCancelProgressBar );
		WRITE_BYTE( m_pPoint->m_iIndex );
		WRITE_BYTE( m_pPoint->m_iTeam );
		MESSAGE_END();
	}

	UTIL_SendDirectorMessage( this, NULL, 12 );
}

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

class CDodPointRelay : public CBaseEntity
{
  public:
	int m_nTeam;  // dod_relay_team, default -1

  public:
	virtual void Spawn( void );
	virtual void KeyValue( KeyValueData *pkvd );
	void EXPORT DodRelayUse( CBaseEntity *pA, CBaseEntity *pC, USE_TYPE u, float v );
};

LINK_ENTITY_TO_CLASS( dod_point_relay, CDodPointRelay );

void CDodPointRelay::Spawn()
{
	pev->solid = SOLID_NOT;
	SetUse( &CDodPointRelay::DodRelayUse );
}

// ---- dod_point_relay (CDodPointRelay, triggers.cpp) ----
// KeyValue: dod_relay_team => m_nTeam (default -1), else unhandled.
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
		if ( p->pev->spawnflags & SF_CONTROLPOINT_HIDDEN )
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
		if ( p->pev->spawnflags & SF_CONTROLPOINT_HIDDEN )
			return;
		MESSAGE_BEGIN( MSG_ALL, gmsgCancelProgressBar );
		WRITE_BYTE( p->m_iIndex );
		WRITE_BYTE( p->m_iTeam );
		MESSAGE_END();
		return;
	}
	if ( !p->m_bActive )
	{
		SET_MODEL( p->edict(), p->sz_ResetModel );
		p->SetBodygroup( 0, p->m_iResetModelBodygroup );
		return;
	}
	// the point goes to the team of the relay, the activator only gets the credit for it
	CBasePlayer *pPlayer = NULL;
	if ( pA && pA->IsPlayer() )
		pPlayer = (CBasePlayer *) pA;
	p->SetOwner( m_nTeam, TRUE, pPlayer );
}

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

LINK_ENTITY_TO_CLASS( dod_round_timer, CDodRoundTimer );

void CDodRoundTimer::Spawn()
{
	m_bTimer = false;
	pev->solid = SOLID_NOT;
	m_fLastThinkTime = 0.0;
	pev->nextthink = gpGlobals->time + 0.1;	 // _DAT_0018ee74
}

void CDodRoundTimer::Think()
{
	pev->nextthink = gpGlobals->time + 0.1;	 // _DAT_0018ee74
	if ( !m_bTimer )
		return;
	m_fRoundTime -= 0.1f;
	m_fUpdateTimer -= 0.1f;
	if ( m_fUpdateTimer <= 0.0f )
	{
		if ( ( pev->spawnflags & SF_ROUNDTIMER_SHOW ) && UTIL_IsMasterTriggered( m_sMaster, this ) && m_bTimer )
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
		if ( ( pev->spawnflags & SF_ROUNDTIMER_SHOW ) && UTIL_IsMasterTriggered( m_sMaster, this ) )
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

// CDodRoundTimer (triggers.cpp:6412/6422/6440/6470/6489)
void CDodRoundTimer::StartTimer()
{
	m_bTimer = 1;
	if ( !( pev->spawnflags & SF_ROUNDTIMER_SHOW ) )
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
void CDodRoundTimer::StopTimer()
{
	m_bTimer = 0;
	if ( !( pev->spawnflags & SF_ROUNDTIMER_SHOW ) )
		return;
	if ( !UTIL_IsMasterTriggered( m_sMaster, this ) )
		return;
	MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
		WRITE_BYTE( m_bTimer );
		WRITE_COORD( 0.0 );
	MESSAGE_END();
}
void CDodRoundTimer::ResetTimer()
{
	m_bTimer = 0;
	m_fUpdateTimer = 30.0;
	m_fRoundTime = m_fTimerLength;
	if ( !( pev->spawnflags & SF_ROUNDTIMER_SHOW ) )
		return;
	if ( !UTIL_IsMasterTriggered( m_sMaster, this ) )
		return;
	MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
		WRITE_BYTE( m_bTimer );
		WRITE_COORD( m_fRoundTime );
	MESSAGE_END();
}

void CDodRoundTimer::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !m_bTimer )
	{
		if ( UTIL_IsMasterTriggered( m_sMaster, this ) )
		{
			m_bTimer = true;
			if ( ( pev->spawnflags & SF_ROUNDTIMER_SHOW ) && UTIL_IsMasterTriggered( m_sMaster, this ) && m_bTimer )
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
		if ( ( pev->spawnflags & SF_ROUNDTIMER_SHOW ) && UTIL_IsMasterTriggered( m_sMaster, this ) )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
			WRITE_BYTE( m_bTimer );
			WRITE_COORD( 0.0 );
			MESSAGE_END();
		}
	}
}

void CDodRoundTimer::SendStatus()
{
	if ( !( pev->spawnflags & SF_ROUNDTIMER_SHOW ) )
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
void CDodRoundTimer::SendStatus( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return;
	if ( !( pev->spawnflags & SF_ROUNDTIMER_SHOW ) )
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

// ---- dod_camera (CDoDCamera) ----
// Cutscene camera for win/loose/draw/spawn scenes (see gamerules
// FireDODCamera: allies_win_cut, axis_loose_cut, ...). Spawn hides the model
// (null.mdl, MOVETYPE_NOCLIP, SOLID_NOT, rendermode TransTexture) and caches
// pev->angles/origin plus the mapper-supplied pev->speed; speed then follows
// path_corner targets. Entity size 0x2c8.
// iuser2 selects the scene slot:
//   1 axis spawn | 2 allie spawn | 3 axis win | 4 allie win |
//   5 axis loose | 6 allie loose | 7 axis draw | 8 allie draw

// Factory: GetClassPtr sets the vtable + pev (size 0x2c8).
LINK_ENTITY_TO_CLASS( dod_camera, CDoDCamera );

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

void CDoDCamera::Precache( void )
{
	PRECACHE_MODEL( "models/mapmodels/null.mdl" );
}

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
			 ( (CBasePlayer *) pEnt )->m_iNextClass == -1 ||
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
		if ( !( pev->spawnflags & SF_DODCAMERA_NO_DAMPING ) )
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
		ALERT( at_aiconsole, "**dod_camera** No Path_corner FOUND !!\n" );
		return;
	}
	m_moveDistance = m_moveDistance - pev->speed * gpGlobals->frametime;
	if ( m_moveDistance <= 0 )
	{
		if ( pev->iuser2 < 1 )
		{
			ALERT( at_aiconsole, "**dod_camera** Reached %s: %s\n", STRING( pPath->pev->classname ),
				   STRING( pPath->pev->targetname ) );
		}
		else
		{
			ALERT( at_aiconsole, "**dod_camera** type: %s  Reached %s: %s\n", CutSceneName( pev->iuser2 ),
				   STRING( pPath->pev->classname ), STRING( pPath->pev->targetname ) );
		}
		if ( pPath->pev->message )
		{
			FireTargets( STRING( pPath->pev->message ), this, this, USE_TOGGLE, 0 );
			if ( pPath->pev->spawnflags & SF_CORNER_FIREONCE )
				pPath->pev->message = 0;
		}
		if ( !strcmp( STRING( pPath->pev->classname ), "path_corner" ) )
			SetCameraMode( STRING( pPath->pev->targetname ), ( (CPathCorner *) pPath )->m_iCamFade );
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

float CDoDCamera::ReturnTimeLeft( void )
{
	return m_flReturnTime;
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

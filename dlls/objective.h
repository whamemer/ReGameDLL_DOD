// objective.h -- DoD objective / capture entities, restored from dod.so
// Real sources per addr2line: dod/dlls/triggers.cpp (capture area, control
// point, master, object goal, round timer, point relay, location),
// dod/dlls/objectives.cpp (CObject), dod/dlls/mapmarker.cpp (CMapMarker),
// dod/dlls/dod_gamerules.cpp (CDoDDetect, CDodParaRoundTimer).
// Ghidra addresses below are absolute (image base 0x10000).
// Link/file address = Ghidra address - 0x10000.
#pragma once

#include "cbase.h"
#include "gamerules.h"
class CBasePlayer;
class CItem;
class CBasePlayerAmmo;
#include "dod_gamerules.h"

// capture numbers: area_time_to_cap -> m_nCapTime, numcap min 1 clamp, point_reset_time CapDelay default 1.0 verified 0x3f800000 at 0x149f47 CControlPoint::Spawn:3968, object_resettime default 600 max 600, give_delay m_iGivePointsDelay plus clan 30.0, para-timer default 240.0, round timer 30.0 status and 0.1 think, CPMThink 0.5, AreaThink 0.1 - header defaults match restore NOTES and binary spot-checks
#define MAX_CONTROL_POINTS 20
#define MAX_AREA_INDEX 0x80

// spawnflags of the DoD objective entities
#define SF_AREA_CATCHUP 0x0001			// dod_capture_area: the capture may speed up when the whole team stands in it (catchup)
#define SF_CONTROLPOINT_HIDDEN 0x0001	// dod_control_point: no HUD icon and no progress bar
#define SF_ROUNDTIMER_SHOW 0x0001		// dod_round_timer: the timer status goes to the HUD
#define SF_ROUNDTIMER_MANUAL 0x0002		// dod_round_timer: not started at the start of the round
#define SF_OBJECT_ANNOUNCE 0x0001		// dod_object: the pick up and the loss are printed to everybody
#define SF_OBJECTGOAL_ONE_IS_ENOUGH 0x0001	// dod_object_goal: finished by the first object that arrives
#define SF_OBJECTGOAL_ONCE 0x0002		// dod_object_goal: works only once
#define SF_PARTICLESHOOTER_STARTON 0x0020	// particle_shooter: shoots from the start of the round

// dod_control_point - DWARF-canonical (gdb ptype CControlPoint @ dod.so)
// field order/sizes match DWARF 1:1; all public for TU access (DWARF privates kept public)
class CControlPoint : public CBaseAnimating
{
  public:
	string_t m_sMaster;
	int m_iTeam;			  // 0 neutral, 1 allies, 2 axis
	int m_iResetTime;		  // point_reset_time
	int m_iMainObjective;
	int m_iDefaultOwner;	  // point_default_owner (0..2)
	int m_iIndex;			  // runtime slot / HUD index
	int m_iPointValue;		  // point_pointvalue
	int m_iCapPoints;		  // point_points_for_cap
	int m_iTeamPoints;		  // point_team_points
	float m_fCapDelayTime;	  // touch throttle, default 1.0
	float m_fNextCapTime;
	char sz_WinString[256];		  // point_win_string
	string_t m_sAlliesCapString;
	string_t m_sAxisCapString;
	char sz_AlliesTarget[256];
	char sz_AxisTarget[256];
	char sz_ResetTarget[256];
	char sz_AlliesModel[256];
	char sz_AxisModel[256];
	char sz_ResetModel[256];
	int m_iAlliesModelBodygroup;
	int m_iAxisModelBodygroup;
	int m_iResetModelBodygroup;
	int m_iAlliesIcon;
	int m_iAxisIcon;
	int m_iNeutralIcon;
	bool m_bAlliesCantTouch;
	bool m_bAxisCantTouch;
	int m_bPointVisible;
	int m_iPointIndex;		  // point_index (1-based in map, stored 0-based)
	char m_szGroup[256];	 // point_group
	bool m_bActive;
  public:
	CControlPoint();
	void SetOwner( int owner, BOOL bMakeSound, CBasePlayer *pPlayer );
	int GetOwner( void );
	const char *GetName( void );
	char *GetGroup( void );
	void SetIndex( int i_index );
	int PointValue( void );
	void Reset( void );
	virtual void RoundRespawn( void );
	void TriggerTargets( void );
	int GetPointIndex( void );
	int GetIndex( void );
	void SetActive( bool active );
	int PointIsVisible( void );
	void HideThink( void );
	int GetAlliesIcon( void );
	int GetAxisIcon( void );
	int GetNeutralIcon( void );
	int GetPointValue( void );
	void SendCapString( int team, CBasePlayer *pPlayer );
	virtual void Spawn( void );
	virtual void Touch( CBaseEntity *pOther );
	virtual void KeyValue( KeyValueData *pkvd );
	virtual void Precache( void );
};

// dod_control_point_master - DWARF-canonical (gdb ptype CControlPointMaster)
class CControlPointMaster : public CBaseEntity
{
  public:
	CControlPoint *ControlPointArray[MAX_CONTROL_POINTS];
	int m_iNumPoints;
	BOOL FoundPoints;  // discovery done
	BOOL m_bActive;
	float m_fGivePointsTime;			// next periodic score time
	int m_iGivePointsDelay;				// point_give_delay_time (seconds)
	int m_iMainObjective;  // main
	char sz_AlliesTarget[256];			// allies_capture_target
	char sz_AxisTarget[256];				// axis_capture_target
	char sz_InterimPointsMessage[256];
	char sz_AllPointsCapMessage[256];
	string_t m_sMaster;					// master
	int m_bMasteredOn;
	bool m_bTeamHasWon;
	char m_szGroup[256];					// cpm_group
	float m_flNextSpectatorUpdateTime;	// +20.0 s InitObjDisplay refresh
	CControlPointMaster();
	int GetNumPoints( void );
	int GetPointOwner( int point );
	void Reset( void );
	void SendObjectiveStatus( CBasePlayer *pPlayer );
	bool IsMasteredOn( void );
	virtual void RoundRespawn( void );
	void BecomeActive( void );
	void BecomeInactive( void );
	virtual void Spawn( void );
	virtual void KeyValue( KeyValueData *pkvd );
	void EXPORT CPMThink( void );
	int TeamOwnsAllPoints();
};

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

	virtual int ObjectCaps( void ) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};
// dod_capture_area - DWARF-canonical (gdb ptype CAreaCapture)
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

// dod_object (carried) - DWARF-canonical (gdb ptype /o CObject @ dod.so)
// field order/sizes match DWARF 1:1; base CBaseEntity ends at 343
class CObject : public CBaseEntity
{
  public:
	Vector m_vStartOrigin;		// 344
	int m_iTeam;				// 356 object_owner
	int m_iMain;				// 360 object_main
	int m_iResetOnTouch;		// 364 object_resetontouch
	int m_iResetTime;			// 368 object_resettime, default 600, max 600
	int m_iOtherPoints;			// 372 object_otherpoints (team award)
	int m_iCapPoints;			// 376 object_cappoints (carrier award)
	int ObjectIndex;			// 380
	int m_iObjectHIcon;			// 384
	float m_fCarrySpeed;		// 388 object_carryspeed
	float m_flNextTouchTime;	// 392 +1.0 s touch throttle after reset
	float m_flReset;			// 396 drop expiry time
	char sz_Name[256];			// 400 object_name
	char sz_Group[256];			// 656 object_group
	char sz_DoneTarget[256];	// 912 object_donetarget
	char m_szObjectPIcon[256];	// 1168 object_player_sprite
	char m_szObjectHIcon[256];	// 1424 object_hud_sprite
	string_t m_sMaster;			// 1680 master
	EHANDLE m_hOwner;			// 1684 carrier
	BOOL m_bDropped;			// 1692
	BOOL m_bCarried;			// 1696

  public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT ObjectTouch( CBaseEntity *pOther );
	void EXPORT ObjectThink( void );
	void EXPORT AbsentPlayerThink( void );
	void Drop( void );
	void Materialize( void );
	void Disappear( void );
	void ResetObject( void );
	void RoundRespawn( void );
	void SendDroppedMsg( CBasePlayer *pPlayer );
	void SendPickupMsg( CBasePlayer *pPlayer );
};

// dod_object_goal - DWARF-canonical (gdb ptype CObjectCapture)
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

// dod_round_timer - DWARF-canonical (gdb ptype CDodRoundTimer)
class CDodRoundTimer : public CBaseEntity
{
  public:
	float m_fTimerLength;  // round_timer_length
	float m_fRoundTime;	   // remaining
	bool m_bTimer;
	float m_fLastThinkTime;
	float m_fUpdateTimer;  // 30.0 s HUD refresh cadence
	string_t m_sMaster;	 // master

	void StartTimer();
	void StopTimer();
	void ResetTimer();
	void SendStatus();
	void SendStatus( class CBasePlayer *p );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void Spawn( void );
	virtual void KeyValue( KeyValueData *pkvd );
	virtual void EXPORT Think( void );
};

// dod_point_relay
class CDodPointRelay : public CBaseEntity
{
  public:
	int m_nTeam;  // dod_relay_team, default -1

  public:
	virtual void Spawn( void );
	virtual void KeyValue( KeyValueData *pkvd );
	void EXPORT DodRelayUse( CBaseEntity *pA, CBaseEntity *pC, USE_TYPE u, float v );
};

// dod_location
class CLocation : public CBaseEntity
{
  public:
	string_t m_sMaster;	 // master (netname = location_name)
  public:
	virtual void Spawn( void );
	virtual void RoundRespawn( void );
	virtual void KeyValue( KeyValueData *pkvd );
};



// para timer (code-created, dod_gamerules.cpp) - DWARF-canonical
class CDodParaRoundTimer : public CBaseEntity
{
  public:
	float m_fUpdateTimer;  // 344, 30.0 s
	float m_fRoundTimer;   // 348, default 240.0 on reset
	float m_fLastThinkTime;// 352
	bool m_bTimer;		   // 356
	void Spawn( void );
	void StartTimer( void );
	void StopTimer( void );
	void ResetTimer( void );
	void SendStatus( class CBasePlayer *p );
	void EXPORT TimerThink( void );
};

// particle_shooter (CParticleShooter, objectives.cpp:661-775)
class CParticleShooter : public CBaseDelay
{
  public:
	BYTE m_iGroupId;	 // group_id
	int m_iSpriteIndex;	 // precached model index
	Vector m_vShootDir;	 // computed direction * speed
	int m_iNumParticles; // num_particles, -1 continuous?
	int m_iState;		 // on/off
	float m_flSpeed;	 // particle_speed / m_flVelocity
  public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void FindTargetEntity( void );
	void SendUpdate( CBasePlayer *pPlayer );
	void UpdateJoiningPlayer( CBasePlayer *pPlayer );
	void RoundRespawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

// env_weather (CWeatherSystem, objectives.cpp:813-855)
class CWeatherSystem : public CBaseEntity
{
  public:
	int m_iType;		// type bitmask (|4 lightning)
	float m_flDensity;	// density
  public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT InitThink( void );
	void SendForecast( CBasePlayer *pPlayer );
	void SendData( void );
};

// dod_score_ent (CScoreEvent, objectives.cpp:445-539)
class CScoreEvent : public CBaseEntity
{
  public:
	int m_iMain;
	int m_iResetItemsTime;	 // score_resetitemtime
	int m_iResetPlayersTime; // score_resetplayerstime
	int m_iTeamPoints;		 // score_points
	float ResetItemsTime;	 // next reset time
	BOOL ResetItems;		 // pending reset flag
	char szNextMap[256];	 // score_nextmap
	BOOL bChangeLevel;		 // pending changelevel
	float ChangeLevelTime;	 // changelevel time
	int m_iChangeLevelDelay; // score_nextmapdelay
  public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT ScoreEventUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT ScoreThink( void );
};

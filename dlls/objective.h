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
#pragma once

#include "cbase.h"
#include "gamerules.h"
class CBasePlayer;
class CItem;
class CBasePlayerAmmo;
#include "dod_gamerules.h"

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

class CObject : public CBaseEntity
{
public:
	Vector m_vStartOrigin;
	int m_iTeam;				// 356 object_owner
	int m_iMain;				// 360 object_main
	int m_iResetOnTouch;		// 364 object_resetontouch
	int m_iResetTime;			// 368 object_resettime, default 600, max 600
	int m_iOtherPoints;			// 372 object_otherpoints (team award)
	int m_iCapPoints;			// 376 object_cappoints (carrier award)
	int ObjectIndex;
	int m_iObjectHIcon;
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
	BOOL m_bDropped;
	BOOL m_bCarried;

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

// dod_point_relay
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

	EHANDLE m_hPlayer[33];	 // indices 1..32 used (MAX clients)
	EHANDLE m_hTarget;
	string_t m_sPath;		 // moveto target (first path_corner)
	int m_teamBound;		 // 3 = both teams
	string_t m_sTrigrStart;
	string_t m_sTrigrEnd;
	int m_iOverlay;
	int m_iFOV;				 // zoomlevel clamped 0..90
	int m_iSubtitle;
	int m_iCamFade;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	int m_state;
	CBaseEntity
		*m_pentPath;
	Vector m_angles;
	Vector
		m_origin;
};

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

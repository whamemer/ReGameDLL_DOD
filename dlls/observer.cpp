//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Functionality for the observer chase camera (DoD version)
//
//=============================================================================
// DoD restoration: verified against dod.so (link addresses; Ghidra = link+0x10000).
// HL SDK version only cycled players with IsObserver/EF_NODRAW checks.
// DoD adds team/infinite-lives/forcecamera logic, deadflag/iuser1/effects
// filtering, weapon-anim sync (SendObserverTargetWeaponAnim), and
// Start/Stop/SpectatePlayer paths. See decodes in ~/rev/work/dec/00121xxx.
//=============================================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "pm_shared.h"
#include "gamerules.h"
#include "dod_gamerules.h"

extern int gmsgCurWeapon;
extern int gmsgSetFOV;
extern int gmsgSpectator;
extern cvar_t cvar_fadetoblack;
extern cvar_t cvar_forcechasecam;
extern cvar_t cvar_forcecamera;

// .text:001212b0 IsTeamInfinite (observer.cpp:35)
BOOL IsTeamInfinite( int team )
{
	return ( (CDoDTeamPlay *)g_pGameRules )->IsTeamInfinite( team );
}

// .text:001212d0 GetForceCamera (observer.cpp:55)
int GetForceCamera( void )
{
	if ( cvar_fadetoblack.value != 0.0f )
		return 2;
	if ( cvar_forcechasecam.value != 0.0f )
		return 1;
	if ( cvar_forcecamera.value != 0.0f )
		return 1;
	return 0;
}

// .text:00121330 CBasePlayer::Observer_PlayerValidTarget (observer.cpp:67)
bool CBasePlayer::Observer_PlayerValidTarget( CBaseEntity *pEnt, bool bSameTeam )
{
	bool bRet = false;

	if ( pEnt && pEnt != this && !( ( (CBasePlayer *)pEnt )->has_disconnected )
		&& pEnt->pev->deadflag < 2 && pEnt->pev->iuser1 < 1 && !( pEnt->pev->effects & EF_NODRAW ) )
	{
		if ( !IsTeamInfinite( pev->team ) || pev->team == 3 || pev->team == pEnt->pev->team )
		{
			bRet = true;
			if ( bSameTeam )
				bRet = ( pEnt->pev->team == pev->team );
		}
	}

	return bRet;
}

// .text:00121400 CBasePlayer::Observer_FindNextPlayer (observer.cpp:109)
void CBasePlayer::Observer_FindNextPlayer( bool bReverse )
{
	int iStart;
	if ( m_hObserverTarget )
		iStart = ENTINDEX( m_hObserverTarget->edict() );
	else
		iStart = ENTINDEX( edict() );

	int iCurrent = iStart;
	m_hObserverTarget = NULL;

	bool bForceSameTeam = false;
	if ( cvar_fadetoblack.value != 0.0f || cvar_forcechasecam.value != 0.0f || cvar_forcecamera.value != 0.0f )
		bForceSameTeam = ( pev->team != 3 );

	int iDir = bReverse ? -1 : 1;

	do
	{
		iCurrent += iDir;

		if ( iCurrent > gpGlobals->maxClients )
			iCurrent = 1;
		if ( iCurrent < 1 )
			iCurrent = gpGlobals->maxClients;

		CBaseEntity *pEnt = UTIL_PlayerByIndex( iCurrent );
		if ( !pEnt || pEnt == this )
			continue;
		if ( ( (CBasePlayer *)pEnt )->has_disconnected )
			continue;
		if ( pEnt->pev->deadflag >= 2 || pEnt->pev->iuser1 >= 1 )
			continue;
		if ( pEnt->pev->effects & EF_NODRAW )
			continue;

		if ( IsTeamInfinite( pev->team ) && pev->team != 3 && pEnt->pev->team != pev->team )
			continue;
		if ( bForceSameTeam && pEnt->pev->team != pev->team )
			continue;

		m_hObserverTarget = pEnt;
		break;

	} while ( iCurrent != iStart );

	// Did we find a target?
	if ( m_hObserverTarget )
	{
		// Move to the target
		UTIL_SetOrigin( pev, m_hObserverTarget->pev->origin );

		// Store the target in pev so the physics DLL can get to it
		if ( pev->iuser1 != OBS_ROAMING )
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );

		if ( pev->iuser1 == OBS_IN_EYE && m_hObserverTarget )
			SendObserverTargetWeaponAnim();
	}
}

// .text:00121730 CBasePlayer::SendObserverTargetWeaponAnim (observer.cpp:164)
void CBasePlayer::SendObserverTargetWeaponAnim()
{
	if ( !m_hObserverTarget )
		return;

	int team = m_hObserverTarget->pev->team;
	int body = 2;
	if ( team == 1 )
		body = ( ( m_hObserverTarget->pev->playerclass - 0x15 ) < 6 ) ? 1 : 0;

	MESSAGE_BEGIN( MSG_ONE, 0x23, NULL, pev );
	WRITE_BYTE( 0 );
	WRITE_BYTE( body );
	MESSAGE_END();
}

// .text:00121800 CBasePlayer::Observer_CheckProperties (observer.cpp:283)
void CBasePlayer::Observer_CheckProperties()
{
	// try to find a target if we have no current one
	if ( pev->iuser1 == OBS_IN_EYE && m_hObserverTarget != NULL )
	{
		CBasePlayer *target = (CBasePlayer *)( UTIL_PlayerByIndex( ENTINDEX( m_hObserverTarget->edict() ) ) );

		if ( !target )
			return;

		int weapon = ( target->m_pActiveItem != NULL ) ? target->m_pActiveItem->m_iId : 0;
		// use fov of tracked client
		if ( m_iFOV != target->m_iFOV || m_iObserverWeapon != weapon )
		{
			m_iFOV = target->m_iFOV;
			m_iClientFOV = m_iFOV;
			// write fov before weapon data, so zoomed crosshair is set correctly
			MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
			WRITE_BYTE( m_iFOV );
			MESSAGE_END();

			m_iObserverWeapon = weapon;
			// send weapon update
			MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
			WRITE_BYTE( 1 );  // 1 = current weapon, not on target
			WRITE_BYTE( m_iObserverWeapon );
			WRITE_BYTE( 0 );  // clip
			MESSAGE_END();
		}
	}
	else
	{
		m_iFOV = 90;

		if ( m_iObserverWeapon != 0 )
		{
			m_iObserverWeapon = 0;

			MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
			WRITE_BYTE( 1 );  // 1 = current weapon
			WRITE_BYTE( m_iObserverWeapon );
			WRITE_BYTE( 0 );  // clip
			MESSAGE_END();
		}
	}
}

// .text:00121970 CBasePlayer::Observer_SetMode (observer.cpp:332)
void CBasePlayer::Observer_SetMode( int iMode )
{
	// Just abort if we're changing to the mode we're already in
	if ( iMode == pev->iuser1 )
	{
		pev->iuser3 = 0;
		return;
	}

	int forcecamera = GetForceCamera();

	// is valid mode ?
	if ( iMode < 1 || iMode > 4 )
		iMode = OBS_IN_EYE;	 // now it is

	// TEAM_SPECTATOR = 3 in DoD; forcecamera restricts roaming/chase
	if ( pev->team != 3 )
	{
		if ( iMode == OBS_ROAMING && forcecamera == 1 )
			iMode = OBS_CHASE_LOCKED;
		else if ( forcecamera == 2 )
			iMode = OBS_IN_EYE;
	}

	// verify observer target again
	if ( m_hObserverTarget != NULL )
	{
		CBaseEntity *pEnt = m_hObserverTarget;

		if ( ( pEnt == this ) || ( pEnt == NULL ) )
			m_hObserverTarget = NULL;
		else if ( ( (CBasePlayer *)pEnt )->has_disconnected || pEnt->pev->iuser1 > 0 || ( pEnt->pev->effects & EF_NODRAW ) )
			m_hObserverTarget = NULL;
		else if ( forcecamera != 0 && pEnt->pev->team != pev->team )
			m_hObserverTarget = NULL;
	}

	// set spectator mode
	pev->iuser1 = iMode;

	// if we are not roaming, we need a valid target to track
	if ( ( iMode != OBS_ROAMING ) && ( m_hObserverTarget == NULL ) )
	{
		Observer_FindNextPlayer( false );

		// if we didn't find a valid target switch to roaming
		if ( m_hObserverTarget == NULL )
		{
			pev->iuser1 = OBS_ROAMING;
		}
	}

	// set target if not roaming
	if ( pev->iuser1 == OBS_ROAMING )
	{
		pev->iuser2 = 0;
	}
	else
		pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );

	pev->iuser3 = 0;  // clear second target from death cam

	if ( m_hObserverTarget )
		UTIL_SetOrigin( pev, m_hObserverTarget->pev->origin );

	// print spectator mode on client screen
	char modemsg[16];
	sprintf( modemsg, "#Spec_Mode%i", pev->iuser1 );
	ClientPrint( pev, HUD_PRINTCENTER, modemsg );

	m_iObserverLastMode = iMode;

	if ( pev->iuser1 == OBS_IN_EYE && m_hObserverTarget != NULL )
		SendObserverTargetWeaponAnim();
}

// .text:00121cf0 CBasePlayer::Observer_CheckTarget (observer.cpp:239)
void CBasePlayer::Observer_CheckTarget()
{
	if ( pev->iuser1 == OBS_ROAMING )
		return;

	// try to find a target if we have no current one
	if ( m_hObserverTarget == NULL )
	{
		Observer_FindNextPlayer( false );

		if ( m_hObserverTarget == NULL )
		{
			// no target found at all
			int lastMode = pev->iuser1;

			Observer_SetMode( OBS_ROAMING );

			m_iObserverLastMode = lastMode;	 // don't overwrite users lastmode

			return;	 // we still have no target return
		}
	}

	CBasePlayer *target = (CBasePlayer *)( UTIL_PlayerByIndex( ENTINDEX( m_hObserverTarget->edict() ) ) );

	if ( !target )
	{
		Observer_FindNextPlayer( false );
		return;
	}

	// check target
	if ( target->pev->deadflag == DEAD_DYING )
	{
		if ( ( target->m_fDeadTime + 2.0f ) < gpGlobals->time )
		{
			// 2 secs after death change target
			Observer_FindNextPlayer( false );
			return;
		}
	}
}

// .text:00121dc0 CBasePlayer::Observer_HandleButtons (observer.cpp:191)
void CBasePlayer::Observer_HandleButtons()
{
	// Slow down mouse clicks
	if ( m_flNextObserverInput > gpGlobals->time )
		return;

	// Jump changes from modes: Chase to Roaming
	if ( m_afButtonPressed & IN_JUMP )
	{
		if ( pev->iuser1 == OBS_CHASE_LOCKED )
			Observer_SetMode( OBS_CHASE_FREE );
		else if ( pev->iuser1 == OBS_CHASE_FREE )
			Observer_SetMode( OBS_IN_EYE );
		else if ( pev->iuser1 == OBS_IN_EYE )
			Observer_SetMode( OBS_ROAMING );
		else if ( pev->iuser1 == OBS_ROAMING )
			Observer_SetMode( OBS_CHASE_FREE );
		else
			Observer_SetMode( OBS_CHASE_FREE );

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack moves to the next player
	if ( m_afButtonPressed & IN_ATTACK )
	{
		Observer_FindNextPlayer( false );
		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack2 moves to the prev player
	if ( m_afButtonPressed & IN_ATTACK2 )
	{
		Observer_FindNextPlayer( true );
		m_flNextObserverInput = gpGlobals->time + 0.2;
	}
}

// .text:00121ee0 CBasePlayer::StartObserver (observer.cpp:424)
void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle )
{
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( 125 );
	WRITE_BYTE( ENTINDEX( edict() ) & 0xFF );
	MESSAGE_END();

	if ( m_pActiveItem )
		m_pActiveItem->Holster( 0 );

	if ( m_pTank )
	{
		m_pTank->Use( this, this, USE_OFF, 0 );
		m_pTank = NULL;
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
	WRITE_BYTE( 0 );
	WRITE_BYTE( 0xFF );
	WRITE_BYTE( 0xFF );
	MESSAGE_END();

	m_iFOV = 0;
	m_iClientFOV = 0;
	pev->fov = 0.0;
	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
	WRITE_BYTE( 0 );
	MESSAGE_END();

	CheckPlayerSpeed();
	m_iHideHUD = 9;
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->v_angle = vecViewAngle;
	pev->angles = pev->v_angle;
	pev->fixangle = 1;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin( pev, vecPosition );
	m_afPhysicsFlags &= ~PFLAG_DUCKING;
	pev->flags &= ~FL_ONGROUND;
	pev->deadflag = DEAD_DEAD;
	pev->health = 1.0;
	m_bIsObserver = TRUE;
	RemoveAllItems( 0 );
	m_flNextObserverInput = 0.0;
	Observer_SetMode( m_iObserverLastMode );
	(*g_engfuncs.pfnSetClientMaxspeed)( edict(), 350.0 );

	MESSAGE_BEGIN( MSG_ALL, gmsgSpectator, NULL );
	WRITE_BYTE( ENTINDEX( edict() ) );
	WRITE_BYTE( 1 );
	MESSAGE_END();
}

// .text:001221f0 CBasePlayer::StopObserver (observer.cpp:495)
void CBasePlayer::StopObserver()
{
	if ( pev->iuser1 == 0 && pev->iuser2 == 0 && !m_bIsObserver )
	{
		m_iHideHUD = 0;
		return;
	}

	MESSAGE_BEGIN( MSG_ALL, gmsgSpectator, NULL );
	WRITE_BYTE( ENTINDEX( edict() ) );
	WRITE_BYTE( 0 );
	MESSAGE_END();

	pev->iuser1 = 0;
	pev->iuser2 = 0;
	m_bIsObserver = FALSE;
	m_hObserverTarget = NULL;
	m_imissedwave = 0;
	m_irdytospawn = 0;
	m_iHideHUD = 0;
}

// .text:001222e0 CBasePlayer::Observer_SpectatePlayer (observer.cpp:518)
void CBasePlayer::Observer_SpectatePlayer( int iCurrent )
{
	if ( pev->iuser1 == 0 )
		return;

	bool bForceSameTeam = false;
	if ( cvar_fadetoblack.value != 0.0f || cvar_forcechasecam.value != 0.0f || cvar_forcecamera.value != 0.0f )
		bForceSameTeam = ( pev->team != 3 );

	CBaseEntity *pEnt = UTIL_PlayerByIndex( iCurrent );
	if ( pEnt && pEnt != this && !( ( (CBasePlayer *)pEnt )->has_disconnected )
		&& pEnt->pev->deadflag < 2 && pEnt->pev->iuser1 < 1 && !( pEnt->pev->effects & EF_NODRAW ) )
	{
		if ( ( !IsTeamInfinite( pev->team ) || pev->team == 3 || pev->team == pEnt->pev->team )
			&& ( !bForceSameTeam || pEnt->pev->team == pev->team ) )
		{
			m_hObserverTarget = pEnt;
			if ( !m_hObserverTarget )
			{
				ALERT( at_console, "No observer targets.\n" );
				Observer_SetMode( OBS_ROAMING );
				return;
			}

			if ( pev->iuser1 == OBS_ROAMING )
				Observer_SetMode( OBS_CHASE_FREE );

			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
			UTIL_SetOrigin( pev, m_hObserverTarget->pev->origin );
			ALERT( at_console, "Now Tracking %s\n", STRING( m_hObserverTarget->pev->netname ) );

			if ( pev->iuser1 == OBS_IN_EYE && m_hObserverTarget )
				SendObserverTargetWeaponAnim();
		}
	}
}

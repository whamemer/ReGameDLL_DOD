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

===== lights.cpp ========================================================

  spawn and think functions for editor-placed lights

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

// DoD light states use the canonical enum STATE from cbase.h.
const char *GetStdLightStyle( int iStyle );
static const char *s_stdLightStyles[] = {
	"mmnmmommommnonmmonqnmmo",
	"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",
	"mamamamamama",
	"mamamamamama",
	"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
	"nmonqnmomnmomomno",
	"mmmaaaabcdefgmmmmaaaammmaamm",
	"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",
	"aaaaaaaazzzzzzzz",
	"mmamammmmammamamaaamammma",
	"abcdefghijklmnopqrrqponmlkjihgfedcba",
	"mmnnmmnnnmmnn",
	"a",
	"aabbccddeeffgghhiijjkkllmmmmmmmmmmmmmm",
	"abcdefghijklmmmmmmmmmmmmmmmmmmmmmmmmmm",
	"acegikmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
	"llkkjjiihhggffeeddccbbaaaaaaaaaaaaaaaa",
	"lkjihgfedcbaaaaaaaaaaaaaaaaaaaaaaaaaaa",
	"kigecaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
};

class CLight : public CPointEntity
{
  public:
	virtual void KeyValue( KeyValueData *pkvd );
	virtual void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

  public:
	int m_iStyle;
	BOOL m_bDefaultOn;
	STATE m_iState;
	int m_iOnStyle;
	int m_iOffStyle;
	int m_iTurnOnStyle;
	int m_iTurnOffStyle;
	int m_iTurnOnTime;
	int m_iTurnOffTime;
	int m_iszPattern;
	void SetCorrectStyle( void );
	STATE GetState( void );
	void RoundRespawn( void );
	void EXPORT Think( void );
};
LINK_ENTITY_TO_CLASS( light, CLight );

TYPEDESCRIPTION CLight::m_SaveData[] = {
	DEFINE_FIELD( CLight, m_iState, FIELD_INTEGER ),		DEFINE_FIELD( CLight, m_iStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iszPattern, FIELD_STRING ),		DEFINE_FIELD( CLight, m_iOnStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iOffStyle, FIELD_INTEGER ),		DEFINE_FIELD( CLight, m_iTurnOnStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iTurnOffStyle, FIELD_INTEGER ), DEFINE_FIELD( CLight, m_iTurnOnTime, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iTurnOffTime, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CLight, CPointEntity );

//
// Cache user-entity-field values until spawn is called.
//
void CLight::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "style" ) )
	{
		m_iStyle = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iOnStyle" ) )
	{
		m_iOnStyle = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iOffStyle" ) )
	{
		m_iOffStyle = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iTurnOnStyle" ) )
	{
		m_iTurnOnStyle = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iTurnOffStyle" ) )
	{
		m_iTurnOffStyle = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iTurnOnTime" ) )
	{
		m_iTurnOnTime = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iTurnOffTime" ) )
	{
		m_iTurnOffTime = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "pitch" ) )
	{
		pev->angles.x = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "pattern" ) )
	{
		m_iszPattern = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "firetarget" ) )
	{
		pev->target = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
	// binary 0x10bc10: unknown fHandled=0, no base
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) LIGHT_START_OFF
Non-displayed light.
Default light value is 300
Default style is 0
If targeted, it will toggle between on or off.
*/
void CLight::Spawn( void )
{
	if ( FStringNull( pev->targetname ) )
	{
		REMOVE_ENTITY( ENT( pev ) );
		return;
	}

	m_bDefaultOn = FBitSet( pev->spawnflags, SF_LIGHT_START_OFF ) ? FALSE : TRUE;
	m_iState = FBitSet( pev->spawnflags, SF_LIGHT_START_OFF ) ? STATE_OFF : STATE_ON;

	if ( m_iStyle >= 32 )
		SetCorrectStyle();

	RoundStore();
}
void CLight::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_iStyle < 32 )
		return;
	if ( !ShouldToggle( useType, m_iState ) )
		return;
	STATE state = GetState();
	if ( state == STATE_OFF || state == STATE_TURN_OFF )
	{
		if ( m_iTurnOnTime == 0 )
			m_iState = STATE_ON;
		else
		{
			m_iState = STATE_TURN_ON;
			pev->nextthink = gpGlobals->time + m_iTurnOnTime;
		}
	}
	else if ( state == STATE_ON || state == STATE_TURN_ON )
	{
		if ( m_iTurnOffTime == 0 )
			m_iState = STATE_OFF;
		else
		{
			m_iState = STATE_TURN_OFF;
			pev->nextthink = gpGlobals->time + m_iTurnOffTime;
		}
	}
	else
		return;
	if ( m_iStyle < 32 )
		return;
	SetCorrectStyle();
}

//
// shut up spawn functions for new spotlights
//
LINK_ENTITY_TO_CLASS( light_spot, CLight );

class CEnvLight : public CLight
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( light_environment, CEnvLight );
void CEnvLight::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "_light" ) )
	{
		int r, g, b, v, j;
		char szColor[64];
		j = sscanf( pkvd->szValue, "%d %d %d %d\n", &r, &g, &b, &v );
		if ( j == 1 )
		{
			g = b = r;
		}
		else if ( j == 4 )
		{
			r = r * ( v / 255.0 );
			g = g * ( v / 255.0 );
			b = b * ( v / 255.0 );
		}

		// simulate qrad direct, ambient,and gamma adjustments, as well as engine scaling
		r = pow( r / 114.0, 0.6 ) * 264;
		g = pow( g / 114.0, 0.6 ) * 264;
		b = pow( b / 114.0, 0.6 ) * 264;

		pkvd->fHandled = TRUE;
		sprintf( szColor, "%d", r );
		CVAR_SET_STRING( "sv_skycolor_r", szColor );
		sprintf( szColor, "%d", g );
		CVAR_SET_STRING( "sv_skycolor_g", szColor );
		sprintf( szColor, "%d", b );
		CVAR_SET_STRING( "sv_skycolor_b", szColor );
	}
	else if ( FStrEq( pkvd->szKeyName, "style" ) )
	{
		m_iStyle = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iOnStyle" ) )
	{
		m_iOnStyle = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CLight::KeyValue( pkvd );
}

void CEnvLight ::Spawn( void )
{
	char szVector[64];
	UTIL_MakeAimVectors( pev->angles );

	sprintf( szVector, "%f", gpGlobals->v_forward.x );
	CVAR_SET_STRING( "sv_skyvec_x", szVector );
	sprintf( szVector, "%f", gpGlobals->v_forward.y );
	CVAR_SET_STRING( "sv_skyvec_y", szVector );
	sprintf( szVector, "%f", gpGlobals->v_forward.z );
	CVAR_SET_STRING( "sv_skyvec_z", szVector );

	CLight::Spawn();
}

void CLight::Think( void )
{
	STATE state = GetState();
	if ( state == STATE_TURN_OFF )
	{
		m_iState = STATE_OFF;
		FireTargets( STRING( pev->target ), this, this, USE_OFF, 0 );
	}
	else if ( state == STATE_TURN_ON )
	{
		m_iState = STATE_ON;
		FireTargets( STRING( pev->target ), this, this, USE_ON, 0 );
	}
	if ( m_iStyle >= 32 )
		SetCorrectStyle();
}

STATE CLight::GetState( void )
{
	return m_iState;
}

void CLight::SetCorrectStyle( void )
{
	if ( m_iStyle < 32 )
		return;
	int style;
	if ( m_iState == STATE_TURN_ON )
	{
		style = m_iTurnOnStyle;
		if ( style == 0 )
		{
			LIGHT_STYLE( m_iStyle, "a" );
			return;
		}
	}
	else if ( m_iState > STATE_TURN_ON )
	{
		if ( m_iState == STATE_ON )
		{
			if ( m_iszPattern )
			{
				LIGHT_STYLE( m_iStyle, STRING( m_iszPattern ) );
				return;
			}
			style = m_iOnStyle;
			if ( style == 0 )
			{
				LIGHT_STYLE( m_iStyle, "m" );
				return;
			}
		}
		else
		{
			if ( m_iState != STATE_TURN_OFF )
				return;
			style = m_iTurnOffStyle;
			if ( style == 0 )
			{
				LIGHT_STYLE( m_iStyle, "m" );
				return;
			}
		}
	}
	else
	{
		if ( m_iState != STATE_OFF )
			return;
		style = m_iOffStyle;
		if ( style == 0 )
		{
			LIGHT_STYLE( m_iStyle, "a" );
			return;
		}
	}
	const char *pat = "m";
	if ( (unsigned) ( style - 1 ) < 0x13 )
		pat = GetStdLightStyle( style );
	LIGHT_STYLE( m_iStyle, pat );
}

void CLight::RoundRespawn( void )
{
	if ( !m_bDefaultOn )
	{
		LIGHT_STYLE( m_iStyle, "a" );
		return;
	}
	if ( !m_iszPattern )
	{
		LIGHT_STYLE( m_iStyle, "m" );
		return;
	}
	LIGHT_STYLE( m_iStyle, STRING( m_iszPattern ) );
}

const char *GetStdLightStyle( int iStyle )
{
	if ( (unsigned) ( iStyle - 1 ) < 19 )
		return s_stdLightStyles[iStyle - 1];  // CSWTCH_75
	return "m";
}

class CTriggerLightstyle : public CPointEntity
{
  public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Think( void );
	void KeyValue( KeyValueData *pkvd );
	int ObjectCaps( void );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
	int m_iStyle;
	int m_iszPattern;
	int m_iWait;
};

void CTriggerLightstyle::Spawn( void )
{
	return;
}

void CTriggerLightstyle::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( FStringNull( pev->target ) )
		return;
	CBaseEntity *pTarget = NULL;
	while ( ( pTarget = UTIL_FindEntityByTargetname( pTarget, STRING( pev->target ) ) ) != NULL )
	{
		if ( FNullEnt( pTarget->edict() ) || !ENTINDEX( pTarget->edict() ) )
			break;
		if ( !FClassnameIs( pTarget->pev, "light" ) && !FClassnameIs( pTarget->pev, "light_spot" ) &&
			 !FClassnameIs( pTarget->pev, "env_light" ) )
			continue;
		CLight *pLight = (CLight *) pTarget;
		if ( pLight->m_iStyle < 32 )
			continue;
		if ( m_iszPattern )
			LIGHT_STYLE( pLight->m_iStyle, STRING( m_iszPattern ) );
		else if ( m_iStyle != 0 )
		{
			const char *pat = ( m_iStyle - 1 < 19 ) ? GetStdLightStyle( m_iStyle ) : "m";
			LIGHT_STYLE( pLight->m_iStyle, (char *) pat );
		}
		if ( m_iWait != -1 )
			pev->nextthink = gpGlobals->time + m_iWait;
	}
}

void CTriggerLightstyle::Think( void )
{
	if ( FStringNull( pev->target ) )
		return;
	CBaseEntity *pTarget = NULL;
	while ( ( pTarget = UTIL_FindEntityByTargetname( pTarget, STRING( pev->target ) ) ) != NULL )
	{
		if ( FNullEnt( pTarget->edict() ) || !ENTINDEX( pTarget->edict() ) )
			return;
		if ( !FClassnameIs( pTarget->pev, "light" ) && !FClassnameIs( pTarget->pev, "light_spot" ) &&
			 !FClassnameIs( pTarget->pev, "env_light" ) )
			continue;
		if ( ( (CLight *) pTarget )->m_iStyle >= 32 )
			( (CLight *) pTarget )->SetCorrectStyle();
	}
}

void CTriggerLightstyle::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "style" ) )
	{
		m_iStyle = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "pattern" ) )
	{
		m_iszPattern = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iWait" ) )
	{
		m_iWait = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

int CTriggerLightstyle::ObjectCaps( void )
{
	return 0;
}

TYPEDESCRIPTION CTriggerLightstyle::m_SaveData[] = {
	DEFINE_FIELD( CTriggerLightstyle, m_iStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerLightstyle, m_iszPattern, FIELD_STRING ),
	DEFINE_FIELD( CTriggerLightstyle, m_iWait, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CTriggerLightstyle, CPointEntity );
LINK_ENTITY_TO_CLASS( trigger_lightstyle, CTriggerLightstyle );

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
#include "extdll.h"
#include "util.h"
#include "objective.h"
#include "player.h"
extern DLL_GLOBAL BOOL g_fGameOver;

extern int gmsgWeather;
extern int gmsgPShoot;
extern int gmsgPStatus;
extern cvar_t *pcvar_mp_clan_scoring;
extern cvar_t *pcvar_mp_clan_scoring_bonus_allies;
extern cvar_t *pcvar_mp_clan_scoring_bonus_axis;
extern void FireDODCamera( const char *p1, CBaseEntity *p2, CBaseEntity *p3, USE_TYPE t, float f );

extern int gmsgTimerStatus;

LINK_ENTITY_TO_CLASS( dod_object, CObject );

enum Object_Animations
{
	OBJECT_IDLE = 1,
};

void CObject::Spawn()
{
	Precache();
	pev->solid = SOLID_BBOX;		// 1
	pev->movetype = MOVETYPE_TOSS;	// 6
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	m_vStartOrigin = Vector( 0, 0, 0 );
	pev->sequence = OBJECT_IDLE;
	pev->framerate = 1.0;
	m_bCarried = 0;
	m_bDropped = 0;
	SetTouch( &CObject::ObjectTouch );
	SetThink( &CObject::ObjectThink );
	PRECACHE_MODEL( STRING( pev->model ) );
	SET_MODEL( edict(), STRING( pev->model ) );
	pev->nextthink = gpGlobals->time + 0.1;
	m_iObjectHIcon = 0;
	if ( m_szObjectHIcon[0] )
		m_iObjectHIcon = PRECACHE_MODEL( m_szObjectHIcon );
	RoundStore();
}

void CObject::Precache()
{
	if ( pev->noise )
		PRECACHE_SOUND( STRING( pev->noise ) );
	if ( pev->noise1 )
		PRECACHE_SOUND( STRING( pev->noise1 ) );
	if ( pev->noise2 )
		PRECACHE_SOUND( STRING( pev->noise2 ) );
	if ( pev->noise3 )
		PRECACHE_SOUND( STRING( pev->noise3 ) );
}

void CObject::RoundRespawn()
{
	ResetObject();
}

void CObject::ResetObject()
{
	if ( m_vStartOrigin.Length() == 0.0 )
		m_vStartOrigin = pev->origin;
	if ( UTIL_IsMasterTriggered( m_sMaster, this ) )
	{
		pev->effects &= ~EF_NODRAW;
		SetTouch( &CObject::ObjectTouch );
		SetThink( &CObject::ObjectThink );
		pev->nextthink = gpGlobals->time + 0.1;
		m_flNextTouchTime = gpGlobals->time + 1.0;
	}

	UTIL_SetOrigin( pev, m_vStartOrigin );
	m_bDropped = 0;
	m_bCarried = 0;
	if ( pev->noise3 )
		EMIT_SOUND_DYN( edict(), CHAN_STATIC, STRING( pev->noise3 ), 1.0, 0.8, 0, 100 );
	DROP_TO_FLOOR( edict() );
}

void CObject::Drop()
{
	if ( UTIL_IsMasterTriggered( m_sMaster, this ) )
	{
		pev->effects &= ~EF_NODRAW;
		SetTouch( &CObject::ObjectTouch );
		SetThink( &CObject::ObjectThink );
		pev->nextthink = gpGlobals->time + 0.1;
		m_flNextTouchTime = gpGlobals->time + 1.0;
	}

	m_bDropped = 1;
	m_flReset = gpGlobals->time + (float)m_iResetTime;
}

void CObject::Materialize()
{
	if ( UTIL_IsMasterTriggered( m_sMaster, this ) )
	{
		pev->effects &= ~EF_NODRAW;
		SetThink( &CObject::ObjectThink );
		SetTouch( &CObject::ObjectTouch );
		pev->nextthink = gpGlobals->time + 0.1;
		m_flNextTouchTime = gpGlobals->time + 1.0;
	}
}

void CObject::Disappear()
{
	pev->effects |= EF_NODRAW;
	SetTouch( NULL );
	if ( m_bCarried )
		SetThink( &CObject::AbsentPlayerThink );
	else
		SetThink( NULL );
	pev->nextthink = gpGlobals->time + 0.1;
	m_bDropped = 0;
}

// Enemy-team pickup: team-1 object taken by team 2 and vice versa, neutral
// by anyone. RoundState==1, master gate, 1.0 s touch throttle, player must
// be free-handed. GetObject, hide (EF_NODRAW), Touch=NULL,
// Think=AbsentPlayerThink 0.1 s, hint #Hint_picked_up_object, take sound.
// Same-team dropped object + object_resetontouch => ResetObject.
void CObject::ObjectTouch( CBaseEntity *pOther )
{
	if ( g_pGameRules->RoundState() != ROUNDSTATE_RUNNING )
		return;
	if ( !UTIL_IsMasterTriggered( m_sMaster, this ) )
		return;
	if ( m_flNextTouchTime > gpGlobals->time )
		return;
	if ( !pOther->IsPlayer() )
		return;
	if ( ( (CBasePlayer *) pOther )->m_pObject )
		return;
	int taker = 0;
	BOOL any = FALSE;
	if ( m_iTeam == TEAM_AXIS )
		taker = 1;
	else if ( m_iTeam == TEAM_ALLIES )
		taker = 2;
	else
		any = TRUE;
	if ( pOther->pev->team == taker || any )
	{
		( (CBasePlayer *) pOther )->GetObject( this );
		if ( pev->target )
			FireTargets( STRING( pev->target ), pOther, this, USE_TOGGLE, 0.0 );
		m_bCarried = 1;
		m_hOwner = pOther;
		pev->effects |= EF_NODRAW;
		SetTouch( NULL );
		SetThink( &CObject::AbsentPlayerThink );
		pev->nextthink = gpGlobals->time + 0.1;
		m_bDropped = 0;
		if ( ( ( (CBasePlayer *) pOther )->m_flDisplayHistory & HINT_PICKED_UP_OBJECT ) == 0 )
		{
			( (CBasePlayer *) pOther )->m_flDisplayHistory |= HINT_PICKED_UP_OBJECT;
			( (CBasePlayer *) pOther )->HintMessage( "#Hint_picked_up_object", 0 );
		}

		if ( pev->noise )
			EMIT_SOUND_DYN( pOther->edict(), CHAN_ITEM, STRING( pev->noise ), 1.0, 0.8, 0, 100 );
	}
	else if ( m_bDropped && m_iResetOnTouch )
		ResetObject();
}

void CObject::ObjectThink( void )
{
	if ( m_bDropped )
	{
		if ( m_iResetTime >= 1 && m_flReset <= gpGlobals->time )
			ResetObject();
	}

	pev->nextthink = gpGlobals->time + 0.1f;
}

void CObject::AbsentPlayerThink( void )
{
	if ( m_bDropped )
	{
		SetThink( &CObject::ObjectThink );
	}
	else if ( m_bCarried )
	{
		CBaseEntity *pCarrier = m_hOwner;
		if ( !pCarrier || !pCarrier->edict() || !g_engfuncs.pfnEntOffsetOfPEntity( pCarrier->edict() ) )
		{
			m_bDropped = 0;
			ResetObject();
		}
	}

	pev->nextthink = gpGlobals->time + 0.1f;
}

// CObject KeyValue fields (rest unhandled):
// object_name | object_owner => m_iTeam | object_main => m_iMain
// object_group | object_takesound => pev->noise
// object_capsound => pev->noise1 | object_dropsound => pev->noise2
// object_returnsound => pev->noise3 | object_carryspeed (float)
// object_resetontouch | object_cappoints => m_iCapPoints
// object_otherpoints => m_iOtherPoints | object_resettime (default 600,
// clamp 600) | object_donetarget | object_hud_sprite => m_szObjectHIcon
// object_player_sprite => m_szObjectPIcon
void CObject::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "object_name" ) )
	{
		strcpy( sz_Name, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_owner" ) )
	{
		m_iTeam = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_main" ) )
	{
		m_iMain = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_group" ) )
	{
		strcpy( sz_Group, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_takesound" ) )
	{
		if ( pkvd->szValue )
			pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_capsound" ) )
	{
		if ( pkvd->szValue )
			pev->noise1 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_dropsound" ) )
	{
		if ( pkvd->szValue )
			pev->noise2 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_returnsound" ) )
	{
		if ( pkvd->szValue )
			pev->noise3 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_carryspeed" ) )
	{
		m_fCarrySpeed = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_resetontouch" ) )
	{
		m_iResetOnTouch = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_cappoints" ) )
	{
		m_iCapPoints = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_otherpoints" ) )
	{
		m_iOtherPoints = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_resettime" ) )
	{
		int v = atoi( pkvd->szValue );
		if ( v > 600 )
			v = 600;
		m_iResetTime = v;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_donetarget" ) )
	{
		strcpy( sz_DoneTarget, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_player_sprite" ) )
	{
		strcpy( m_szObjectPIcon, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_hud_sprite" ) )
	{
		strcpy( m_szObjectHIcon, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CObject::SendDroppedMsg( CBasePlayer *pPlayer )
{
	if ( pPlayer && ( pev->spawnflags & SF_OBJECT_ANNOUNCE ) )
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_lost_object", STRING( pPlayer->pev->netname ), sz_Name );
}

void CObject::SendPickupMsg( CBasePlayer *pPlayer )
{
	if ( pPlayer && ( pev->spawnflags & SF_OBJECT_ANNOUNCE ) )
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_has_object", STRING( pPlayer->pev->netname ), sz_Name );
}

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

LINK_ENTITY_TO_CLASS( dod_score_ent, CScoreEvent );

void CScoreEvent::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	SetTouch( NULL );
	SetUse( &CScoreEvent::ScoreEventUse );
	SetThink( &CScoreEvent::ScoreThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CScoreEvent::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "score_resetitemtime" ) )
	{
		m_iResetItemsTime = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "score_points" ) )
	{
		m_iTeamPoints = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "score_resetplayerstime" ) )
	{
		m_iResetPlayersTime = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "score_nextmap" ) )
	{
		strcpy( szNextMap, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "score_nextmapdelay" ) )
	{
		m_iChangeLevelDelay = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CScoreEvent::ScoreThink()
{
	if ( ResetItems && ResetItemsTime <= gpGlobals->time )
	{
		CBaseEntity *pEnt = UTIL_FindEntityByClassname( NULL, "dod_object" );
		while ( pEnt )
		{
			CObject *pObj = (CObject *)pEnt;
			if ( !pev->noise || !strcmp( STRING( pev->noise ), pObj->sz_Group ) )
			{
				if ( UTIL_IsMasterTriggered( pObj->m_sMaster, pEnt ) )
				{
					pEnt->pev->effects &= ~EF_NODRAW;
					pEnt->SetTouch( &CObject::ObjectTouch );
					pEnt->SetThink( &CObject::ObjectThink );
					pEnt->pev->nextthink = gpGlobals->time + 0.1;
					pObj->m_flNextTouchTime = gpGlobals->time + 1.0;
				}
			}

			pEnt = UTIL_FindEntityByClassname( pEnt, "dod_object" );
		}

		ResetItems = 0;
	}

	if ( bChangeLevel && ChangeLevelTime <= gpGlobals->time )
	{
		char buf[320];
		sprintf( buf, "changelevel %s\n", szNextMap );
		SERVER_COMMAND( buf );
		ResetItems = 0;
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CScoreEvent::ScoreEventUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( pCaller && pCaller->IsPlayer() )
		return;
	if ( szNextMap[0] )
	{
		bChangeLevel = 1;
		ChangeLevelTime = (float)m_iChangeLevelDelay + gpGlobals->time;
	}

	if ( m_iResetPlayersTime )
	{
		int team = pev->team;
		if ( team == 0 )
			( (CDoDTeamPlay *)g_pGameRules )->SetRoundState( ROUNDSTATE_DRAW );
		else if ( team == TEAM_ALLIES )
			( (CDoDTeamPlay *)g_pGameRules )->SetRoundState( ROUNDSTATE_ALLIES_WON );
		else if ( team == TEAM_AXIS )
			( (CDoDTeamPlay *)g_pGameRules )->SetRoundState( ROUNDSTATE_AXIS_WON );

		g_pGameRules->SetRestartTime( (float)m_iResetPlayersTime + gpGlobals->time );
	}

	string_t msg = 0;
	if ( !m_iResetItemsTime )
		msg = pev->message;
	else
	{
		ResetItems = 1;
		ResetItemsTime = (float)m_iResetItemsTime + gpGlobals->time;
		msg = pev->message;
	}

	if ( msg )
		UTIL_ShowMessageAll( STRING( msg ), FALSE );
	// the clan scoring replaces the points of the entity
	BOOL bGiveRegular = TRUE;
	int team = pev->team;
	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;
	if ( pRules->IsClanMatch() && pRules->IsClanMatchActive() && pcvar_mp_clan_scoring )
	{
		if ( pcvar_mp_clan_scoring->value > 0.0 )
		{
			if ( team == TEAM_ALLIES && pcvar_mp_clan_scoring_bonus_allies && pcvar_mp_clan_scoring_bonus_allies->value >= 0.0 )
			{
				int pts = (int)pcvar_mp_clan_scoring_bonus_allies->value;
				if ( pts > 1000 )
					pts = 1000;
				pRules->AddToTeamScores( 1, pts );
				int allies = pRules->TeamScores( 1 );
				int axis = pRules->TeamScores( 2 );
				UTIL_LogPrintf( "Team \"%s\" won the round. Scores: Allies: %d, Axis: %d\n", "Allies", allies, axis );
				bGiveRegular = FALSE;
			}
			else if ( team == TEAM_AXIS && pcvar_mp_clan_scoring_bonus_axis && pcvar_mp_clan_scoring_bonus_axis->value >= 0.0 )
			{
				int pts = (int)pcvar_mp_clan_scoring_bonus_axis->value;
				if ( pts > 1000 )
					pts = 1000;
				pRules->AddToTeamScores( 2, pts );
				int allies = pRules->TeamScores( 1 );
				int axis = pRules->TeamScores( 2 );
				UTIL_LogPrintf( "Team \"%s\" won the round. Scores: Allies: %d, Axis: %d\n", "Axis", allies, axis );
				bGiveRegular = FALSE;
			}
		}
	}

	if ( m_iTeamPoints && bGiveRegular )
	{
		if ( team < 2 )
			pRules->AddToTeamScores( 1, m_iTeamPoints );
		if ( team == 0 || team == TEAM_AXIS )
			pRules->AddToTeamScores( 2, m_iTeamPoints );
		team = pev->team;
	}

	if ( team < 2 )
	{
		FireDODCamera( "allies_win_cut", NULL, NULL, USE_TOGGLE, 0.0 );
		FireDODCamera( "axis_loose_cut", NULL, NULL, USE_TOGGLE, 0.0 );
		team = pev->team;
	}

	if ( team == 0 || team == TEAM_AXIS )
	{
		FireDODCamera( "axis_win_cut", NULL, NULL, USE_TOGGLE, 0.0 );
		FireDODCamera( "allies_loose_cut", NULL, NULL, USE_TOGGLE, 0.0 );
	}

	pRules->ResetForDoDCamera();
	if ( pev->noise2 )
		g_pGameRules->EndMultiplayerGame();
}

LINK_ENTITY_TO_CLASS( particle_shooter, CParticleShooter );

void CParticleShooter::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "group_id" ) )
	{
		m_iGroupId = (BYTE)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "particle_speed" ) || FStrEq( pkvd->szKeyName, "m_flVelocity" ) )
	{
		m_flSpeed = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "num_particles" ) )
	{
		m_iNumParticles = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
}

void CParticleShooter::Precache()
{
	m_iSpriteIndex = PRECACHE_MODEL( STRING( pev->model ) );
}

void CParticleShooter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int newState = 0;
	if ( useType != USE_OFF )
	{
		if ( useType == USE_TOGGLE )
			newState = ( m_iState == 0 );
		else
			newState = 1;
	}

	if ( newState != m_iState )
	{
		m_iState = newState;
		FindTargetEntity();
		SendUpdate( NULL );
		if ( m_iNumParticles > -1 )
			m_iState = 0;
	}
}


void CParticleShooter::Spawn( void )
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	RoundStore();
	m_iState = 0;
}

void CParticleShooter::RoundRespawn()
{
	if ( pev->spawnflags & SF_PARTICLESHOOTER_STARTON )
	{
		m_iState = 1;
		FindTargetEntity();
		SendUpdate( NULL );
		if ( m_iNumParticles > -1 )
			m_iState = 0;
		return;
	}

	m_iState = 0;
	SendUpdate( NULL );
}

void CParticleShooter::FindTargetEntity()
{
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
	if ( !pTarget )
	{
		m_vShootDir = pev->velocity;
		return;
	}

	Vector vecDir = pTarget->pev->origin - pev->origin;
	float flLen = vecDir.Length();
	if ( flLen == 0.0 )
		m_vShootDir = Vector( 0, 0, m_flSpeed );
	else
		m_vShootDir = vecDir * ( m_flSpeed / flLen );
}

void CParticleShooter::UpdateJoiningPlayer( CBasePlayer *pPlayer )
{
	SendUpdate( pPlayer );
}

void CParticleShooter::SendUpdate( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		MESSAGE_BEGIN( MSG_ALL, gmsgPShoot );
	else
		MESSAGE_BEGIN( MSG_ONE, gmsgPShoot, NULL, pPlayer->edict() );
	WRITE_BYTE( m_iGroupId );
	WRITE_BYTE( m_iState );
	if ( m_iState == 1 )
	{
		WRITE_COORD( m_vShootDir.x );
		WRITE_COORD( m_vShootDir.y );
		WRITE_COORD( m_vShootDir.z );
	}

	MESSAGE_END();
}

void CWeatherSystem::Spawn( void )
{
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	pev->rendermode = 2;
	pev->renderamt = 0.0f;
	SetThink( &CWeatherSystem::InitThink );
	pev->nextthink = gpGlobals->time + 1.0f;
}

void CWeatherSystem::InitThink()
{
	MESSAGE_BEGIN( MSG_ALL, gmsgWeather );
	SendData();
	MESSAGE_END();
}

void CWeatherSystem::Precache()
{
	PRECACHE_MODEL( "sprites/white.spr" );
}

void CWeatherSystem::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_iType |= atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "density" ) )
	{
		m_flDensity = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "lightning" ) )
	{
		if ( atoi( pkvd->szValue ) > 0 )
			m_iType |= 4;
		pkvd->fHandled = TRUE;
	}
}

void CWeatherSystem::SendForecast( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgWeather, NULL, pPlayer->edict() );
	WRITE_BYTE( ENTINDEX( edict() ) );
	WRITE_BYTE( m_iType );
	MESSAGE_END();
}

void CWeatherSystem::SendData()
{
	WRITE_BYTE( ENTINDEX( edict() ) );
	WRITE_BYTE( m_iType );
}



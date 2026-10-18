// object_goals.cpp -- dod_object (CObject) + dod_object_goal (CObjectCapture)
// CObject: objectives.cpp. CObjectCapture: triggers.cpp.
// Restored from dod.so; Ghidra = link/file + 0x10000.
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

// CObject KeyValue fields (rest unhandled):
// object_name | object_owner => m_iTeam | object_main => m_iMain
// object_group | object_takesound => pev->noise
// object_capsound => pev->noise1 | object_dropsound => pev->noise2
// object_returnsound => pev->noise3 | object_carryspeed (float)
// object_resetontouch | object_cappoints => m_iCapPoints
// object_otherpoints => m_iOtherPoints | object_resettime (default 600,
// clamp 600) | object_donetarget | object_hud_sprite => m_szObjectHIcon
// object_player_sprite => m_szObjectPIcon
// .text:00120230 KeyValue (file:objectives.cpp:296)
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

// .text:0011fbc0 CObject::Spawn() (file:objectives.cpp, ~140)
void CObject::Spawn()
{
	Precache();
	pev->solid = SOLID_BBOX;		// 1
	pev->movetype = MOVETYPE_TOSS;	// 6
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	m_vStartOrigin = Vector( 0, 0, 0 );
	pev->sequence = 1;
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

// .text:0011f380 CObject::Precache() (file:objectives.cpp:89)
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

// .text:00120930 CObject::ObjectTouch (file:objectives.cpp:179)
// Enemy-team pickup: team-1 object taken by team 2 and vice versa, neutral
// by anyone. RoundState==1, master gate, 1.0 s touch throttle, player must
// be free-handed. GetObject, hide (EF_NODRAW), Touch=NULL,
// Think=AbsentPlayerThink 0.1 s, hint #Hint_picked_up_object, take sound.
// Same-team dropped object + object_resetontouch => ResetObject.
void CObject::ObjectTouch( CBaseEntity *pOther )
{
	if ( g_pGameRules->RoundState() != 1 )
		return;
	if ( !UTIL_IsMasterTriggered( m_sMaster, this ) )
		return;
	if ( m_flNextTouchTime > gpGlobals->time )
		return;
	if ( !pOther->IsPlayer() )
		return;
	if ( ( (CBasePlayer *) pOther )->IsCarryingObject() )
		return;
	int taker = 0;
	BOOL any = FALSE;
	if ( m_iTeam == 2 )
		taker = 1;
	else if ( m_iTeam == 1 )
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
		if ( ( ( (CBasePlayer *) pOther )->m_flDisplayHistory & 0x100 ) == 0 )
		{
			( (CBasePlayer *) pOther )->m_flDisplayHistory |= 0x100;
			( (CBasePlayer *) pOther )->HintMessageOnce( "#Hint_picked_up_object" );
		}
		if ( pev->noise )
			EMIT_SOUND_DYN( pOther->edict(), CHAN_ITEM, STRING( pev->noise ), 1.0, 0.8, 0, 100 );
	}
	else if ( m_bDropped && m_iResetOnTouch )
		ResetObject();
}

// .text:001208b0 ObjectThink link 0x1108b0 size 0x63 Ghidra 0x1208b0: dropped + resetTime>=1 + time>=m_flReset => ResetObject; nextthink=time+0.1
void CObject::ObjectThink( void )
{
	if ( m_bDropped )
	{
		if ( m_iResetTime >= 1 && m_flReset <= gpGlobals->time )
			ResetObject();
	}
	pev->nextthink = gpGlobals->time + 0.1f;
}

// .text:00120810 AbsentPlayerThink link 0x110810 size 0x96 Ghidra 0x120810: carrier gone => ResetObject; dropped => Think=ObjectThink; nextthink=time+0.1
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

// .text:00120690 CObject::ResetObject (file:objectives.cpp:109)
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

// .text:00120920 CObject::RoundRespawn (file:objectives.cpp:105, size5 thunk to ResetObject)
void CObject::RoundRespawn()
{
	ResetObject();
}

// .text:00120ba0 CObject::Drop (file:objectives.cpp:128)
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

// .text:00120c40 CObject::Materialize (file:objectives.cpp:140)
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

// .text:00120cc0 CObject::Disappear (file:objectives.cpp:154)
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

// .text:00120d40 CObject::SendDroppedMsg (file:objectives.cpp:394)
void CObject::SendDroppedMsg( CBasePlayer *pPlayer )
{
	if ( pPlayer && ( pev->spawnflags & 1 ) )
		UTIL_ClientPrintAll( 3, "#game_lost_object", STRING( pPlayer->pev->netname ), sz_Name );
}

// .text:00120db0 CObject::SendPickupMsg (file:objectives.cpp:402)
void CObject::SendPickupMsg( CBasePlayer *pPlayer )
{
	if ( pPlayer && ( pev->spawnflags & 1 ) )
		UTIL_ClientPrintAll( 3, "#game_has_object", STRING( pPlayer->pev->netname ), sz_Name );
}

// CObjectCapture KeyValue: capobj_group | capobj_donetarget |
// capobj_hud_sprite, rest => CBaseTrigger. .text:0015d5d0 (triggers.cpp:3609)
// .text:0014fbf0 Precache (triggers.cpp:3820): hud sprite.
// .text:00157260 Spawn (triggers.cpp:3646): InitTrigger, m_bCapped = false.
// .text:00157230 RoundRespawn (triggers.cpp:3656): re-init, m_bCapped = false.
// .text:00150b80 area_SetIndex / .text:00150c00 area_SendStatus
// (triggers.cpp:3849/3827): ClientAreas(index, -1, icon) when index <= 0x80.

// .text:00151310 CObjectCapture::Touch (file:triggers.cpp:3667)
// link 0x141310 size 0x56a Ghidra 0x151310
void CObjectCapture::Touch( CBaseEntity *pOther )
{
	if ( g_pGameRules->RoundState() != 1 )
		return;
	if ( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;
	if ( g_pGameRules != NULL && g_pGameRules->RoundState() != 1 )
		return;
	if ( !pOther->IsPlayer() || !pOther->IsAlive() )
		return;
	if ( m_bCapped )
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	pPlayer->m_signals.m_flSignal |= 4;
	pPlayer->m_iObjectAreaIndex = m_iAreaIndex;
	pPlayer->SetClientAreaIcon( m_iAreaIndex, true );
	if ( !pPlayer->HasObject || !pPlayer->m_pObject )
	{
		if ( ( pPlayer->m_flDisplayHistory & 0x8000 ) == 0 )
		{
			pPlayer->m_flDisplayHistory |= 0x8000;
			pPlayer->HintMessageOnce( "#Hint_area_requires_object" );
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
	if ( NumObjects == 0 || ( pev->spawnflags & 1 ) )
	{
		FireTargets( sz_DoneTarget, pOther, this, USE_ON, 0.0 );
		if ( pObj->sz_DoneTarget[0] )
			FireTargets( pObj->sz_DoneTarget, pOther, this, USE_ON, 0.0 );
		if ( pev->spawnflags & 2 )
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
			UTIL_ClientPrintAll( 3, "#game_left_object", UTIL_dtos1( 1 ), 0, 0, 0 );
		else
			UTIL_ClientPrintAll( 3, "#game_left_plural_object", UTIL_dtos1( NumObjects ), 0, 0, 0 );
		if ( pev->target )
			FireTargets( STRING( pev->target ), pOther, this, USE_TOGGLE, 0.0 );
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlr = (CBasePlayer *)UTIL_PlayerByIndex( i );
			if ( pPlr && pPlr->pev->team == pOther->pev->team && pPlr != pPlayer && pRules->m_RoundDoesCount )
				pPlr->AddObjScore( pObj->m_iOtherPoints, TRUE );
		}
	}
	const char *teamName = ( pOther->pev->team == 1 ) ? "Allies" : "Axis";
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"dod_object_goal\"\n", STRING( pOther->pev->netname ),
		GETPLAYERUSERID( pOther->edict() ), GETPLAYERAUTHID( pOther->edict() ), teamName );
	if ( pLast && pLast->pev->noise1 )
		EMIT_SOUND_DYN( pOther->edict(), CHAN_STATIC, STRING( ( (CObject *)pLast )->pev->noise1 ), 1.0, 0.0, 0, 100 );
	else if ( pObj->pev->noise1 )
		EMIT_SOUND_DYN( pOther->edict(), CHAN_STATIC, STRING( pObj->pev->noise1 ), 1.0, 0.0, 0, 100 );
	pPlayer->HasObject = FALSE;
	pPlayer->RemoveObjectIcon();
}

// .text:0011f480 CParticleShooter::Spawn link 0x10f480 size 0x3e Ghidra 0x11f480: Precache, solid=0, effects=0x80, RoundStore, m_iState=0
void CParticleShooter::Spawn( void )
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	RoundStore();
	m_iState = 0;
}

// .text:0011f440 CParticleShooter::Precache (file:objectives.cpp:688)
void CParticleShooter::Precache()
{
	m_iSpriteIndex = PRECACHE_MODEL( STRING( pev->model ) );
}

// .text:0011fe20 CParticleShooter::KeyValue (file:objectives.cpp:664)
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

// .text:00120f00 CParticleShooter::FindTargetEntity (file:objectives.cpp:755)
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

// .text:00121030 CParticleShooter::SendUpdate (file:objectives.cpp:775)
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

// .text:00121110 CParticleShooter::UpdateJoiningPlayer (file:objectives.cpp:771, size5 thunk to SendUpdate)
void CParticleShooter::UpdateJoiningPlayer( CBasePlayer *pPlayer )
{
	SendUpdate( pPlayer );
}

// .text:00121120 CParticleShooter::RoundRespawn (file:objectives.cpp:734)
void CParticleShooter::RoundRespawn()
{
	if ( pev->spawnflags & 32 )
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

// .text:00121190 CParticleShooter::Use (file:objectives.cpp:693)
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

// .text:0011f530 CWeatherSystem::Spawn link 0x10f530 size 0x96 Ghidra 0x11f530: SetModel, SetSize mins/maxs, rendermode=2, renderamt=0, Think=InitThink +1.0
void CWeatherSystem::Spawn( void )
{
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	pev->rendermode = 2;
	pev->renderamt = 0.0f;
	SetThink( &CWeatherSystem::InitThink );
	pev->nextthink = gpGlobals->time + 1.0f;
}

// .text:0011f4c0 CWeatherSystem::InitThink (file:objectives.cpp:813)
void CWeatherSystem::InitThink()
{
	MESSAGE_BEGIN( MSG_ALL, gmsgWeather );
	SendData();
	MESSAGE_END();
}

// .text:0011f520 CWeatherSystem::Precache (file:objectives.cpp:821)
void CWeatherSystem::Precache()
{
	PRECACHE_MODEL( "sprites/white.spr" );
}

// .text:0011fd20 CWeatherSystem::KeyValue (file:objectives.cpp:825)
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

// .text:00121200 CWeatherSystem::SendForecast (file:objectives.cpp:848)
void CWeatherSystem::SendForecast( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgWeather, NULL, pPlayer->edict() );
	WRITE_BYTE( ENTINDEX( edict() ) );
	WRITE_BYTE( m_iType );
	MESSAGE_END();
}

// .text:00121270 CWeatherSystem::SendData (file:objectives.cpp:855)
void CWeatherSystem::SendData()
{
	WRITE_BYTE( ENTINDEX( edict() ) );
	WRITE_BYTE( m_iType );
}

// .text:0011fb10 CScoreEvent::Spawn link 0x10fb10 size 0xad Ghidra 0x11fb10: solid=0 movetype=0 effects=0x80, origin/size0, Touch=NULL Use=ScoreEventUse Think=ScoreThink +0.1
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

// .text:0011ff20 CScoreEvent::KeyValue (file:objectives.cpp:463)
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

// .text:001200a0 CScoreEvent::ScoreThink (file:objectives.cpp:494)
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
		char buf[256];
		sprintf( buf, "changelevel %s\n", szNextMap );
		SERVER_COMMAND( buf );
		ResetItems = 0;
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

// .text:0011f5d0 CScoreEvent::ScoreEventUse (file:objectives.cpp:539)
void CScoreEvent::ScoreEventUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// slot 0x2d gate approx IsPlayer: ignore player callers
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
			( (CDoDTeamPlay *)g_pGameRules )->SetRoundState( 5 );
		else if ( team == 1 )
			( (CDoDTeamPlay *)g_pGameRules )->SetRoundState( 3 );
		else if ( team == 2 )
			( (CDoDTeamPlay *)g_pGameRules )->SetRoundState( 4 );
		// slot 0x41 single-float timer set approx restart time
		( (CDoDTeamPlay *)g_pGameRules )->m_flRestartTime = (float)m_iResetPlayersTime + gpGlobals->time;
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
	// clan vs regular scoring: slots 0x4e/0x4f approx IsClanMatch/IsTeamplay
	BOOL bGiveRegular = TRUE;
	int team = pev->team;
	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;
	if ( pRules->IsClanMatch() && pRules->IsTeamplay() && pcvar_mp_clan_scoring )
	{
		if ( pcvar_mp_clan_scoring->value > 0.0 )
		{
			if ( team == 1 && pcvar_mp_clan_scoring_bonus_allies && pcvar_mp_clan_scoring_bonus_allies->value >= 0.0 )
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
			else if ( team == 2 && pcvar_mp_clan_scoring_bonus_axis && pcvar_mp_clan_scoring_bonus_axis->value >= 0.0 )
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
		if ( team == 0 || team == 2 )
			pRules->AddToTeamScores( 2, m_iTeamPoints );
		team = pev->team;
	}
	if ( team < 2 )
	{
		FireDODCamera( "allies_win_cut", NULL, NULL, USE_TOGGLE, 0.0 );
		FireDODCamera( "axis_loose_cut", NULL, NULL, USE_TOGGLE, 0.0 );
		team = pev->team;
	}
	if ( team == 0 || team == 2 )
	{
		FireDODCamera( "axis_win_cut", NULL, NULL, USE_TOGGLE, 0.0 );
		FireDODCamera( "allies_loose_cut", NULL, NULL, USE_TOGGLE, 0.0 );
	}
	pRules->ResetForDoDCamera();
	// slot 0x3c no-arg end call approx GoToIntermission, gated by noise2
	if ( pev->noise2 )
		pRules->GoToIntermission();
}

// .text:00120620 LINK dod_object (file:objectives.cpp:36, size 0x63)
LINK_ENTITY_TO_CLASS( dod_object, CObject );
// .text:00120e20 LINK dod_score_ent (file:objectives.cpp:445, size 0x63)
LINK_ENTITY_TO_CLASS( dod_score_ent, CScoreEvent );
// .text:00120e90 LINK particle_shooter (file:objectives.cpp:661, size 0x63)
LINK_ENTITY_TO_CLASS( particle_shooter, CParticleShooter );

// .text:000de0b0 CDoDDetect::IsMasteredOn (file:objectives.cpp)
bool CDoDDetect::IsMasteredOn( void )
{
	if ( m_sMaster != 0 )
		return UTIL_IsMasterTriggered( m_sMaster, this ) > 0;
	return true;
}

extern int gmsgTimerStatus;

// .text:000e3b90 CDodParaRoundTimer::Spawn (file:objectives.cpp)
void CDodParaRoundTimer::Spawn( void )
{
	m_bTimer = false;
	pev->solid = SOLID_NOT;
	m_fUpdateTimer = 30.0f;
	m_fRoundTimer = 0.0f;
	m_fLastThinkTime = gpGlobals->time;
	SetThink( &CDodParaRoundTimer::TimerThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

// .text:000ee150 CDodParaRoundTimer::StartTimer
void CDodParaRoundTimer::StartTimer( void )
{
	m_bTimer = true;
	float t = m_fRoundTimer;
	if ( t <= 0.0f ) t = 0.0f;
	MESSAGE_BEGIN( MSG_BROADCAST, gmsgTimerStatus, NULL, (edict_t *)NULL );
	WRITE_BYTE( (int)m_bTimer );
	WRITE_COORD( t );
	MESSAGE_END();
	SetThink( &CDodParaRoundTimer::TimerThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

// .text:000ee1f0 CDodParaRoundTimer::StopTimer
void CDodParaRoundTimer::StopTimer( void )
{
	m_bTimer = false;
	float t = m_fRoundTimer;
	if ( t <= 0.0f ) t = 0.0f;
	MESSAGE_BEGIN( MSG_BROADCAST, gmsgTimerStatus, NULL, (edict_t *)NULL );
	WRITE_BYTE( (int)m_bTimer );
	WRITE_COORD( t );
	MESSAGE_END();
	SetThink( NULL );
}

// .text:000ee270 CDodParaRoundTimer::ResetTimer
void CDodParaRoundTimer::ResetTimer( void )
{
	m_fRoundTimer = 240.0f;
	m_bTimer = true;
	m_fUpdateTimer = 30.0f;
	MESSAGE_BEGIN( MSG_BROADCAST, gmsgTimerStatus, NULL, (edict_t *)NULL );
	WRITE_BYTE( (int)m_bTimer );
	WRITE_COORD( 240.0f );
	MESSAGE_END();
	SetThink( &CDodParaRoundTimer::TimerThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

// .text:000ee380 CDodParaRoundTimer::SendStatus
void CDodParaRoundTimer::SendStatus( CBasePlayer *pPlayer )
{
	float t = m_fRoundTimer;
	MESSAGE_BEGIN( MSG_ONE, gmsgTimerStatus, NULL, pPlayer->pev );
	WRITE_BYTE( (int)m_bTimer );
	if ( t <= 0.0f ) t = 0.0f;
	WRITE_COORD( t );
	MESSAGE_END();
}

// .text:000eaa80 CDodParaRoundTimer::TimerThink
void CDodParaRoundTimer::TimerThink( void )
{
	float now = gpGlobals->time;
	float last = m_fLastThinkTime;
	m_fLastThinkTime = now;
	pev->nextthink = now + 0.1f;

	if ( g_pGameRules->RoundState() != 1 )
	{
		if ( !m_bTimer )
			return;
		m_bTimer = false;
		MESSAGE_BEGIN( MSG_BROADCAST, gmsgTimerStatus, NULL, (edict_t *)NULL );
		WRITE_BYTE( (int)m_bTimer );
		WRITE_COORD( m_fRoundTimer > 0.0f ? m_fRoundTimer : 0.0f );
		MESSAGE_END();
		SetThink( NULL );
	}

	if ( m_bTimer )
	{
		m_fRoundTimer -= ( now - last );
		if ( m_fRoundTimer > 0.0f )
		{
			m_fUpdateTimer -= 0.1f;
			if ( m_fUpdateTimer <= 0.0f )
			{
				MESSAGE_BEGIN( MSG_BROADCAST, gmsgTimerStatus, NULL, (edict_t *)NULL );
				WRITE_BYTE( (int)m_bTimer );
				WRITE_COORD( m_fRoundTimer > 0.0f ? m_fRoundTimer : 0.0f );
				MESSAGE_END();
				m_fUpdateTimer = 30.0f;
			}
		}
		else
		{
			if ( g_pGameRules != NULL )
				((CDoDTeamPlay *)g_pGameRules)->TimerExpired();
			m_bTimer = false;
			MESSAGE_BEGIN( MSG_BROADCAST, gmsgTimerStatus, NULL, (edict_t *)NULL );
			WRITE_BYTE( (int)m_bTimer );
			WRITE_COORD( m_fRoundTimer > 0.0f ? m_fRoundTimer : 0.0f );
			MESSAGE_END();
			SetThink( NULL );
		}
	}
}

// .text:000ea0a0 CDoDDetect::Spawn
void CDoDDetect::Spawn( void )
{
	AddEntityHashValue( pev, "info_doddetect", CLASSNAME );
	pev->solid = SOLID_NOT;
	((CDoDTeamPlay *)g_pGameRules)->DetectGameRules();
}

// .text:000ee020 CDoDDetect::CDoDDetect
CDoDDetect::CDoDDetect( void )
{
	m_GamePlayRules.m_iTimerTeam = 0;
	m_GamePlayRules.m_sAxisElimTarget = 0;
	m_GamePlayRules.m_sAlliesElimTarget = 0;
	m_GamePlayRules.m_iPointsAlliesElim = 5;
	m_GamePlayRules.m_sTimerExpireTarget = 0;
	m_GamePlayRules.m_iPointsTimerExpires = 0;
	m_GamePlayRules.m_bAlliesInfiniteLives = true;
	m_GamePlayRules.m_bAxisInfiniteLives = true;
	m_GamePlayRules.m_bAlliesArePara = false;
	m_GamePlayRules.m_bAlliesAreBrit = false;
	m_GamePlayRules.m_bAxisArePara = false;
	m_GamePlayRules.m_bUseTimer = false;
	m_GamePlayRules.m_iPointsAxisElim = 5;
	m_GamePlayRules.m_fAlliesRespawnFactor = 1.0f;
	m_GamePlayRules.m_fAxisRespawnFactor = 1.0f;
}

// .text:000d5420 CDoDDetect::KeyValue (file:objectives.cpp)
void CDoDDetect::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "detect_axis_infinite" ) || FStrEq( pkvd->szKeyName, "axis_infinite_lifes" ) )
	{
		m_GamePlayRules.m_bAxisInfiniteLives = ( atoi( pkvd->szValue ) == 1 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_allies_infinite" ) || FStrEq( pkvd->szKeyName, "allies_infinite_lifes" ) )
	{
		m_GamePlayRules.m_bAlliesInfiniteLives = ( atoi( pkvd->szValue ) == 1 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_axis_paras" ) )
	{
		m_GamePlayRules.m_bAxisArePara = ( atoi( pkvd->szValue ) > 0 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_allies_country" ) )
	{
		m_GamePlayRules.m_bAlliesAreBrit = ( atoi( pkvd->szValue ) == 1 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_allies_paras" ) )
	{
		m_GamePlayRules.m_bAlliesArePara = ( atoi( pkvd->szValue ) > 0 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_points_allieseliminated" ) )
	{
		m_GamePlayRules.m_iPointsAlliesElim = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_points_axiseliminated" ) )
	{
		m_GamePlayRules.m_iPointsAxisElim = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_target_allieseliminated" ) )
	{
		m_GamePlayRules.m_sAlliesElimTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_target_axiseliminated" ) )
	{
		m_GamePlayRules.m_sAxisElimTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_use_timer" ) )
	{
		m_GamePlayRules.m_bUseTimer = ( atoi( pkvd->szValue ) > 0 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_target_timerexpired" ) )
	{
		m_GamePlayRules.m_sTimerExpireTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_timer_team" ) )
	{
		m_GamePlayRules.m_iTimerTeam = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_points_timerexpired" ) )
	{
		m_GamePlayRules.m_iPointsTimerExpires = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_allies_respawnfactor" ) )
	{
		m_GamePlayRules.m_fAlliesRespawnFactor = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_axis_respawnfactor" ) )
	{
		m_GamePlayRules.m_fAxisRespawnFactor = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

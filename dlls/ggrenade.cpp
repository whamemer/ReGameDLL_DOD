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
 
===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "gamerules.h"
#include "player.h"
#include "func_break.h"
#include "combat.h"

//===================grenade

LINK_ENTITY_TO_CLASS( grenade, CGrenade );

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE 0x0001

//
// Grenade Explode
//
void CGrenade::Explode( Vector vecSrc, Vector vecAim )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -32 ), ignore_monsters, ENT( pev ), &tr );

	Explode( &tr, DMG_BLAST | DMG_DOD_NOBLOOD );
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CGrenade::Explode( TraceResult *pTrace, int bitsDamageType )
{
	pev->model = iStringNull;  //invisible
	pev->solid = SOLID_NOT;	   // intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( UTIL_PointContents( pTrace->vecEndPos ) == CONTENTS_SOLID )
	{
		pev->origin = pTrace->vecEndPos + ( pTrace->vecPlaneNormal * 8 );
	}

	// the grenade of a spectator, or of a player who changed the team, does no damage
	int iOwnerTeam = CBaseEntity::Instance( pev->owner )->pev->team;

	if ( iOwnerTeam == 3 )
	{
		pev->dmg = 0;
	}
	else if ( FClassnameIs( pev, "grenade" ) )
	{
		if ( iOwnerTeam != pev->team )
			pev->dmg = 0;
	}
	else if ( FClassnameIs( pev, "grenade2" ) && iOwnerTeam != pev->team )
	{
		pev->dmg = 0;
	}

	int iContents = UTIL_PointContents( pev->origin );

	PLAYBACK_EVENT_FULL( 0, ENT( pev ), g_pGameRules->Explosion(), 0, (float *)&g_vecZero, (float *)&g_vecZero,
						 ( pev->dmg - 100 ) * 0.6, 10.0, g_sModelIndexFireball, 0, iContents == CONTENTS_WATER,
						 bitsDamageType & DMG_MORTAR );

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL;	// can't traceline attack owner if this is set

	RadiusDamage( pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType );

	if ( RANDOM_FLOAT( 0, 1 ) < 0.5 )
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	}

	PLAYBACK_EVENT_FULL( 0, NULL, g_pGameRules->SparkShower(), 0, (float *)&pev->origin, (float *)&g_vecZero, 0, 0, 0, 0,
						 0, 0 );

	pev->effects |= EF_NODRAW;
	pev->velocity = g_vecZero;

	UTIL_Remove( this );
}

void CGrenade::Smoke( void )
{
}

void CGrenade::Killed( entvars_t *pevAttacker, int iGib )
{
	Detonate();
}

// Timed grenade, this think is called when time runs out.
void CGrenade::DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time;
}

void CGrenade::PreDetonate( void )
{
	CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, 400, 0.3 );

	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time + 1;
}

void CGrenade::Detonate( void )
{
	TraceResult tr;
	Vector vecSpot;	 // trace starts here!

	vecSpot = pev->origin + Vector( 0, 0, 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, ENT( pev ), &tr );

	UTIL_ScreenShake( tr.vecEndPos, 25.0, 150.0, 1.0, 750 );

	Explode( &tr, DMG_BLAST );
}

//
// Contact grenade, explode when it touches something
//
void CGrenade::ExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector vecSpot;	 // trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT( pev ), &tr );

	Explode( &tr, DMG_BLAST );
}

void CGrenade::DangerSoundThink( void )
{
	if ( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, pev->velocity.Length(), 0.2 );
	pev->nextthink = gpGlobals->time + 0.2;

	if ( pev->waterlevel != 0 )
	{
		pev->velocity = pev->velocity * 0.5;
	}
}

void CGrenade::BounceTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;

	pev->avelocity.x = 0;

	// the grenade rolls with the angles it was thrown with
	pev->angles = pev->vuser1;

	// only do damage if we're moving fairly fast
	if ( m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100 )
	{
		if ( pev->owner )
		{
			entvars_t *pevOwner = VARS( pev->owner );

			// the glass lets the grenade through and breaks
			if ( FClassnameIs( pOther->pev, "func_breakable" ) )
			{
				CBreakable *pBreakable = (CBreakable *)CBaseEntity::Instance( pOther->edict() );

				if ( pBreakable->IsBreakable() && pBreakable->m_Material == matGlass )
				{
					m_iOldMoveType = pev->movetype;
					pev->movetype = MOVETYPE_NOCLIP;
					m_flClipTime = (int)( gpGlobals->time + 0.5 );

					TraceResult tr = UTIL_GetGlobalTrace();
					ClearMultiDamage();
					pOther->TraceAttack( pevOwner, 1, gpGlobals->v_forward, &tr, DMG_DOD_NOBLOOD );
					ApplyMultiDamage( pev, pevOwner );
					return;
				}
			}

			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			pOther->TraceAttack( pevOwner, 1, gpGlobals->v_forward, &tr, DMG_DOD_NOBLOOD );
			ApplyMultiDamage( pev, pevOwner );
		}
		m_flNextAttack = gpGlobals->time + 1.0;	 // debounce
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity.
	// trimming the Z velocity a bit seems to help quite a bit.
	if ( !m_fRegisteredSound )
	{
		vecTestVelocity = pev->velocity;
		vecTestVelocity.z *= 0.45;

		if ( vecTestVelocity.Length() <= 60 )
		{
			// grenade is moving really slow. It's probably very close to where it will ultimately stop moving.
			// go ahead and emit the danger sound.

			// register a radius louder than the explosion, so we make sure everyone gets out of the way
			CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, pev->dmg / 0.4, 0.3 );
			m_fRegisteredSound = TRUE;
		}
	}

	if ( pev->flags & FL_ONGROUND )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		pev->sequence = RANDOM_LONG( 1, 1 );
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if ( pev->framerate > 1.0 )
		pev->framerate = 1;
	else if ( pev->framerate < 0.5 )
		pev->framerate = 0;
}

void CGrenade::SlideTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;

	// pev->avelocity = Vector (300, 300, 300);

	if ( pev->flags & FL_ONGROUND )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;

		if ( pev->velocity.x != 0 || pev->velocity.y != 0 )
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CGrenade ::BounceSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM );
		break;
	}
}

void CGrenade ::TumbleThink( void )
{
	if ( !IsInWorld() || pev->iuser1 == 1 )
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	// the grenade passed the glass, give its movement back
	if ( pev->movetype == MOVETYPE_NOCLIP && gpGlobals->time > m_flClipTime )
	{
		pev->movetype = m_iOldMoveType;
		pev->velocity = pev->velocity + gpGlobals->v_forward * 50;
	}

	if ( pev->dmgtime - 1 < gpGlobals->time )
	{
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin + pev->velocity * ( pev->dmgtime - gpGlobals->time ),
								400, 0.1 );

		if ( pev->dmgtime <= gpGlobals->time )
		{
			SetThink( &CGrenade::Detonate );
		}
	}
	if ( pev->waterlevel != 0 )
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}

void CGrenade::Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCE;

	// the stick grenade has its own classname
	if ( g_GrenType == G_STICKGRENADE )
		MAKE_STRING_CLASS( "grenade2", pev );
	else
		MAKE_STRING_CLASS( "grenade", pev );

	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/w_grenade.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->dmg = 100;
	m_fRegisteredSound = FALSE;
	pev->iuser1 = 0;
}

CGrenade *CGrenade::ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *) NULL );
	pGrenade->Spawn();
	// contact grenades arc lower
	pGrenade->pev->gravity = 0.5;  // lower gravity since grenade is aerodynamic and engine doesn't know it.
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles( pGrenade->pev->velocity );
	pGrenade->pev->owner = ENT( pevOwner );

	// make monsters afaid of it while in the air
	pGrenade->SetThink( &CGrenade::DangerSoundThink );
	pGrenade->pev->nextthink = gpGlobals->time;

	// Tumble in air
	pGrenade->pev->avelocity.x = RANDOM_FLOAT( -100, -500 );

	// Explode on contact
	pGrenade->SetTouch( &CGrenade::ExplodeTouch );

	pGrenade->pev->dmg = gSkillData.plrDmgM203Grenade;

	return pGrenade;
}

CGrenade *CGrenade::ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *) NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles( pGrenade->pev->velocity );
	pGrenade->pev->owner = ENT( pevOwner );

	pGrenade->SetTouch( &CGrenade::BounceTouch );  // Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed().

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( &CGrenade::TumbleThink );
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if ( time < 0.1 )
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector( 0, 0, 0 );
	}

	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1.0;

	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8;

	SET_MODEL( ENT( pGrenade->pev ), "models/w_grenade.mdl" );
	pGrenade->pev->dmg = 100;

	return pGrenade;
}

void CGrenade ::UseSatchelCharges( entvars_t *pevOwner, SATCHELCODE code )
{
	edict_t *pentFind;
	edict_t *pentOwner;

	if ( !pevOwner )
		return;

	CBaseEntity *pOwner = CBaseEntity::Instance( pevOwner );

	pentOwner = pOwner->edict();

	pentFind = FIND_ENTITY_BY_CLASSNAME( NULL, "grenade" );
	while ( !FNullEnt( pentFind ) )
	{
		CBaseEntity *pEnt = Instance( pentFind );
		if ( pEnt )
		{
			if ( FBitSet( pEnt->pev->spawnflags, SF_DETONATE ) && pEnt->pev->owner == pentOwner )
			{
				if ( code == SATCHEL_DETONATE )
					pEnt->Use( pOwner, pOwner, USE_ON, 0 );
				else  // SATCHEL_RELEASE
					pEnt->pev->owner = NULL;
			}
		}
		pentFind = FIND_ENTITY_BY_CLASSNAME( pentFind, "grenade" );
	}
}

//======================end grenade

void CGrenade::Precache( void )
{
	m_iBodyGibs = PRECACHE_MODEL( "models/rockgibs.mdl" );
}

void CGrenade::RemoveMe( void )
{
	pev->nextthink = gpGlobals->time;
	pev->model = iStringNull;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->effects |= EF_NODRAW;
	pev->velocity = g_vecZero;
	UTIL_Remove( this );
}

// DoD grenade touches (restore/weapons/ggrenade.cpp: MortarShell/Rocket/ShootPickup PORTED)
// SDK base (Bounce/Slide/Explode/Smoke/Tumble/Danger) untouched above.
#include "func_break.h"
#include "combat.h"
#include "enginecallback.h"
void CGrenade::MortarShellExplodeTouch( CBaseEntity *pOther )
{
	if ( FClassnameIs( pOther->pev, "func_breakable" ) )
	{
		edict_t *pentBreak = pOther->pev->pContainingEntity;
		if ( !pentBreak )
			pentBreak = ( *g_engfuncs.pfnPEntityOfEntOffset )( 0 );
		CBreakable *pBreak = NULL;
		if ( pentBreak )
			pBreak = (CBreakable *)GET_PRIVATE( pentBreak );
		if ( pBreak && pBreak->m_Material == matGlass )
		{
			if ( !pBreak->IsBreakable() )
				return;
			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			entvars_t *pevOwner = pev->owner ? VARS( pev->owner ) : NULL;
			pOther->TraceAttack( pevOwner, 1.0, gpGlobals->v_forward, &tr, DMG_DOD_NOBLOOD );
			ApplyMultiDamage( pev, pevOwner );
			UTIL_MakeVectors( pev->angles );
			CBaseEntity *pOwnerEnt = pev->owner ? CBaseEntity::Instance( pev->owner ) : NULL;
			CBaseEntity *pNewRocket = NULL;
			if ( FClassnameIs( pev, "shell_bazooka" ) )
				pNewRocket = CBazookaShell::CreateRocket( pev->origin, gpGlobals->v_forward, pOwnerEnt );
			else if ( FClassnameIs( pev, "shell_piat" ) )
				pNewRocket = CPIATShell::CreateRocket( pev->origin, gpGlobals->v_forward, pOwnerEnt );
			else if ( FClassnameIs( pev, "shell_pschreck" ) )
				pNewRocket = CPschreckShell::CreateRocket( pev->origin, gpGlobals->v_forward, pOwnerEnt );
			else
			{
				UTIL_Remove( this );
				return;
			}
			pNewRocket->pev->velocity = pev->velocity;
			UTIL_Remove( this );
			return;
		}
	}
	pev->enemy = pOther->edict();
	Vector vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	TraceResult tr;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, dont_ignore_monsters, ENT( pev ), &tr );
	const char *pTexture = TRACE_TEXTURE( ENT( 0 ), vecSpot, vecSpot + pev->velocity.Normalize() * 64 );
	if ( pTexture && !strnicmp( pTexture, "sky", 3 ) )
	{
		UTIL_Remove( this );
		return;
	}
	Explode( &tr, DMG_BLAST | DMG_SONIC | DMG_MORTAR );
}
void CGrenade::RocketExplodeTouch( CBaseEntity *pOther )
{
	if ( FClassnameIs( pOther->pev, "func_breakable" ) )
	{
		edict_t *pentBreak = pOther->pev->pContainingEntity;
		if ( !pentBreak )
			pentBreak = ( *g_engfuncs.pfnPEntityOfEntOffset )( 0 );
		CBreakable *pBreak = NULL;
		if ( pentBreak )
			pBreak = (CBreakable *)GET_PRIVATE( pentBreak );
		if ( pBreak && pBreak->m_Material == matGlass )
		{
			if ( !pBreak->IsBreakable() )
				return;
			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			entvars_t *pevOwner = pev->owner ? VARS( pev->owner ) : NULL;
			pOther->TraceAttack( pevOwner, 1.0, gpGlobals->v_forward, &tr, DMG_DOD_NOBLOOD );
			ApplyMultiDamage( pev, pevOwner );
			UTIL_MakeVectors( pev->angles );
			if ( pOther->pev->health > 0.0f )
			{
				UTIL_Remove( this );
				return;
			}
			CBaseEntity *pOwnerEnt = pev->owner ? CBaseEntity::Instance( pev->owner ) : NULL;
			CBaseEntity *pNewRocket = NULL;
			if ( FClassnameIs( pev, "shell_bazooka" ) )
				pNewRocket = CBazookaShell::CreateRocket( pev->origin, gpGlobals->v_forward, pOwnerEnt );
			else if ( FClassnameIs( pev, "shell_piat" ) )
				pNewRocket = CPIATShell::CreateRocket( pev->origin, gpGlobals->v_forward, pOwnerEnt );
			else if ( FClassnameIs( pev, "shell_pschreck" ) )
				pNewRocket = CPschreckShell::CreateRocket( pev->origin, gpGlobals->v_forward, pOwnerEnt );
			else
			{
				UTIL_Remove( this );
				return;
			}
			pNewRocket->pev->velocity = pev->velocity;
			UTIL_Remove( this );
			return;
		}
	}
	pev->enemy = pOther->edict();
	Vector vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	TraceResult tr;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, dont_ignore_monsters, ENT( pev ), &tr );
	const char *pTexture = TRACE_TEXTURE( ENT( 0 ), vecSpot, vecSpot + pev->velocity.Normalize() * 64 );
	if ( pTexture && !strnicmp( pTexture, "sky", 3 ) )
	{
		UTIL_Remove( this );
		return;
	}
	Explode( &tr, DMG_BLAST | DMG_SONIC );
}
CGrenade *CGrenade::ShootPickup( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, grenType value, float fDelay )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = pevOwner->angles;
	pGrenade->pev->vuser1 = pevOwner->angles;
	pGrenade->pev->owner = ENT( pevOwner );
	pGrenade->pev->team = pevOwner->team;
	pGrenade->pev->iuser2 = pevOwner->team;
	pGrenade->SetTouch( &CGrenade::BounceTouch );
	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( &CGrenade::TumbleThink );
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if ( time < 0.1 )
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector( 0, 0, 0 );
	}
	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1.0;
	pGrenade->pev->avelocity.x = -400.0;
	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = fDelay;
	if ( value == 0 )
	{
		SET_MODEL( ENT( pGrenade->pev ), "models/w_mills.mdl" );
		pGrenade->pev->dmg = gSkillData.plrDmgHandGrenade;
	}
	else if ( value == 1 )
	{
		SET_MODEL( ENT( pGrenade->pev ), "models/w_grenade.mdl" );
		pGrenade->pev->dmg = gSkillData.plrDmgHandGrenade;
	}
	else
	{
		SET_MODEL( ENT( pGrenade->pev ), "models/w_stick.mdl" );
		pGrenade->pev->dmg = gSkillData.plrDmgStickGrenade;
	}
	return pGrenade;
}
// NOTE(DOD): DetonateUse/PreDetonate/UseSatchelCharges/ShootTimed/ShootContact keep SDK bodies
// above (SDK-IDENTICAL per restore, DoD speed-damage tweaks owned by barrel balance pass).

void CGrenade::PickupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !pActivator->IsPlayer() )
		return;
	if ( pActivator->pev->iuser3 == 2 )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
	if ( pPlayer->PlayerHasExplodingGrenades() )
		return;

	if ( (int)g_GrenType >= 0 )
	{
		if ( (int)g_GrenType < 2 )
			pPlayer->GiveNamedItem( "weapon_handgrenade_ex" );
		else if ( g_GrenType == G_STICKGRENADE )
			pPlayer->GiveNamedItem( "weapon_stickgrenade_ex" );
	}

	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		CBasePlayerItem *pWeapon = pPlayer->m_rgpPlayerItems[i];
		while ( pWeapon != NULL )
		{
			if ( FClassnameIs( pWeapon->pev, "weapon_stickgrenade_ex" ) ||
				 FClassnameIs( pWeapon->pev, "weapon_handgrenade_ex" ) )
			{
				pPlayer->SwitchWeapon( pWeapon, 0 );

				// the picked up grenade goes on burning in the hand
				if ( pPlayer->m_pActiveItem )
					pPlayer->m_pActiveItem->SetDmgTime( pev->dmgtime );

				UTIL_Remove( this );
				break;
			}
			pWeapon = pWeapon->m_pNext;
		}
	}
}

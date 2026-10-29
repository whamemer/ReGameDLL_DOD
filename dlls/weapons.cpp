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

===== weapons.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "gamerules.h"

extern CGraph WorldGraph;
extern int gEvilImpulse101;

extern int gmsgCurWeapon;

#define NOT_USED 255

DLL_GLOBAL short g_sModelIndexLaser;  // holds the index for the laser beam
DLL_GLOBAL const char *g_pModelNameLaser = "sprites/laserbeam.spr";
DLL_GLOBAL short g_sModelIndexLaserDot;	   // holds the index for the laser beam dot
DLL_GLOBAL short g_sModelIndexFireball;	   // holds the index for the fireball
DLL_GLOBAL short g_sModelIndexSmoke;	   // holds the index for the smoke cloud
DLL_GLOBAL short g_sModelIndexBubbles;	   // holds the index for the bubbles model
DLL_GLOBAL short g_sModelIndexBloodDrop;   // holds the sprite index for the initial blood
DLL_GLOBAL short g_sModelIndexBloodSpray;  // holds the sprite index for splattered blood
unsigned short m_usPlayWhizz;

ItemInfo CBasePlayerItem::ItemInfoArray[MAX_WEAPONS];
AmmoInfo CBasePlayerItem::AmmoInfoArray[MAX_AMMO_SLOTS];

MULTIDAMAGE gMultiDamage;

#define TRACER_FREQ 4  // Tracers fire every fourth bullet


//=========================================================
// MaxAmmoCarry - pass in a name and this function will tell
// you the maximum amount of that type of ammunition that a
// player can carry.
//=========================================================
int MaxAmmoCarry( int iszName )
{
	for ( int i = 0; i < 64; i++ )
	{
		if ( CBasePlayerItem::ItemInfoArray[i].pszAmmo1 &&
			 !strcmp( STRING( iszName ), CBasePlayerItem::ItemInfoArray[i].pszAmmo1 ) )
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo1;
		if ( CBasePlayerItem::ItemInfoArray[i].pszAmmo2 &&
			 !strcmp( STRING( iszName ), CBasePlayerItem::ItemInfoArray[i].pszAmmo2 ) )
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo2;
	}
	ALERT( at_console, "MaxAmmoCarry() doesn't recognize '%s'!\n", STRING( iszName ) );
	return -1;
}

/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/

//
// ClearMultiDamage - resets the global multi damage accumulator
//
void ClearMultiDamage( void )
{
	gMultiDamage.pEntity = NULL;
	gMultiDamage.amount = 0;
	gMultiDamage.type = 0;
}

//
// ApplyMultiDamage - inflicts contents of global multi damage register on gMultiDamage.pEntity
//
// GLOBALS USED:
//		gMultiDamage

void ApplyMultiDamage( entvars_t *pevInflictor, entvars_t *pevAttacker )
{
	Vector vecSpot1;  //where blood comes from
	Vector vecDir;	  //direction blood should go
	TraceResult tr;

	if ( !gMultiDamage.pEntity )
		return;

	gMultiDamage.pEntity->TakeDamage( pevInflictor, pevAttacker, gMultiDamage.amount, gMultiDamage.type );
}

// GLOBALS USED:
//		gMultiDamage

void AddMultiDamage( entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType )
{
	if ( !pEntity )
		return;

	gMultiDamage.type |= bitsDamageType;

	if ( pEntity != gMultiDamage.pEntity )
	{
		ApplyMultiDamage( pevInflictor, pevInflictor );	 // UNDONE: wrong attacker!
		gMultiDamage.pEntity = pEntity;
		gMultiDamage.amount = 0;
	}

	gMultiDamage.amount += flDamage;
}

/*
================
SpawnBlood
================
*/
void SpawnBlood( Vector vecSpot, int bloodColor, float flDamage )
{
	UTIL_BloodDrips( vecSpot, g_vecAttackDir, bloodColor, (int) flDamage );
}

int DamageDecal( CBaseEntity *pEntity, int bitsDamageType )
{
	if ( !pEntity )
		return ( DECAL_GUNSHOT1 + RANDOM_LONG( 0, 4 ) );

	return pEntity->DamageDecal( bitsDamageType );
}

void DecalGunshot( TraceResult *pTrace, int iBulletType )
{
	// Is the entity valid
	if ( !UTIL_IsValidEntity( pTrace->pHit ) )
		return;

	if ( VARS( pTrace->pHit )->solid == SOLID_BSP || VARS( pTrace->pHit )->movetype == MOVETYPE_PUSHSTEP )
	{
		CBaseEntity *pEntity = NULL;
		// Decal the wall with a gunshot
		if ( !FNullEnt( pTrace->pHit ) )
			pEntity = CBaseEntity::Instance( pTrace->pHit );

		switch ( iBulletType )
		{
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		default:
			// smoke and decal
			UTIL_GunshotDecalTrace( pTrace, DamageDecal( pEntity, DMG_BULLET ) );
			break;
		case BULLET_MONSTER_12MM:
			// smoke and decal
			UTIL_GunshotDecalTrace( pTrace, DamageDecal( pEntity, DMG_BULLET ) );
			break;
		case BULLET_PLAYER_CROWBAR:
			// wall decal
			UTIL_DecalTrace( pTrace, DamageDecal( pEntity, DMG_CLUB ) );
			break;
		}
	}
}

//
// EjectBrass - tosses a brass shell from passed origin at passed velocity
//
void EjectBrass( const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype )
{
	// FIX: when the player shoots, their gun isn't in the same position as it is on the model other players see.

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
	WRITE_BYTE( TE_MODEL );
	WRITE_COORD( vecOrigin.x );
	WRITE_COORD( vecOrigin.y );
	WRITE_COORD( vecOrigin.z );
	WRITE_COORD( vecVelocity.x );
	WRITE_COORD( vecVelocity.y );
	WRITE_COORD( vecVelocity.z );
	WRITE_ANGLE( rotation );
	WRITE_SHORT( model );
	WRITE_BYTE( soundtype );
	WRITE_BYTE( 25 );  // 2.5 seconds
	MESSAGE_END();
}

#if 0
// UNDONE: This is no longer used?
void ExplodeModel( const Vector &vecOrigin, float speed, int model, int count )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
		WRITE_BYTE ( TE_EXPLODEMODEL );
		WRITE_COORD( vecOrigin.x );
		WRITE_COORD( vecOrigin.y );
		WRITE_COORD( vecOrigin.z );
		WRITE_COORD( speed );
		WRITE_SHORT( model );
		WRITE_SHORT( count );
		WRITE_BYTE ( 15 );// 1.5 seconds
	MESSAGE_END();
}
#endif

int giAmmoIndex = 0;

// Precaches the ammo and queues the ammo info for sending to clients
void AddAmmoNameToAmmoRegistry( const char *szAmmoname )
{
	// make sure it's not already in the registry
	for ( int i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		if ( !CBasePlayerItem::AmmoInfoArray[i].pszName )
			continue;

		if ( stricmp( CBasePlayerItem::AmmoInfoArray[i].pszName, szAmmoname ) == 0 )
			return;	 // ammo already in registry, just quite
	}

	giAmmoIndex++;
	ASSERT( giAmmoIndex < MAX_AMMO_SLOTS );
	if ( giAmmoIndex >= MAX_AMMO_SLOTS )
		giAmmoIndex = 0;

	CBasePlayerItem::AmmoInfoArray[giAmmoIndex].pszName = szAmmoname;
	CBasePlayerItem::AmmoInfoArray[giAmmoIndex].iId = giAmmoIndex;	// yes, this info is redundant
}

// Precaches the weapon and queues the weapon info for sending to clients
void UTIL_PrecacheOtherWeapon( const char *szClassname )
{
	edict_t *pent;

	pent = CREATE_NAMED_ENTITY( MAKE_STRING( szClassname ) );
	if ( FNullEnt( pent ) )
	{
		ALERT( at_console, "NULL Ent in UTIL_PrecacheOtherWeapon\n" );
		return;
	}

	CBaseEntity *pEntity = CBaseEntity::Instance( VARS( pent ) );

	if ( pEntity )
	{
		ItemInfo II;
		pEntity->Precache();
		memset( &II, 0, sizeof II );
		if ( ( (CBasePlayerItem *) pEntity )->GetItemInfo( &II ) )
		{
			CBasePlayerItem::ItemInfoArray[II.iId] = II;

			if ( II.pszAmmo1 && *II.pszAmmo1 )
			{
				AddAmmoNameToAmmoRegistry( II.pszAmmo1 );
			}

			if ( II.pszAmmo2 && *II.pszAmmo2 )
			{
				AddAmmoNameToAmmoRegistry( II.pszAmmo2 );
			}

			memset( &II, 0, sizeof II );
		}
	}

	REMOVE_ENTITY( pent );
}

// called by worldspawn
void W_Precache( void )
{
	memset( CBasePlayerItem::ItemInfoArray, 0, sizeof( CBasePlayerItem::ItemInfoArray ) );
	memset( CBasePlayerItem::AmmoInfoArray, 0, sizeof( CBasePlayerItem::AmmoInfoArray ) );
	giAmmoIndex = 0;
	UTIL_PrecacheOtherWeapon( "weapon_thompson" );
	UTIL_PrecacheOtherWeapon( "weapon_m1carbine" );
	UTIL_PrecacheOtherWeapon( "weapon_garand" );
	UTIL_PrecacheOtherWeapon( "weapon_scopedkar" );
	UTIL_PrecacheOtherWeapon( "weapon_luger" );
	UTIL_PrecacheOtherWeapon( "weapon_spring" );
	UTIL_PrecacheOtherWeapon( "weapon_colt" );
	UTIL_PrecacheOtherWeapon( "weapon_amerknife" );
	UTIL_PrecacheOtherWeapon( "weapon_gerknife" );
	UTIL_PrecacheOtherWeapon( "weapon_spade" );
	UTIL_PrecacheOtherWeapon( "weapon_kar" );
	UTIL_PrecacheOtherWeapon( "weapon_mp44" );
	UTIL_PrecacheOtherWeapon( "weapon_mp40" );
	UTIL_PrecacheOtherWeapon( "weapon_mg42" );
	UTIL_PrecacheOtherWeapon( "weapon_mg34" );
	UTIL_PrecacheOtherWeapon( "weapon_30cal" );
	UTIL_PrecacheOtherWeapon( "weapon_bar" );
	UTIL_PrecacheOtherWeapon( "weapon_handgrenade" );
	UTIL_PrecacheOtherWeapon( "weapon_handgrenade_ex" );
	UTIL_PrecacheOtherWeapon( "weapon_stickgrenade" );
	UTIL_PrecacheOtherWeapon( "weapon_stickgrenade_ex" );
	UTIL_PrecacheOtherWeapon( "weapon_greasegun" );
	UTIL_PrecacheOtherWeapon( "weapon_fg42" );
	UTIL_PrecacheOtherWeapon( "weapon_k43" );
	UTIL_PrecacheOtherWeapon( "weapon_enfield" );
	UTIL_PrecacheOtherWeapon( "weapon_sten" );
	UTIL_PrecacheOtherWeapon( "weapon_bren" );
	UTIL_PrecacheOtherWeapon( "weapon_webley" );
	UTIL_PrecacheOtherWeapon( "weapon_bazooka" );
	UTIL_PrecacheOtherWeapon( "weapon_pschreck" );
	UTIL_PrecacheOtherWeapon( "weapon_piat" );
	UTIL_PrecacheOther( "shell_bazooka" );
	UTIL_PrecacheOther( "shell_piat" );
	UTIL_PrecacheOther( "shell_pschreck" );
	UTIL_PrecacheOther( "ammo_garand" );
	UTIL_PrecacheOther( "ammo_m1carbine" );
	UTIL_PrecacheOther( "ammo_kar" );
	UTIL_PrecacheOther( "ammo_scopedkar" );
	UTIL_PrecacheOther( "ammo_spring" );
	UTIL_PrecacheOther( "ammo_thompson" );
	UTIL_PrecacheOther( "ammo_mp40" );
	UTIL_PrecacheOther( "ammo_mp44" );
	UTIL_PrecacheOther( "ammo_bar" );
	UTIL_PrecacheOther( "ammo_mg42" );
	UTIL_PrecacheOther( "ammo_mg34" );
	UTIL_PrecacheOther( "ammo_30cal" );
	UTIL_PrecacheOther( "ammo_colt" );
	UTIL_PrecacheOther( "ammo_luger" );
	UTIL_PrecacheOther( "ammo_greasegun" );
	UTIL_PrecacheOther( "ammo_fg42" );
	UTIL_PrecacheOther( "ammo_k43" );
	UTIL_PrecacheOther( "ammo_enfield" );
	UTIL_PrecacheOther( "ammo_sten" );
	UTIL_PrecacheOther( "ammo_bren" );
	UTIL_PrecacheOther( "ammo_webley" );
	UTIL_PrecacheOther( "ammo_bazooka" );
	UTIL_PrecacheOther( "ammo_pschreck" );
	UTIL_PrecacheOther( "ammo_piat" );
	UTIL_PrecacheOther( "ammo_bazooka" );
	UTIL_PrecacheOther( "ammo_pschreck" );
	UTIL_PrecacheOther( "ammo_piat" );
	if ( g_pGameRules->IsDeathmatch() )
	{
		UTIL_PrecacheOther( "weaponbox" );
	}
	g_sModelIndexFireball = PRECACHE_MODEL( "sprites/zerogxplode.spr" );
	g_sModelIndexSmoke = PRECACHE_MODEL( "sprites/steam1.spr" );
	g_sModelIndexBubbles = PRECACHE_MODEL( "sprites/bubble.spr" );
	g_sModelIndexBloodSpray = PRECACHE_MODEL( "sprites/bloodspray.spr" );
	g_sModelIndexBloodDrop = PRECACHE_MODEL( "sprites/blood-narrow.spr" );
	PRECACHE_MODEL( "sprites/explode1.spr" );
	m_usPlayWhizz = PRECACHE_EVENT( 1, "events/misc/whizz.sc" );
	PRECACHE_SOUND( "weapons/grenade_hit1.wav" );
	PRECACHE_SOUND( "weapons/grenade_hit2.wav" );
	PRECACHE_SOUND( "weapons/grenade_hit3.wav" );
	PRECACHE_SOUND( "items/weapondrop.wav" );
}

TYPEDESCRIPTION CBasePlayerItem::m_SaveData[] = {
	DEFINE_FIELD( CBasePlayerItem, m_pPlayer, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayerItem, m_pNext, FIELD_CLASSPTR ),
	//DEFINE_FIELD( CBasePlayerItem, m_fKnown, FIELD_INTEGER ),Reset to zero on load
	DEFINE_FIELD( CBasePlayerItem, m_iId, FIELD_INTEGER ),
	// DEFINE_FIELD( CBasePlayerItem, m_iIdPrimary, FIELD_INTEGER ),
	// DEFINE_FIELD( CBasePlayerItem, m_iIdSecondary, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CBasePlayerItem, CBaseAnimating );

TYPEDESCRIPTION CBasePlayerWeapon::m_SaveData[] = {
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iPrimaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iSecondaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iClip, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iDefaultAmmo, FIELD_INTEGER ),
	//	DEFINE_FIELD( CBasePlayerWeapon, m_iClientClip, FIELD_INTEGER )	 , reset to zero on load so hud gets updated correctly
	//  DEFINE_FIELD( CBasePlayerWeapon, m_iClientWeaponState, FIELD_INTEGER ), reset to zero on load so hud gets updated correctly
};

IMPLEMENT_SAVERESTORE( CBasePlayerWeapon, CBasePlayerItem );

void CBasePlayerItem ::SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector( -24, -24, 0 );
	pev->absmax = pev->origin + Vector( 24, 24, 16 );
}

//=========================================================
// Sets up movetype, size, solidtype for a new weapon.
//=========================================================
void CBasePlayerItem ::FallInit( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );	//pointsize until it lands on the ground.

	SetUse( NULL );
	SetTouch( &CBasePlayerItem::DefaultTouch );

	m_vRespawnOrigin = pev->origin;
	m_vRespawnAngles = pev->angles;

	SetThink( &CBasePlayerItem::FallThink );
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
}

//=========================================================
// FallThink - Items that have just spawned run this think
// to catch them when they hit the ground. Once we're sure
// that the object is grounded, we change its solid type
// to trigger and set it in a large box that helps the
// player get it.
//=========================================================
void CBasePlayerItem::FallThink( void )
{
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;

	if ( pev->flags & FL_ONGROUND )
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
		if ( !FNullEnt( pev->owner ) )
		{
			int pitch = 95 + RANDOM_LONG( 0, 29 );
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "items/weapondrop.wav", 1, ATTN_NORM, 0, pitch );
		}

		// lie flat
		pev->angles.x = 0;
		pev->angles.z = 0;

		Materialize();
	}
}

char *szRespawnWeapons[3] = { "weapon_bazooka", "weapon_pschreck", "weapon_piat" };

//=========================================================
// Materialize - make a CBasePlayerItem visible and tangible
//=========================================================
void CBasePlayerItem::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	pev->solid = SOLID_TRIGGER;

	UTIL_SetOrigin( pev, pev->origin );	 // link into world.
	SetTouch( &CBasePlayerItem::DefaultTouch );
	m_bReadyToRespawn = FALSE;
	SetThink( NULL );
	SetUse( NULL );
}

//=========================================================
// AttemptToMaterialize - the item is trying to rematerialize,
// should it do so now or wait longer?
//=========================================================
void CBasePlayerItem::AttemptToMaterialize( void )
{
	float time = g_pGameRules->FlWeaponTryRespawn( this );

	if ( time == 0 )
	{
		Materialize();
		return;
	}

	pev->nextthink = UTIL_WeaponTimeBase() + time;
}

//=========================================================
// CheckRespawn - a player is taking this weapon, should
// it respawn?
//=========================================================
void CBasePlayerItem ::CheckRespawn( void )
{
	switch ( g_pGameRules->WeaponShouldRespawn( this ) )
	{
	case GR_WEAPON_RESPAWN_YES:
		Respawn();
		break;
	case GR_WEAPON_RESPAWN_NO:
		return;
		break;
	}
}

//=========================================================
// Respawn- this item is already in the world, but it is
// invisible and intangible. Make it visible and tangible.
//=========================================================
CBaseEntity *CBasePlayerItem::Respawn( void )
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	CBaseEntity *pNewWeapon = CBaseEntity::Create(
		(char *) STRING( pev->classname ), g_pGameRules->VecWeaponRespawnSpot( this ), pev->angles, pev->owner );

	if ( pNewWeapon )
	{
		pNewWeapon->pev->effects |= EF_NODRAW;	// invisible for now
		pNewWeapon->SetThink( &CBasePlayerItem::AttemptToMaterialize );
		pNewWeapon->SetTouch( NULL );			// no touch
		( (CBasePlayerItem *) pNewWeapon )->m_bReadyToRespawn = TRUE;

		DROP_TO_FLOOR( ENT( pev ) );

		// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		pNewWeapon->pev->nextthink = g_pGameRules->FlWeaponRespawnTime( this );
	}
	else
	{
		ALERT( at_console, "Respawn failed to create %s!\n", STRING( pev->classname ) );
	}

	return pNewWeapon;
}

void CBasePlayerItem::RoundRespawn( void )
{
	if ( m_bReadyToRespawn )
	{
		SetThink( &CBasePlayerItem::AttemptToMaterialize );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CBasePlayerItem::DefaultTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *) pOther;

	// can I have this?
	if ( g_pGameRules->CanHavePlayerItem( pPlayer, this ) )
	{
		if ( pOther->AddPlayerItem( this ) )
		{
			AttachToPlayer( pPlayer );
			EMIT_SOUND_DYN( ENT( pPlayer->pev ), CHAN_ITEM, "items/weaponpickup.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		}

		SUB_UseTargets( pOther, USE_TOGGLE, 0 );  // UNDONE: when should this happen?
	}
	else if ( gEvilImpulse101 )
	{
		UTIL_Remove( this );
	}
}

void CBasePlayerItem::PickupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !pActivator->IsPlayer() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
	if ( g_pGameRules->CanHavePlayerItem( pPlayer, this ) )
	{
		if ( pPlayer->AddPlayerItem( this ) )
		{
			AttachToPlayer( pPlayer );
			EMIT_SOUND_DYN( ENT( pActivator->pev ), CHAN_ITEM, "items/weaponpickup.wav", 1.0, 0.8, 0, 100 );
		}
	}
	else
	{
		if ( gEvilImpulse101 )
			UTIL_Remove( this );
	}
}

BOOL CanAttack( float attack_time, float curtime, BOOL isPredicted )
{
#if defined( CLIENT_WEAPONS )
	if ( !isPredicted )
#else
	if ( 1 )
#endif
	{
		return ( attack_time <= curtime ) ? TRUE : FALSE;
	}
	else
	{
		return ( attack_time <= 0.0 ) ? TRUE : FALSE;
	}
}

void CBasePlayerWeapon::ItemPostFrame( void )
{
	if ( m_pPlayer->pev->flags & FL_IMMUNE_LAVA )
		return;

	if ( ( m_fInReload ) && ( UTIL_WeaponTimeBase() >= m_flNextPrimaryAttack ) )
	{
		// complete the reload.
		int j;

		if ( m_iId == WEAPON_ENFIELD )
		{
			j = 10 - m_iClip;

			if ( j > 5 )
				j = 5;
		}
		else
		{
			j = min( CBasePlayerItem::ItemInfoArray[m_iId].iMaxClip - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] );
		}

		if ( Classify() == CLASS_ROCKET )
		{
			if ( m_iWeaponState & WPNSTATE_ROCKET_SLOW )
			{
				m_iWeaponState &= ~WPNSTATE_ROCKET_SLOW;
				( (CRocketWeapon *) this )->ReSlow();
			}
		}

		// Add them to the clip
		m_iClip += j;

		MESSAGE_BEGIN( MSG_ONE, gmsgReloadDone, NULL, m_pPlayer->pev );
		MESSAGE_END();

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;

		m_pPlayer->TabulateAmmo();

		m_fInReload = FALSE;

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
	}

	if ( m_fInAttack && !( m_pPlayer->pev->button & IN_ATTACK ) )
		m_fInAttack = 0;

	ItemInfo ii;
	GetItemInfo( &ii );

	if ( ( m_pPlayer->pev->button & IN_ATTACK2 ) && UTIL_WeaponTimeBase() >= m_flNextSecondaryAttack )
	{
		if ( CBasePlayerItem::ItemInfoArray[m_iId].pszAmmo2 && !m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] )
		{
			m_fFireOnEmpty = TRUE;
		}

		m_pPlayer->TabulateAmmo();

		if ( g_pGameRules->RoundState() == ROUNDSTATE_RUNNING )
			SecondaryAttack();

		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ( ( m_pPlayer->pev->button & IN_ATTACK ) &&
			  ( m_iId == WEAPON_HANDGRENADE || m_iId == WEAPON_STICKGRENADE || m_iId == WEAPON_STICKGRENADEX ||
				m_iId == WEAPON_HANDGRENADEX ) &&
			  gpGlobals->time >= m_flNextPrimaryAttack )
	{
		// the grenades are not predicted, they work on the real time
		m_pPlayer->TabulateAmmo();

		if ( g_pGameRules->RoundState() == ROUNDSTATE_RUNNING )
			PrimaryAttack();
	}
	else if ( ( m_pPlayer->pev->button & IN_ATTACK ) && UTIL_WeaponTimeBase() >= m_flNextPrimaryAttack )
	{
		if ( ( !m_iClip && CBasePlayerItem::ItemInfoArray[m_iId].pszAmmo1 ) ||
			 ( CBasePlayerItem::ItemInfoArray[m_iId].iMaxClip == -1 && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] ) )
		{
			m_fFireOnEmpty = TRUE;
		}

		m_pPlayer->TabulateAmmo();

		if ( g_pGameRules->RoundState() == ROUNDSTATE_RUNNING )
			PrimaryAttack();
	}
	else if ( ( m_pPlayer->pev->button & IN_RELOAD ) && CBasePlayerItem::ItemInfoArray[m_iId].iMaxClip != WEAPON_NOCLIP &&
			  !m_fInReload )
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		if ( m_pPlayer->pev->waterlevel <= 2 )
			Reload();
	}
	else if ( !( m_pPlayer->pev->button & ( IN_ATTACK | IN_ATTACK2 ) ) )
	{
		// no fire buttons down

		m_fFireOnEmpty = FALSE;

		if ( !m_iClip && !( CBasePlayerItem::ItemInfoArray[m_iId].iFlags & ITEM_FLAG_NOAUTORELOAD ) &&
			 m_flNextPrimaryAttack < 0 && m_pPlayer->m_bAutoReload &&
			 ( ( !m_pPlayer->IsProneDeployed() && !m_pPlayer->IsSandbagDeployed() ) || Classify() != CLASS_MACHINEGUN ) &&
			 m_pPlayer->pev->waterlevel <= 2 )
		{
			Reload();
			return;
		}

		WeaponIdle();
		return;
	}

	// catch all
	if ( ShouldWeaponIdle() )
	{
		WeaponIdle();
	}
}

void CBasePlayerItem::DestroyItem( void )
{
	if ( m_pPlayer )
	{
		// if attached to a player, remove.
		m_pPlayer->RemovePlayerItem( this );
	}

	Kill();
}

int CBasePlayerItem::AddToPlayer( CBasePlayer *pPlayer )
{
	m_pPlayer = pPlayer;

	return TRUE;
}

void CBasePlayerItem::Drop( void )
{
	// another weapon of this kind that is waiting for the death of this one comes back
	if ( m_bTriggerSpawnOnDie )
	{
		CBasePlayerItem *pItem = (CBasePlayerItem *) UTIL_FindEntityByClassname( NULL, STRING( pev->classname ) );

		while ( pItem )
		{
			if ( pItem->m_bReadyToRespawn )
			{
				pItem->SetThink( &CBasePlayerItem::AttemptToMaterialize );
				pItem->pev->nextthink = gpGlobals->time + 0.1;
				break;
			}

			pItem = (CBasePlayerItem *) UTIL_FindEntityByClassname( pItem, STRING( pev->classname ) );
		}
	}

	SetTouch( NULL );
	SetThink( &CBasePlayerItem::SUB_Remove );
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
}

void CBasePlayerItem::Kill( void )
{
	// another weapon of this kind that is waiting for the death of this one comes back
	if ( m_bTriggerSpawnOnDie )
	{
		CBasePlayerItem *pItem = (CBasePlayerItem *) UTIL_FindEntityByClassname( NULL, STRING( pev->classname ) );

		while ( pItem )
		{
			if ( pItem->m_bReadyToRespawn )
			{
				pItem->SetThink( &CBasePlayerItem::AttemptToMaterialize );
				pItem->pev->nextthink = gpGlobals->time + 0.1;
				break;
			}

			pItem = (CBasePlayerItem *) UTIL_FindEntityByClassname( pItem, STRING( pev->classname ) );
		}
	}

	SetTouch( NULL );
	SetThink( &CBasePlayerItem::SUB_Remove );
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
}

void CBasePlayerItem::SpawnReplacement( void )
{
	if ( !m_bTriggerSpawnOnDie )
		return;

	for ( CBaseEntity *p = UTIL_FindEntityByClassname( NULL, STRING( pev->classname ) ); p != NULL;
		  p = UTIL_FindEntityByClassname( p, STRING( pev->classname ) ) )
	{
		if ( ( (CBasePlayerItem *)p )->m_bTriggerSpawnOnDie == 0 )
		{
			p->SetThink( &CBasePlayerItem::AttemptToMaterialize );
			p->pev->nextthink = gpGlobals->time + 0.1f;
			return;
		}
	}
}

void CBasePlayerItem::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
}

void CBasePlayerItem::AttachToPlayer( CBasePlayer *pPlayer )
{
	ItemInfo ii;

	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	pev->aiment = pPlayer->edict();
	pev->effects = EF_NODRAW;  // ??
	pev->modelindex = 0;	   // server won't send down to clients if modelindex == 0
	pev->model = iStringNull;
	pev->owner = pPlayer->edict();
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
	SetTouch( NULL );

	GetItemInfo( &ii );

	if ( !( ii.iFlags & ITEM_FLAG_HEAT ) )
		SetThink( NULL );
}

// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
int CBasePlayerWeapon::AddDuplicate( CBasePlayerItem *pOriginal )
{
	if ( m_iDefaultAmmo )
	{
		return ExtractAmmo( (CBasePlayerWeapon *) pOriginal );
	}
	else
	{
		// a dead player dropped this.
		return ExtractClipAmmo( (CBasePlayerWeapon *) pOriginal );
	}
}

int CBasePlayerWeapon::AddToPlayer( CBasePlayer *pPlayer )
{
	m_pPlayer = pPlayer;

	pPlayer->SetHasWeapon( m_iId );

	if ( !m_iPrimaryAmmoType )
	{
		m_iPrimaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo1() );
		m_iSecondaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo2() );
	}

	return AddWeapon();
}

int CBasePlayerWeapon::UpdateClientData( CBasePlayer *pPlayer )
{
	BOOL bSend = FALSE;
	int state = ( pPlayer->m_pActiveItem == this ) ? 1 : 0;

	// Forcing send of all data!
	if ( !pPlayer->m_fWeapon )
	{
		bSend = TRUE;
	}

	// This is the current or last weapon, so the state will need to be updated
	if ( this == pPlayer->m_pActiveItem || this == pPlayer->m_pClientActiveItem )
	{
		if ( pPlayer->m_pActiveItem != pPlayer->m_pClientActiveItem )
		{
			bSend = TRUE;
		}
	}

	// If the ammo, state, or fov has changed, update the weapon
	if ( m_iClip != m_iClientClip || state != m_iClientWeaponState || pPlayer->m_iFOV != pPlayer->m_iClientFOV )
	{
		bSend = TRUE;
	}

	if ( bSend )
	{
		int iClip = m_iClip;

		// weapons without a clip show the ammo they carry
		if ( iClip == -1 )
			iClip = m_pPlayer->m_rgAmmo[pPlayer->GetAmmoIndex( ItemInfoArray[m_iId].pszAmmo1 )];

		MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pPlayer->pev );
		WRITE_BYTE( state );
		WRITE_BYTE( m_iId );
		WRITE_BYTE( iClip );
		MESSAGE_END();

		m_iClientClip = m_iClip;
		m_iClientWeaponState = state;
		pPlayer->m_fWeapon = TRUE;
	}

	if ( m_pNext )
		m_pNext->UpdateClientData( pPlayer );

	return 1;
}

void CBasePlayerWeapon::SendWeaponAnim( int iAnim, int skiplocal )
{
	if ( !m_pPlayer )
		return;

	m_pPlayer->pev->weaponanim = iAnim;

	if ( skiplocal && ENGINE_CANSKIP( m_pPlayer->edict() ) )
		return;

	if ( m_pPlayer->pev->team == TEAM_ALLIES )
	{
		if ( (unsigned) ( m_pPlayer->pev->playerclass - 0x15 ) < 6 )
			pev->body = 1;
		else
			pev->body = 0;
	}
	else
	{
		pev->body = 2;
	}

	MESSAGE_BEGIN( MSG_ONE, SVC_WEAPONANIM, NULL, m_pPlayer->pev );
	WRITE_BYTE( iAnim );	  // sequence number
	WRITE_BYTE( pev->body );  // weaponmodel bodygroup.
	MESSAGE_END();
}

BOOL CBasePlayerWeapon ::AddPrimaryAmmo( int iCount, char *szName, int iMaxClip, int iMaxCarry )
{
	int iIdAmmo;

	if ( iMaxClip < 1 )
	{
		m_iClip = -1;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName, iMaxCarry );
	}
	else if ( m_iClip == 0 )
	{
		int i;
		i = min( m_iClip + iCount, iMaxClip ) - m_iClip;
		m_iClip += i;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount - i, szName, iMaxCarry );
	}
	else
	{
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName, iMaxCarry );
	}

	if ( iIdAmmo > 0 )
	{
		m_iPrimaryAmmoType = iIdAmmo;
		if ( m_pPlayer->HasPlayerItem( this ) )
		{
			// play the "got ammo" sound only if we gave some ammo to a player that already had this gun.
			// if the player is just getting this gun for the first time, DefaultTouch will play the "picked up gun" sound for us.
			EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		}
	}

	return iIdAmmo > 0 ? TRUE : FALSE;
}

BOOL CBasePlayerWeapon ::AddSecondaryAmmo( int iCount, char *szName, int iMax )
{
	int iIdAmmo;

	iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName, iMax );

	if ( iIdAmmo > 0 )
	{
		m_iSecondaryAmmoType = iIdAmmo;
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	}

	return iIdAmmo > 0 ? TRUE : FALSE;
}

//=========================================================
// IsUseable - this function determines whether or not a
// weapon is useable by the player in its current state.
// (does it have ammo loaded? do I have any ammo for the
// weapon?, etc)
//=========================================================
BOOL CBasePlayerWeapon ::IsUseable( void )
{
	if ( m_iClip <= 0 )
	{
		if ( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] <= 0 && iMaxAmmo1() != -1 )
		{
			// clip is empty (or nonexistant) and the player has no more ammo of this type.
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CBasePlayerWeapon ::CanDeploy( void )
{
	return TRUE;
}

int CBasePlayerWeapon::Classify( void )
{
	return 0;
}

BOOL CBasePlayerWeapon ::DefaultDeploy( char *szViewModel, char *szWeaponModel, int iAnim, char *szAnimExt,
										char *szAnimReloadExt, int skiplocal )
{
	if ( !CanDeploy() )
		return FALSE;

	m_pPlayer->TabulateAmmo();
	m_pPlayer->pev->viewmodel = MAKE_STRING( szViewModel );
	m_pPlayer->pev->weaponmodel = MAKE_STRING( szWeaponModel );
	strcpy( m_pPlayer->m_szAnimExtention, szAnimExt );
	strcpy( m_pPlayer->m_szAnimReloadExt, szAnimReloadExt );
	m_pPlayer->m_iCurrentAnimationState = 0;
	m_pPlayer->m_fGunDropTime = 0;
	SendWeaponAnim( iAnim, skiplocal );
	m_pPlayer->SetAnimation( PLAYER_IDLE );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;

	return TRUE;
}

BOOL CBasePlayerWeapon::TimedDeploy( char *szViewModel, char *szWeaponModel, int iAnim, char *szAnimExt,
									 char *szAnimReloadExt, float idleTime, float attackTime, int skiplocal )
{
	if ( !CanDeploy() )
		return FALSE;

	m_pPlayer->TabulateAmmo();
	m_pPlayer->pev->viewmodel = MAKE_STRING( szViewModel );
	m_pPlayer->pev->weaponmodel = MAKE_STRING( szWeaponModel );
	strcpy( m_pPlayer->m_szAnimExtention, szAnimExt );
	strcpy( m_pPlayer->m_szAnimReloadExt, szAnimReloadExt );
	m_pPlayer->m_iCurrentAnimationState = 0;
	m_pPlayer->m_fGunDropTime = 0;
	SendWeaponAnim( iAnim, skiplocal );
	m_pPlayer->SetAnimation( PLAYER_IDLE );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + attackTime;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + idleTime;

	return TRUE;
}

Vector CBasePlayerWeapon::Aim( float accuracyFactor, CBasePlayer *pOther, int shared_rand )
{
	float targetx = UTIL_SharedRandomFloat( shared_rand, accuracyFactor, 0.0f );
	float targety = UTIL_SharedRandomFloat( shared_rand, accuracyFactor, 0.0f );
	float targetz = UTIL_SharedRandomFloat( shared_rand, accuracyFactor, 0.0f );
	return Vector( targetx, targety, targetz );
}
float CBasePlayerWeapon::flAim( float accuracyFactor, CBasePlayer *pOther )
{
	return accuracyFactor;
}

void CBasePlayerWeapon::RemoveStamina( float removeAmount, CBasePlayer *pOther )
{
	if ( !pOther )
		pOther = m_pPlayer;
	if ( !pOther )
		return;
	if ( ( pOther->pev->button & IN_DUCK ) == 0 && !CBasePlayer::IsProne( pOther ) )
	{
		float f = pOther->pev->fuser4 - removeAmount;
		if ( f < 0.0f )
			f = 0.0f;
		else if ( f > 100.0f )
			f = 100.0f;
		pOther->pev->fuser4 = f;
	}
}

int CBasePlayerWeapon::ChangeFOV( int fov )
{
	if ( m_pPlayer->m_iFOV > 0 )
	{
		m_pPlayer->UpdateScope( 0 );
		return 0;
	}

	return fov;
}

int CBasePlayerWeapon::ZoomIn( void )
{
	if ( Classify() == CLASS_SCOPED_RIFLE || Classify() == 0x400 )
	{
		m_pPlayer->UpdateScope( m_iId );
		return 20;
	}

	return 0;
}

int CBasePlayerWeapon::ZoomOut( void )
{
	if ( m_pPlayer->pev->vuser1.z != 0.0f )
		return m_pPlayer->m_iFOV;

	m_pPlayer->CheckPlayerSpeed();
	m_pPlayer->UpdateScope( 0 );

	return 0;
}

// DoD zoom/stamina/mortar/water-sniping impls
void CBasePlayerWeapon::UpdateZoomSpeed( void )
{
	if ( !m_pPlayer )
		return;
	if ( Classify() != 0x20 )
		return;
	m_pPlayer->CheckPlayerSpeed();
}

int CBasePlayerWeapon::GetFOV( void )
{
	return m_pPlayer ? m_pPlayer->m_iFOV : 0;
}

bool CBasePlayerWeapon::PlayerIsWaterSniping( void )
{
	if ( !m_pPlayer )
		return false;
	int wl = m_pPlayer->pev->waterlevel;
	if ( wl == 3 )
		return true;
	if ( wl > 0 )
		return CBasePlayer::IsProne( m_pPlayer ) != 0;
	return false;
}

void CBasePlayerWeapon::ThinkZoomOut( void )
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
}
void CBasePlayerWeapon::ThinkZoomIn( void )
{
	if ( m_pPlayer->pev->velocity.Length() <= 45.0f && !( m_pPlayer->pev->button & IN_ATTACK ) )
	{
		m_pPlayer->m_iFOV = ZoomIn();
		UpdateZoomSpeed();
	}
}

BOOL CBasePlayerWeapon ::DefaultReload( int iClipSize, int iAnim, float fDelay )
{
	int iAmmo = m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];

	if ( iAmmo <= 0 )
		return FALSE;

	int j;

	if ( m_iId == WEAPON_ENFIELD )
		j = min( 2 * iClipSize - m_iClip, iAmmo );
	else
		j = min( iClipSize - m_iClip, iAmmo );

	if ( j == 0 || iAmmo <= m_iClip )
		return FALSE;

	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/reloadgarand.wav", RANDOM_FLOAT( 0.8, 0.9 ), ATTN_NORM, 0, PITCH_NORM );

	m_pPlayer->SetAnimation( PLAYER_RELOAD );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + fDelay;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + fDelay;

	SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0 );

	m_fInReload = TRUE;

	ItemInfo ii;
	GetItemInfo( &ii );

	if ( ii.iFlags & ITEM_FLAG_HEAT )
		m_flWeaponHeat = 0;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + max( 3.0f, fDelay );
	return TRUE;
}

BOOL CBasePlayerWeapon ::PlayEmptySound( void )
{
	if ( m_iPlayEmptySound )
		m_iPlayEmptySound = 0;
	return 0;
}

void CBasePlayerWeapon ::ResetEmptySound( void )
{
	m_iPlayEmptySound = 1;
}

//=========================================================
//=========================================================
int CBasePlayerWeapon::PrimaryAmmoIndex( void )
{
	return m_iPrimaryAmmoType;
}

//=========================================================
//=========================================================
int CBasePlayerWeapon::SecondaryAmmoIndex( void )
{
	return -1;
}

int CBasePlayerWeapon::GetRoundState( void )
{
	return g_pGameRules->RoundState();
}

void CBasePlayerWeapon::Holster( int skiplocal /* = 0 */ )
{
	m_fInAttack = 0;
	m_fInReload = FALSE;  // cancel any reload in progress.
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
}

void CBasePlayerAmmo::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CBasePlayerAmmo::DefaultTouch );
}

CBaseEntity *CBasePlayerAmmo::Respawn( void )
{
	pev->effects |= EF_NODRAW;
	SetTouch( NULL );

	UTIL_SetOrigin( pev, g_pGameRules->VecAmmoRespawnSpot( this ) );  // move to wherever I'm supposed to repawn.

	SetThink( &CBasePlayerAmmo::Materialize );
	pev->nextthink = g_pGameRules->FlAmmoRespawnTime( this );

	return this;
}

void CBasePlayerAmmo::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch( &CBasePlayerAmmo::DefaultTouch );
}

void CBasePlayerAmmo ::DefaultTouch( CBaseEntity *pOther )
{
	if ( !( pev->flags & FL_ONGROUND ) )
		return;

	if ( !pOther->IsPlayer() )
		return;

	if ( pev->team && pev->team != pOther->pev->team )
		return;

	if ( !FNullEnt( pev->owner ) && pOther->pev->pContainingEntity == pev->owner )
	{
		// the owner takes his own dropped ammo back
		( (CBasePlayer *) pOther )->m_nGenericAmmoCount++;

		EMIT_SOUND_DYN( pOther->edict(), CHAN_ITEM, "items/weaponpickup.wav", 1, ATTN_NORM, 0, PITCH_NORM );

		SetTouch( NULL );
		UTIL_Remove( this );
	}
	else if ( AddAmmo( pOther ) )
	{
		if ( g_pGameRules->AmmoShouldRespawn( this ) == GR_AMMO_RESPAWN_YES )
		{
			Respawn();
		}
		else
		{
			SetTouch( NULL );
			SetThink( &CBasePlayerAmmo::SUB_Remove );
			pev->nextthink = UTIL_WeaponTimeBase() + .1;
		}
	}
	else if ( gEvilImpulse101 )
	{
		// evil impulse 101 hack, kill always
		SetTouch( NULL );
		SetThink( &CBasePlayerAmmo::SUB_Remove );
		pev->nextthink = UTIL_WeaponTimeBase() + .1;
	}
}

bool CBasePlayerAmmo::GiveGenericAmmo( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
		return false;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		for ( CBasePlayerItem *pItem = pPlayer->m_rgpPlayerItems[i]; pItem != NULL; pItem = pItem->m_pNext )
		{
			if ( pItem->Classify() & 0x3f0 )
			{
				ItemInfo ii;
				memset( &ii, 0, sizeof( ii ) );
				pItem->GetItemInfo( &ii );

				int iClip = ii.iMaxClip;
				if ( iClip == -1 )
					iClip = P_WpnInfo[ii.iId].ammo_default;

				return pPlayer->GiveAmmo( iClip, ii.pszAmmo1, ii.iMaxAmmo1 ) != -1;
			}
		}
	}
	return false;
}

//=========================================================
// called by the new item with the existing item as parameter
//
// if we call ExtractAmmo(), it's because the player is picking up this type of weapon for
// the first time. If it is spawned by the world, m_iDefaultAmmo will have a default ammo amount in it.
// if  this is a weapon dropped by a dying player, has 0 m_iDefaultAmmo, which means only the ammo in
// the weapon clip comes along.
//=========================================================
int CBasePlayerWeapon::ExtractAmmo( CBasePlayerWeapon *pWeapon )
{
	int iReturn = 0;

	if ( pszAmmo1() != NULL )
	{
		// blindly call with m_iDefaultAmmo. It's either going to be a value or zero. If it is zero,
		// we only get the ammo in the weapon's clip, which is what we want.
		if ( m_iId == WEAPON_ENFIELD )
			iReturn = pWeapon->AddPrimaryAmmo( m_iDefaultAmmo, (char *) pszAmmo1(), 10, iMaxAmmo1() );
		else
			iReturn = pWeapon->AddPrimaryAmmo( m_iDefaultAmmo, (char *) pszAmmo1(), iMaxClip(), iMaxAmmo1() );

		m_iDefaultAmmo = 0;
	}

	if ( pszAmmo2() != NULL )
	{
		iReturn = pWeapon->AddSecondaryAmmo( 0, (char *) pszAmmo2(), iMaxAmmo2() );
	}

	return iReturn;
}

//=========================================================
// called by the new item's class with the existing item as parameter
//=========================================================
int CBasePlayerWeapon::ExtractClipAmmo( CBasePlayerWeapon *pWeapon )
{
	int iAmmo;

	if ( m_iClip == WEAPON_NOCLIP )
	{
		iAmmo = 0;	// guns with no clips always come empty if they are second-hand
	}
	else
	{
		iAmmo = m_iClip;
	}

	return pWeapon->m_pPlayer->GiveAmmo( iAmmo, (char *) pszAmmo1(), iMaxAmmo1() );	 // , &m_iPrimaryAmmoType
}

//=========================================================
// RetireWeapon - no more ammo for this gun, put it away.
//=========================================================
void CBasePlayerWeapon::RetireWeapon( void )
{
	// first, no viewmodel at all.
	m_pPlayer->pev->viewmodel = iStringNull;
	m_pPlayer->pev->weaponmodel = iStringNull;
	//m_pPlayer->pev->viewmodelindex = NULL;

	g_pGameRules->GetNextBestWeapon( m_pPlayer, this );
}

void CBasePlayerWeapon::PostMortarValue( float v )
{
	return;
}
void CBasePlayerWeapon::SendMortarFireCommand( char *c )
{
	return;
}

//*********************************************************
// weaponbox code:
//*********************************************************

LINK_ENTITY_TO_CLASS( weaponbox, CWeaponBox );

TYPEDESCRIPTION CWeaponBox::m_SaveData[] = {
	DEFINE_ARRAY( CWeaponBox, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS ),
	DEFINE_ARRAY( CWeaponBox, m_rgiszAmmo, FIELD_STRING, MAX_AMMO_SLOTS ),
	DEFINE_ARRAY( CWeaponBox, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES ),
	DEFINE_FIELD( CWeaponBox, m_cAmmoTypes, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CWeaponBox, CBaseEntity );

//=========================================================
//
//=========================================================
void CWeaponBox::Precache( void )
{
	PRECACHE_MODEL( "models/allied_ammo.mdl" );
	PRECACHE_MODEL( "models/axis_ammo.mdl" );
}

//=========================================================
//=========================================================
void CWeaponBox ::KeyValue( KeyValueData *pkvd )
{
	if ( m_cAmmoTypes < MAX_AMMO_SLOTS )
	{
		PackAmmo( ALLOC_STRING( pkvd->szKeyName ), atoi( pkvd->szValue ) );
		m_cAmmoTypes++;	 // count this new ammo type.

		pkvd->fHandled = TRUE;
	}
	else
	{
		ALERT( at_console, "WeaponBox too full! only %d ammotypes allowed\n", MAX_AMMO_SLOTS );
	}
}

//=========================================================
// CWeaponBox - Spawn
//=========================================================
void CWeaponBox::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	m_bTouch = TRUE;

	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
}

//=========================================================
// CWeaponBox - Kill - the think function that removes the
// box from the world.
//=========================================================
void CWeaponBox::Kill( void )
{
	CBasePlayerItem *pWeapon;
	int i;

	ALERT( at_aiconsole, "weapon box killed.\n" );

	// destroy the weapons
	for ( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		pWeapon = m_rgpPlayerItems[i];

		while ( pWeapon )
		{
			// another weapon of this kind that is waiting for the death of this one comes back
			if ( pWeapon->m_bTriggerSpawnOnDie )
			{
				CBasePlayerItem *pItem =
					(CBasePlayerItem *) UTIL_FindEntityByClassname( NULL, STRING( pWeapon->pev->classname ) );

				while ( pItem )
				{
					if ( pItem->m_bReadyToRespawn )
					{
						pItem->SetThink( &CBasePlayerItem::AttemptToMaterialize );
						pItem->pev->nextthink = gpGlobals->time + 0.1;
						break;
					}

					pItem = (CBasePlayerItem *) UTIL_FindEntityByClassname( pItem, STRING( pWeapon->pev->classname ) );
				}
			}

			pWeapon->SetThink( &CBasePlayerItem::SUB_Remove );
			pWeapon->pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
			pWeapon = pWeapon->m_pNext;
		}
	}

	// remove the box
	UTIL_Remove( this );
}

//=========================================================
// CWeaponBox - Touch: try to add my contents to the toucher
// if the toucher is a player.
//=========================================================
void CWeaponBox::Touch( CBaseEntity *pOther )
{
	if ( !( pev->flags & FL_ONGROUND ) )
	{
		return;
	}

	if ( !pOther->IsPlayer() )
	{
		// only players may touch a weaponbox.
		return;
	}

	if ( !pOther->IsAlive() )
	{
		// no dead guys.
		return;
	}

	if ( !m_bTouch )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *) pOther;
	BOOL bGotWeapon = FALSE;
	int i;

	// go through my weapons and try to give the usable ones to the player.
	for ( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		CBasePlayerItem *pItem = m_rgpPlayerItems[i];

		while ( pItem )
		{
			CBasePlayerItem *pNext = pItem->m_pNext;

			if ( g_pGameRules->CanHavePlayerItem( pPlayer, pItem ) )
			{
				if ( pPlayer->AddPlayerItem( pItem ) )
					pItem->AttachToPlayer( pPlayer );

				bGotWeapon = TRUE;

				// unlink this weapon from the box
				m_rgpPlayerItems[i] = m_rgpPlayerItems[i]->m_pNext;
			}

			pItem = pNext;
		}
	}

	if ( bGotWeapon )
	{
		// dole out ammo
		for ( i = 0; i < MAX_AMMO_SLOTS; i++ )
		{
			if ( !FStringNull( m_rgiszAmmo[i] ) )
			{
				// there's some ammo of this type.
				pPlayer->GiveAmmo( m_rgAmmo[i], (char *) STRING( m_rgiszAmmo[i] ), m_rgAmmo[i] );

				// now empty the ammo from the weaponbox since we just gave it to the player
				m_rgiszAmmo[i] = iStringNull;
				m_rgAmmo[i] = 0;
			}
		}

		EMIT_SOUND_DYN( pOther->edict(), CHAN_ITEM, "items/weaponpickup.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		SetTouch( NULL );
		UTIL_Remove( this );
	}
}

void CWeaponBox::ActivateThink( void )
{
	m_bTouch = TRUE;
	SetThink( &CWeaponBox::Kill );
	pev->nextthink = gpGlobals->time + 20.0f;
}

//=========================================================
// CWeaponBox - PackWeapon: Add this weapon to the box
//=========================================================
BOOL CWeaponBox::PackWeapon( CBasePlayerItem *pWeapon )
{
	// is one of these weapons already packed in this box?
	if ( HasWeapon( pWeapon ) )
	{
		return FALSE;  // box can only hold one of each weapon type
	}

	if ( pWeapon->m_pPlayer )
	{
		if ( pWeapon->m_pPlayer->m_pActiveItem == pWeapon )
			pWeapon->Holster();

		if ( !pWeapon->m_pPlayer->RemovePlayerItem( pWeapon ) )
		{
			// failed to unhook the weapon from the player!
			return FALSE;
		}
	}

	int iWeaponSlot = pWeapon->iItemSlot();

	if ( m_rgpPlayerItems[iWeaponSlot] )
	{
		// there's already one weapon in this slot, so link this into the slot's column
		pWeapon->m_pNext = m_rgpPlayerItems[iWeaponSlot];
		m_rgpPlayerItems[iWeaponSlot] = pWeapon;
	}
	else
	{
		// first weapon we have for this slot
		m_rgpPlayerItems[iWeaponSlot] = pWeapon;
		pWeapon->m_pNext = NULL;
	}

	pWeapon->pev->spawnflags |= SF_NORESPAWN;  // never respawn
	pWeapon->pev->movetype = MOVETYPE_NONE;
	pWeapon->pev->solid = SOLID_NOT;
	pWeapon->pev->effects = EF_NODRAW;
	pWeapon->pev->modelindex = 0;
	pWeapon->pev->model = iStringNull;
	pWeapon->pev->owner = edict();

	ItemInfo ii;
	pWeapon->GetItemInfo( &ii );

	// the weapon that heats up keeps its think to cool down
	if ( !( ii.iFlags & ITEM_FLAG_HEAT ) )
		pWeapon->SetThink( NULL );	// crowbar may be trying to swing again, etc.

	pWeapon->SetTouch( NULL );
	pWeapon->m_pPlayer = NULL;

	//ALERT ( at_console, "packed %s\n", STRING(pWeapon->pev->classname) );

	return TRUE;
}

//=========================================================
// CWeaponBox - PackAmmo
//=========================================================
BOOL CWeaponBox::PackAmmo( int iszName, int iCount )
{
	if ( FStringNull( iszName ) )
	{
		// error here
		ALERT( at_console, "NULL String in PackAmmo!\n" );
		return FALSE;
	}

	if ( iCount > 0 )
	{
		GiveAmmo( iCount, (char *) STRING( iszName ), iCount );
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CWeaponBox - GiveAmmo
//=========================================================
int CWeaponBox::GiveAmmo( int iCount, const char *szName, int iMax, int *pIndex /* = NULL*/ )
{
	int i;

	for ( i = 1; i < MAX_AMMO_SLOTS && !FStringNull( m_rgiszAmmo[i] ); i++ )
	{
		if ( stricmp( szName, STRING( m_rgiszAmmo[i] ) ) == 0 )
		{
			if ( pIndex )
				*pIndex = i;

			int iAdd = min( iCount, iMax - m_rgAmmo[i] );
			if ( iCount == 0 || iAdd > 0 )
			{
				m_rgAmmo[i] += iAdd;

				return i;
			}
			return -1;
		}
	}
	if ( i < MAX_AMMO_SLOTS )
	{
		if ( pIndex )
			*pIndex = i;

		m_rgiszAmmo[i] = MAKE_STRING( szName );
		m_rgAmmo[i] = iCount;

		return i;
	}
	ALERT( at_console, "out of named ammo slots\n" );
	return i;
}

//=========================================================
// CWeaponBox::HasWeapon - is a weapon of this type already
// packed in this box?
//=========================================================
BOOL CWeaponBox::HasWeapon( CBasePlayerItem *pCheckItem )
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while ( pItem )
	{
		if ( FClassnameIs( pItem->pev, STRING( pCheckItem->pev->classname ) ) )
		{
			return TRUE;
		}
		pItem = pItem->m_pNext;
	}

	return FALSE;
}

//=========================================================
// CWeaponBox::IsEmpty - is there anything in this box?
//=========================================================
BOOL CWeaponBox::IsEmpty( void )
{
	int i;

	for ( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if ( m_rgpPlayerItems[i] )
		{
			return FALSE;
		}
	}

	for ( i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		if ( !FStringNull( m_rgiszAmmo[i] ) )
		{
			// still have a bit of this type of ammo
			return FALSE;
		}
	}

	return TRUE;
}

//=========================================================
//=========================================================
void CWeaponBox::SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector( -16, -16, 0 );
	pev->absmax = pev->origin + Vector( 16, 16, 16 );
}

void CWeaponBox::RoundRespawn( void )
{
	Kill();
}

void CBasePlayerWeapon::PrintState( void )
{
	ALERT( at_console, "primary:  %f\n", m_flNextPrimaryAttack );
	ALERT( at_console, "idle   :  %f\n", m_flTimeWeaponIdle );
	ALERT( at_console, "m_finre:  %i\n", m_fInReload );
	ALERT( at_console, "m_iclip:  %i\n", m_iClip );
	ALERT( at_console, "m_iWeaponState:  %i\n", m_iWeaponState );
}

LINK_ENTITY_TO_CLASS( dod_ammo, CDodAmmoBox )

void CDodAmmoBox::Spawn( void )
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
}

void CDodAmmoBox::Precache( void )
{
	PRECACHE_MODEL( "models/allied_ammo.mdl" );
	PRECACHE_MODEL( "models/axis_ammo.mdl" );
}

void CDodAmmoBox::KillThink( void )
{
	UTIL_Remove( this );
}

void CDodAmmoBox::Touch( CBaseEntity *pOther )
{
	if ( !( pev->flags & FL_ONGROUND ) )
		return;
	if ( !pOther->IsPlayer() || !pOther->IsAlive() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if ( pOther == m_hOwner )
	{
		pPlayer->pev->frags += 1;
	}
	else
	{
		CBasePlayerWeapon *pWeapon = pPlayer->GetPrimaryWeapon();
		if ( pWeapon == NULL )
			return;

		ItemInfo ii;
		memset( &ii, 0, sizeof( ii ) );
		pWeapon->GetItemInfo( &ii );
		int iAmmoIndex = pPlayer->GetAmmoIndex( ii.pszAmmo1 );
		if ( iAmmoIndex < 0 )
			return;

		int iGive;
		int cls = pWeapon->Classify();
		if ( cls == CLASS_SMG )
			iGive = 2;
		else if ( cls == CLASS_RIFLE || cls == CLASS_SCOPED_RIFLE )
			iGive = 2;
		else if ( cls == CLASS_MACHINEGUN || cls == CLASS_ROCKET )
			iGive = 1;
		else
			return;

		for ( int i = 0; i < iGive; i++ )
		{
			if ( pPlayer->m_rgAmmo[iAmmoIndex] + ii.iMaxClip <= ii.iMaxAmmo1 )
				pPlayer->m_rgAmmo[iAmmoIndex] += ii.iMaxClip;
		}

		if ( m_hOwner )
			( (CBasePlayer *) (CBaseEntity *)m_hOwner )->AddObjScore( 1, FALSE );
	}

	EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/weaponpickup.wav", 1.0, 0.8, 0, 100 );
	SetTouch( NULL );
	UTIL_Remove( this );
}

LINK_ENTITY_TO_CLASS( ammo_generic_american, GenericAmmoAmerican );
LINK_ENTITY_TO_CLASS( ammo_generic_german, GenericAmmoGerman );
LINK_ENTITY_TO_CLASS( ammo_generic_british, GenericAmmoBritish );

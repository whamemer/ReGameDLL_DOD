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

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"


class CWorldItem : public CBaseEntity
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	int m_iType;
};

LINK_ENTITY_TO_CLASS( world_items, CWorldItem );

void CWorldItem::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_iType = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CWorldItem::Spawn( void )
{
	CBaseEntity *pEntity = NULL;

	switch ( m_iType )
	{
	case 44:  // ITEM_BATTERY:
		pEntity = CBaseEntity::Create( "item_battery", pev->origin, pev->angles );
		break;
	case 42:  // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create( "item_antidote", pev->origin, pev->angles );
		break;
	case 43:  // ITEM_SECURITY:
		pEntity = CBaseEntity::Create( "item_security", pev->origin, pev->angles );
		break;
	case 45:  // ITEM_SUIT:
		pEntity = CBaseEntity::Create( "item_suit", pev->origin, pev->angles );
		break;
	}

	if ( !pEntity )
	{
		ALERT( at_console, "unable to create world_item %d\n", m_iType );
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY( edict() );
}

void CItem::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	SetTouch( &CItem::ItemTouch );

	if ( DROP_TO_FLOOR( ENT( pev ) ) == 0 )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ), pev->origin.x,
			   pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
		return;
	}
}

extern int gEvilImpulse101;

void CItem::ItemTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
	{
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *) pOther;

	// ok, a player is touching this item, but can he have it?
	if ( !g_pGameRules->CanHaveItem( pPlayer, this ) )
	{
		// no? Ignore the touch.
		return;
	}

	if ( MyTouch( pPlayer ) )
	{
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );

		// player grabbed the item.
		g_pGameRules->PlayerGotItem( pPlayer, this );
		if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
		{
			Respawn();
		}
		else
		{
			UTIL_Remove( this );
		}
	}
	else if ( gEvilImpulse101 )
	{
		UTIL_Remove( this );
	}
}

CBaseEntity *CItem::Respawn( void )
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin( pev, g_pGameRules->VecItemRespawnSpot( this ) );  // blip to whereever you should respawn.

	SetThink( &CItem::Materialize );
	pev->nextthink = g_pGameRules->FlItemRespawnTime( this );
	return this;
}

void CItem::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		// DoD binary plays no sound here (unlike HLSDK).
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch( &CItem::ItemTouch );
}

// DOD GenericAmmoAmerican German British - source restore/triggers/items.cpp - original files dod/dlls/weapons.cpp plus weapons.h
// items.cpp -- DoD ammo pickups (dod_ammo family) + items.cpp parity notes
// Restored from dod.so. Ghidra addr = link addr + 0x10000.
//
// NOTE ON FILE HOMES (per addr2line, not the brief): the dod_ammo family
// lives in dod/dlls/weapons.cpp + weapons.h, NOT in dod/dlls/items.cpp.
// dod/dlls/items.cpp itself is SDK-parity (CWorldItem/CItem, see bottom).
//
// SCOPE:
//   PORTED here: GenericAmmoAmerican / GenericAmmoGerman / GenericAmmoBritish
//     (LINKs ammo_generic_american/german/british, weapons.cpp:2104-2106).
//   REFERENCED (weapons agent owns bodies): CDodAmmoBox (dod_ammo,
//     weapons.cpp:2006-2060) -> ~/rev/restore/weapons/weapons.cpp + weapons.h;
//     CBasePlayerAmmo::GiveGenericAmmo (weapons.cpp:1514) -> same;
//     all ammo_* clips (ammo_30cal ...) -> weapons_allied/_axis agents.
//   SDK-IDENTICAL (not ported): CWorldItem::KeyValue/Spawn,
//     CItem::Spawn/ItemTouch/Respawn/Materialize (dod/dlls/items.cpp).

// Generic team ammo box. German extends American; British extends American.
// AddAmmo/Precache are inherited from GenericAmmoAmerican by German/British
// (no overrides in the binary: only _ZN19GenericAmmoAmerican7AddAmmoEv and
// GenericAmmoAmerican::Precache exist).

class GenericAmmoAmerican : public CBasePlayerAmmo
{
  public:
	void Spawn();
	void Precache();
	BOOL AddAmmo( CBaseEntity *pOther );
};

class GenericAmmoGerman : public GenericAmmoAmerican
{
  public:
	void Spawn();
};

class GenericAmmoBritish : public GenericAmmoAmerican
{
  public:
	void Spawn();
};

// Factory only allocates the entity (GetClassPtr); Spawn is invoked by the
// engine via DispatchSpawn. DoD binary factory does NOT call Spawn.
LINK_ENTITY_TO_CLASS( ammo_generic_american, GenericAmmoAmerican );
LINK_ENTITY_TO_CLASS( ammo_generic_german, GenericAmmoGerman );
LINK_ENTITY_TO_CLASS( ammo_generic_british, GenericAmmoBritish );

void GenericAmmoAmerican::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	UTIL_SetOrigin( pev, pev->origin );
	SetTouch( &CBasePlayerAmmo::DefaultTouch );
	pev->team = TEAM_ALLIES;	// Allies
}

void GenericAmmoAmerican::Precache()
{
	PRECACHE_MODEL( "models/allied_ammo.mdl" );
	PRECACHE_MODEL( "models/axis_ammo.mdl" );
	PRECACHE_SOUND( "items/ammopickup.wav" );
}

// Shared give logic lives in CBasePlayerAmmo::GiveGenericAmmo (weapons agent).
BOOL GenericAmmoAmerican::AddAmmo( CBaseEntity *pOther )
{
	if ( CBasePlayerAmmo::GiveGenericAmmo( pOther ) )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, 0.8, 0, 100 );
		return TRUE;
	}
	return FALSE;
}

void GenericAmmoGerman::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	UTIL_SetOrigin( pev, pev->origin );
	SetTouch( &CBasePlayerAmmo::DefaultTouch );
	pev->team = TEAM_AXIS;	// Axis
}

// QUIRK: British box uses the Allied model and team 1 (same as American).
void GenericAmmoBritish::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	UTIL_SetOrigin( pev, pev->origin );
	SetTouch( &CBasePlayerAmmo::DefaultTouch );
	pev->team = TEAM_ALLIES;	// Allies (shared box)
}

// ---- dod/dlls/items.cpp: SDK-IDENTICAL, bodies NOT ported ----
// CWorldItem::KeyValue (items.cpp:49, Ghidra 0010b250): "type" -> m_iType,
//   else CBaseEntity::KeyValue. Identical to SDK.
// CWorldItem::Spawn (items.cpp:62-68, Ghidra 0010af30): switch on m_iType:
//   42 -> item_antidote | 43 -> item_security | 44 -> item_battery |
//   45 -> item_suit (CBaseEntity::Create + target/targetname/spawnflags
//   copy), else "unable to create world_item %d". Identical to SDK.
// CItem::Spawn (SDK shape): MOVETYPE_TOSS/SOLID_TRIGGER, origin/size,
//   touch = ItemTouch, DROP_TO_FLOOR or remove with "Item %s fell out" error.
// CItem::ItemTouch (items.cpp:128-134, Ghidra 0010b080+) and
// CItem::Respawn (items.cpp:151) : SDK shape.
// CItem::Materialize (Ghidra 0010ae00, size 0x3c): SDK shape but the DoD
//   binary has NO suit charge sound (EMIT_SOUND_DYN removed to match).
// The item_suit/item_battery/item_antidote/item_security classes exist only
// as CWorldItem::Spawn targets; their LINKs/DebugInfo match SDK layout.

// mapmarker.cpp -- DoD CMapMarker - original file dod/dlls/mapmarker.cpp
// Restored from dod.so - source notes in restore/objectives/timers_relays.cpp
#include "extdll.h"
#include "util.h"
#include "objective.h"
// Full bodies in restore/objectives/timers_relays.cpp comments - player side in player agent

// rehash classname dod_mapmarker, nextthink = time + max(pcvar_mp_markerstaytime,2.0),
// Think=Die, FL_CUSTOMENTITY.
void CMapMarker::Spawn( void )
{
	pev->solid = SOLID_NOT;
	SET_MODEL( edict(), "sprites/markernull.spr" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	pev->classname = MAKE_STRING( "dod_mapmarker" );
	float stay = 10.0;
	extern cvar_t *pcvar_mp_markerstaytime;
	if ( pcvar_mp_markerstaytime && 2.0 <= pcvar_mp_markerstaytime->value )
		stay = pcvar_mp_markerstaytime->value;
	pev->nextthink = gpGlobals->time + stay;
	SetThink( &CMapMarker::Die );
	pev->flags |= FL_CUSTOMENTITY;
}

void CMapMarker::Die( void )
{
	CBaseEntity::UpdateOnRemove();
	pev->flags |= FL_KILLME;
	pev->targetname = 0;
}

// classname dod_mapmarker, body=icon, owner/team from creator, angles (0,90,0),
// fuser1=now, counter++.
CMapMarker *CMapMarker::CreateMapMarker( Vector origin, Vector angles, CBaseEntity *pOwner, int icon )
{
	edict_t *pent = CREATE_ENTITY();
	CMapMarker *pMarker = GetClassPtr( (CMapMarker *)VARS( pent ) );
	UTIL_SetOrigin( pMarker->pev, origin );
	pMarker->Spawn();
	if ( pOwner )
	{
		pMarker->pev->owner = pOwner->edict();
		pMarker->pev->team = pOwner->pev->team;
	}
	pMarker->pev->body = icon;
	pMarker->pev->angles = Vector( 0, 90, 0 );
	pMarker->pev->fuser1 = gpGlobals->time;
	return pMarker;
}

LINK_ENTITY_TO_CLASS( dod_mapmarker, CMapMarker );

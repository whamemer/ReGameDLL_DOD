// mapmarker.cpp -- DoD CMapMarker - original file dod/dlls/mapmarker.cpp
// Restored from dod.so - source notes in restore/objectives/timers_relays.cpp
#include "extdll.h"
#include "util.h"
#include "objective.h"
#include "mapmarker.h"

LINK_ENTITY_TO_CLASS( dod_mapmarker, CMapMarker );

CMapMarker *CMapMarker::CreateMapMarker( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, int markerIcon )
{
	static int i = 0;

	edict_t *pent = CREATE_ENTITY();
	CMapMarker *pMarker = GetClassPtr( (CMapMarker *) VARS( pent ) );
	UTIL_SetOrigin( pMarker, vecOrigin );
	pMarker->Spawn();

	if ( pOwner )
	{
		pMarker->pev->owner = pOwner->edict();
		pMarker->pev->team = pOwner->pev->team;
	}

	pMarker->pev->body = markerIcon;
	pMarker->pev->angles = Vector( 0, 90, 0 );
	pMarker->pev->fuser1 = gpGlobals->time;
	MAKE_STRING_CLASS( "dod_mapmarker", pMarker->pev );
	i++;

	return pMarker;
}

// Full bodies in restore/objectives/timers_relays.cpp comments - player side in player agent

void CMapMarker::Spawn( void )
{
	pev->solid = SOLID_NOT;
	SET_MODEL( edict(), "sprites/markernull.spr" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	MAKE_STRING_CLASS( "dod_mapmarker", pev );

	float stay = 10.0;
	extern cvar_t *pcvar_mp_markerstaytime;
	if ( pcvar_mp_markerstaytime )
		stay = ( pcvar_mp_markerstaytime->value >= 2.0 ) ? pcvar_mp_markerstaytime->value : 2.0;

	pev->nextthink = gpGlobals->time + stay;
	SetThink( &CMapMarker::Die );
	pev->flags |= FL_CUSTOMENTITY;
}

int CMapMarker::GetIcon( void )
{
	return pev->body;
}

void CMapMarker::Die( void )
{
	UTIL_Remove( this );
}

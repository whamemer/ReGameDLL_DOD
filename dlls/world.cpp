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

===== world.cpp ========================================================

  precaches and defs for entities and other data that must always be available.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "nodes.h"
#include "soundent.h"
#include "client.h"
#include "decals.h"
#include "skill.h"
#include "effects.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

extern const char *GetStdLightStyle( int iStyle );

extern CGraph WorldGraph;
extern CSoundEnt *pSoundEnt;

extern CBaseEntity *g_pLastSpawn;
CGlobalState gGlobalState;
extern DLL_GLOBAL int gDisplayTitle;

extern void W_Precache( void );

//
// This must match the list in util.h
//
DLL_DECALLIST gDecals[] = {
	{ "{shot1", 0 },	  // DECAL_GUNSHOT1
	{ "{shot2", 0 },	  // DECAL_GUNSHOT2
	{ "{shot3", 0 },	  // DECAL_GUNSHOT3
	{ "{shot4", 0 },	  // DECAL_GUNSHOT4
	{ "{shot5", 0 },	  // DECAL_GUNSHOT5
	{ "{lambda01", 0 },	  // DECAL_LAMBDA1
	{ "{lambda02", 0 },	  // DECAL_LAMBDA2
	{ "{lambda03", 0 },	  // DECAL_LAMBDA3
	{ "{lambda04", 0 },	  // DECAL_LAMBDA4
	{ "{lambda05", 0 },	  // DECAL_LAMBDA5
	{ "{lambda06", 0 },	  // DECAL_LAMBDA6
	{ "{scorch1", 0 },	  // DECAL_SCORCH1
	{ "{scorch2", 0 },	  // DECAL_SCORCH2
	{ "{blood1", 0 },	  // DECAL_BLOOD1
	{ "{blood2", 0 },	  // DECAL_BLOOD2
	{ "{blood3", 0 },	  // DECAL_BLOOD3
	{ "{blood4", 0 },	  // DECAL_BLOOD4
	{ "{blood5", 0 },	  // DECAL_BLOOD5
	{ "{blood6", 0 },	  // DECAL_BLOOD6
	{ "{yblood1", 0 },	  // DECAL_YBLOOD1
	{ "{yblood2", 0 },	  // DECAL_YBLOOD2
	{ "{yblood3", 0 },	  // DECAL_YBLOOD3
	{ "{yblood4", 0 },	  // DECAL_YBLOOD4
	{ "{yblood5", 0 },	  // DECAL_YBLOOD5
	{ "{yblood6", 0 },	  // DECAL_YBLOOD6
	{ "{break1", 0 },	  // DECAL_GLASSBREAK1
	{ "{break2", 0 },	  // DECAL_GLASSBREAK2
	{ "{break3", 0 },	  // DECAL_GLASSBREAK3
	{ "{bigshot1", 0 },	  // DECAL_BIGSHOT1
	{ "{bigshot2", 0 },	  // DECAL_BIGSHOT2
	{ "{bigshot3", 0 },	  // DECAL_BIGSHOT3
	{ "{bigshot4", 0 },	  // DECAL_BIGSHOT4
	{ "{bigshot5", 0 },	  // DECAL_BIGSHOT5
	{ "{spit1", 0 },	  // DECAL_SPIT1
	{ "{spit2", 0 },	  // DECAL_SPIT2
	{ "{bproof1", 0 },	  // DECAL_BPROOF1
	{ "{gargstomp", 0 },  // DECAL_GARGSTOMP1,	// Gargantua stomp crack
	{ "{smscorch1", 0 },  // DECAL_SMALLSCORCH1,	// Small scorch mark
	{ "{smscorch2", 0 },  // DECAL_SMALLSCORCH2,	// Small scorch mark
	{ "{smscorch3", 0 },  // DECAL_SMALLSCORCH3,	// Small scorch mark
	{ "{mommablob", 0 },  // DECAL_MOMMABIRTH		// BM Birth spray
	{ "{mommablob", 0 },  // DECAL_MOMMASPLAT		// BM Mortar spray?? need decal
};

/*
==============================================================================

BODY QUE

==============================================================================
*/

#define SF_DECAL_NOTINDEATHMATCH 2048

class CDecal : public CBaseEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT StaticDecal( void );
	void EXPORT TriggerDecal( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( infodecal, CDecal );

// UNDONE:  These won't get sent to joining players in multi-player
void CDecal::Spawn( void )
{
	if( pev->skin < 0 || ( gpGlobals->deathmatch && FBitSet( pev->spawnflags, SF_DECAL_NOTINDEATHMATCH ) ) )
	{
		REMOVE_ENTITY( ENT( pev ) );
		return;
	}

	if( FStringNull( pev->targetname ) )
	{
		SetThink( &CDecal::StaticDecal );
		// if there's no targetname, the decal will spray itself on as soon as the world is done spawning.
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		// if there IS a targetname, the decal sprays itself on when it is triggered.
		SetThink( &CDecal::SUB_DoNothing );
		SetUse( &CDecal::TriggerDecal );
	}
}

void CDecal::TriggerDecal( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// this is set up as a USE function for infodecals that have targetnames, so that the
	// decal doesn't get applied until it is fired. (usually by a scripted sequence)
	TraceResult trace;
	int entityIndex;

	UTIL_TraceLine( pev->origin - Vector( 5, 5, 5 ), pev->origin + Vector( 5, 5, 5 ), ignore_monsters, ENT( pev ),
		&trace );

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BSPDECAL );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_SHORT( ( int ) pev->skin );
	entityIndex = ( short ) ENTINDEX( trace.pHit );
	WRITE_SHORT( entityIndex );
	if( entityIndex )
		WRITE_SHORT( ( int ) VARS( trace.pHit )->modelindex );
	MESSAGE_END();

	SetThink( &CDecal::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CDecal::StaticDecal( void )
{
	TraceResult trace;
	int entityIndex, modelIndex;

	UTIL_TraceLine( pev->origin - Vector( 5, 5, 5 ), pev->origin + Vector( 5, 5, 5 ), ignore_monsters, ENT( pev ),
		&trace );

	entityIndex = ( short ) ENTINDEX( trace.pHit );
	if( entityIndex )
		modelIndex = ( int ) VARS( trace.pHit )->modelindex;
	else
		modelIndex = 0;

	g_engfuncs.pfnStaticDecal( pev->origin, ( int ) pev->skin, entityIndex, modelIndex );

	SUB_Remove();
}

void CDecal::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "texture" ) )
	{
		pev->skin = DECAL_INDEX( pkvd->szValue );

		// Found
		if( pev->skin >= 0 )
			return;
		ALERT( at_console, "Can't find decal %s\n", pkvd->szValue );
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

CGlobalState::CGlobalState( void )
{
	Reset();
}

void CGlobalState::Reset( void )
{
	m_pList = NULL;
	m_listCount = 0;
}

globalentity_t *CGlobalState::Find( string_t globalname )
{
	if( !globalname )
		return NULL;

	globalentity_t *pTest;
	const char *pEntityName = STRING( globalname );

	pTest = m_pList;
	while( pTest )
	{
		if( FStrEq( pEntityName, pTest->name ) )
			break;

		pTest = pTest->pNext;
	}

	return pTest;
}

// This is available all the time now on impulse 104, remove later
//
//#ifdef _DEBUG
void CGlobalState::DumpGlobals( void )
{
	static const char *estates[] = { "Off", "On", "Dead" };
	globalentity_t *pTest;

	ALERT( at_console, "-- Globals --\n" );
	pTest = m_pList;
	while( pTest )
	{
		ALERT( at_console, "%s: %s (%s)\n", pTest->name, pTest->levelName, estates[pTest->state] );
		pTest = pTest->pNext;
	}
}
//#endif

void CGlobalState::EntityAdd( string_t globalname, string_t mapName, GLOBALESTATE state )
{
	ASSERT( !Find( globalname ) );

	globalentity_t *pNewEntity = ( globalentity_t * ) calloc( sizeof( globalentity_t ), 1 );
	ASSERT( pNewEntity != NULL );
	pNewEntity->pNext = m_pList;
	m_pList = pNewEntity;
	strcpy( pNewEntity->name, STRING( globalname ) );
	strcpy( pNewEntity->levelName, STRING( mapName ) );
	pNewEntity->state = state;
	m_listCount++;
}

void CGlobalState::EntitySetState( string_t globalname, GLOBALESTATE state )
{
	globalentity_t *pEnt = Find( globalname );

	if( pEnt )
		pEnt->state = state;
}

const globalentity_t *CGlobalState::EntityFromTable( string_t globalname )
{
	globalentity_t *pEnt = Find( globalname );

	return pEnt;
}

GLOBALESTATE CGlobalState::EntityGetState( string_t globalname )
{
	globalentity_t *pEnt = Find( globalname );
	if( pEnt )
		return pEnt->state;

	return GLOBAL_OFF;
}

// Global Savedata for Delay
TYPEDESCRIPTION CGlobalState::m_SaveData[] = {
	DEFINE_FIELD( CGlobalState, m_listCount, FIELD_INTEGER ),
};

// Global Savedata for Delay
TYPEDESCRIPTION gGlobalEntitySaveData[] = {
	DEFINE_ARRAY( globalentity_t, name, FIELD_CHARACTER, 64 ),
	DEFINE_ARRAY( globalentity_t, levelName, FIELD_CHARACTER, 32 ),
	DEFINE_FIELD( globalentity_t, state, FIELD_INTEGER ),
};

int CGlobalState::Save( CSave &save )
{
	int i;
	globalentity_t *pEntity;

	if( !save.WriteFields( "GLOBAL", this, m_SaveData, ARRAYSIZE( m_SaveData ) ) )
		return 0;

	pEntity = m_pList;

	for( i = 0; i < m_listCount && pEntity; i++ )
	{
		if( !save.WriteFields( "GENT", pEntity, gGlobalEntitySaveData, ARRAYSIZE( gGlobalEntitySaveData ) ) )
			return 0;

		pEntity = pEntity->pNext;
	}

	return 1;
}

int CGlobalState::Restore( CRestore &restore )
{
	int i, listCount;
	globalentity_t tmpEntity;

	ClearStates();

	if( !restore.ReadFields( "GLOBAL", this, m_SaveData, ARRAYSIZE( m_SaveData ) ) )
		return 0;

	listCount = m_listCount;  // Get new list count
	m_listCount = 0;		  // Clear loaded data

	for( i = 0; i < listCount; i++ )
	{
		if( !restore.ReadFields( "GENT", &tmpEntity, gGlobalEntitySaveData, ARRAYSIZE( gGlobalEntitySaveData ) ) )
			return 0;

		EntityAdd( MAKE_STRING( tmpEntity.name ), MAKE_STRING( tmpEntity.levelName ), tmpEntity.state );
	}

	return 1;
}

void CGlobalState::EntityUpdate( string_t globalname, string_t mapname )
{
	globalentity_t *pEnt = Find( globalname );

	if( pEnt )
		strcpy( pEnt->levelName, STRING( mapname ) );
}

void CGlobalState::ClearStates( void )
{
	globalentity_t *pFree = m_pList;

	while( pFree )
	{
		globalentity_t *pNext = pFree->pNext;
		free( pFree );
		pFree = pNext;
	}

	Reset();
}

void SaveGlobalState( SAVERESTOREDATA *pSaveData )
{
	CSave saveHelper( pSaveData );
	gGlobalState.Save( saveHelper );
}

void RestoreGlobalState( SAVERESTOREDATA *pSaveData )
{
	CRestore restoreHelper( pSaveData );
	gGlobalState.Restore( restoreHelper );
}

void ResetGlobalState( void )
{
	gGlobalState.ClearStates();
	gInitHUD = TRUE;  // Init the HUD on a new game / load game
}

// moved CWorld class definition to cbase.h
//=======================
// CWorld
//
// This spawns first when each level begins.
//=======================

LINK_ENTITY_TO_CLASS( worldspawn, CWorld );

#define SF_WORLD_DARK 0x0001	   // Fade from black at startup
#define SF_WORLD_TITLE 0x0002	   // Display game title at startup
#define SF_WORLD_FORCETEAM 0x0004  // Force teams

extern DLL_GLOBAL BOOL g_fGameOver;

void CWorld::Spawn( void )
{
	EmptyEntityHashTable();
	g_fGameOver = FALSE;
	Precache();
}
void CWorld::Precache( void )
{
	g_pLastSpawn = NULL;
	CVAR_SET_FLOAT( "sv_gravity", 800 );
	CVAR_SET_FLOAT( "sv_stepsize", 18 );
	CVAR_SET_STRING( "room_type", "0" );

	if( g_pGameRules )
		::operator delete( g_pGameRules );

	g_pGameRules = InstallGameRules();
	g_pGameRules->Init();

	CSoundEnt *pSoundEnt = GetClassPtr( ( CSoundEnt * ) NULL );
	::pSoundEnt = pSoundEnt;
	pSoundEnt->Spawn();

	if( !::pSoundEnt )
		ALERT( at_console, "**COULD NOT CREATE SOUNDENT**\n" );

	SENTENCEG_Init();
	TEXTURETYPE_Init();
	W_Precache();
	ClientPrecache();

	PRECACHE_SOUND( "common/null.wav" );
	PRECACHE_SOUND( "items/weaponpickup.wav" );

	g_Language = ( int ) CVAR_GET_FLOAT( "sv_language" );

	for( int i = 0; i <= 13; i++ )
		LIGHT_STYLE( i, GetStdLightStyle( i ) );

	LIGHT_STYLE( 63, "a" );

	for( int i = 0; i < 42; i++ )
		gDecals[i].index = DECAL_INDEX( gDecals[i].name );

	WorldGraph.InitGraph();

	if( WorldGraph.CheckNODFile( ( char * ) STRING( gpGlobals->mapname ) ) )
	{
		if( WorldGraph.FLoadGraph( ( char * ) STRING( gpGlobals->mapname ) ) )
			ALERT( at_console, "\n*Graph Loaded!\n" );
		else
			ALERT( at_console, "*Error opening .NOD file\n" );
	}
	else
		WorldGraph.AllocNodes();

	if( pev->speed <= 0 )
	{
		CVAR_SET_FLOAT( "sv_zmax", 4096 );
	}
	else
		CVAR_SET_FLOAT( "sv_zmax", pev->speed );
	if( FStringNull( pev->netname ) )
		;
	else
	{
		ALERT( at_aiconsole, "Chapter title: %s\n", STRING( pev->netname ) );
		CBaseEntity *pEntity = CBaseEntity::Create( "env_message", g_vecZero, g_vecZero, NULL );

		if( pEntity )
		{
			pEntity->SetThink( &CBaseEntity::SUB_CallUseToggle );
			pEntity->pev->message = pev->netname;
			pev->netname = 0;
			pEntity->pev->nextthink = gpGlobals->time + 0.3;
			pEntity->pev->spawnflags = SF_MESSAGE_ONCE;
		}
	}

	if( FBitSet( pev->spawnflags, SF_WORLD_DARK ) )
		CVAR_SET_FLOAT( "v_dark", 1.0 );
	else
		CVAR_SET_FLOAT( "v_dark", 0.0 );

	gDisplayTitle = FBitSet( pev->spawnflags, SF_WORLD_TITLE ) ? 1 : 0;

	if( FBitSet( pev->spawnflags, 4 ) == 0 )
		CVAR_SET_FLOAT( "mp_defaultteam", 0.0 );
	else
		CVAR_SET_FLOAT( "mp_defaultteam", 1.0 );
}

//
// Just to ignore the "wad" field.
//
void CWorld::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "skyname" ) )
	{
		CVAR_SET_STRING( "sv_skyname", pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "sounds" ) )
	{
		gpGlobals->cdAudioTrack = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "WaveHeight" ) )
	{
		pev->scale = atof( pkvd->szValue ) * ( 1.0 / 8.0 );
		pkvd->fHandled = TRUE;
		CVAR_SET_FLOAT( "sv_wateramp", pev->scale );
	}
	else if( FStrEq( pkvd->szKeyName, "MaxRange" ) )
	{
		pev->speed = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "chaptertitle" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "startdark" ) )
	{
		if( atoi( pkvd->szValue ) )
			SetBits( pev->spawnflags, SF_WORLD_DARK );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "newunit" ) )
	{
		if( atoi( pkvd->szValue ) )
			CVAR_SET_FLOAT( "sv_newunit", 1.0 );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "gametitle" ) )
	{
		if( atoi( pkvd->szValue ) )
			SetBits( pev->spawnflags, SF_WORLD_TITLE );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "mapteams" ) )
	{
		pev->team = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "defaultteam" ) )
	{
		if( atoi( pkvd->szValue ) )
			SetBits( pev->spawnflags, 4 );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "allowmonsters" ) )
	{
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "mapfog" ) )
	{
		CVAR_SET_FLOAT( "map_fog", atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "map_fog_start" ) )
	{
		CVAR_SET_FLOAT( "map_fog_start", atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "map_fog_end" ) )
	{
		CVAR_SET_FLOAT( "map_fog_end", atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "map_fog_red" ) )
	{
		CVAR_SET_FLOAT( "map_fog_red", atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "map_fog_green" ) )
	{
		CVAR_SET_FLOAT( "map_fog_green", atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "map_fog_blue" ) )
	{
		CVAR_SET_FLOAT( "map_fog_blue", atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "player_team" ) )
	{
		m_nPlayerTeam = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

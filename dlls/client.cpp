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

===== client.cpp ========================================================

  client/server game specific stuff

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "player.h"
#include "spectator.h"
#include "client.h"
#include "soundent.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "game.h"
#include "customentity.h"
#include "objective.h"
#include "weapons.h"
#include "weaponinfo.h"
#include "usercmd.h"
#include "netadr.h"
#include "pm_shared.h"

#if !defined( _WIN32 )
#include <ctype.h>
#endif

extern DLL_GLOBAL ULONG g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL g_fGameOver;
extern DLL_GLOBAL int g_iSkillLevel;
extern DLL_GLOBAL ULONG g_ulFrameCount;

extern int giPrecacheGrunt;
extern int gmsgSayText;
extern int gmsgVGUIMenu;

extern int g_teamplay;

extern cvar_t cvar_timelimit;
extern cvar_t cvar_teamplay;
float g_flTimeLimit;
extern cvar_t *pcvar_mp_clan_match;
extern cvar_t *pcvar_mp_chatmsg;
extern void GetCVARPointers( void );

void LinkUserMessages( void );

// Returns the number of seconds left on the map timer (clamped to >= 0),
// or 0 when mp_timelimit is disabled.
int get_maptime_left( void )
{
	if ( cvar_timelimit.value == 0.0f )
		return 0;

	int iTimeLeft = (int) ( cvar_timelimit.value * 60.0f + g_flTimeLimit - gpGlobals->time );

	if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value > 0.0f )
	{
		iTimeLeft = (int) ( cvar_timelimit.value * 60.0f + ( (CDoDTeamPlay *) g_pGameRules )->getMapTime() - gpGlobals->time );
	}

	if ( iTimeLeft < 0 )
		return 0;

	return iTimeLeft;
}

/*
===========
ClientConnect

called when a player connects to a server
============
*/
BOOL ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] )
{
	return g_pGameRules->ClientConnected( pEntity, pszName, pszAddress, szRejectReason );

	// a client connecting during an intermission can cause problems
	//	if (intermission_running)
	//		ExitIntermission ();
}

/*
===========
ClientDisconnect

called when a player disconnects from a server

GLOBALS ASSUMED SET:  g_fGameOver
============
*/
void ClientDisconnect( edict_t *pEntity )
{
	if ( g_fGameOver )
		return;

	// DoD: binary prints the localized disconnect notice (was a saytext in HL SDK)
	UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_disconnected", STRING( pEntity->v.netname ) );

	CSound *pSound;
	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( pEntity ) );
	{
		// since this client isn't around to think anymore, reset their sound.
		if ( pSound )
		{
			pSound->Reset();
		}
	}

	// since the edict doesn't get deleted, fix it so it doesn't interfere.
	pEntity->v.flags = FL_DORMANT;
	pEntity->v.takedamage = DAMAGE_NO;	// don't attract autoaim
	pEntity->v.solid = SOLID_NOT;		// nonsolid

	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE( pEntity );
	if ( pPlayer )
		pPlayer->SetThink( NULL );

	UTIL_SetEdictOrigin( pEntity, pEntity->v.origin );

	g_pGameRules->ClientDisconnected( pEntity );
}

// called by ClientKill and DeadThink
void respawn( entvars_t *pev, BOOL fCopyCorpse )
{
	if ( gpGlobals->coop || gpGlobals->deathmatch )
	{
		// respawn player
		GetClassPtr( (CBasePlayer *) pev )->Spawn();
	}
	else
	{  // restart the entire server
		SERVER_COMMAND( "reload\n" );
	}
}

/*
============
ClientKill

Player entered the suicide command

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
============
*/
void ClientKill( edict_t *pEntity )
{
	CBasePlayer *pl = (CBasePlayer *) CBasePlayer::Instance( &pEntity->v );
	if ( pEntity->v.iuser1 != 0 || pEntity->v.iuser2 != 0 )
		return;
	if ( pl->m_iJoiningState != 2 )
		return;
	if ( pl->m_fNextSuicideTime > gpGlobals->time )
		return;
	pl->m_fNextSuicideTime = gpGlobals->time + 1.0;
	pEntity->v.health = 0;
	pl->m_deathMsg = 0;
	pl->Killed( &pEntity->v, GIB_NEVER );
}

/*
===========
ClientPutInServer

called each time a player is spawned
============
*/
void ClientPutInServer( edict_t *pEntity )
{
	CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *) &pEntity->v );
	pPlayer->SetCustomDecalFrames( -1 );
	char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer( pEntity );
	pPlayer->SetPrefsFromUserinfo( infobuffer );
	if ( !g_pGameRules->IsMultiplayer() )
	{
		pPlayer->Spawn();
		return;
	}
	pPlayer->pev->iuser1 = pPlayer->pev->iuser2 = 0;
	pPlayer->m_bIsObserver = false;
	pPlayer->m_hObserverTarget = NULL;
	pPlayer->m_imissedwave = 0;
	pPlayer->m_irdytospawn = 0;
	pPlayer->has_disconnected = false;
	pPlayer->m_iJoiningState = JUSTJOINED;
	pPlayer->Spawn();
	pPlayer->i_seenAxisWinScene = 0;
	pPlayer->i_seenAllieLooseScene = 0;
	pPlayer->i_seenAxisLooseScene = 0;
	pPlayer->i_seenAllieDrawScene = 0;
	pPlayer->i_seenAxisDrawScene = 0;
	pPlayer->i_seenAllieSpawnScene = 0;
	pPlayer->i_seenAxisSpawnScene = 0;
	pPlayer->i_seenAllieWinScene = 0;
	pPlayer->m_iNextClass = -1;
	pPlayer->m_deathMsg = 1;
	pPlayer->m_fGameHUDInitialized = false;
	pPlayer->m_bIsObserver = false;
	pPlayer->m_iNumTKs = 0;
	pPlayer->m_voteid = -1;
	pPlayer->m_chatMsg = 1;
	pPlayer->pev->team = 0;
	pPlayer->pev->solid = SOLID_NOT;
	pPlayer->pev->movetype = MOVETYPE_NOCLIP;
	pPlayer->pev->effects = EF_NODRAW | EF_NOINTERP;
	pPlayer->pev->takedamage = DAMAGE_NO;
	pPlayer->pev->deadflag = DEAD_RESPAWNABLE;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = 1;
	pPlayer->m_bBazookaDeployed = false;
	g_engfuncs.pfnSetClientMaxspeed( ENT( pPlayer->pev ), 1.0 );
	SET_MODEL( ENT( pPlayer->pev ), "models/player.mdl" );
	pPlayer->SetThink( NULL );
}

#include "voice_gamemgr.h"
extern CVoiceGameMgr g_VoiceGameMgr;

//// HOST_SAY
// String comes in as
// say blah blah blah
// or as
// blah blah blah
//
void Host_Say( edict_t *pEntity, int teamonly )
{
	// DoD: allow the server to disable chat entirely
	if ( pcvar_mp_chatmsg && pcvar_mp_chatmsg->value <= 0.0f )
		return;

	CBasePlayer *client;
	int j;
	char *p;
	char text[128];
	char szTemp[256];
	const char *pcmd = CMD_ARGV( 0 );

	CBasePlayer *pSender = (CBasePlayer *) CBaseEntity::Instance( pEntity );

	// DoD: anti-spam gate (0.66s between messages)
	if ( pSender->m_flLastTalkTime != 0.0f &&
		 gpGlobals->time - pSender->m_flLastTalkTime < 0.66f )
		return;
	pSender->m_flLastTalkTime = gpGlobals->time;

	// We can get a raw string now, without the "say " prepended
	if ( CMD_ARGC() == 0 )
		return;

	if ( !stricmp( pcmd, "say" ) || !stricmp( pcmd, "say_team" ) )
	{
		if ( CMD_ARGC() >= 2 )
		{
			p = (char *) CMD_ARGS();
		}
		else
		{
			// say with a blank message, nothing to do
			return;
		}
	}
	else  // Raw text, need to prepend argv[0]
	{
		if ( CMD_ARGC() >= 2 )
		{
			sprintf( szTemp, "%s %s", (char *) pcmd, (char *) CMD_ARGS() );
		}
		else
		{
			// Just a one word command, use the first word...sigh
			sprintf( szTemp, "%s", (char *) pcmd );
		}
		p = szTemp;
	}

	// remove quotes if present
	if ( *p == '"' )
	{
		p++;
		p[strlen( p ) - 1] = 0;
	}

	// make sure the text has content (at least one printable, non-space char)
	{
		bool bHasContent = false;

		for ( char *pc = p; *pc != 0; pc++ )
		{
			if ( isprint( (unsigned char) *pc ) && !isspace( (unsigned char) *pc ) )
			{
				bHasContent = true;
				break;
			}
		}

		if ( !bHasContent )
			return;	 // no character found, so say nothing
	}

	// DoD: %-substitution expansion ( %t time, %h health, %i target name,
	// %r target health, %c class, %l location, %s/%S identity )
	if ( *p != 0 )
	{
		char *pc = p;

		do
		{
			if ( *pc == '%' )
			{
				if ( strlen( p ) > 124 )
					break;

				char type = pc[1];
				*pc = 0;

				switch ( type )
				{
				case 't':
					{
						char szTime[16];

						if ( cvar_timelimit.value == 0.0f )
						{
							strcpy( szTime, "--:--" );
						}
						else
						{
							int iTimeLeft = get_maptime_left();
							int iMins = iTimeLeft / 60;
							int iSecs = iTimeLeft % 60;
							char szSecs[8];

							if ( iSecs < 10 )
								sprintf( szSecs, "0%d", iSecs );
							else
								sprintf( szSecs, "%d", iSecs );

							sprintf( szTime, "%d:%.2s", iMins, szSecs );
						}

						strcat( p, UTIL_VarArgs( (char *)"%s%s", szTime, pc + 2 ) );
					}
					break;

				case 'h':
					strcat( p, UTIL_VarArgs( (char *)"%d%s", (int) pSender->pev->health, pc + 2 ) );
					break;

				case 'i':
					if ( pSender->m_hLastIDTarget &&
						 ( pSender->pev->team == pSender->m_hLastIDTarget->pev->team || pSender->pev->iuser1 != 0 ) )
					{
						strcat( p, UTIL_VarArgs( (char *)"%s%s", STRING( pSender->m_hLastIDTarget->pev->netname ), pc + 2 ) );
					}
					else
					{
						strcat( p, UTIL_VarArgs( (char *)"%s", pc + 2 ) );
					}
					break;

				case 'r':
					if ( pSender->m_hLastIDTarget &&
						 ( pSender->pev->team == pSender->m_hLastIDTarget->pev->team || pSender->pev->iuser1 != 0 ) )
					{
						int iTargetHealth = 0;

						if ( pSender->m_hLastIDTarget->IsAlive() )
							iTargetHealth = (int) pSender->m_hLastIDTarget->pev->health;

						strcat( p, UTIL_VarArgs( (char *)"%d%s", iTargetHealth, pc + 2 ) );
					}
					else
					{
						strcat( p, UTIL_VarArgs( (char *)"%s", pc + 2 ) );
					}
					break;

				case 'c':
					{
						int cls = pSender->pev->playerclass;

						if ( cls < 29 )
							strcat( p, UTIL_VarArgs( (char *)"%s%s", ( (CDoDTeamPlay *) g_pGameRules )->GetPlayerClassName( cls ), pc + 2 ) );
						else
							strcat( p, UTIL_VarArgs( (char *)"%s", pc + 2 ) );
					}
					break;

				case 'l':
					{
						CControlPoint *pNearest = NULL;
						float flBest = 999999.0f;

						for ( CControlPoint *pCP = (CControlPoint *) UTIL_FindEntityByClassname( NULL, "dod_control_point" );
							  pCP != NULL; pCP = (CControlPoint *) UTIL_FindEntityByClassname( pCP, "dod_control_point" ) )
						{
							if ( UTIL_IsMasterTriggered( pCP->m_sMaster, pCP ) && ( pCP->pev->spawnflags & SF_CONTROLPOINT_HIDDEN ) == 0 )
							{
								float flDist = ( pSender->pev->origin - pCP->pev->origin ).Length();

								if ( flDist < flBest )
								{
									flBest = flDist;
									pNearest = pCP;
								}
							}
						}

						for ( CLocation *pL = (CLocation *) UTIL_FindEntityByClassname( NULL, "dod_location" );
							  pL != NULL; pL = (CLocation *) UTIL_FindEntityByClassname( pL, "dod_location" ) )
						{
							if ( UTIL_IsMasterTriggered( pL->m_sMaster, pL ) )
							{
								float flDist = ( pSender->pev->origin - pL->pev->origin ).Length();

								if ( flDist < flBest )
								{
									flBest = flDist;
									pNearest = (CControlPoint *) pL;
								}
							}
						}

						if ( pNearest != NULL )
						{
							char szLocation[64];

							strncpy( szLocation, STRING( pNearest->pev->netname ), sizeof( szLocation ) - 1 );
							szLocation[sizeof( szLocation ) - 1] = 0;

							for ( char *pLoc = szLocation; *pLoc != 0; pLoc++ )
							{
								if ( *pLoc == ' ' || *pLoc == '-' )
									*pLoc = '_';
							}

							strcat( p, UTIL_VarArgs( (char *)"#%s%s", szLocation, pc + 2 ) );
						}
					}
					break;

				case 's':
				case 'S':
					strcat( p, UTIL_VarArgs( (char *)"%s", pc + 2 ) );
					break;

				default:
					strcat( p, UTIL_VarArgs( (char *)"%%%s", pc + 1 ) );
					break;
				}
			}

			pc++;
		} while ( *pc != 0 );
	}

	// turn on color set 2  (color on,  no sound)
	if ( pSender->pev->deadflag != DEAD_NO && !g_fGameOver )
	{
		if ( teamonly )
			sprintf( text, "%c(DEAD)(TEAM) %s: ", 2, STRING( pEntity->v.netname ) );
		else
			sprintf( text, "%c(DEAD) %s: ", 2, STRING( pEntity->v.netname ) );
	}
	else
	{
		if ( teamonly )
			sprintf( text, "%c(TEAM) %s: ", 2, STRING( pEntity->v.netname ) );
		else
			sprintf( text, "%c%s: ", 2, STRING( pEntity->v.netname ) );
	}

	j = sizeof( text ) - 2 - strlen( text );  // -2 for /n and null terminator
	if ( (int) strlen( p ) > j )
		p[j] = 0;

	strcat( text, p );
	strcat( text, "\n" );

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	client = NULL;
	for ( ;; )
	{
		client = (CBasePlayer *) UTIL_FindEntityByClassname( client, "player" );

		if ( client == NULL || client->edict() == NULL || ENTINDEX( client->edict() ) == 0 )
			break;

		if ( !client->pev )
			continue;

		if ( client->edict() == pEntity )
			continue;

		if ( !( client->IsNetClient() ) )  // Not a client ? (should never be true)
			continue;

		// can the receiver hear the sender? or has he muted him?
		if ( g_VoiceGameMgr.PlayerHasBlockedPlayer( client, pSender ) )
			continue;

		if ( teamonly )
		{
			if ( g_pGameRules->PlayerRelationship( client, pSender ) != GR_TEAMMATE )
				continue;

			// dead players cannot talk to the living
			if ( pSender->pev->deadflag != DEAD_NO && client->pev->deadflag == DEAD_NO )
				continue;
		}
		else
		{
			if ( pSender->pev->deadflag != DEAD_NO && client->pev->deadflag == DEAD_NO && !g_fGameOver )
				continue;
		}

		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, client->pev->pContainingEntity );
		WRITE_BYTE( ENTINDEX( pEntity ) );
		WRITE_STRING( text );
		MESSAGE_END();
	}

	// server log
	const char *pszTeam = ( pSender->pev->team == TEAM_ALLIES ) ? "Allies" : "Axis";

	if ( teamonly )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" say_team \"%s\"\n", STRING( pEntity->v.netname ), GETPLAYERUSERID( pEntity ),
						GETPLAYERAUTHID( pEntity ), pszTeam, p );
	}
	else
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" say \"%s\"\n", STRING( pEntity->v.netname ), GETPLAYERUSERID( pEntity ),
						GETPLAYERAUTHID( pEntity ), pszTeam, p );
	}

	// print to the sending client
	MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pEntity );
	WRITE_BYTE( ENTINDEX( pEntity ) );
	WRITE_STRING( text );
	MESSAGE_END();

	// echo to server console
	g_engfuncs.pfnServerPrint( text );
}

/*
===========
ClientCommand
called each time a player uses a "cmd" command
============
*/
// Use CMD_ARGV,  CMD_ARGV, and CMD_ARGC to get pointers the character string command.
void ClientCommand( edict_t *pEntity )
{
	const char *pcmd = g_engfuncs.pfnCmd_Argv( 0 );
	CBasePlayer *pPlayer = (CBasePlayer *) pEntity->pvPrivateData;
	// DoD: pev->vuser1.z is a command-disable flag (set by triggers); HL used deadflag
	if ( pPlayer != NULL && pPlayer->pev->vuser1.z == 0.0f )
	{
		g_pGameRules->ClientCommand( pPlayer, pcmd );
	}
}

/*
========================
ClientUserInfoChanged

called after the player changes
userinfo - gives dll a chance to modify it before
it gets sent into the rest of the engine.
========================
*/
void ClientUserInfoChanged( edict_t *pEntity, char *infobuffer )
{
	// Is the client spawned yet?
	if ( !pEntity->pvPrivateData )
		return;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	if ( pEntity->v.netname && STRING( pEntity->v.netname )[0] != 0 &&
		 strcmp( STRING( pEntity->v.netname ), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) ) != 0 )
	{
		char sName[256];

		// DoD: only allow a live, in-team, non-spectating player to rename
		if ( pEntity->v.iuser2 == 0 && pEntity->v.iuser1 == 0 && pEntity->v.team < 3 &&
			 pEntity->v.deadflag == DEAD_NO )
		{
			const char *pName = g_engfuncs.pfnInfoKeyValue( infobuffer, "name" );
			strncpy( sName, pName, sizeof( sName ) - 1 );
			sName[sizeof( sName ) - 1] = '\0';

			// DoD: strip the leading '#' and any '%' characters
			if ( sName[0] == '#' )
				sName[0] = '*';

			for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
			{
				if ( *pApersand == '%' )
					*pApersand = ' ';
			}

			// Set the name
			g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), infobuffer, "name", sName );

			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_changed_name", STRING( pEntity->v.netname ),
								 g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );

			const char *pszTeam = ( pEntity->v.team == TEAM_ALLIES ) ? "Allies" : "Axis";
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed name to \"%s\"\n", STRING( pEntity->v.netname ),
							GETPLAYERUSERID( pEntity ), GETPLAYERAUTHID( pEntity ), pszTeam,
							g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );
		}
		else
		{
			// DoD: reject the rename and restore the old name
			strncpy( sName, STRING( pEntity->v.netname ), sizeof( sName ) - 1 );
			sName[sizeof( sName ) - 1] = '\0';

			g_engfuncs.pfnClientCommand( pEntity, "name %s\n", sName );
			g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), infobuffer, "name", sName );
			ClientPrint( &pEntity->v, HUD_PRINTTALK, "#game_cant_change_name" );
		}
	}

	g_pGameRules->ClientUserInfoChanged( GetClassPtr( (CBasePlayer *) &pEntity->v ), infobuffer );
}

static int g_serveractive = 0;

void ServerDeactivate( void )
{
	// It's possible that the engine will call this function more times than is necessary
	//  Therefore, only run it one time for each call to ServerActivate
	if ( g_serveractive != 1 )
	{
		return;
	}

	g_serveractive = 0;

	// Peform any shutdown operations here...
	//
}

void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	int i;
	CBaseEntity *pClass;

	// Every call to ServerActivate should be matched by a call to ServerDeactivate
	g_serveractive = 1;

	EmptyEntityHashTable();

	// Clients have not been initialized yet
	for ( i = 0; i < edictCount; i++ )
	{
		if ( pEdictList[i].free )
			continue;

		// Clients aren't necessarily initialized until ClientPutInServer()
		if ( i < clientMax || !pEdictList[i].pvPrivateData )
			continue;

		pClass = CBaseEntity::Instance( &pEdictList[i] );
		// Activate this entity if it's got a class & isn't dormant
		if ( pClass && !( pClass->pev->flags & FL_DORMANT ) )
		{
			AddEntityHashValue( &pEdictList[i].v, STRING( pEdictList[i].v.classname ), CLASSNAME );
			pClass->Activate();
		}
		else
		{
			ALERT( at_console, "Can't instance %s\n", STRING( pEdictList[i].v.classname ) );
		}
	}

	// Link user messages here to make sure first client can get them...
	LinkUserMessages();
	GetCVARPointers();
}

/*
================
PlayerPreThink

Called every frame before physics are run
================
*/
void PlayerPreThink( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = (CBasePlayer *) GET_PRIVATE( pEntity );

	if ( pPlayer )
		pPlayer->PreThink();

	// DoD: if the player moved into a different set of leaves, throw away the
	// cached per-entity PVS visibility for this client.
	int clientnum = g_engfuncs.pfnIndexOfEdict( pEntity ) - 1;

	if ( CheckPlayerPVSLeafChanged( pEntity, clientnum ) )
		ResetPlayerPVS( pEntity, clientnum );
}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/
void PlayerPostThink( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = (CBasePlayer *) GET_PRIVATE( pEntity );

	if ( pPlayer )
		pPlayer->PostThink();
}

void ParmsNewLevel( void ) {}

void ParmsChangeLevel( void )
{
	// retrieve the pointer to the save data
	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *) gpGlobals->pSaveData;

	if ( pSaveData )
		pSaveData->connectionCount = BuildChangeList( pSaveData->levelList, MAX_LEVEL_CONNECTIONS );
}

//
// GLOBALS ASSUMED SET:  g_ulFrameCount
//
void StartFrame( void )
{
	if ( g_pGameRules )
		g_pGameRules->Think();

	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = cvar_teamplay.value;
	g_ulFrameCount++;
}

extern char *sPlayerModelFiles[];
extern char *s_USVoiceFiles[];
extern char *s_GERVoiceFiles[];
extern char *s_BRITVoiceFiles[];
extern char *sHelmetModels[];

void ClientPrecache( void )
{
	// setup precaches always needed
	PRECACHE_SOUND( "player/sprayer.wav" );	 // spray paint sound for PreAlpha

	PRECACHE_SOUND( "player/pl_fallpain.wav" );

	PRECACHE_SOUND( "player/pl_step1.wav" );  // walk on concrete
	PRECACHE_SOUND( "player/pl_step2.wav" );
	PRECACHE_SOUND( "player/pl_step3.wav" );
	PRECACHE_SOUND( "player/pl_step4.wav" );

	PRECACHE_SOUND( "player/pl_wood1.wav" );  // walk on wood
	PRECACHE_SOUND( "player/pl_wood2.wav" );
	PRECACHE_SOUND( "player/pl_wood3.wav" );
	PRECACHE_SOUND( "player/pl_wood4.wav" );

	PRECACHE_SOUND( "player/pl_gravel1.wav" );	// walk on gravel
	PRECACHE_SOUND( "player/pl_gravel2.wav" );
	PRECACHE_SOUND( "player/pl_gravel3.wav" );
	PRECACHE_SOUND( "player/pl_gravel4.wav" );

	PRECACHE_SOUND( "player/pl_metal1.wav" );  // walk on metal
	PRECACHE_SOUND( "player/pl_metal2.wav" );
	PRECACHE_SOUND( "player/pl_metal3.wav" );
	PRECACHE_SOUND( "player/pl_metal4.wav" );

	PRECACHE_SOUND( "player/pl_dirt1.wav" );  // walk on dirt
	PRECACHE_SOUND( "player/pl_dirt2.wav" );
	PRECACHE_SOUND( "player/pl_dirt3.wav" );
	PRECACHE_SOUND( "player/pl_dirt4.wav" );

	PRECACHE_SOUND( "player/pl_duct1.wav" );  // walk in duct
	PRECACHE_SOUND( "player/pl_duct2.wav" );
	PRECACHE_SOUND( "player/pl_duct3.wav" );
	PRECACHE_SOUND( "player/pl_duct4.wav" );

	PRECACHE_SOUND( "player/pl_grate1.wav" );  // walk on grate
	PRECACHE_SOUND( "player/pl_grate2.wav" );
	PRECACHE_SOUND( "player/pl_grate3.wav" );
	PRECACHE_SOUND( "player/pl_grate4.wav" );

	PRECACHE_SOUND( "player/pl_slosh1.wav" );  // walk in shallow water
	PRECACHE_SOUND( "player/pl_slosh2.wav" );
	PRECACHE_SOUND( "player/pl_slosh3.wav" );
	PRECACHE_SOUND( "player/pl_slosh4.wav" );

	PRECACHE_SOUND( "player/pl_tile1.wav" );  // walk on tile
	PRECACHE_SOUND( "player/pl_tile2.wav" );
	PRECACHE_SOUND( "player/pl_tile3.wav" );
	PRECACHE_SOUND( "player/pl_tile4.wav" );
	PRECACHE_SOUND( "player/pl_tile5.wav" );

	PRECACHE_SOUND( "player/pl_swim1.wav" );  // breathe bubbles
	PRECACHE_SOUND( "player/pl_swim2.wav" );
	PRECACHE_SOUND( "player/pl_swim3.wav" );
	PRECACHE_SOUND( "player/pl_swim4.wav" );

	PRECACHE_SOUND( "player/pl_ladder1.wav" );	// climb ladder rung
	PRECACHE_SOUND( "player/pl_ladder2.wav" );
	PRECACHE_SOUND( "player/pl_ladder3.wav" );
	PRECACHE_SOUND( "player/pl_ladder4.wav" );

	PRECACHE_SOUND( "player/pl_wade1.wav" );  // wade in water
	PRECACHE_SOUND( "player/pl_wade2.wav" );
	PRECACHE_SOUND( "player/pl_wade3.wav" );
	PRECACHE_SOUND( "player/pl_wade4.wav" );

	PRECACHE_SOUND( "player/pl_snow1.wav" );  // walk in snow
	PRECACHE_SOUND( "player/pl_snow2.wav" );
	PRECACHE_SOUND( "player/pl_snow3.wav" );
	PRECACHE_SOUND( "player/pl_snow4.wav" );

	PRECACHE_SOUND( "debris/wood1.wav" );  // hit wood texture
	PRECACHE_SOUND( "debris/wood2.wav" );
	PRECACHE_SOUND( "debris/wood3.wav" );

	PRECACHE_SOUND( "buttons/spark5.wav" );	 // hit computer texture
	PRECACHE_SOUND( "buttons/spark6.wav" );
	PRECACHE_SOUND( "debris/glass1.wav" );
	PRECACHE_SOUND( "debris/glass2.wav" );
	PRECACHE_SOUND( "debris/glass3.wav" );

	PRECACHE_SOUND( "items/flashlight1.wav" );
	PRECACHE_SOUND( "items/flashlight1.wav" );

	// player gib sounds
	PRECACHE_SOUND( "common/bodysplat.wav" );

	PRECACHE_SOUND( "ambience/thunder1.wav" );
	PRECACHE_SOUND( "ambience/thunder2.wav" );
	PRECACHE_SOUND( "ambience/thunder3.wav" );
	PRECACHE_SOUND( "ambience/thunder4.wav" );

	PRECACHE_SOUND( "player/damage1.wav" );
	PRECACHE_SOUND( "player/damage2.wav" );
	PRECACHE_SOUND( "player/damage3.wav" );
	PRECACHE_SOUND( "player/damage4.wav" );
	PRECACHE_SOUND( "player/damage5.wav" );
	PRECACHE_SOUND( "player/damage6.wav" );
	PRECACHE_SOUND( "player/damage7.wav" );
	PRECACHE_SOUND( "player/damage8.wav" );
	PRECACHE_SOUND( "player/damage9.wav" );
	PRECACHE_SOUND( "player/damage10.wav" );
	PRECACHE_SOUND( "player/damage11.wav" );

	PRECACHE_SOUND( "player/usstartround.wav" );
	PRECACHE_SOUND( "player/usstartround2.wav" );
	PRECACHE_SOUND( "player/britstartround.wav" );
	PRECACHE_SOUND( "player/britstartround2.wav" );
	PRECACHE_SOUND( "player/gerstartround.wav" );
	PRECACHE_SOUND( "player/gerstartround2.wav" );
	PRECACHE_SOUND( "player/prewarmup.wav" );

	// voice command sounds
	for ( int i = 1; i < 28; i++ )
	{
		PRECACHE_SOUND( s_USVoiceFiles[i] );
		PRECACHE_SOUND( s_BRITVoiceFiles[i] );
		PRECACHE_SOUND( s_GERVoiceFiles[i] );
	}
	PRECACHE_SOUND( s_GERVoiceFiles[28] );

	PRECACHE_SOUND( "player/jump.wav" );
	PRECACHE_SOUND( "player/jumplanding.wav" );
	PRECACHE_SOUND( "player/sprintgrunts.wav" );
	PRECACHE_SOUND( "player/helmet_hit.wav" );
	PRECACHE_SOUND( "player/stopbleed.wav" );
	PRECACHE_SOUND( "player/goprone.wav" );

	// hud sounds
	PRECACHE_SOUND( "common/wpn_select.wav" );
	PRECACHE_SOUND( "common/wpn_denyselect.wav" );

	PRECACHE_SOUND( "weapons/whizz1.wav" );
	PRECACHE_SOUND( "weapons/whizz2.wav" );
	PRECACHE_SOUND( "weapons/whizz3.wav" );
	PRECACHE_SOUND( "weapons/whizz4.wav" );
	PRECACHE_SOUND( "weapons/whizz5.wav" );
	PRECACHE_SOUND( "weapons/whizz6.wav" );
	PRECACHE_SOUND( "weapons/whizz7.wav" );
	PRECACHE_SOUND( "weapons/whizz8.wav" );
	PRECACHE_SOUND( "weapons/whizz9.wav" );
	PRECACHE_SOUND( "weapons/whizz10.wav" );
	PRECACHE_SOUND( "weapons/whizz11.wav" );
	PRECACHE_SOUND( "weapons/whizz12.wav" );
	PRECACHE_SOUND( "weapons/whizz13.wav" );
	PRECACHE_SOUND( "weapons/whizz14.wav" );
	PRECACHE_SOUND( "weapons/whizz15.wav" );
	PRECACHE_SOUND( "weapons/whizz16.wav" );
	PRECACHE_SOUND( "weapons/whizz17.wav" );

	PRECACHE_SOUND( "weapons/hit_grass1.wav" );
	PRECACHE_SOUND( "weapons/hit_grass2.wav" );
	PRECACHE_SOUND( "weapons/hit_glass1.wav" );
	PRECACHE_SOUND( "weapons/hit_glass2.wav" );
	PRECACHE_SOUND( "weapons/hit_leaves1.wav" );
	PRECACHE_SOUND( "weapons/hit_leaves2.wav" );
	PRECACHE_SOUND( "weapons/hit_metal1.wav" );
	PRECACHE_SOUND( "weapons/hit_metal2.wav" );
	PRECACHE_SOUND( "weapons/hit_sand1.wav" );
	PRECACHE_SOUND( "weapons/hit_sand2.wav" );
	PRECACHE_SOUND( "weapons/hit_snow1.wav" );
	PRECACHE_SOUND( "weapons/hit_snow2.wav" );
	PRECACHE_SOUND( "weapons/hit_water1.wav" );
	PRECACHE_SOUND( "weapons/hit_water2.wav" );
	PRECACHE_SOUND( "weapons/hit_wood1.wav" );
	PRECACHE_SOUND( "weapons/hit_wood2.wav" );
	PRECACHE_SOUND( "weapons/hit_heavymetal1.wav" );
	PRECACHE_SOUND( "weapons/hit_heavymetal2.wav" );

	PRECACHE_SOUND( "weapons/ric1.wav" );
	PRECACHE_SOUND( "weapons/ric2.wav" );
	PRECACHE_SOUND( "weapons/ric3.wav" );
	PRECACHE_SOUND( "weapons/ric4.wav" );
	PRECACHE_SOUND( "weapons/ric5.wav" );
	PRECACHE_SOUND( "weapons/ric6.wav" );
	PRECACHE_SOUND( "weapons/ric7.wav" );
	PRECACHE_SOUND( "weapons/ric8.wav" );
	PRECACHE_SOUND( "weapons/ric9.wav" );
	PRECACHE_SOUND( "weapons/ric10.wav" );
	PRECACHE_SOUND( "weapons/ric11.wav" );

	// custom event scripts
	CDoDTeamPlay *pGameRules = (CDoDTeamPlay *) g_pGameRules;
	pGameRules->SetusVoice( PRECACHE_EVENT( 1, "events/misc/usvoice.sc" ) );
	pGameRules->SetgerVoice( PRECACHE_EVENT( 1, "events/misc/gervoice.sc" ) );
	pGameRules->SetbodyDamage( PRECACHE_EVENT( 1, "events/effects/bodydamage.sc" ) );
	pGameRules->SetroundRestartSound( PRECACHE_EVENT( 1, "events/misc/roundrestart.sc" ) );
	pGameRules->SetPain( PRECACHE_EVENT( 1, "events/misc/pain.sc" ) );
	pGameRules->SetExplosion( PRECACHE_EVENT( 1, "events/effects/explosion.sc" ) );
	pGameRules->SetSmoke( PRECACHE_EVENT( 1, "events/effects/smoke.sc" ) );
	pGameRules->SetBloodSprite( PRECACHE_EVENT( 1, "events/effects/blood.sc" ) );
	pGameRules->SetBloodStream( PRECACHE_EVENT( 1, "events/effects/bloodstream.sc" ) );
	pGameRules->SetBulletTracers( PRECACHE_EVENT( 1, "events/effects/tracers.sc" ) );
	pGameRules->SetSparkShower( PRECACHE_EVENT( 1, "events/effects/sparkshower.sc" ) );
	pGameRules->SetBubbleTrails( PRECACHE_EVENT( 1, "events/effects/bubbletrails.sc" ) );
	pGameRules->SetBubbles( PRECACHE_EVENT( 1, "events/effects/bubbles.sc" ) );
	pGameRules->SetPopHelmet( PRECACHE_EVENT( 1, "events/effects/helmet.sc" ) );
	pGameRules->SetRoundReset( PRECACHE_EVENT( 1, "events/misc/roundreset.sc" ) );
	pGameRules->SetRocketTrail( PRECACHE_EVENT( 1, "events/effects/rockettrail.sc" ) );

	PRECACHE_MODEL( "sprites/shot_smoke1.spr" );
	PRECACHE_MODEL( "sprites/shot_smoke2.spr" );
	PRECACHE_MODEL( "sprites/shot_smoke3.spr" );
	PRECACHE_MODEL( "sprites/shot_smoke4.spr" );
	PRECACHE_MODEL( "sprites/blood-narrow.spr" );
	PRECACHE_MODEL( "sprites/shot-dust.spr" );

	for ( int i = 0; i < 6; i++ )
		PRECACHE_MODEL( sPlayerModelFiles[i] );

	// weapon view/world models
	for ( int i = 1; i < 41; i++ )
	{
		PRECACHE_MODEL( P_WpnInfo[i].pmodel );

		if ( P_WpnInfo[i].leftreload[0] )
			PRECACHE_MODEL( P_WpnInfo[i].leftreload );
		if ( P_WpnInfo[i].bipodup[0] )
			PRECACHE_MODEL( P_WpnInfo[i].bipodup );
		if ( P_WpnInfo[i].bipoddown[0] )
			PRECACHE_MODEL( P_WpnInfo[i].bipoddown );
		if ( P_WpnInfo[i].sandbagreload[0] )
			PRECACHE_MODEL( P_WpnInfo[i].sandbagreload );
		if ( P_WpnInfo[i].pronereload[0] )
			PRECACHE_MODEL( P_WpnInfo[i].pronereload );
	}

	for ( int i = 0; i < 7; i++ )
		PRECACHE_MODEL( sHelmetModels[i] );

	PRECACHE_MODEL( "models/shells.mdl" );

	Vector vMin = Vector( -99, -25, -43 );
	Vector vMax = Vector( 99, 25, 43 );
	for ( int i = 0; i < 6; i++ )
		g_engfuncs.pfnForceUnmodified( force_model_specifybounds, vMin, vMax, sPlayerModelFiles[i] );

	g_engfuncs.pfnForceUnmodified( force_exactfile, g_vecZero, g_vecZero, "sprites/scope_spring.spr" );
	g_engfuncs.pfnForceUnmodified( force_exactfile, g_vecZero, g_vecZero, "sprites/scope_k43.spr" );
	g_engfuncs.pfnForceUnmodified( force_exactfile, g_vecZero, g_vecZero, "sprites/scope_binoc.spr" );
	g_engfuncs.pfnForceUnmodified( force_exactfile, g_vecZero, g_vecZero, "sprites/scope_enfield.spr" );

	PRECACHE_MODEL( "sprites/muzzleflash1.spr" );
	PRECACHE_MODEL( "sprites/muzzleflash2.spr" );
	PRECACHE_MODEL( "sprites/gas_puff_01.spr" );
	PRECACHE_MODEL( "sprites/grenade_flash.spr" );
	PRECACHE_MODEL( "sprites/bazookapuff.spr" );
	PRECACHE_MODEL( "sprites/large_smoke_01_ind.spr" );
	PRECACHE_MODEL( "sprites/puff.spr" );
	PRECACHE_MODEL( "sprites/effects/debris_dirt3.spr" );
	PRECACHE_MODEL( "sprites/effects/debris_grass1.spr" );
	PRECACHE_MODEL( "sprites/effects/debris_wood1.spr" );
	PRECACHE_MODEL( "sprites/effects/debris_glass.spr" );
	PRECACHE_MODEL( "sprites/effects/debris_foliage.spr" );
	PRECACHE_MODEL( "sprites/effects/debris_leaf1.spr" );
	PRECACHE_MODEL( "sprites/effects/debris_leaf2.spr" );
	PRECACHE_MODEL( "sprites/effects/debris_tile1.spr" );
	PRECACHE_MODEL( "sprites/metalspark.spr" );
	PRECACHE_MODEL( "sprites/rain.spr" );
	PRECACHE_MODEL( "sprites/snowflake.spr" );
	PRECACHE_MODEL( "sprites/ripple.spr" );
	PRECACHE_MODEL( "sprites/rock_small.spr" );
	PRECACHE_MODEL( "sprites/rock_big.spr" );
	PRECACHE_MODEL( "sprites/dirt.spr" );
	PRECACHE_MODEL( "sprites/effects/adrian/bazooka_smoke.spr" );
	PRECACHE_MODEL( "sprites/markernull.spr" );
	PRECACHE_MODEL( "sprites/ring.spr" );
}

/*
===============
GetGameDescription

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription( void )
{
	if ( g_pGameRules )
		return g_pGameRules->GetGameDescription();
	return "Day of Defeat";
}

/*
================
Sys_Error

Engine is going to shut down, allows setting a breakpoint in game .dll to catch that occasion
================
*/
void Sys_Error( const char *error_string )
{
	// Default case, do nothing.  MOD AUTHORS:  Add code ( e.g., _asm { int 3 }; here to cause a breakpoint for debugging your game .dlls
}

/*
================
PlayerCustomization

A new player customization has been registered on the server
UNDONE:  This only sets the # of frames of the spray can logo
animation right now.
================
*/
void PlayerCustomization( edict_t *pEntity, customization_t *pCust )
{
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = (CBasePlayer *) GET_PRIVATE( pEntity );

	if ( !pPlayer )
	{
		ALERT( at_console, "PlayerCustomization:  Couldn't get player!\n" );
		return;
	}

	if ( !pCust )
	{
		ALERT( at_console, "PlayerCustomization:  NULL customization!\n" );
		return;
	}

	switch ( pCust->resource.type )
	{
	case t_decal:
		pPlayer->SetCustomDecalFrames( pCust->nUserData2 );	 // Second int is max # of frames.
		break;
	case t_sound:
	case t_skin:
	case t_model:
		// Ignore for now.
		break;
	default:
		ALERT( at_console, "PlayerCustomization:  Unknown customization type!\n" );
		break;
	}
}

/*
================
SpectatorConnect

A spectator has joined the game
================
*/
void SpectatorConnect( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBaseSpectator *pPlayer = (CBaseSpectator *) GET_PRIVATE( pEntity );

	if ( pPlayer )
		pPlayer->SpectatorConnect();
}

/*
================
SpectatorConnect

A spectator has left the game
================
*/
void SpectatorDisconnect( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBaseSpectator *pPlayer = (CBaseSpectator *) GET_PRIVATE( pEntity );

	if ( pPlayer )
		pPlayer->SpectatorDisconnect();
}

/*
================
SpectatorConnect

A spectator has sent a usercmd
================
*/
void SpectatorThink( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBaseSpectator *pPlayer = (CBaseSpectator *) GET_PRIVATE( pEntity );

	if ( pPlayer )
		pPlayer->SpectatorThink();
}

////////////////////////////////////////////////////////
// PAS and PVS routines for client messaging
//

/*
================
SetupVisibility

A client can have a separate "view entity" indicating that his/her view should depend on the origin of that
view entity.  If that's the case, then pViewEntity will be non-NULL and will be used.  Otherwise, the current
entity's origin is used.  Either is offset by the view_ofs to get the eye position.

From the eye position, we set up the PAS and PVS to use for filtering network messages to the client.  At this point, we could
 override the actual PAS or PVS values, or use a different origin.

NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
================
*/
void SetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas )
{
	Vector org;
	edict_t *pView = pClient;

	// Find the client's PVS
	if ( pViewEntity )
	{
		pView = pViewEntity;
	}

	if ( pClient->v.flags & FL_PROXY )
	{
		*pvs = NULL;  // the spectator proxy sees
		*pas = NULL;  // and hears everything
		return;
	}

	// DoD: when spectating (not roaming), view from the observer target
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE( pClient );
	if ( pPlayer && pPlayer->pev->iuser2 != 0 && pPlayer->m_hObserverTarget )
	{
		pView = pPlayer->m_hObserverTarget->pev->pContainingEntity;
	}

	org = pView->v.origin + pView->v.view_ofs;
	if ( pView->v.flags & FL_DUCKING )
	{
		org = org + ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
	}

	*pvs = ENGINE_SET_PVS( (float *) &org );
	*pas = ENGINE_SET_PAS( (float *) &org );
}

#include "entity_state.h"

////////////////////////////////////////////////////////
// DoD per-player PVS tracking (client.cpp:1510-1585)
//
// The server keeps a small cache of which entities were last known to be
// visible to each player, so that entities which leave the PVS can still be
// transmitted for a short grace period (team markers / minimap).
//

struct ENTITYPVSSTATUS
{
	float m_fTimeEnteredPVS;
};

struct PLAYERPVSSTATUS
{
	ENTITYPVSSTATUS m_Status[1380];
	int headnode;
	int num_leafs;
	short leafnums[MAX_ENT_LEAFS];
};

static PLAYERPVSSTATUS g_PVSStatus[32];
static const float g_flPVSUpdateTime = 1.0f;  // .rodata 0x0018f3b4

void ResetPlayerPVS( edict_t *client, int clientnum )
{
	PLAYERPVSSTATUS *pvs = &g_PVSStatus[clientnum];

	for ( int i = 0; i < (int)( sizeof( pvs->m_Status ) / sizeof( pvs->m_Status[0] ) ); i++ )
	{
		pvs->m_Status[i].m_fTimeEnteredPVS = 0.0f;
	}

	pvs->headnode = client->headnode;
	pvs->num_leafs = client->num_leafs;

	for ( int i = 0; i < MAX_ENT_LEAFS; i++ )
	{
		pvs->leafnums[i] = client->leafnums[i];
	}
}

bool CheckPlayerPVSLeafChanged( edict_t *client, int clientnum )
{
	PLAYERPVSSTATUS *pvs = &g_PVSStatus[clientnum];

	if ( pvs->headnode == client->headnode && pvs->num_leafs == client->num_leafs )
	{
		for ( int i = 0; i < pvs->num_leafs; i++ )
		{
			if ( pvs->leafnums[i] != client->leafnums[i] )
				return true;
		}
		return false;
	}

	return true;
}

void MarkEntityInPVS( int clientnum, int entitynum, float time, bool inpvs )
{
	g_PVSStatus[clientnum].m_Status[entitynum].m_fTimeEnteredPVS = inpvs ? time : 0.0f;
}

bool CheckEntityRecentlyInPVS( int clientnum, int entitynum, float currenttime )
{
	float fTimeEnteredPVS = g_PVSStatus[clientnum].m_Status[entitynum].m_fTimeEnteredPVS;

	if ( fTimeEnteredPVS != 0.0f )
		return currenttime <= fTimeEnteredPVS + g_flPVSUpdateTime;

	return false;
}

/*
AddToFullPack

Return 1 if the entity state has been filled in for the ent and the entity will be propagated to the client, 0 otherwise

state is the server maintained copy of the state info that is transmitted to the client
a MOD could alter values copied into state to send the "host" a different look for a particular entity update, etc.
e and ent are the entity that is being added to the update, if 1 is returned
host is the player's edict of the player whom we are sending the update to
player is 1 if the ent/e is a player and 0 otherwise
pSet is either the PAS or PVS that we previous set up.  We can use it to ask the engine to filter the entity against the PAS or PVS.
we could also use the pas/ pvs that we set in SetupVisibility, if we wanted to.  Caching the value is valid in that case, but still only for the current frame
*/
int AddToFullPack( struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player,
				   unsigned char *pSet )
{
	int i;

	// don't send if flagged for NODRAW and it's not the host getting the message
	if ( ( ent->v.effects & EF_NODRAW ) && ( ent != host ) )
		return 0;

	// Ignore ents without valid / visible models
	if ( !ent->v.modelindex || !STRING( ent->v.model ) )
		return 0;

	// Don't send spectators to other players
	if ( ( ent->v.flags & FL_SPECTATOR ) && ( ent != host ) )
	{
		return 0;
	}

	// DoD: minimap / team-marker / PVS-delay filtering
	CBasePlayer *pHost = (CBasePlayer *) GET_PRIVATE( host );

	bool bSendMinimapData = ( pHost != NULL && pHost->m_iMinimap > 0 );
	bool bTeammate = ( player != 0 && ( ent->v.team == host->v.team || host->v.team == TEAM_SPECTATOR ) && ent->v.health > 0 );
	bool bTeamMarker = false;

	if ( ( ent->v.flags & FL_CUSTOMENTITY ) && ent->v.team != 0 )
	{
		if ( ent->v.team != host->v.team )
			return 0;
		bTeamMarker = true;
	}

	// Ignore if not the host and not touching a PVS/PAS leaf
	// If pSet is NULL, then the test will always succeed and the entity will be added to the update
	if ( ent != host && !bTeamMarker )
	{
		int hostnum = g_engfuncs.pfnIndexOfEdict( host ) - 1;
		ENTITYPVSSTATUS *es = &g_PVSStatus[hostnum].m_Status[e];

		if ( es->m_fTimeEnteredPVS == 0.0f ||
			 es->m_fTimeEnteredPVS + g_flPVSUpdateTime < gpGlobals->time )
		{
			if ( !ENGINE_CHECK_VISIBILITY( (const struct edict_s *) ent, pSet ) )
			{
				es->m_fTimeEnteredPVS = 0.0f;

				if ( !bSendMinimapData )
					return 0;

				if ( !bTeammate )
					return 0;
			}
			else
			{
				es->m_fTimeEnteredPVS = gpGlobals->time;
			}
		}
	}

	// Don't send entity to local client if the client says it's predicting the entity itself.
	if ( ent->v.flags & FL_SKIPLOCALHOST )
	{
		if ( ( hostflags & 1 ) && ( ent->v.owner == host ) )
			return 0;
	}

	if ( host->v.groupinfo )
	{
		UTIL_SetGroupTrace( host->v.groupinfo, GROUP_OP_AND );

		// Should always be set, of course
		if ( ent->v.groupinfo )
		{
			if ( g_groupop == GROUP_OP_AND )
			{
				if ( !( ent->v.groupinfo & host->v.groupinfo ) )
					return 0;
			}
			else if ( g_groupop == GROUP_OP_NAND )
			{
				if ( ent->v.groupinfo & host->v.groupinfo )
					return 0;
			}
		}

		UTIL_UnsetGroupTrace();
	}

	memset( state, 0, sizeof( *state ) );

	// Assign index so we can track this entity from frame to frame and
	//  delta from it.
	state->number = e;
	state->entityType = ENTITY_NORMAL;

	// Flag custom entities.
	if ( ent->v.flags & FL_CUSTOMENTITY )
	{
		state->entityType = ENTITY_BEAM;
	}

	//
	// Copy state data
	//

	// Round animtime to nearest millisecond
	state->animtime = (int) ( 1000.0 * ent->v.animtime ) / 1000.0;

	memcpy( state->origin, ent->v.origin, 3 * sizeof( float ) );
	memcpy( state->angles, ent->v.angles, 3 * sizeof( float ) );
	memcpy( state->mins, ent->v.mins, 3 * sizeof( float ) );
	memcpy( state->maxs, ent->v.maxs, 3 * sizeof( float ) );

	memcpy( state->startpos, ent->v.startpos, 3 * sizeof( float ) );
	memcpy( state->endpos, ent->v.endpos, 3 * sizeof( float ) );

	state->impacttime = ent->v.impacttime;
	state->starttime = ent->v.starttime;

	state->modelindex = ent->v.modelindex;

	state->frame = ent->v.frame;

	state->skin = ent->v.skin;
	state->effects = ent->v.effects;

	// This non-player entity is being moved by the game .dll and not the physics simulation system
	//  make sure that we interpolate it's position on the client if it moves
	if ( !player && ent->v.animtime && ent->v.velocity[0] == 0 && ent->v.velocity[1] == 0 && ent->v.velocity[2] == 0 )
	{
		state->eflags |= EFLAG_SLERP;
	}

	state->scale = ent->v.scale;
	state->solid = ent->v.solid;
	state->colormap = ent->v.colormap;

	state->movetype = ent->v.movetype;
	state->sequence = ent->v.sequence;
	state->framerate = ent->v.framerate;
	state->body = ent->v.body;

	for ( i = 0; i < 4; i++ )
	{
		state->controller[i] = ent->v.controller[i];
	}

	for ( i = 0; i < 2; i++ )
	{
		state->blending[i] = ent->v.blending[i];
	}

	state->rendermode = ent->v.rendermode;
	state->renderamt = ent->v.renderamt;
	state->renderfx = ent->v.renderfx;
	state->rendercolor.r = ent->v.rendercolor.x;
	state->rendercolor.g = ent->v.rendercolor.y;
	state->rendercolor.b = ent->v.rendercolor.z;

	state->aiment = 0;
	if ( ent->v.aiment )
	{
		state->aiment = ENTINDEX( ent->v.aiment );
	}

	state->owner = 0;
	if ( ent->v.owner )
	{
		int owner = ENTINDEX( ent->v.owner );

		// Only care if owned by a player
		if ( owner >= 1 && owner <= gpGlobals->maxClients )
		{
			state->owner = owner;
		}
	}

	// HACK:  Somewhat...
	// Class is overridden for non-players to signify a breakable glass object ( sort of a class? )
	state->playerclass = ent->v.playerclass;

	// Special stuff for players only
	if ( player )
	{
		memcpy( state->basevelocity, ent->v.basevelocity, 3 * sizeof( float ) );

		state->weaponmodel = MODEL_INDEX( STRING( ent->v.weaponmodel ) );
		state->gaitsequence = ent->v.gaitsequence;
		state->spectator = ent->v.flags & FL_SPECTATOR;
		state->friction = ent->v.friction;

		state->gravity = ent->v.gravity;
		state->team = ent->v.team;
		// DoD: return 1 (prone/crouch hull) when pev->iuser3 set, else duck hull flag
		state->usehull = ( ent->v.iuser3 == STANCE_STANDING ) ? ( ( ent->v.flags & FL_DUCKING ) ? 1 : 0 ) : 1;
		state->health = ent->v.health;
	}

	return 1;
}

// defaults for clientinfo messages
#define DEFAULT_VIEWHEIGHT 28

/*
===================
CreateBaseline

Creates baselines used for network encoding, especially for player data since players are not spawned until connect time.
===================
*/
void CreateBaseline( int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity,
					 int playermodelindex, vec3_t player_mins, vec3_t player_maxs )
{
	baseline->origin = entity->v.origin;
	baseline->angles = entity->v.angles;
	baseline->frame = entity->v.frame;
	baseline->skin = (short) entity->v.skin;

	// render information
	baseline->rendermode = (byte) entity->v.rendermode;
	baseline->renderamt = (byte) entity->v.renderamt;
	baseline->rendercolor.r = (byte) entity->v.rendercolor.x;
	baseline->rendercolor.g = (byte) entity->v.rendercolor.y;
	baseline->rendercolor.b = (byte) entity->v.rendercolor.z;
	baseline->renderfx = (byte) entity->v.renderfx;

	if ( player )
	{
		baseline->mins = player_mins;
		baseline->maxs = player_maxs;

		baseline->colormap = eindex;
		baseline->modelindex = playermodelindex;
		baseline->friction = 1.0;
		baseline->movetype = MOVETYPE_WALK;

		baseline->scale = entity->v.scale;
		baseline->solid = SOLID_SLIDEBOX;
		baseline->framerate = 1.0;
		baseline->gravity = 1.0;

		baseline->playerclass = entity->v.playerclass;
		baseline->fov = entity->v.fov;
	}
	else
	{
		baseline->mins = entity->v.mins;
		baseline->maxs = entity->v.maxs;

		baseline->colormap = 0;
		baseline->modelindex = entity->v.modelindex;  //SV_ModelIndex(pr_strings + entity->v.model);
		baseline->movetype = entity->v.movetype;

		baseline->scale = entity->v.scale;
		baseline->solid = entity->v.solid;
		baseline->framerate = entity->v.framerate;
		baseline->gravity = entity->v.gravity;
	}
}

typedef struct
{
	char name[32];
	int field;
} entity_field_alias_t;

#define FIELD_ORIGIN0 0
#define FIELD_ORIGIN1 1
#define FIELD_ORIGIN2 2
#define FIELD_ANGLES0 3
#define FIELD_ANGLES1 4
#define FIELD_ANGLES2 5

static entity_field_alias_t entity_field_alias[] = {
	{ "origin[0]", 0 }, { "origin[1]", 0 }, { "origin[2]", 0 },
	{ "angles[0]", 0 }, { "angles[1]", 0 }, { "angles[2]", 0 },
};

void Entity_FieldInit( struct delta_s *pFields )
{
	entity_field_alias[FIELD_ORIGIN0].field = DELTA_FINDFIELD( pFields, entity_field_alias[FIELD_ORIGIN0].name );
	entity_field_alias[FIELD_ORIGIN1].field = DELTA_FINDFIELD( pFields, entity_field_alias[FIELD_ORIGIN1].name );
	entity_field_alias[FIELD_ORIGIN2].field = DELTA_FINDFIELD( pFields, entity_field_alias[FIELD_ORIGIN2].name );
	entity_field_alias[FIELD_ANGLES0].field = DELTA_FINDFIELD( pFields, entity_field_alias[FIELD_ANGLES0].name );
	entity_field_alias[FIELD_ANGLES1].field = DELTA_FINDFIELD( pFields, entity_field_alias[FIELD_ANGLES1].name );
	entity_field_alias[FIELD_ANGLES2].field = DELTA_FINDFIELD( pFields, entity_field_alias[FIELD_ANGLES2].name );
}

/*
==================
Entity_Encode

Callback for sending entity_state_t info over network. 
FIXME:  Move to script
==================
*/
void Entity_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int localplayer = 0;
	static int initialized = 0;

	if ( !initialized )
	{
		Entity_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *) from;
	t = (entity_state_t *) to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	localplayer = ( t->number - 1 ) == ENGINE_CURRENT_PLAYER();
	if ( localplayer )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN0].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN1].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN2].field );
	}

	if ( ( t->impacttime != 0 ) && ( t->starttime != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN0].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN1].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN2].field );

		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ANGLES0].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ANGLES1].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ANGLES2].field );
	}

	if ( ( t->movetype == MOVETYPE_FOLLOW ) && ( t->aiment != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN0].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN1].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN2].field );
	}
	else if ( t->aiment != f->aiment )
	{
		DELTA_SETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN0].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN1].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN2].field );
	}
}

static entity_field_alias_t player_field_alias[] = {
	{ "origin[0]", 0 },
	{ "origin[1]", 0 },
	{ "origin[2]", 0 },
};

void Player_FieldInit( struct delta_s *pFields )
{
	player_field_alias[FIELD_ORIGIN0].field = DELTA_FINDFIELD( pFields, player_field_alias[FIELD_ORIGIN0].name );
	player_field_alias[FIELD_ORIGIN1].field = DELTA_FINDFIELD( pFields, player_field_alias[FIELD_ORIGIN1].name );
	player_field_alias[FIELD_ORIGIN2].field = DELTA_FINDFIELD( pFields, player_field_alias[FIELD_ORIGIN2].name );
}

/*
==================
Player_Encode

Callback for sending entity_state_t for players info over network. 
==================
*/
void Player_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int localplayer = 0;
	static int initialized = 0;

	if ( !initialized )
	{
		Player_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *) from;
	t = (entity_state_t *) to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	localplayer = ( t->number - 1 ) == ENGINE_CURRENT_PLAYER();
	if ( localplayer )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN0].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN1].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN2].field );
	}

	if ( ( t->movetype == MOVETYPE_FOLLOW ) && ( t->aiment != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN0].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN1].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN2].field );
	}
	else if ( t->aiment != f->aiment )
	{
		DELTA_SETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN0].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN1].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[FIELD_ORIGIN2].field );
	}
}

#define CUSTOMFIELD_ORIGIN0 0
#define CUSTOMFIELD_ORIGIN1 1
#define CUSTOMFIELD_ORIGIN2 2
#define CUSTOMFIELD_ANGLES0 3
#define CUSTOMFIELD_ANGLES1 4
#define CUSTOMFIELD_ANGLES2 5
#define CUSTOMFIELD_SKIN 6
#define CUSTOMFIELD_SEQUENCE 7
#define CUSTOMFIELD_ANIMTIME 8

entity_field_alias_t custom_entity_field_alias[] = {
	{ "origin[0]", 0 }, { "origin[1]", 0 }, { "origin[2]", 0 }, { "angles[0]", 0 }, { "angles[1]", 0 },
	{ "angles[2]", 0 }, { "skin", 0 },		{ "sequence", 0 },	{ "animtime", 0 },
};

void Custom_Entity_FieldInit( struct delta_s *pFields )
{
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].field =
		DELTA_FINDFIELD( pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].name );
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].field =
		DELTA_FINDFIELD( pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].name );
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].field =
		DELTA_FINDFIELD( pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].name );
	custom_entity_field_alias[CUSTOMFIELD_ANGLES0].field =
		DELTA_FINDFIELD( pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES0].name );
	custom_entity_field_alias[CUSTOMFIELD_ANGLES1].field =
		DELTA_FINDFIELD( pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES1].name );
	custom_entity_field_alias[CUSTOMFIELD_ANGLES2].field =
		DELTA_FINDFIELD( pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES2].name );
	custom_entity_field_alias[CUSTOMFIELD_SKIN].field =
		DELTA_FINDFIELD( pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].name );
	custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field =
		DELTA_FINDFIELD( pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].name );
	custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field =
		DELTA_FINDFIELD( pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].name );
}

/*
==================
Custom_Encode

Callback for sending entity_state_t info ( for custom entities ) over network. 
FIXME:  Move to script
==================
*/
void Custom_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int beamType;
	static int initialized = 0;

	if ( !initialized )
	{
		Custom_Entity_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *) from;
	t = (entity_state_t *) to;

	beamType = t->rendermode & 0x0f;

	if ( beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].field );
	}

	if ( beamType != BEAM_POINTS )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES0].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES1].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES2].field );
	}

	if ( beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field );
	}

	// animtime is compared by rounding first
	// see if we really shouldn't actually send it
	if ( (int) f->animtime == (int) t->animtime )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field );
	}
}

/*
=================
RegisterEncoders

Allows game .dll to override network encoding of certain types of entities and tweak values, etc.
=================
*/
void RegisterEncoders( void )
{
	DELTA_ADDENCODER( "Entity_Encode", Entity_Encode );
	DELTA_ADDENCODER( "Custom_Encode", Custom_Encode );
	DELTA_ADDENCODER( "Player_Encode", Player_Encode );
}

int GetWeaponData( struct edict_s *player, struct weapon_data_s *info )
{
#if defined( CLIENT_WEAPONS )
	int i;
	weapon_data_t *item;
	entvars_t *pev = &player->v;
	CBasePlayer *pl = dynamic_cast<CBasePlayer *>( CBasePlayer::Instance( pev ) );
	CBasePlayerWeapon *gun;

	ItemInfo II;

	memset( info, 0, 32 * sizeof( weapon_data_t ) );

	if ( !pl )
		return 1;

	// go through all of the weapons and make a list of the ones to pack
	for ( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if ( pl->m_rgpPlayerItems[i] )
		{
			// there's a weapon here. Should I pack it?
			CBasePlayerItem *pPlayerItem = pl->m_rgpPlayerItems[i];

			while ( pPlayerItem )
			{
				gun = dynamic_cast<CBasePlayerWeapon *>( pPlayerItem->GetWeaponPtr() );
				if ( gun && gun->UseDecrement() )
				{
					// Get The ID.
					memset( &II, 0, sizeof( II ) );
					gun->GetItemInfo( &II );

					if ( II.iId >= 0 && II.iId < MAX_WEAPONS )
					{
						item = &info[II.iId];

						item->m_iId = II.iId;
						item->m_iClip = gun->m_iClip;

						item->m_flTimeWeaponIdle = max( gun->m_flTimeWeaponIdle, -0.001f );
						item->m_flNextPrimaryAttack = max( gun->m_flNextPrimaryAttack, -0.001f );
						item->m_flNextSecondaryAttack = max( gun->m_flNextSecondaryAttack, -0.001f );
						item->m_fInReload = gun->m_fInReload;
						item->m_fInSpecialReload = gun->m_fInSpecialReload;
						item->fuser1 = max( gun->pev->fuser1, -0.001f );
						item->fuser2 = gun->m_flStartThrow;
						item->fuser3 = gun->m_flReleaseThrow;
						item->iuser1 = gun->m_chargeReady;
						item->iuser2 = gun->m_fInAttack;
						item->iuser3 = gun->m_fireState;

						//						item->m_flPumpTime				= max( gun->m_flPumpTime, -0.001f );
					}
				}
				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}
#else
	memset( info, 0, 32 * sizeof( weapon_data_t ) );
#endif
	return 1;
}

/*
=================
UpdateClientData

Data sent to current client only
engine sets cd to 0 before calling.
=================
*/
void UpdateClientData( const edict_t *ent, int sendweapons, struct clientdata_s *cd )
{
	if ( !ent || !ent->pvPrivateData )
		return;
	entvars_t *pev = (entvars_t *) &ent->v;
	CBasePlayer *pl = dynamic_cast<CBasePlayer *>( CBasePlayer::Instance( pev ) );
	entvars_t *pevOrg = NULL;

	// if user is spectating different player in First person, override some vars
	if ( pl && pl->pev->iuser1 == OBS_IN_EYE )
	{
		if ( pl->m_hObserverTarget )
		{
			pevOrg = pev;
			pev = pl->m_hObserverTarget->pev;
			pl = dynamic_cast<CBasePlayer *>( CBasePlayer::Instance( pev ) );
		}
	}

	cd->flags = pev->flags;
	cd->health = pev->health;

	cd->viewmodel = MODEL_INDEX( STRING( pev->viewmodel ) );

	cd->waterlevel = pev->waterlevel;
	cd->watertype = pev->watertype;
	cd->weapons = pev->weapons;

	// Vectors
	cd->origin = pev->origin;
	cd->velocity = pev->velocity;
	cd->view_ofs = pev->view_ofs;
	cd->punchangle = pev->punchangle;

	cd->bInDuck = pev->bInDuck;
	cd->flTimeStepSound = pev->flTimeStepSound;
	cd->flDuckTime = pev->flDuckTime;
	cd->flSwimTime = pev->flSwimTime;
	cd->waterjumptime = pev->teleport_time;

	strcpy( cd->physinfo, ENGINE_GETPHYSINFO( ent ) );

	cd->maxspeed = pev->maxspeed;
	cd->fov = pev->fov;
	cd->weaponanim = pev->weaponanim;

	cd->pushmsec = pev->pushmsec;
	cd->deadflag = pev->deadflag;

	if ( pevOrg != NULL )
	{
		// don't use spec vars from chased player
		cd->iuser1 = pevOrg->iuser1;
		cd->iuser2 = pevOrg->iuser2;
		cd->iuser4 = pevOrg->iuser4;
	}
	else
	{
		cd->iuser1 = pev->iuser1;
		cd->iuser2 = pev->iuser2;
		cd->iuser4 = pl ? pl->m_iWeapons2 : 0;
	}

	cd->iuser3 = pev->iuser3;
	cd->fuser4 = pev->fuser4;
	cd->vuser1[0] = ent->v.vuser1.x;
	cd->vuser1[1] = ent->v.vuser1.y;
	cd->vuser1[2] = ent->v.vuser1.z;

	if ( sendweapons )
	{
		if ( pl )
		{
			cd->fuser2 = (float) pl->m_iFOV;
			cd->m_flNextAttack = pl->m_flNextAttack;

			if ( pl->m_pActiveItem == NULL )
			{
				cd->ammo_shells = 0;
				return;
			}

			CBasePlayerWeapon *gun = (CBasePlayerWeapon *) pl->m_pActiveItem->GetWeaponPtr();
			if ( gun != NULL )
			{
				if ( gun->UseDecrement() )
				{
					ItemInfo II;
					memset( &II, 0, sizeof( II ) );
					gun->GetItemInfo( &II );
					cd->m_iId = II.iId;
				}

				cd->ammo_shells = pl->m_rgAmmo[gun->m_iPrimaryAmmoType];
			}
		}
	}
}

/*
=================
CmdStart

We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
=================
*/
void CmdStart( const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed )
{
	entvars_t *pev = (entvars_t *) &player->v;
	CBasePlayer *pl = dynamic_cast<CBasePlayer *>( CBasePlayer::Instance( pev ) );

	if ( !pl )
		return;

	if ( pl->pev->groupinfo != 0 )
	{
		UTIL_SetGroupTrace( pl->pev->groupinfo, GROUP_OP_AND );
	}

	pl->random_seed = random_seed;
}

/*
=================
CmdEnd

Each cmdstart is exactly matched with a cmd end, clean up any group trace flags, etc. here
=================
*/
void CmdEnd( const edict_t *player )
{
	entvars_t *pev = (entvars_t *) &player->v;
	CBasePlayer *pl = dynamic_cast<CBasePlayer *>( CBasePlayer::Instance( pev ) );

	if ( !pl )
		return;
	if ( pl->pev->groupinfo != 0 )
	{
		UTIL_UnsetGroupTrace();
	}
}

/*
================================
ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer,
						  int *response_buffer_size )
{
	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

/*
================================
GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	int iret = 0;

	switch ( hullnumber )
	{
	case 0:	 // Normal player
		VEC_HULL_MIN.CopyToArray( mins );
		VEC_HULL_MAX.CopyToArray( maxs );
		iret = 1;
		break;
	case 1:	 // Crouched player
		VEC_DUCK_HULL_MIN.CopyToArray( mins );
		VEC_DUCK_HULL_MAX.CopyToArray( maxs );
		iret = 1;
		break;
	case 2:	 // Point based hull
		Vector( 0, 0, 0 ).CopyToArray( mins );
		Vector( 0, 0, 0 ).CopyToArray( maxs );
		iret = 1;
		break;
	}

	return iret;
}

/*
================================
CreateInstancedBaselines

Create pseudo-baselines for items that aren't placed in the map at spawn time, but which are likely
to be created during play ( e.g., grenades, ammo packs, projectiles, corpses, etc. )
================================
*/
void CreateInstancedBaselines( void )
{
	int iret = 0;
	entity_state_t state;

	memset( &state, 0, sizeof( state ) );

	// Create any additional baselines here for things like grendates, etc.
	// iret = ENGINE_INSTANCE_BASELINE( pc->pev->classname, &state );

	// Destroy objects.
	//UTIL_Remove( pc );
}

/*
================================
InconsistentFile

One of the ENGINE_FORCE_UNMODIFIED files failed the consistency check for the specified player
 Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
================================
*/
int InconsistentFile( const edict_t *player, const char *filename, char *disconnect_message )
{
	// DoD: binary unconditionally enforces consistency (no mp_consistency check)
	sprintf( disconnect_message, "Server is enforcing file consistency for %s\n", filename );

	// Kick now with specified disconnect message.
	return 1;
}

/*
================================
AllowLagCompensation

 The game .dll should return 1 if lag compensation should be allowed ( could also just set
  the sv_unlag cvar.
 Most games right now should return 0, until client-side weapon prediction code is written
  and tested for them ( note you can predict weapons, but not do lag compensation, too, 
  if you want.
================================
*/
int AllowLagCompensation( void )
{
	return 1;
}

void ShowVGUI( CBasePlayer *pPlayer, int iMenu, int iParam )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgVGUIMenu, NULL, pPlayer->pev->pContainingEntity );
	WRITE_BYTE( iMenu );
	WRITE_LONG( iParam );
	MESSAGE_END();
}

void ResetPlayers( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if ( plr && plr->pev->team != 0 && plr->m_iNextClass != 0 )
		{
			plr->RemoveAllItems( 1 );
			plr->Spawn();
		}
	}
}

void ResetEnt( const char *classname )
{
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = UTIL_FindEntityByClassname( pEnt, classname ) ) != NULL )
		pEnt->RoundRespawnEnt();
}

void CallRoundRespawn( const char *classname )
{
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = UTIL_FindEntityByClassname( pEnt, classname ) ) != NULL )
		pEnt->RoundRespawn();
}

void ResetEnts( void )
{
	ResetEnt( "func_breakable" );
	ResetEnt( "func_door" );
	ResetEnt( "func_door_rot" );
	ResetEnt( "func_door_rotating" );
	ResetEnt( "light" );
	ResetEnt( "env_sprite" );
	ResetEnt( "dod_object" );
	ResetEnt( "dod_control_point" );
	CallRoundRespawn( "dod_control_point_master" );
	ResetEnt( "dod_capture_area" );
	ResetEnt( "dod_object_goal" );
	ResetEnt( "func_train" );
	ResetEnt( "trigger_auto" );
	ResetEnt( "trigger_once" );
	ResetEnt( "trigger_multiple" );
	ResetEnt( "func_wall_toggle" );
	ResetEnt( "game_counter" );
	ResetEnt( "trigger_hurt" );
	ResetEnt( "env_bubbles" );
	ResetEnt( "env_model" );
	ResetEnt( "func_wall" );
	ResetEnt( "func_illusionary" );
	ResetEnt( "multi_manager" );
	ResetEnt( "multisource" );
	ResetEnt( "info_alias" );
	ResetEnt( "func_conveyor" );
	ResetEnt( "func_monsterclip" );
	ResetEnt( "func_rotating" );
	ResetEnt( "func_pendulum" );
	ResetEnt( "env_state" );
	ResetEnt( "func_button" );
	ResetEnt( "func_rot_button" );
	ResetEnt( "momentary_rot_button" );
	ResetEnt( "env_spark" );
	ResetEnt( "env_debris" );
	ResetEnt( "button_target" );
	ResetEnt( "func_water" );
	ResetEnt( "env_fade" );
	ResetEnt( "env_message" );
	ResetEnt( "func_pushable" );
	ResetEnt( "trigger_relay" );
	ResetEnt( "multi_watcher" );
	ResetEnt( "watcher" );
	ResetEnt( "env_render" );
	ResetEnt( "func_ladder" );
	ResetEnt( "trigger_push" );
	ResetEnt( "trigger_teleport" );
	ResetEnt( "dod_location" );
	ResetEnt( "dod_trigger_sandbag" );
	ResetEnt( "game_counter_set" );
	ResetEnt( "game_player_hurt" );
	ResetEnt( "game_score" );
	ResetEnt( "game_team_master" );
	ResetEnt( "game_team_set" );
	ResetEnt( "game_text" );
	ResetEnt( "path_corner" );
	ResetEnt( "path_track" );
	ResetEnt( "func_tracktrain" );
	ResetEnt( "func_trackautochange" );
	ResetEnt( "func_trackchange" );
	ResetEnt( "func_traincontrols" );
	CallRoundRespawn( "particle_shooter" );
	CallRoundRespawn( "weapon_bazooka" );
	CallRoundRespawn( "weapon_piat" );
	CallRoundRespawn( "weapon_pschreck" );
	CallRoundRespawn( "weaponbox" );
	CallRoundRespawn( "ambient_generic" );
	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, NULL, ((CDoDTeamPlay *)g_pGameRules)->RoundReset(), 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );
}

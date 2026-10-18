// DoD mapcycle helpers (dod/dlls/dod_mapcycle.cpp)
//
// ReloadMapCycleFile / DestroyMapCycle / GetNextMap are declared extern in
// dod_gamerules.cpp but were never defined in the DoD tree: in the original
// build they were linked from the base HL gamerules objects, which this tree
// does not ship (DoD-only game logic). Reimplemented here from the HL logic,
// const-correct. `com_token` (used by the global COM_Parse in
// dod_gamerules.cpp) is defined here as well — it had no definition either.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "enginecallback.h"

#define MAX_RULE_BUFFER 1024

char com_token[1024];

// NOTE: layout must match the local structs in dod_gamerules.cpp
// (same struct names => same C++ linkage, identical layout => link-safe).
struct mapcycle_item_s
{
	struct mapcycle_item_s *next;
	char mapname[32];
	int minplayers;
	int maxplayers;
	char rulebuffer[MAX_RULE_BUFFER];
};
struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
};
typedef struct mapcycle_s mapcycle_t;

extern char *COM_Parse( char *data );
extern BOOL COM_TokenWaiting( char *buffer );

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void DestroyMapCycle( mapcycle_t *cycle )
{
	mapcycle_item_s *p, *n, *start;
	p = cycle->items;
	if ( p )
	{
		start = p;
		p = p->next;
		while ( p != start )
		{
			n = p->next;
			delete p;
			p = n;
		}

		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}

/*
==============
ReloadMapCycleFile

Parses mapcycle.txt file into mapcycle_t structure
==============
*/
int ReloadMapCycleFile( const char *filename, mapcycle_t *cycle )
{
	char szBuffer[MAX_RULE_BUFFER];
	char szMap[32];
	int length;
	char *pFileList;
	char *aFileList = pFileList = (char *)LOAD_FILE_FOR_ME( filename, &length );
	int hasbuffer;
	mapcycle_item_s *item, *newlist = NULL, *next;

	if ( pFileList && length )
	{
		// the first map name in the file becomes the default
		while ( 1 )
		{
			hasbuffer = 0;
			memset( szBuffer, 0, MAX_RULE_BUFFER );

			pFileList = COM_Parse( pFileList );
			if ( strlen( com_token ) <= 0 )
				break;

			strncpy( szMap, com_token, sizeof( szMap ) );
			szMap[sizeof( szMap ) - 1] = '\0';

			// Any more tokens on this line?
			if ( COM_TokenWaiting( pFileList ) )
			{
				pFileList = COM_Parse( pFileList );
				if ( strlen( com_token ) > 0 )
				{
					hasbuffer = 1;
					strncpy( szBuffer, com_token, sizeof( szBuffer ) );
					szBuffer[sizeof( szBuffer ) - 1] = '\0';
				}
			}

			// Check map
			if ( IS_MAP_VALID( szMap ) )
			{
				// Create entry
				char *s;

				item = new mapcycle_item_s;

				strcpy( item->mapname, szMap );

				item->minplayers = 0;
				item->maxplayers = 0;

				memset( item->rulebuffer, 0, MAX_RULE_BUFFER );

				if ( hasbuffer )
				{
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "minplayers" );
					if ( s && s[0] )
					{
						item->minplayers = atoi( s );
						item->minplayers = max( item->minplayers, 0 );
						item->minplayers = min( item->minplayers, gpGlobals->maxClients );
					}
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "maxplayers" );
					if ( s && s[0] )
					{
						item->maxplayers = atoi( s );
						item->maxplayers = max( item->maxplayers, 0 );
						item->maxplayers = min( item->maxplayers, gpGlobals->maxClients );
					}

					// Remove keys
					//
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "minplayers" );
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "maxplayers" );

					strcpy( item->rulebuffer, szBuffer );
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				ALERT( at_console, "Skipping %s from mapcycle, not a valid map\n", szMap );
			}
		}

		FREE_FILE( aFileList );
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while ( item )
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if ( !item )
	{
		return 0;
	}

	while ( item->next )
	{
		item = item->next;
	}
	item->next = cycle->items;

	cycle->next_item = item->next;

	return 1;
}

/*
==============
GetNextMap

Pick the next map honoring min/maxplayers against the current player count.
Rotates through the circular list across ChangeLevel calls; resyncs when the
mapcycle file was reloaded (cycle->items pointer changed).
==============
*/
void GetNextMap( mapcycle_t cycle, int cur, char *nextMap, char *firstMap )
{
	static mapcycle_item_s *s_seenItems = NULL;
	static mapcycle_item_s *s_next = NULL;

	if ( !cycle.items )
		return;

	if ( s_seenItems != cycle.items )
	{
		s_seenItems = cycle.items;
		s_next = cycle.next_item ? cycle.next_item : cycle.items;
	}

	strncpy( firstMap, cycle.items->mapname, 31 );
	firstMap[31] = '\0';
	strncpy( nextMap, firstMap, 31 );
	nextMap[31] = '\0';

	mapcycle_item_s *start = s_next ? s_next : cycle.items;
	mapcycle_item_s *item = start;

	do
	{
		if ( ( item->minplayers == 0 || cur >= item->minplayers ) &&
			 ( item->maxplayers == 0 || cur <= item->maxplayers ) )
			break;
		item = item->next;
	} while ( item && item != start );

	if ( item )
	{
		strncpy( nextMap, item->mapname, 31 );
		nextMap[31] = '\0';
		s_next = item->next ? item->next : cycle.items;
	}
}

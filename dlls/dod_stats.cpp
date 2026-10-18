// DoD stats restore - dod/dlls/dod_stats.cpp
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gamerules.h"
#include "dod_gamerules.h"

extern char StatNames[4][64];

// .text:000ded70 void CDoDStatMgr::CDoDStatMgr() (dod/dlls/dod_stats.cpp:23)
CDoDStatMgr::CDoDStatMgr( void )
{
	memset( m_PlayerStats, 0, sizeof( m_PlayerStats ) );
}
// .text:000dee30 void CDoDStatMgr::InitPlayer(int) (dod/dlls/dod_stats.cpp:31)
void CDoDStatMgr::InitPlayer( int index )
{
	if ( (unsigned) index > 0x1f )
		return;
	m_PlayerStats[index].stat[0] = 0;
	m_PlayerStats[index].stat[1] = 0;
	m_PlayerStats[index].stat[2] = 0;
	m_PlayerStats[index].stat[3] = 0;
	m_PlayerStats[index].starttime = gpGlobals->time;
}
// .text:000dee80 DoDPlayerStats_t* CDoDStatMgr::GetPlayer(int) (dod/dlls/dod_stats.cpp:40)
DoDPlayerStats_t *CDoDStatMgr::GetPlayer( int index )
{
	if ( (unsigned) index < 0x20 )
		return &m_PlayerStats[index];
	return NULL;
}
// .text:000deea0 void CDoDStatMgr::PrintPlayerStats(int) (dod/dlls/dod_stats.cpp:53)
void CDoDStatMgr::PrintPlayerStats( int index )
{
	DoDPlayerStats_t *p = ( (unsigned) index < 0x20 ) ? &m_PlayerStats[index] : NULL;
	CBaseEntity *pEnt = UTIL_PlayerByIndex( index );
	if ( pEnt && p )
	{
		char buf[32];
		sprintf( buf, "%s: %d\n", StatNames[0], p->stat[0] );
		ClientPrint( pEnt->pev, HUD_PRINTCONSOLE, buf, 0, 0, 0, 0 );
		sprintf( buf, "%s: %d\n", StatNames[1], p->stat[1] );
		ClientPrint( pEnt->pev, HUD_PRINTCONSOLE, buf, 0, 0, 0, 0 );
		sprintf( buf, "%s: %d\n", StatNames[2], p->stat[2] );
		ClientPrint( pEnt->pev, HUD_PRINTCONSOLE, buf, 0, 0, 0, 0 );
		sprintf( buf, "%s: %d\n", StatNames[3], p->stat[3] );
		ClientPrint( pEnt->pev, HUD_PRINTCONSOLE, buf, 0, 0, 0, 0 );
	}
}

// .text:000dec30 void dod_stats(entvars_t*) (dod/dlls/dod_stats.cpp:21)
LINK_ENTITY_TO_CLASS( dod_stats, CDoDStatMgr );

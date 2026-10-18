// alias.cpp - DoD alias entities, restored from dod.so
// Ghidra addr = link + 0x10000. file:line via addr2line -e dod.so -f -C (link = Ghidra - 0x10000).
// Scope: CInfoAlias, CInfoGroup, CMultiAlias, CTriggerChangeAlias plus factories.
// Duplicates: CBaseEntity::Illumination already inline in cbase.h, skipped.
// CDoDDetect and info_doddetect already ported in maprules.cpp and objective.h, not here.
// Helpers UTIL_Follow* originally util.cpp:822, provided here for link since util.cpp lacks them.
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"

class CInfoAlias : public CBaseEntity
{
  public:
	void Spawn();
	void RoundRespawn();
	void Think();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int ObjectCaps();
	STATE GetState();
};

class CInfoGroup : public CBaseEntity
{
  public:
	void KeyValue( KeyValueData *pkvd );
	void Think();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int Save( CSave &save );
	int Restore( CRestore &restore );
	int GetMember( const char *szMemberName );
	static TYPEDESCRIPTION m_SaveData[];
	int m_cMembers;
	int m_iszMemberName[16];
	int m_iszMemberValue[16];
};

class CMultiAlias : public CBaseEntity
{
  public:
	void KeyValue( KeyValueData *pkvd );
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
	int m_cTargets;
	int m_iszTargets[16];
};

class CTriggerChangeAlias : public CBaseEntity
{
  public:
	void Spawn();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int ObjectCaps();
};

TYPEDESCRIPTION CMultiAlias::m_SaveData[] = {
	DEFINE_FIELD( CMultiAlias, m_cTargets, FIELD_INTEGER ),
	DEFINE_ARRAY( CMultiAlias, m_iszTargets, FIELD_STRING, 16 ),
};
IMPLEMENT_SAVERESTORE( CMultiAlias, CBaseEntity )

TYPEDESCRIPTION CInfoGroup::m_SaveData[] = {
	DEFINE_FIELD( CInfoGroup, m_cMembers, FIELD_INTEGER ),
	DEFINE_ARRAY( CInfoGroup, m_iszMemberName, FIELD_STRING, 16 ),
	DEFINE_ARRAY( CInfoGroup, m_iszMemberValue, FIELD_STRING, 16 ),
};
IMPLEMENT_SAVERESTORE( CInfoGroup, CBaseEntity )

LINK_ENTITY_TO_CLASS( info_alias, CInfoAlias )
LINK_ENTITY_TO_CLASS( info_group, CInfoGroup )
LINK_ENTITY_TO_CLASS( multi_alias, CMultiAlias )
LINK_ENTITY_TO_CLASS( trigger_changealias, CTriggerChangeAlias )

// engine STRING() returns const char*: const-ify read-only string params (was char* in older SDKs)
CBaseEntity *UTIL_FollowAliasReference( CBaseEntity *pStartEntity, const char *szValue );
CBaseEntity *UTIL_FollowGroupReference( CBaseEntity *pStartEntity, const char *szGroupName, const char *szMemberName );
CBaseEntity *UTIL_FollowReference( CBaseEntity *pStartEntity, const char *szName );

// .text:00161bc0 UTIL_FollowAliasReference (util.cpp:800, helper for alias, provided for link)
CBaseEntity *UTIL_FollowAliasReference( CBaseEntity *pStartEntity, const char *szValue )
{
	CBaseEntity *pBest = 0;
	int bestOff = -1;
	CBaseEntity *pEnt = UTIL_FindEntityByString( 0, "targetname", szValue );
	while ( pEnt != 0 )
	{
		if ( FClassnameIs( pEnt->pev, "info_alias" ) )
		{
			CBaseEntity *pT = UTIL_FindEntityByString( pStartEntity, "targetname", STRING( pEnt->pev->target ) );
			if ( pT != 0 )
			{
				int off = ENTINDEX( pT->edict() );
				if ( bestOff == -1 || off < bestOff )
				{
					bestOff = off;
					pBest = pT;
				}
			}
		}
		else if ( FClassnameIs( pEnt->pev, "multi_alias" ) )
		{
			CMultiAlias *pMult = (CMultiAlias *) pEnt;
			for ( int i = 0; i < pMult->m_cTargets; i++ )
			{
				CBaseEntity *pT =
					UTIL_FindEntityByString( pStartEntity, "targetname", STRING( pMult->m_iszTargets[i] ) );
				if ( pT != 0 )
				{
					int off = ENTINDEX( pT->edict() );
					if ( bestOff == -1 || off < bestOff )
					{
						bestOff = off;
						pBest = pT;
					}
				}
			}
		}
		pEnt = UTIL_FindEntityByString( pEnt, "targetname", szValue );
	}
	return pBest;
}
// .text:00161d60 UTIL_FollowGroupReference (util.cpp:810, helper for alias, provided for link)
CBaseEntity *UTIL_FollowGroupReference( CBaseEntity *pStartEntity, const char *szGroupName, const char *szMemberName )
{
	char buf[80];
	const char *tail = 0;
	const char *head = szMemberName;
	if ( *szMemberName != 0 )
	{
		if ( *szMemberName == 46 )
		{
			head = szMemberName + 1;
		}
		else
		{
			int n = 0;
			while ( szMemberName[n] != 0 && szMemberName[n] != 46 )
				n++;
			if ( szMemberName[n] == 0 )
			{
				head = szMemberName;
			}
			else
			{
				strncpy( buf, szMemberName, n );
				buf[n] = 0;
				head = buf;
				tail = szMemberName + n + 1;
			}
		}
	}
	CBaseEntity *pBest = 0;
	int bestOff = -1;
	CBaseEntity *pGrp = UTIL_FindEntityByString( 0, "targetname", szGroupName );
	while ( pGrp != 0 )
	{
		if ( FClassnameIs( pGrp->pev, "info_group" ) )
		{
			int iszVal = ( (CInfoGroup *) pGrp )->GetMember( head );
			if ( iszVal != 0 )
			{
				CBaseEntity *pT = 0;
				if ( tail == 0 )
					pT = UTIL_FindEntityByString( pStartEntity, "targetname", STRING( iszVal ) );
				else
					pT = UTIL_FollowGroupReference( pStartEntity, STRING( iszVal ), tail );
				if ( pT != 0 )
				{
					int off = ENTINDEX( pT->edict() );
					if ( bestOff == -1 || off < bestOff )
					{
						bestOff = off;
						pBest = pT;
					}
				}
			}
		}
		pGrp = UTIL_FindEntityByString( pGrp, "targetname", szGroupName );
	}
	return pBest;
}
// .text:00161f40 UTIL_FollowReference (util.cpp:822, helper for alias, provided for link)
CBaseEntity *UTIL_FollowReference( CBaseEntity *pStartEntity, const char *szName )
{
	if ( szName == 0 || *szName == 0 )
		return 0;
	if ( *szName == 42 )
		return UTIL_FollowAliasReference( pStartEntity, szName + 1 );
	int n = 0;
	if ( *szName == 46 )
		n = 0;
	else
	{
		while ( szName[n] != 0 && szName[n] != 46 )
			n++;
		if ( szName[n] == 0 )
			return 0;
	}
	char root[96];
	strncpy( root, szName, n );
	root[n] = 0;
	return UTIL_FollowGroupReference( pStartEntity, root, szName + n + 1 );
}

// .text:000b9d10 CInfoAlias::Spawn (alias.cpp:47)
void CInfoAlias::Spawn()
{
	RoundStore();
	if ( pev->spawnflags & 1 )
	{
		int tmp = pev->target;
		pev->target = pev->netname;
		pev->netname = tmp;
	}
}
// .text:000b9d50 CInfoAlias::RoundRespawn (alias.cpp:60)
void CInfoAlias::RoundRespawn()
{
	if ( pev->spawnflags & 1 )
	{
		int tmp = pev->target;
		pev->target = pev->netname;
		pev->netname = tmp;
	}
}
// .text:000b98b0 CInfoAlias::Think (alias.cpp:79)
void CInfoAlias::Think()
{
	if ( ( pev->spawnflags & 1 ) == 0 )
	{
		if ( ( pev->spawnflags & 2 ) == 0 )
			pev->spawnflags |= 1;
		else
		{
			ALERT( at_console, "DEBUG: info_alias %s turns off", STRING( pev->targetname ) );
			pev->spawnflags |= 1;
		}
	}
	else
	{
		if ( pev->spawnflags & 2 )
			ALERT( at_console, "DEBUG: info_alias %s turns on", STRING( pev->targetname ) );
		pev->spawnflags &= 0xFFFFFFFE;
	}
	int tmp = pev->netname;
	pev->netname = pev->target;
	pev->target = tmp;
	if ( pev->spawnflags & 2 )
		ALERT( at_console, ": target becomes %s", STRING( pev->target ) );
}
// .text:000ba0c0 CInfoAlias::Use (alias.cpp:71)
void CInfoAlias::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( ShouldToggle( useType, GetState() ) )
		pev->nextthink = gpGlobals->time;
}
// .text:000bad20 CInfoAlias::ObjectCaps (alias.cpp:36)
int CInfoAlias::ObjectCaps()
{
	return 0;
}
// .text:000bad30 CInfoAlias::GetState (alias.cpp:41)
STATE CInfoAlias::GetState()
{
	return ( pev->spawnflags & 1 ) ? STATE_OFF : STATE_ON;
}
// .text:000ba100 info_alias (alias.cpp:44)
// .text:000b9e50 CInfoGroup::KeyValue (alias.cpp:124)
void CInfoGroup::KeyValue( KeyValueData *pkvd )
{
	if ( m_cMembers < 16 )
	{
		char tmp[128];
		UTIL_StripToken( pkvd->szKeyName, tmp, 128 );
		m_iszMemberName[m_cMembers] = ALLOC_STRING( tmp );
		m_iszMemberValue[m_cMembers] = ALLOC_STRING( pkvd->szValue );
		m_cMembers++;
		pkvd->fHandled = TRUE;
	}
	else
		ALERT( at_error, "Too many members for info_group %s (limit is %d)", STRING( pev->targetname ), 16 );
}
// .text:000b9bd0 CInfoGroup::Think (alias.cpp:148)
void CInfoGroup::Think()
{
	CBaseEntity *pT = UTIL_FindEntityByTargetname( 0, STRING( pev->target ) );
	if ( pT != 0 && FClassnameIs( pT->pev, "info_alias" ) )
	{
		pT->pev->target = pev->targetname;
		if ( ( pev->spawnflags & 2 ) == 0 )
			return;
		ALERT( at_console, "info_group %s changed the contents of alias %s to %s", STRING( pev->targetname ),
			   STRING( pT->pev->targetname ), STRING( pT->pev->target ) );
		return;
	}
	if ( pev->target != 0 )
		ALERT( at_console, "info_group %s: alias %s was not found or not an alias!", STRING( pev->targetname ),
			   STRING( pev->target ) );
}
// .text:000b99e0 CInfoGroup::Use (alias.cpp:144)
void CInfoGroup::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	pev->nextthink = gpGlobals->time;
}
// .text:000ba060 CInfoGroup::Save (alias.cpp:121)
// .text:000b9fa0 CInfoGroup::Restore (alias.cpp:121)
// .text:000ba1e0 CInfoGroup::GetMember (alias.cpp:165)
int CInfoGroup::GetMember( const char *szMemberName )
{
	if ( szMemberName == 0 )
	{
		ALERT( at_console, "info_group: GetMember called with null szMemberName!" );
		return 0;
	}
	for ( int i = 0; i < m_cMembers; i++ )
	{
		if ( strcmp( szMemberName, STRING( m_iszMemberName[i] ) ) == 0 )
			return m_iszMemberValue[i];
	}
	ALERT( at_console, "info_group %s has no member called %s.", STRING( pev->targetname ), szMemberName );
	return 0;
}
// .text:000ba170 info_group (alias.cpp:112)
// .text:000b9d80 CMultiAlias::KeyValue (alias.cpp:203)
void CMultiAlias::KeyValue( KeyValueData *pkvd )
{
	if ( m_cTargets < 16 )
	{
		char tmp[128];
		UTIL_StripToken( pkvd->szKeyName, tmp, 128 );
		m_iszTargets[m_cTargets] = ALLOC_STRING( tmp );
		m_cTargets++;
		pkvd->fHandled = TRUE;
	}
	else
		ALERT( at_error, "Too many targets for multi_alias %s (limit is %d)", STRING( pev->targetname ), 16 );
}
// .text:000ba000 CMultiAlias::Save (alias.cpp:200)
// .text:000b9f40 CMultiAlias::Restore (alias.cpp:200)
// .text:000ba2a0 multi_alias (alias.cpp:192)
// .text:000b9a00 CTriggerChangeAlias::Spawn (alias.cpp:241)
void CTriggerChangeAlias::Spawn()
{
	return;
}
// .text:000b9a10 CTriggerChangeAlias::Use (alias.cpp:244)
void CTriggerChangeAlias::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pT = UTIL_FindEntityByTargetname( 0, STRING( pev->target ) );
	if ( pT != 0 && FClassnameIs( pT->pev, "info_alias" ) )
	{
		int newVal = 0;
		if ( pev->spawnflags & 1 )
		{
			CBaseEntity *pF = UTIL_FollowReference( 0, STRING( pev->netname ) );
			if ( pF != 0 )
				newVal = pF->pev->targetname;
		}
		if ( newVal == 0 )
			newVal = pev->netname;
		pT->pev->target = newVal;
		if ( pev->spawnflags & 2 )
			ALERT( at_console, "DEBUG: trigger_changealias %s activated: changed contents of alias %s to %s" );
		return;
	}
	ALERT( at_console, "trigger_changealias %s: alias %s was not found or not an alias!" );
}
// .text:000bad50 CTriggerChangeAlias::ObjectCaps (alias.cpp:235)
int CTriggerChangeAlias::ObjectCaps()
{
	return 0;
}
// .text:000ba310 trigger_changealias (alias.cpp:237)
// NOTE: CBaseEntity::Illumination (alias.cpp:270) skipped, already inline in cbase.h

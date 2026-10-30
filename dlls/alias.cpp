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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"

// engine STRING() returns const char*: const-ify read-only string params (was char* in older SDKs)
CBaseEntity *UTIL_FollowAliasReference( CBaseEntity *pStartEntity, const char *szValue );
CBaseEntity *UTIL_FollowGroupReference( CBaseEntity *pStartEntity, const char *szGroupName, const char *szMemberName );
CBaseEntity *UTIL_FollowReference( CBaseEntity *pStartEntity, const char *szName );

#define SF_ALIAS_OFF 0x0001		// info_alias starts switched off (target and netname are swapped)
#define SF_ALIAS_DEBUG 0x0002	// the alias entities print what they change
#define SF_CHANGEALIAS_FOLLOW 0x0001	// trigger_changealias takes the name of the entity its netname refers to

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

int CInfoAlias::ObjectCaps()
{
	return 0;
}

STATE CInfoAlias::GetState()
{
	return ( pev->spawnflags & SF_ALIAS_OFF ) ? STATE_OFF : STATE_ON;
}

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

void CInfoAlias::Spawn()
{
	RoundStore();
	if( pev->spawnflags & SF_ALIAS_OFF )
	{
		int tmp = pev->target;
		pev->target = pev->netname;
		pev->netname = tmp;
	}
}

void CInfoAlias::RoundRespawn()
{
	if( pev->spawnflags & SF_ALIAS_OFF )
	{
		int tmp = pev->target;
		pev->target = pev->netname;
		pev->netname = tmp;
	}
}

void CInfoAlias::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( ShouldToggle( useType, GetState() ) )
		pev->nextthink = gpGlobals->time;
}

void CInfoAlias::Think()
{
	if( ( pev->spawnflags & SF_ALIAS_OFF ) == 0 )
	{
		if( ( pev->spawnflags & SF_ALIAS_DEBUG ) == 0 )
			pev->spawnflags |= SF_ALIAS_OFF;
		else
		{
			ALERT( at_console, "DEBUG: info_alias %s turns off", STRING( pev->targetname ) );
			pev->spawnflags |= SF_ALIAS_OFF;
		}
	}
	else
	{
		if( pev->spawnflags & SF_ALIAS_DEBUG )
			ALERT( at_console, "DEBUG: info_alias %s turns on", STRING( pev->targetname ) );

		pev->spawnflags &= ~SF_ALIAS_OFF;
	}

	int tmp = pev->netname;
	pev->netname = pev->target;
	pev->target = tmp;

	if( pev->spawnflags & SF_ALIAS_DEBUG )
		ALERT( at_console, ": target becomes \"%s\"\n", STRING( pev->target ) );
}

LINK_ENTITY_TO_CLASS( info_group, CInfoGroup )

void CInfoGroup::KeyValue( KeyValueData *pkvd )
{
	if( m_cMembers < 16 )
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

void CInfoGroup::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	pev->nextthink = gpGlobals->time;
}

void CInfoGroup::Think()
{
	CBaseEntity *pT = UTIL_FindEntityByTargetname( 0, STRING( pev->target ) );
	if( pT != 0 && FClassnameIs( pT->pev, "info_alias" ) )
	{
		pT->pev->target = pev->targetname;
		if( ( pev->spawnflags & SF_ALIAS_DEBUG ) == 0 )
			return;

		ALERT( at_console, "info_group %s changed the contents of alias %s to %s", STRING( pev->targetname ),
			STRING( pT->pev->targetname ), STRING( pT->pev->target ) );

		return;
	}

	if( pev->target != 0 )
		ALERT( at_console, "info_group %s: alias %s was not found or not an alias!", STRING( pev->targetname ),
			STRING( pev->target ) );
}

int CInfoGroup::GetMember( const char *szMemberName )
{
	if( szMemberName == 0 )
	{
		ALERT( at_console, "info_group: GetMember called with null szMemberName!" );
		return 0;
	}

	for( int i = 0; i < m_cMembers; i++ )
	{
		if( strcmp( szMemberName, STRING( m_iszMemberName[i] ) ) == 0 )
			return m_iszMemberValue[i];
	}

	ALERT( at_console, "info_group %s has no member called %s.", STRING( pev->targetname ), szMemberName );
	return 0;
}

LINK_ENTITY_TO_CLASS( multi_alias, CMultiAlias )

void CMultiAlias::KeyValue( KeyValueData *pkvd )
{
	if( m_cTargets < 16 )
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

class CTriggerChangeAlias : public CBaseEntity
{
public:
	void Spawn();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int ObjectCaps();
};

int CTriggerChangeAlias::ObjectCaps()
{
	return 0;
}

LINK_ENTITY_TO_CLASS( trigger_changealias, CTriggerChangeAlias )

void CTriggerChangeAlias::Spawn()
{
	return;
}

void CTriggerChangeAlias::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pAlias = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );

	if( !pAlias || !FClassnameIs( pAlias->pev, "info_alias" ) )
	{
		ALERT( at_console, "trigger_changealias %s: alias \"%s\" was not found or not an alias!", STRING( pev->targetname ),
			STRING( pev->target ) );
		return;
	}

	int iNewValue = 0;

	if( pev->spawnflags & SF_CHANGEALIAS_FOLLOW )
	{
		CBaseEntity *pFollow = UTIL_FollowReference( NULL, STRING( pev->netname ) );

		if( pFollow )
			iNewValue = pFollow->pev->targetname;
	}

	if( !iNewValue )
		iNewValue = pev->netname;

	pAlias->pev->target = iNewValue;

	if( pev->spawnflags & SF_ALIAS_DEBUG )
		ALERT( at_console, "DEBUG: trigger_changealias %s activated: changed contents of alias \"%s\" to \"%s\"\n",
			STRING( pev->targetname ), STRING( pAlias->pev->targetname ), STRING( iNewValue ) );
}
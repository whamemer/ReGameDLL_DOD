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
#include "player.h"
#include "hintmessage.h"

void CHintMessageQueue::Reset()
{
	m_iCount = 0;
	m_aszMessage[0] = 0;
	m_aszMessage[1] = 0;
	m_aszMessage[2] = 0;
	m_aszMessage[3] = 0;
	m_aszMessage[4] = 0;
	m_aszMessage[5] = 0;
	m_aszMessage[6] = 0;
	m_aszMessage[7] = 0;
	m_iCurrentMessage = 0;
	m_iLastMessage = 1;
	m_tmMessageEnd = 0.0;
}

void CHintMessageQueue::Update( CBaseEntity &client )
{
	if( m_tmMessageEnd < gpGlobals->time && m_iCount != 0 )
	{
		int cur = m_iCurrentMessage;
		m_aszMessage[cur] = 0;
		m_iCount--;
		m_iCurrentMessage = ( cur + 1 ) % 8;
		m_tmMessageEnd = gpGlobals->time + 6.0;

		if( m_aszMessage[m_iCurrentMessage] != 0 )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgHudText, 0, client.pev->pContainingEntity );
			const char *msg = m_aszMessage[m_iCurrentMessage];
			char tmp[512];

			if( strlen( msg ) > 499 )
			{
				strncpy( tmp, msg, 499 );
				tmp[499] = 0;
				msg = tmp;
			}

			WRITE_STRING( msg );
			WRITE_BYTE( 1 );
			MESSAGE_END();
		}
	}
}

bool CHintMessageQueue::AddMessage( const char *message )
{
	int last = m_iLastMessage;

	if( m_aszMessage[last] != 0 && strcmp( m_aszMessage[last], message ) == 0 )
		return false;

	int cnt = m_iCount;

	if( cnt < 8 )
	{
		m_aszMessage[last] = message;
		m_iLastMessage = ( last + 1 ) % 8;
		m_iCount = cnt + 1;
	}

	return cnt < 8;
}
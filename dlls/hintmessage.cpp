// hintmessage.cpp - DoD HUD hint queue, DoD-new, absent in SDK
// Restored from dod.so. Ghidra addr = link + 0x10000.
// DoD predates the later engine-era heap-based hint queue: it uses a STATIC ring (char* m_aszMessage[8],
// int count/cur/last + float end, +6.0s HudText, strcmp-dedup on last slot),
// verified 3 funcs (Reset hintmessage.cpp:14, Update :27, AddMessage :61).
// Later engine-era code instead uses a heap queue of message objects
// (CloneString args, duration, isHint, Send via UTIL_ShowMessageArgs).
// Do NOT backport CUtlVector here: binary layout is 0x30 bytes @+0x79c.
// NOTE: CHintMessageQueue::AddMessage has no callers in dod.so (x-ref empty);
// live HintMessage path (player.cpp:6308-6331) sends HudText directly.
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "hintmessage.h"
// .text:0010ac50 CHintMessageQueue::Reset (hintmessage.cpp:14)
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
// .text:0010ac80 CHintMessageQueue::Update (hintmessage.cpp:27)
void CHintMessageQueue::Update( CBaseEntity &client )
{
	if ( m_tmMessageEnd < gpGlobals->time && m_iCount != 0 )
	{
		int cur = m_iCurrentMessage;
		m_aszMessage[cur] = 0;
		m_iCount--;
		m_iCurrentMessage = ( cur + 1 ) % 8;
		m_tmMessageEnd = gpGlobals->time + 6.0;
		if ( m_aszMessage[m_iCurrentMessage] != 0 )
		{
			MESSAGE_BEGIN( 1, gmsgHudText, 0, client.pev->pContainingEntity );
			const char *msg = m_aszMessage[m_iCurrentMessage];
			if ( strlen( msg ) > 499 )
			{
				char tmp[500];
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
// .text:0010ad80 CHintMessageQueue::AddMessage (hintmessage.cpp:61)
bool CHintMessageQueue::AddMessage( const char *message )
{
	int last = m_iLastMessage;
	if ( m_aszMessage[last] != 0 && strcmp( m_aszMessage[last], message ) == 0 )
		return false;
	int cnt = m_iCount;
	if ( cnt < 8 )
	{
		m_aszMessage[last] = message;
		m_iLastMessage = ( last + 1 ) % 8;
		m_iCount = cnt + 1;
	}
	return cnt < 8;
}

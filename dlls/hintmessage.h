#pragma once
// NOTE: no cbase.h here by design: cbase.h has NO include guard in this tree,
// and player.h -> hintmessage.h -> cbase.h would double-include it via hintmessage.cpp.
// CBaseEntity is only a reference parameter below, forward decl suffices.
class CBaseEntity;
class CHintMessageQueue
{
  public:
	int m_iCount;
	int m_iCurrentMessage;
	int m_iLastMessage;
	float m_tmMessageEnd;
	const char *m_aszMessage[8];

	void Reset();
	void Update( CBaseEntity &client );
	bool AddMessage( const char *message );
	bool IsEmpty();
};

/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#pragma once
#if !defined(UNISIGNALS_H)
#define UNISIGNALS_H

// The areas a player stands in (SIGNAL_* of player.h). The entities raise a signal while they
// touch the player, Update() makes the raised signals the new state once per frame and returns
// the bits that changed.
struct CUnifiedSignals
{
  public:
	CUnifiedSignals()
	{
		m_flSignal = 0;
		m_flState = 0;
	}

	int Update()
	{
		int old = m_flState;

		m_flState = m_flSignal;
		m_flSignal = 0;

		return old ^ m_flState;
	}

	void Signal( int flSignal )
	{
		m_flSignal |= flSignal;
	}

	int GetState()
	{
		return m_flState;
	}

  private:
	int m_flSignal;
	int m_flState;
};

#endif // UNISIGNALS_H

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
#if !defined(DOD_STATS_H)
#define DOD_STATS_H

struct DoDPlayerStats_t
{
	int stat[4];
	float starttime;
};
class CDoDStatMgr : public CBaseEntity
{
  public:
	CDoDStatMgr( void );
	void InitPlayer( int index );
	DoDPlayerStats_t *GetPlayer( int index );
	void PrintPlayerStats( int index );
	DoDPlayerStats_t m_PlayerStats[32];
};

#endif // DOD_STATS_H

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
#if !defined(MAPMARKER_H)
#define MAPMARKER_H

// dod_mapmarker
class CMapMarker : public CBaseEntity
{
  public:
	// pev->body = icon, pev->fuser1 = create time, angles fixed (0,90,0)
	static CMapMarker *CreateMapMarker( Vector o, Vector a, CBaseEntity *w, int icon );
	void Spawn( void );
	void EXPORT Die();
	void SetIcon( int icon ) { pev->body = icon; }
	int GetIcon( void );
};

#endif // MAPMARKER_H

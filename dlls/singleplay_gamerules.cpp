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
#include "weapons.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "skill.h"
#include "items.h"

CSPDoDRules::CSPDoDRules( void )
{
	CGameRules::RefreshSkillData();
}

void CSPDoDRules::Init( void )
{
	/* Nothing. */
}

void CSPDoDRules::Think( void )
{
	return;
}

BOOL CSPDoDRules::IsMultiplayer( void )
{
	return FALSE;
}

BOOL CSPDoDRules::IsDeathmatch( void )
{
	return FALSE;
}

BOOL CSPDoDRules::IsCoOp( void )
{
	return FALSE;
}

BOOL CSPDoDRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if( !pPlayer->m_pActiveItem )
		return TRUE;
	if( !pPlayer->m_pActiveItem->CanHolster() )
		return FALSE;
	return TRUE;
}

BOOL CSPDoDRules::GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{
	return FALSE;
}

BOOL CSPDoDRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *szRejectReason )
{
	return TRUE;
}

void CSPDoDRules::InitHUD( CBasePlayer *pPlayer )
{
	return;
}
void CSPDoDRules::ClientDisconnected( edict_t *pClient )
{
	return;
}

float CSPDoDRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	pPlayer->m_flFallVelocity -= 500.0f;
	return pPlayer->m_flFallVelocity * 100.0f / 400.0f;
}

void CSPDoDRules::PlayerSpawn( CBasePlayer *pPlayer )
{
	CBaseEntity *e = NULL;
	while( ( e = UTIL_FindEntityByClassname( e, "game_player_equip" ) ) != NULL )
		e->Touch( pPlayer );
	pPlayer->RemoveAllWeaponFlags();
	pPlayer->m_iClassSpeed = 600;
	pPlayer->SetSpeed( 600 );
}

BOOL CSPDoDRules::AllowAutoTargetCrosshair( void )
{
	return ( g_iSkillLevel == SKILL_EASY );
}

void CSPDoDRules::PlayerThink( CBasePlayer *pPlayer )
{
	return;
}

BOOL CSPDoDRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
}

float CSPDoDRules::FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return 0;
}

int CSPDoDRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}

void CSPDoDRules::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags )
{
	return;
}
void CSPDoDRules::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags )
{
	return;
}

void CSPDoDRules::PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	/* Nothing. */
}

float CSPDoDRules::FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	return 0;
}

float CSPDoDRules::FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	return 0;
}

Vector CSPDoDRules::VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}

int CSPDoDRules::WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	return 0;
}

BOOL CSPDoDRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
}

void CSPDoDRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
	/* Nothing. */
}

int CSPDoDRules::ItemShouldRespawn( CItem *pItem )
{
	return 0;
}

float CSPDoDRules::FlItemRespawnTime( CItem *pItem )
{
	return 0;
}

Vector CSPDoDRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

BOOL CSPDoDRules::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	return TRUE;
}

void CSPDoDRules::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
	/* Nothing. */
}

int CSPDoDRules::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	return 0;
}

float CSPDoDRules::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return 0;
}

Vector CSPDoDRules::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

float CSPDoDRules::FlHealthChargerRechargeTime( void )
{
	return 0;
}

int CSPDoDRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return 0;
}

int CSPDoDRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return 0;
}

int CSPDoDRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	return 0;
}

BOOL CSPDoDRules::FAllowMonsters( void )
{
	return TRUE;
}

int CSPDoDRules::RoundState( void ) { return 1; }
void CSPDoDRules::SetRoundState( int iState )
{
	/* Nothing. */
}
void CSPDoDRules::SetRestartTime( float fTime )
{
	/* Nothing. */
}
int CSPDoDRules::TeamScores( int iTeam ) { return 1; }
void CSPDoDRules::SetTeamScores( int iTeam, int iScore )
{
	/* Nothing. */
}
void CSPDoDRules::AddToTeamScores( int iTeam, int iScore )
{
	/* Nothing. */
}

unsigned short CSPDoDRules::usVoice( void ) { return 0; }
unsigned short CSPDoDRules::bodyDamage( void ) { return 0; }
unsigned short CSPDoDRules::roundRestartSound( void ) { return 0; }
unsigned short CSPDoDRules::gerVoice( void ) { return 0; }
unsigned short CSPDoDRules::Pain( void ) { return 0; }
unsigned short CSPDoDRules::Smoke( void ) { return 0; }
unsigned short CSPDoDRules::Prone( void ) { return 0; }
unsigned short CSPDoDRules::BloodSprite( void ) { return 0; }
unsigned short CSPDoDRules::BloodStream( void ) { return 0; }
unsigned short CSPDoDRules::BulletTracers( void ) { return 0; }
unsigned short CSPDoDRules::SparkShower( void ) { return 0; }
unsigned short CSPDoDRules::ScreenFades( void ) { return 0; }
unsigned short CSPDoDRules::BubbleTrails( void ) { return 0; }
unsigned short CSPDoDRules::Bubbles( void ) { return 0; }
unsigned short CSPDoDRules::Shards( void ) { return 0; }
unsigned short CSPDoDRules::Explosion( void ) { return 0; }
unsigned short CSPDoDRules::PopHelmet( void ) { return 0; }
unsigned short CSPDoDRules::RoundReset( void ) { return 0; }
unsigned short CSPDoDRules::StartTrail( void ) { return 0; }

void CSPDoDRules::SetCamera( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetusVoice( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetbodyDamage( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetroundRestartSound( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetgerVoice( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetPain( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetSmoke( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetProne( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetBloodSprite( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetBloodStream( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetBulletTracers( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetSparkShower( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetScreenFades( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetBubbleTrails( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetBubbles( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetShards( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetExplosion( short s )
{
	/* Nothing. */
}
int CSPDoDRules::ReturnGameRules( void ) { return 0; }
void CSPDoDRules::SetPopHelmet( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetRoundReset( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetRocketTrail( short s )
{
	/* Nothing. */
}
void CSPDoDRules::SetMapChangeFromTime( int iTime )
{
	/* Nothing. */
}
float CSPDoDRules::GetMaxWaveTime( int nTeam ) { return 0.0f; }
float CSPDoDRules::getWaveTime( int iTeam ) { return 0.0f; }
void CSPDoDRules::SendTeamScores( int iTeam )
{
	/* Nothing. */
}
void CSPDoDRules::VoteKick( CBasePlayer *pPlayer, int id, bool b )
{
	/* Nothing. */
}
void CSPDoDRules::setWaveTime( int iTeam, float fTime )
{
	/* Nothing. */
}

BOOL CSPDoDRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	int wid = pWeapon->m_iId;
	if( wid == 4 || wid == 10 )
		return FALSE;
	if( wid != 6 && wid != 8 && wid != 0x0c && wid != 0x0e && wid != 0x11 )
	{
		BOOL hasKnife = FALSE, hasSidearm = FALSE, hasPrimary = FALSE;
		for( int i = 0; i < 6; i++ )
		{
			for( CBasePlayerItem *it = pPlayer->m_rgpPlayerItems[i]; it; it = it->m_pNext )
			{
				int fl = CBasePlayerItem::ItemInfoArray[it->m_iId].iFlags;
				if( fl & 0x20 ) hasKnife = TRUE;

				if( fl & 0x40 ) hasSidearm = TRUE;

				if( fl & 0x80 ) hasPrimary = TRUE;

				if( wid == it->m_iId )
					return FALSE;
			}
		}

		int myfl = CBasePlayerItem::ItemInfoArray[wid].iFlags;
		if( ( myfl & 0x20 ) && hasKnife )
			return FALSE;
		if( ( myfl & 0x40 ) && hasSidearm )
			return FALSE;
		if( ( myfl & 0x80 ) && hasPrimary )
			return FALSE;
		return TRUE;
	}

	return FALSE;
}

void CSPDoDRules::setMapTime( void )
{
	/* Nothing. */
}

void CSPDoDRules::PlayerDeathThink( CBasePlayer *pPlayer )
{
	return;
}

// Single-player is vestigial in DoD; overrides return neutral values.
unsigned short CSPDoDRules::Camera( void ) { return 0; }

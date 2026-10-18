// DoD singleplay restore - dod/dlls/singleplay_gamerules.cpp
// CSPDoDRules: singleplay unused in DoD, mostly empty stubs.
// SDK reference: ~/rev/halflife/dlls/singleplay_gamerules.cpp (CHalfLifeRules)
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "skill.h"
#include "items.h"
// SDK-IDENTICAL: dlls/singleplay_gamerules.cpp CHalfLifeRules Think empty
// .text:00140fd0 void CSPDoDRules::Think() (dod/dlls/singleplay_gamerules.cpp:0)
void CSPDoDRules::Think( void )
{
	return;
}
// .text:00141100 void CSPDoDRules::PlayerKilled(CBasePlayer*, entvars_t*, entvars_t*, int) (dod/dlls/singleplay_gamerules.cpp:0)
void CSPDoDRules::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags )
{
	return;
}
// .text:00141110 void CSPDoDRules::DeathNotice(CBasePlayer*, entvars_t*, entvars_t*, int) (dod/dlls/singleplay_gamerules.cpp:0)
void CSPDoDRules::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags )
{
	return;
}
// .text:001415e0 void CSPDoDRules::PlayerDeathThink(CBasePlayer*) (dod/dlls/singleplay_gamerules.cpp:0)
void CSPDoDRules::PlayerDeathThink( CBasePlayer *pPlayer )
{
	return;
}
// .text:001410c0 void CSPDoDRules::PlayerThink(CBasePlayer*) (dod/dlls/singleplay_gamerules.cpp:0)
void CSPDoDRules::PlayerThink( CBasePlayer *pPlayer )
{
	return;
}
// .text:00141600 void CSPDoDRules::PlayerSpawn(CBasePlayer*) (dod/dlls/singleplay_gamerules.cpp:0)
void CSPDoDRules::PlayerSpawn( CBasePlayer *pPlayer )
{
	CBaseEntity *e = NULL;
	while ( ( e = UTIL_FindEntityByClassname( e, "game_player_equip" ) ) != NULL )
		e->Touch( pPlayer );
	pPlayer->RemoveAllWeaponFlags();
	pPlayer->m_iClassSpeed = 600;
	pPlayer->SetSpeed( 600 );
}
// .text:00141780 void CSPDoDRules::CSPDoDRules() (dod/dlls/singleplay_gamerules.cpp:0)
CSPDoDRules::CSPDoDRules( void )
{
	CGameRules::RefreshSkillData();
}
// .text:00141050 BOOL CSPDoDRules::ClientConnected(edict_t*, char*, char*, char*) (dod/dlls/singleplay_gamerules.cpp:0)
BOOL CSPDoDRules::ClientConnected( edict_t *pEntity, char *pszName, char *pszAddress, char *szRejectReason )
{
	return TRUE;
}
// .text:00141060 void CSPDoDRules::InitHUD(CBasePlayer*) (dod/dlls/singleplay_gamerules.cpp:0)
void CSPDoDRules::InitHUD( CBasePlayer *pPlayer )
{
	return;
}
// .text:00141070 void CSPDoDRules::ClientDisconnected(edict_t*) (dod/dlls/singleplay_gamerules.cpp:0)
void CSPDoDRules::ClientDisconnected( edict_t *pClient )
{
	return;
}

// .text:00141010 BOOL CSPDoDRules::FShouldSwitchWeapon(CBasePlayer*,CBasePlayerItem*) (dod/dlls/singleplay_gamerules.cpp:80)
BOOL CSPDoDRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pPlayer->m_pActiveItem )
		return TRUE;
	if ( !pPlayer->m_pActiveItem->CanHolster() )
		return FALSE;
	return TRUE;
}
// .text:00141080 float CSPDoDRules::FlPlayerFallDamage(CBasePlayer*) (dod/dlls/singleplay_gamerules.cpp:122)
float CSPDoDRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	pPlayer->m_flFallVelocity -= 500.0f;
	return pPlayer->m_flFallVelocity * 100.0f / 400.0f;
}
// .text:00141150 Vector CSPDoDRules::VecWeaponRespawnSpot(CBasePlayerItem*) (dod/dlls/singleplay_gamerules.cpp:232)
Vector CSPDoDRules::VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}
// .text:001411c0 Vector CSPDoDRules::VecItemRespawnSpot(CItem*) (dod/dlls/singleplay_gamerules.cpp:279)
Vector CSPDoDRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}
// .text:00141220 Vector CSPDoDRules::VecAmmoRespawnSpot(CBasePlayerAmmo*) (dod/dlls/singleplay_gamerules.cpp:313)
Vector CSPDoDRules::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}
// .text:001410b0 BOOL CSPDoDRules::AllowAutoTargetCrosshair() (dod/dlls/singleplay_gamerules.cpp:152)
BOOL CSPDoDRules::AllowAutoTargetCrosshair( void )
{
	return ( g_iSkillLevel == SKILL_EASY );
}
// .text:00141660 BOOL CSPDoDRules::CanHavePlayerItem(CBasePlayer*,CBasePlayerItem*) (dod/dlls/singleplay_gamerules.cpp:572)
BOOL CSPDoDRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	int wid = pWeapon->m_iId;
	if ( wid == 4 || wid == 10 )
		return FALSE;
	if ( wid != 6 && wid != 8 && wid != 0x0c && wid != 0x0e && wid != 0x11 )
	{
		BOOL hasKnife = FALSE, hasSidearm = FALSE, hasPrimary = FALSE;
		for ( int i = 0; i < 6; i++ )
		{
			for ( CBasePlayerItem *it = pPlayer->m_rgpPlayerItems[i]; it; it = it->m_pNext )
			{
				int fl = CBasePlayerItem::ItemInfoArray[it->m_iId].iFlags;
				if ( fl & 0x20 ) hasKnife = TRUE;
				if ( fl & 0x40 ) hasSidearm = TRUE;
				if ( fl & 0x80 ) hasPrimary = TRUE;
				if ( wid == it->m_iId )
					return FALSE;
			}
		}
		int myfl = CBasePlayerItem::ItemInfoArray[wid].iFlags;
		if ( ( myfl & 0x20 ) && hasKnife )
			return FALSE;
		if ( ( myfl & 0x40 ) && hasSidearm )
			return FALSE;
		if ( ( myfl & 0x80 ) && hasPrimary )
			return FALSE;
		return TRUE;
	}
	return FALSE;
}

// DoD event-id accessors on the single-player rules object (dod.so CSPDoDRules::*).
// Single-player is vestigial in DoD; overrides return neutral values.
unsigned short CSPDoDRules::Camera( void ) { return 0; }
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
void CSPDoDRules::SetCamera( short s ) {}
void CSPDoDRules::SetusVoice( short s ) {}
void CSPDoDRules::SetbodyDamage( short s ) {}
void CSPDoDRules::SetroundRestartSound( short s ) {}
void CSPDoDRules::SetgerVoice( short s ) {}
void CSPDoDRules::SetPain( short s ) {}
void CSPDoDRules::SetSmoke( short s ) {}
void CSPDoDRules::SetProne( short s ) {}
void CSPDoDRules::SetBloodSprite( short s ) {}
void CSPDoDRules::SetBloodStream( short s ) {}
void CSPDoDRules::SetBulletTracers( short s ) {}
void CSPDoDRules::SetSparkShower( short s ) {}
void CSPDoDRules::SetScreenFades( short s ) {}
void CSPDoDRules::SetBubbleTrails( short s ) {}
void CSPDoDRules::SetBubbles( short s ) {}
void CSPDoDRules::SetShards( short s ) {}
void CSPDoDRules::SetExplosion( short s ) {}
void CSPDoDRules::SetPopHelmet( short s ) {}
void CSPDoDRules::SetRoundReset( short s ) {}
void CSPDoDRules::SetRocketTrail( short s ) {}
int CSPDoDRules::RoundState( void ) { return 1; }
int CSPDoDRules::TeamScores( int iTeam ) { return 1; }
float CSPDoDRules::getWaveTime( int iTeam ) { return 0.0f; }
void CSPDoDRules::setWaveTime( int iTeam, float fTime ) {}
float CSPDoDRules::GetMaxWaveTime( int nTeam ) { return 0.0f; }
void CSPDoDRules::setMapTime( void ) {}
void CSPDoDRules::SetRoundState( int iState ) {}
void CSPDoDRules::SetTeamScores( int iTeam, int iScore ) {}
void CSPDoDRules::SendTeamScores( int iTeam ) {}
void CSPDoDRules::SetRestartTime( float fTime ) {}
void CSPDoDRules::AddToTeamScores( int iTeam, int iScore ) {}
void CSPDoDRules::SetMapChangeFromTime( int iTime ) {}
int CSPDoDRules::ReturnGameRules( void ) { return 0; }
void CSPDoDRules::Init( void ) {}
void CSPDoDRules::VoteKick( CBasePlayer *pPlayer, int id, bool b ) {}

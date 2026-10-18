// Mk2 HandGrenade - Allied frag (CHandGrenade)
// PARAMS: dmg 180 blast | carry 2? | fuse 2.0s? | throw 800 speed | snd grenthrow g_bounce | mdl p_grenade v_grenade w_grenade | ammo ammo_agrens | id 13? slot 4
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "skill.h"

LINK_ENTITY_TO_CLASS( weapon_handgrenade, CHandGrenade );
// .text:00166b40 weapon_handgrenade (wpn_shared/handgrenade.cpp:16) link 00166b40 Ghidra 00176b40
// .text:00166670 CHandGrenade::Spawn (wpn_shared/handgrenade.cpp:20) link 00166670
void CHandGrenade::Spawn()
{
	Precache();
	m_iId = 13;
	SET_MODEL( ENT( pev ), P_WpnInfo[13].wmodel );
	pev->dmg = gSkillData.plrDmgHandGrenade;
	m_iDefaultAmmo = 1;
	FallInit();
}
// .text:00165bd0 CHandGrenade::Precache (wpn_shared/handgrenade.cpp:35) link 00165bd0
void CHandGrenade::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[13].vmodel );
    PRECACHE_MODEL( P_WpnInfo[13].wmodel );
    PRECACHE_MODEL( P_WpnInfo[36].vmodel );
    PRECACHE_MODEL( P_WpnInfo[36].wmodel );

    PRECACHE_SOUND( "weapons/grenthrow.wav" );
}
// .text:00165c20 CHandGrenade::GetItemInfo (wpn_shared/handgrenade.cpp:46) link 00165c20
int CHandGrenade::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_agrens";
	p->iMaxAmmo1 = 3;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = -1;
	p->iSlot = 4;
	p->iPosition = 1;
	p->iFlags = 0x18;
	p->iId = m_iId = 13;
	p->iWeight = 1;
	return 1;
}
// .text:00166560 CHandGrenade::Deploy (wpn_shared/handgrenade.cpp:63) link 00166560
BOOL CHandGrenade::Deploy()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	m_flReleaseThrow = -1.0;
	SendWeaponAnim( 1, UseDecrement() ? 1 : 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.85;
	if ( g_pGameRules != NULL && ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesBrit() )
		return DefaultDeploy( P_WpnInfo[36].vmodel, P_WpnInfo[36].pmodel, 1, P_WpnInfo[36].szAnimExt,
							  P_WpnInfo[36].szAnimReloadExt, 0 );
	return DefaultDeploy( P_WpnInfo[13].vmodel, P_WpnInfo[13].pmodel, 1, P_WpnInfo[13].szAnimExt,
						  P_WpnInfo[13].szAnimReloadExt, 0 );
}
// .text:00165ca0 CHandGrenade::CanHolster (wpn_shared/handgrenade.cpp:84) link 00165ca0
int CHandGrenade::CanHolster()
{
	return m_flStartThrow == 0.0;
}
// .text:001666d0 CHandGrenade::DropGren (wpn_shared/handgrenade.cpp:88) link 001666d0 - drops live gren if pin pulled, dmg gSkillData.plrDmgHandGrenade
void CHandGrenade::DropGren()
{
	if ( m_flStartThrow == 0.0 )
		return;
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0 )
		return;

	SendWeaponAnim( 4, UseDecrement() ? 1 : 0 );
	if ( m_bUnderhand )
		m_pPlayer->SetAnimation( PLAYER_ROLLGRENADE );
	else
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector eyes = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
	Vector vecStart = eyes + gpGlobals->v_forward * 16.0f;
	TraceResult tr;
	UTIL_TraceLine( eyes, vecStart, ignore_monsters, m_pPlayer->pev->pContainingEntity, &tr );
	if ( tr.flFraction < 1.0f )
		vecStart = tr.vecEndPos;

	int grenType = ( g_pGameRules != NULL && ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesBrit() ) ? 0 : 1;
	CGrenade::ShootPickup( m_pPlayer->pev, vecStart, gpGlobals->v_forward, 5.0f, grenType, 0.8f );

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_flStartThrow = 0.0;
	float t = UTIL_WeaponTimeBase() + 0.5;
	m_flNextPrimaryAttack = t;
	m_flNextSecondaryAttack = t;
	m_flTimeWeaponIdle = t;
}
// .text:00166470 CHandGrenade::Holster (wpn_shared/handgrenade.cpp:135) link 00166470
void CHandGrenade::Holster( int i )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		SendWeaponAnim( 3, UseDecrement() ? 1 : 0 );
	}
	else
	{
		SetThink( &CBasePlayerItem::DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM, 0, 100 );
}
// .text:00166a80 CHandGrenade::PrimaryAttack (wpn_shared/handgrenade.cpp:153) link 00166a80
void CHandGrenade::PrimaryAttack()
{
	StartThrow( 0 );
}
// .text:001669c0 CHandGrenade::SecondaryAttack (wpn_shared/handgrenade.cpp:158) link 001669c0
void CHandGrenade::SecondaryAttack()
{
	StartThrow( 1 );
}
// .text:00166bb0 CHandGrenade::StartThrow (wpn_shared/handgrenade.cpp:163) link 00166bb0 - pin pull, throw on release via WeaponIdle timing
void CHandGrenade::StartThrow( int longThrow )
{
	if ( m_flStartThrow != 0.0 )
		return;
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	m_flReleaseThrow = 0.0;
	m_flStartThrow = 1.0;
	m_bUnderhand = longThrow;
	SendWeaponAnim( 2, UseDecrement() ? 1 : 0 );
	m_flTimeWeaponIdle = gpGlobals->time + 0.1;
}
// .text:00165ce0 CHandGrenade::WeaponIdle (wpn_shared/handgrenade.cpp:182) link 00165ce0 - 11KB, releases throw after 0.5s, velocity 800/500
void CHandGrenade::WeaponIdle()
{
	float time = gpGlobals->time;
	if ( m_flReleaseThrow == 0.0f )
		m_flReleaseThrow = time;

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
	{
		if ( time < m_flTimeWeaponIdle )
			return;

		float flStart = m_flStartThrow;
		if ( time <= m_flTimeWeaponIdle || flStart != 1.0f )
		{
			if ( flStart == 0.0f && m_flReleaseThrow < 0.0f )
			{
				m_flTimeWeaponIdle = time + RANDOM_FLOAT( 10, 15 );
				SendWeaponAnim( 0, UseDecrement() ? 1 : 0 );
				return;
			}
		}
		else if ( m_flReleaseThrow != 0.0f )
		{
			m_flStartThrow = time + 0.5f;
			if ( m_bUnderhand )
				m_pPlayer->SetAnimation( PLAYER_ROLLGRENADE );
			else
				m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
			if ( angThrow.x >= 0.0f )
				angThrow.x = angThrow.x * 1.11f - 10.0f;
			else
				angThrow.x = angThrow.x * 0.89f - 10.0f;
			flVel = ( 90.0f - angThrow.x ) * 4.0f;
			if ( flVel > 600.0f )
				flVel = 600.0f;
			UTIL_MakeVectors( angThrow );
			float flPlayerSpeedBonus = DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );
			vecThrow = gpGlobals->v_forward * ( flPlayerSpeedBonus + flVel );
			return;
		}

		if ( flStart < time && flStart != 0.0f && flStart != 1.0f )
		{
			angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
			if ( angThrow.x < 0.0f )
				angThrow.x = angThrow.x * 0.89f - 10.0f;
			else
				angThrow.x = angThrow.x * 1.11f - 10.0f;
			UTIL_MakeVectors( angThrow );

			Vector eyes = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
			vecSrc = eyes + gpGlobals->v_forward * 16.0f;
			float friction = 0.8f;
			if ( m_bUnderhand )
			{
				friction = 0.6f;
				vecSrc = eyes + gpGlobals->v_forward * 16.0f;
				vecThrow = gpGlobals->v_forward * 300.0f;
			}

			TraceResult tr;
			UTIL_TraceLine( eyes, vecSrc, ignore_monsters, m_pPlayer->pev->pContainingEntity, &tr );
			if ( tr.flFraction < 1.0f )
				vecSrc = tr.vecEndPos;

			int grenType = ( g_pGameRules != NULL && ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesBrit() ) ? 0 : 1;
			CGrenade::ShootPickup( m_pPlayer->pev, vecSrc, vecThrow, 5.0f, grenType, friction );
			EMIT_SOUND_DYN( m_pPlayer->pev->pContainingEntity, CHAN_WEAPON, "weapons/grenthrow.wav", 0.8f, 0.8f, 0, 100 );
			SendWeaponAnim( 4, UseDecrement() ? 1 : 0 );

			m_flStartThrow = 0.0f;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0f;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
			return;
		}
	}

	if ( m_flReleaseThrow > 0.0f && m_flStartThrow == 0.0f && m_flTimeWeaponIdle < gpGlobals->time )
	{
		m_flStartThrow = 0.0f;
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 1 )
		{
			RetireWeapon();
		}
		else
		{
			SendWeaponAnim( 1, UseDecrement() ? 1 : 0 );
			m_flReleaseThrow = -1.0f;
			m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT( 10, 15 );
		}
	}
}
// .text:00165cc0 CHandGrenade::CanDeploy (wpn_shared/handgrenade.cpp:319) link 00165cc0
int CHandGrenade::CanDeploy()
{
	return ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0 );
}

// HandGrenadeEx - primed/frag variant (CHandGrenadeEx) allied
// PARAMS: dmg 180 | fuse set via SetDmgTime | ammo ammo_agrensex | snd same as grenade | mdl same grenade
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "skill.h"

LINK_ENTITY_TO_CLASS( weapon_handgrenade_ex, CHandGrenadeEx );
// .text:001677c0 weapon_handgrenade_ex (wpn_shared/handgrenade_ex.cpp:19) link 001677c0 Ghidra 001777c0
// .text:00167150 CHandGrenadeEx::Spawn (wpn_shared/handgrenade_ex.cpp:23) link 00167150
void CHandGrenadeEx::Spawn()
{
	Precache();
	m_iId = 16;
	SET_MODEL( ENT( pev ), P_WpnInfo[16].wmodel );
	pev->dmg = gSkillData.plrDmgStickGrenade;
	m_iDefaultAmmo = 1;
	FallInit();
}
// .text:00166cc0 CHandGrenadeEx::Precache (wpn_shared/handgrenade_ex.cpp:40) link 00166cc0
void CHandGrenadeEx::Precache( void )
{
    PRECACHE_SOUND( "weapons/grenthrow.wav" );
}
// .text:00166cd0 CHandGrenadeEx::GetItemInfo (wpn_shared/handgrenade_ex.cpp:44) link 00166cd0
int CHandGrenadeEx::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_agrensex";
	p->iMaxAmmo1 = 1;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = -1;
	p->iSlot = 4;
	p->iPosition = 3;
	p->iFlags = 0x18;
	p->iId = m_iId = 16;
	p->iWeight = 100;
	return 1;
}
// .text:00167040 CHandGrenadeEx::Deploy (wpn_shared/handgrenade_ex.cpp:62) link 00167040
BOOL CHandGrenadeEx::Deploy()
{
	UTIL_SendDirectorMessage( m_pPlayer, this, 0x26 );
	m_pPlayer->m_iFOV = GetZoomFOV();
	m_flReleaseThrow = -1.0;
	m_flStartThrow = 0.0;
	if ( g_pGameRules != NULL && ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesBrit() )
	{
		pev->iuser1 = 0;
		return DefaultDeploy( P_WpnInfo[36].vmodel, P_WpnInfo[36].pmodel, 6, P_WpnInfo[36].szAnimExt,
							  P_WpnInfo[36].szAnimReloadExt, 0 );
	}
	pev->iuser1 = 1;
	return DefaultDeploy( P_WpnInfo[16].vmodel, P_WpnInfo[16].pmodel, 6, P_WpnInfo[16].szAnimExt,
						  P_WpnInfo[16].szAnimReloadExt, 0 );
}
// .text:00166d50 CHandGrenadeEx::CanHolster (wpn_shared/handgrenade_ex.cpp:91) link 00166d50
int CHandGrenadeEx::CanHolster()
{
	return ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0 );
}
// .text:00166d70 CHandGrenadeEx::Holster (wpn_shared/handgrenade_ex.cpp:103) link 00166d70
void CHandGrenadeEx::Holster( int i )
{
	return;
}
// .text:001676c0 CHandGrenadeEx::PrimaryAttack (wpn_shared/handgrenade_ex.cpp:106) link 001676c0
void CHandGrenadeEx::PrimaryAttack()
{
	StartThrow( 0 );
}
// .text:001675c0 CHandGrenadeEx::SecondaryAttack (wpn_shared/handgrenade_ex.cpp:111) link 001675c0
void CHandGrenadeEx::SecondaryAttack()
{
	StartThrow( 1 );
}
// .text:00167830 CHandGrenadeEx::StartThrow (wpn_shared/handgrenade_ex.cpp:116) link 00167830
void CHandGrenadeEx::StartThrow( int t )
{
	if ( m_flTimeToExplode < gpGlobals->time )
		DropGren();

	if ( m_flStartThrow != 0.0 )
		return;
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	m_flReleaseThrow = 0.0;
	m_flStartThrow = 1.0;
	m_bUnderhand = t;
	SendWeaponAnim( 7, UseDecrement() ? 1 : 0 );
	if ( m_flTimeToExplode >= gpGlobals->time )
		m_flReleaseThrow = gpGlobals->time + 0.01f;
}
// .text:001671b0 CHandGrenadeEx::DropGren (wpn_shared/handgrenade_ex.cpp:135) link 001671b0 - 7.5KB, drops with remaining fuse
void CHandGrenadeEx::DropGren()
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	SendWeaponAnim( 8, UseDecrement() ? 1 : 0 );

	Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	if ( angThrow.x < 0.0f )
		angThrow.x = angThrow.x * 0.89f - 10.0f;
	else
		angThrow.x = angThrow.x * 1.11f - 10.0f;

	float flVel = ( 90.0f - angThrow.x ) * 4.0f;
	if ( flVel > 600.0f )
		flVel = 600.0f;

	UTIL_MakeVectors( angThrow );

	Vector eyes = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
	Vector vecSrc = eyes + gpGlobals->v_forward * 16.0f;
	float time = m_flTimeToExplode - gpGlobals->time;
	if ( time < 0.0f )
		time = 0.0f;

	float friction;
	Vector vecThrow;
	if ( m_bUnderhand )
	{
		friction = 0.6f;
		vecSrc = eyes + gpGlobals->v_forward * 16.0f;
		vecThrow = gpGlobals->v_forward * 300.0f;
	}
	else
	{
		friction = 0.8f;
		float flPlayerSpeedBonus = DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );
		vecThrow = gpGlobals->v_forward * ( flPlayerSpeedBonus + flVel );
	}

	TraceResult tr;
	UTIL_TraceLine( eyes, vecSrc, ignore_monsters, m_pPlayer->pev->pContainingEntity, &tr );
	if ( tr.flFraction < 1.0f )
		vecSrc = tr.vecEndPos;

	if ( pev->iuser1 )
		CGrenade::ShootPickup( m_pPlayer->pev, vecSrc, vecThrow, time, 1, friction );
	else
		CGrenade::ShootPickup( m_pPlayer->pev, vecSrc, vecThrow, time, 0, friction );

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_flStartThrow = 0.0f;
}
// .text:00166d80 CHandGrenadeEx::SetDmgTime (wpn_shared/handgrenade_ex.cpp:192) link 00166d80
void CHandGrenadeEx::SetDmgTime( float t )
{
	m_flTimeToExplode = t;
}
// .text:00166db0 CHandGrenadeEx::WeaponIdle (wpn_shared/handgrenade_ex.cpp:197) link 00166db0 - 5KB, timed throw, cooks grenade
void CHandGrenadeEx::WeaponIdle()
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		RetireWeapon();
		SetThink( &CBasePlayerItem::DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
		m_flStartThrow = 0.0f;
		return;
	}

	if ( m_flTimeToExplode < gpGlobals->time )
		DropGren();

	if ( m_flTimeWeaponIdle < gpGlobals->time )
	{
		if ( m_flReleaseThrow == -1.0f )
		{
			m_flTimeWeaponIdle = gpGlobals->time + 1.0f;
			SendWeaponAnim( 5, UseDecrement() ? 1 : 0 );
		}
	}

	if ( m_flReleaseThrow <= gpGlobals->time )
	{
		if ( m_flStartThrow == 1.0f )
		{
			if ( m_flReleaseThrow != -1.0f && m_flTimeToExplode >= gpGlobals->time )
			{
				if ( m_bUnderhand )
					m_pPlayer->SetAnimation( PLAYER_ROLLGRENADE );
				else
					m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

				m_flStartThrow = 2.0f;
				m_flReleaseThrow = gpGlobals->time + 0.3f;
			}
		}
		else if ( m_flStartThrow == 2.0f && m_flReleaseThrow != -1.0f && m_flTimeToExplode >= gpGlobals->time )
		{
			DropGren();
		}
	}
}
// .text:00166d90 CHandGrenadeEx::CanDeploy (wpn_shared/handgrenade_ex.cpp:239) link 00166d90
int CHandGrenadeEx::CanDeploy()
{
	return ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0 );
}

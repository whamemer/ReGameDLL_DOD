// CBipodWeapon - base for BAR/30cal/Bren/MG42/MG34/FG42 (wpn_shared/bipod.cpp)
// PARAMS: uses P_WpnInfo[id] per weapon | deployed spread base_accuracy2 else base | overheat CoolThink | deploy times up2down/down2up | recoil punchangle
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

// .text:00172680 CBipodWeapon::Spawn (wpn_shared/bipod.cpp:13) link 00162680 Ghidra 00172680
void CBipodWeapon::Spawn( int id )
{
	Precache();
	m_iId = id;
	SET_MODEL( ENT( pev ), P_WpnInfo[id].wmodel );
	m_iDefaultAmmo = P_WpnInfo[id].ammo_default;
	FallInit();
}
// .text:00171b40 CBipodWeapon::Precache link 00161a40 size 0x2a Ghidra 00171a40: overheat event exact (158B bogus = Deploy 0x9e)
void CBipodWeapon::Precache()
{
	m_iOverheatEvent = PRECACHE_EVENT( 1, "events/effects/overheat.sc" );
}
// .text:00171be0 CBipodWeapon::AddToPlayer (wpn_shared/bipod.cpp:27) link 00161be0
int CBipodWeapon::AddToPlayer( CBasePlayer *p )
{
	return CBasePlayerWeapon::AddToPlayer( p ) != 0;
}
// .text:00171b40? CBipodWeapon::Deploy (wpn_shared/bipod.cpp:36) link 00161b40
BOOL CBipodWeapon::Deploy()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	return DefaultDeploy( P_WpnInfo[m_iId].vmodel, P_WpnInfo[m_iId].pmodel, GetDrawAnim(), P_WpnInfo[m_iId].szAnimExt,
						  P_WpnInfo[m_iId].szAnimReloadExt, 0 );
}
// .text:00171a70 CBipodWeapon::CanHolster (wpn_shared/bipod.cpp:45) link 00161a70
int CBipodWeapon::CanHolster()
{
	return !IsDeployed();
}
// .text:00171b30 CBipodWeapon::Holster (wpn_shared/bipod.cpp:54) link 00161b30
void CBipodWeapon::Holster( int skiplocal )
{
	ForceUndeploy();
	CBasePlayerWeapon::Holster( skiplocal );
}
// .text:00171f30 CBipodWeapon::PrimaryAttack (wpn_shared/bipod.cpp:58) link 00161f30 - 12KB decompile, core below
void CBipodWeapon::PrimaryAttack()
{
	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
		return;
	}

	if ( m_iClip < 1 )
	{
		if ( !m_fInAttack )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
			m_fInAttack = 1;
		}
		return;
	}

	ItemInfo bipod;
	GetItemInfo( &bipod );

	if ( ( bipod.iFlags & ITEM_FLAG_HEAT ) != 0 )
	{
		if ( m_fInAttack )
			return;
		if ( m_flWeaponHeat >= 99.0f )
		{
			m_fInAttack = 1;
			return;
		}
		if ( m_flWeaponHeat >= 95.0f )
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_iOverheatEvent, 0, g_vecZero, g_vecZero, 0, 0, 0,
								 0, 0, 0 );
	}

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->m_iWeaponVolume = 600;

	float flSpread = P_WpnInfo[m_iId].base_accuracy;
	if ( IsDeployed() )
		flSpread = P_WpnInfo[m_iId].base_accuracy2;
	else if ( m_pPlayer->pev->velocity.Length() > 45.0f )
		flSpread += P_WpnInfo[m_iId].accuracy_penalty;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, gpGlobals->v_forward, flSpread, 8192, bipod.iBulletId, 3, 0,
											  m_pPlayer->pev, m_pPlayer->random_seed );
	m_iClip -= 1;

	if ( m_iId == WEAPON_MG42 )
	{
		m_iClip -= 1;
		Vector vecDir2 = m_pPlayer->FireBulletsNC( vecSrc, gpGlobals->v_forward, flSpread, 8192, bipod.iBulletId, 3, 0,
												   m_pPlayer->pev, m_pPlayer->random_seed + 5 );
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_iFireEvent, 0, g_vecZero, g_vecZero, vecDir2.x,
							 vecDir2.y, m_iClip, 0, 1, 0 );
	}

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_iFireEvent, 0, g_vecZero, g_vecZero, vecDir.x, vecDir.y,
						 m_iClip, 0, 0, 0 );

	if ( ( bipod.iFlags & ITEM_FLAG_HEAT ) != 0 )
		m_flWeaponHeat += 3.0f;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[m_iId].anim_firedelay;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[m_iId].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + P_WpnInfo[m_iId].anim_firedelay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );

	if ( !IsDeployed() )
		RemoveStamina( 2.0f, m_pPlayer );

	if ( ( bipod.iFlags & ITEM_FLAG_HEAT ) != 0 )
	{
		SetThink( &CBipodWeapon::CoolThink );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}
// .text:00171c10 CBipodWeapon::CoolThink (wpn_shared/bipod.cpp:194) link 00161c10
void CBipodWeapon::CoolThink()
{
	m_flWeaponHeat -= 0.6;
	if ( m_flWeaponHeat > 0 )
	{
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}
	m_flWeaponHeat = 0;
	SetThink( NULL );
}
// .text:00171d20 CBipodWeapon::SecondaryAttack (wpn_shared/bipod.cpp:207) link 00161d20 - toggles bipod deploy/undeploy, plays up/down anim, sets FOV, blocks while proned/sprinting
void CBipodWeapon::SecondaryAttack()
{
	if ( IsDeployed() )
		ForceUndeploy();
	else
		DeployBipod();
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
	m_pPlayer->m_flNextAttack += 0.5;
}
// .text:001726f0 CBipodWeapon::ForceUndeploy (wpn_shared/bipod.cpp:284) link 001626f0
void CBipodWeapon::ForceUndeploy()
{
	m_iWeaponState &= ~2;
}
// .text:00171a90 CBipodWeapon::Reload (wpn_shared/bipod.cpp:290) link 00161a90
void CBipodWeapon::Reload()
{
	float flReloadTime;

	if ( m_iId == WEAPON_BREN && IsDeployed() )
		flReloadTime = P_WpnInfo[m_iId].anim_reloadtime2;
	else
		flReloadTime = P_WpnInfo[m_iId].anim_reloadtime;

	DefaultReload( P_WpnInfo[m_iId].ammo_maxclip, GetReloadAnim(), flReloadTime );
}
// .text:00171c80 CBipodWeapon::WeaponIdle (wpn_shared/bipod.cpp:302) link 00161c80
void CBipodWeapon::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
	if ( m_flNextPrimaryAttack > UTIL_WeaponTimeBase() )
		return;
	if ( m_flNextSecondaryAttack > UTIL_WeaponTimeBase() )
		return;
	int iAnim = GetIdleAnim();
	if ( iAnim >= 0 )
		SendWeaponAnim( iAnim, 1 );
}
// .text:00172730 CBipodWeapon::IsDeployed (wpn_shared/bipod.cpp:322) link 00162730
bool CBipodWeapon::IsDeployed()
{
	return ( m_iWeaponState & 2 ) ? 1 : 0;
}
// .text:00172750 CBipodWeapon::GetBipodSpread (wpn_shared/bipod.cpp:326) link 00162750
float CBipodWeapon::GetBipodSpread()
{
	if ( IsDeployed() )
		return P_WpnInfo[m_iId].base_accuracy2;
	return P_WpnInfo[m_iId].base_accuracy;
}
// NOTE: virtuals GetDrawAnim/GetIdleAnim/GetReloadAnim/GetUpToDownAnim/GetDownToUpAnim overridden per weapon (BAR/30cal/Bren above), UseDecrement/iItemSlot/CanDrop weak defaults skipped as junk

// MP40 - Axis SMG full-auto (dod/dlls/wpn_shared/mp40.cpp)
// | Param | Value |
// | Name | weapon_mp40 / CMP40 / id 12 (0xc) |
// | Damage | 40 (plrDmgMP40 via BULLET_DOD_MP40 (15)) |
// | Mag | 30 / carry 210 / default 210 |
// | FireDelay | 0.09 | Reload | 2.85 | Draw | 1.0 |
// | Spread | 0.025 +0.10 move | Ani | mp40 |
// | Models | p_mp40 / v_mp40 / w_mp40 |
// | Ammo | ammo_44mm | Slot | 2/0 flags 0x80 weight 20 |
// | Sound | weapons/mp40_shoot.wav | Event | events/weapons/mp40.sc |
// | Secondary | empty (return) - no butt |
// preserved .text markers from server:
// .text:0017cac0 weapon_mp40 (wpn_shared/mp40.cpp:27)
// .text:0017cb30 ammo_mp40 (wpn_shared/mp40.cpp:191)
// .text:0017c4d0 CMP40::Precache (wpn_shared/mp40.cpp:43)
// .text:0017c6d0 CMP40::Spawn (wpn_shared/mp40.cpp:33)
// .text:0017c540 CMP40::GetItemInfo (wpn_shared/mp40.cpp:55)
// .text:0017c7a0 CMP40::PrimaryAttack (wpn_shared/mp40.cpp:95)
// .text:0017c5d0 CMP40::SecondaryAttack (wpn_shared/mp40.cpp:149)
// .text:0017c620 CMP40::Deploy (wpn_shared/mp40.cpp:95)
// .text:0017c5f0 CMP40::Reload (wpn_shared/mp40.cpp:149)
// MP40_e moved to dod_shared.h (identical 1:1) — local duplicate removed.

//
// mp40.cpp
//
// implementation of CMP40 class
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS( weapon_mp40, CMP40 )

void CMP40::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[12].wmodel );
	m_iId = 12;
	m_iDefaultAmmo = P_WpnInfo[12].ammo_default;
	FallInit();
}

void CMP40::Precache( void )
{
	PRECACHE_MODEL( P_WpnInfo[12].vmodel );
	PRECACHE_MODEL( P_WpnInfo[12].wmodel );

	PRECACHE_SOUND( "weapons/weaponempty.wav" );
	PRECACHE_SOUND( "weapons/mp44_shoot.wav" );
	PRECACHE_SOUND( "weapons/mp40_shoot.wav" );

	m_usFireMP40 = PRECACHE_EVENT( 1, "events/weapons/mp40.sc" );
}

int CMP40::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_44mm";
	p->iMaxAmmo1 = P_WpnInfo[12].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[12].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 12;
	p->iWeight = P_WpnInfo[12].misc_weight;
	return 1;
}

int CMP40::AddToPlayer( CBasePlayer *pPlayer )
{
	return CBasePlayerWeapon::AddToPlayer( pPlayer ) != 0;
}

BOOL CMP40::Deploy( void )
{
	ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[12].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[12].vmodel, P_WpnInfo[12].pmodel, MP40_DRAW, P_WpnInfo[12].szAnimExt,
						  P_WpnInfo[12].szAnimReloadExt, 0 );
}

BOOL CMP40::CanHolster( void )
{
	return TRUE;
}

void CMP40::PrimaryAttack( void )
{
	float flSpread;

	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
	}
	else if ( m_iClip <= 0 )
	{
		if ( !m_fInAttack )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
			m_fInAttack = TRUE;
		}
	}
	else
	{
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		--m_iClip;

		flSpread = P_WpnInfo[12].base_accuracy;

		if ( m_pPlayer->pev->velocity.Length() > 45.0f )
			flSpread += P_WpnInfo[12].accuracy_penalty;
		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, ( Vector ) gpGlobals->v_forward, flSpread, 8192, BULLET_DOD_MP40, 3, 0, m_pPlayer->pev,
												  m_pPlayer->random_seed );

		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireMP40, 0.0f, g_vecZero, g_vecZero, vecDir.x,
							 vecDir.y, 0, 0, m_iClip == 0, 0 );

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[12].anim_firedelay;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1f;
		m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0f, 15.0f ) + UTIL_WeaponTimeBase();
		RemoveStamina( 2.0f, m_pPlayer );
	}
}

void CMP40::SecondaryAttack( void )
{
	// Nothing.
}

void CMP40::Reload( void )
{
	DefaultReload( P_WpnInfo[12].ammo_maxclip, MP40_RELOAD, P_WpnInfo[12].anim_reloadtime );
}

void CMP40::WeaponIdle( void )
{
	ResetEmptySound();

	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( MP40_IDLE );
		m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0f, 15.0f ) + UTIL_WeaponTimeBase();
	}
}

int CMP40::Classify( void )
{
	return CLASS_AUTO_RIFLE;
}

class CMP40AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	BOOL AddAmmo( CBaseEntity *pOther )
	{
		int bResult =
			( pOther->GiveAmmo( P_WpnInfo[12].ammo_maxclip, "ammo_44mm", P_WpnInfo[12].ammo_maxcarry ) != -1 );
		if ( bResult )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, 0, 100 );
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_mp40, CMP40AmmoClip )
// Gewehr 43 - Axis semi rifle (dod/dlls/wpn_shared/k43.cpp)
// | Param | Value |
// | Name | weapon_k43 / CK43 / id 24 (0x18) |
// | Damage | 120 (plrDmgK43 via BULLET_DOD_K43 (22)) |
// | Mag | 10 / carry 80 / default 80 |
// | FireDelay | 0.4 | Reload | 4.4 |
// | Spread | 0.019 +0.10 move | Ani | rifle / k43 |
// | Models | p_k43 / v_k43 / w_k43 |
// | Ammo | ammo_66mm | Slot | 2/0 flags 0x80 weight 20 |
// | Sound | weapons/k43_shoot.wav | Event | events/weapons/k43.sc |
// | Secondary | RifleMeleeAttack id 24, anim BUT_SWING |
// preserved .text markers from server:
// .text:00178020 weapon_k43 (wpn_shared/k43.cpp:19)
// .text:00178090 ammo_k43 (wpn_shared/k43.cpp:193)
// .text:00177940 CK43::Precache (wpn_shared/k43.cpp:45)
// .text:00177bd0 CK43::Spawn (wpn_shared/k43.cpp:26)
// .text:00177990 CK43::GetItemInfo (wpn_shared/k43.cpp:55)
// .text:00177cf0 CK43::PrimaryAttack (wpn_shared/k43.cpp:85)
// .text:00177a60 CK43::SecondaryAttack (wpn_shared/k43.cpp:143)
// .text:00177b40 CK43::Deploy (wpn_shared/k43.cpp:72)
// .text:00177a30 CK43::Reload (wpn_shared/k43.cpp:155)
// K43_e moved to dod_shared.h (identical 1:1) — local duplicate removed.

//
// k43.cpp
//
// implementation of CK43 class
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS( weapon_k43, CK43 )

void CK43::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[24].wmodel );
	m_iId = 24;
	m_iDefaultAmmo = P_WpnInfo[24].ammo_default;
	FallInit();
}

int CK43::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		SET_MODEL( ENT( pev ), P_WpnInfo[24].wmodel );
		return TRUE;
	}
	return FALSE;
}

void CK43::Precache( void )
{
	PRECACHE_MODEL( P_WpnInfo[24].vmodel );
	PRECACHE_MODEL( P_WpnInfo[24].wmodel );
	PRECACHE_SOUND( "weapons/k43_shoot.wav" );
	m_usFireK43 = PRECACHE_EVENT( 1, "events/weapons/k43.sc" );
}

int CK43::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_66mm";
	p->iMaxAmmo1 = P_WpnInfo[24].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[24].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 24;
	p->iWeight = P_WpnInfo[24].misc_weight;
	return 1;
}

BOOL CK43::Deploy( void )
{
	ZoomOut();
	UpdateZoomSpeed();
	return TimedDeploy( P_WpnInfo[24].vmodel, P_WpnInfo[24].pmodel, K43_DRAW, P_WpnInfo[24].szAnimExt,
						P_WpnInfo[24].szAnimReloadExt, 0.79f, 0.79f, 1 );
}

BOOL CK43::CanHolster( void )
{
	return TRUE;
}

void CK43::PrimaryAttack( void )
{
	float flSpread;

	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
	}
	else if ( !m_fInAttack )
	{
		if ( m_iClip <= 0 )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
			m_fInAttack = TRUE;
		}
		else
		{
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
			m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
			--m_iClip;

			flSpread = P_WpnInfo[24].base_accuracy;

			if ( m_pPlayer->pev->velocity.Length() > 45.0f )
				flSpread += P_WpnInfo[24].accuracy_penalty;

			Vector vecSrc = m_pPlayer->GetGunPosition();
			Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, ( Vector ) gpGlobals->v_forward, flSpread, 8192, BULLET_DOD_K43, 3, 0, m_pPlayer->pev,
													  m_pPlayer->random_seed );

			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireK43, 0.0f, g_vecZero, g_vecZero, vecDir.x,
								 vecDir.y, m_pPlayer->pev->punchangle.x, 0, m_iClip, 0 );

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[24].anim_firedelay;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[24].anim_firedelay;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1f;
			m_fInAttack = TRUE;
			m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0f, 15.0f ) + UTIL_WeaponTimeBase();
			RemoveStamina( 2.0f, m_pPlayer );
		}
	}
}

void CK43::SecondaryAttack( void )
{
	RifleMeleeAttack( m_pPlayer, 24 );
	m_pPlayer->SetAnimation( PLAYER_BUT_SWING );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireK43, 0.0f, g_vecZero, g_vecZero, 0, 0, 1, 0,
						 m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.1f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.1f;
}

void CK43::Reload( void )
{
	DefaultReload( P_WpnInfo[24].ammo_maxclip, K43_RELOAD, P_WpnInfo[24].anim_reloadtime );
}

void CK43::WeaponIdle( void )
{
	ResetEmptySound();

	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( K43_IDLE );
		m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0f, 15.0f ) + UTIL_WeaponTimeBase();
	}
}

int CK43::Classify( void )
{
	return CLASS_RIFLE;
}

class CK43AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	BOOL AddAmmo( CBaseEntity *pOther )
	{
		int bResult =
			( pOther->GiveAmmo( P_WpnInfo[24].ammo_maxclip, "ammo_66mm", P_WpnInfo[24].ammo_maxcarry ) != -1 );
		if ( bResult )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, 0, 100 );
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_k43, CK43AmmoClip )
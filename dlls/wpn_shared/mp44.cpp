// MP44/StG44 - Axis assault rifle auto (dod/dlls/wpn_shared/mp44.cpp)
// | Param | Value |
// | Name | weapon_mp44 / CMP44 / id 8 |
// | Damage | 50 (plrDmgMP44 via BULLET_DOD_MP44 (11)) |
// | Mag | 30 / carry 210 / default 210 |
// | FireDelay | 0.09 | Reload | 3.13 | Draw | 1.0 |
// | Spread | 0.025 +0.10 move | Ani | stg44 |
// | Models | p_stg44 / v_mp44 / w_mp44 |
// | Ammo | ammo_55mm | Slot | 2/0 flags 0x80 weight 20 |
// | Sound | weapons/mp44_shoot.wav | Event | events/weapons/mp44.sc |
// | Secondary | empty (return) |
// preserved .text markers from server:
// .text:0017d250 weapon_mp44 (wpn_shared/mp44.cpp:28)
// .text:0017d2c0 ammo_mp44 (wpn_shared/mp44.cpp:187)
// .text:0017cc80 CMP44::Precache (wpn_shared/mp44.cpp:44)
// .text:0017ce70 CMP44::Spawn (wpn_shared/mp44.cpp:34)
// .text:0017cce0 CMP44::GetItemInfo (wpn_shared/mp44.cpp:55)
// .text:0017cf40 CMP44::PrimaryAttack (wpn_shared/mp44.cpp:94)
// .text:0017cd70 CMP44::SecondaryAttack (wpn_shared/mp44.cpp:146)
// MP44_e moved to dod_shared.h (identical 1:1) — local duplicate removed.

//
// mp44.cpp
//
// implementation of CMP44 class
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS( weapon_mp44, CMP44 )

void CMP44::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[8].wmodel );
	m_iId = 8;
	m_iDefaultAmmo = P_WpnInfo[8].ammo_default;
	FallInit();
}

void CMP44::Precache( void )
{
	PRECACHE_MODEL( P_WpnInfo[8].vmodel );
	PRECACHE_MODEL( P_WpnInfo[8].wmodel );

	PRECACHE_SOUND( "weapons/weaponempty.wav" );
	PRECACHE_SOUND( "weapons/mp44_shoot.wav" );

	m_usFireMP44 = PRECACHE_EVENT( 1, "events/weapons/mp44.sc" );
}

int CMP44::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_55mm";
	p->iMaxAmmo1 = P_WpnInfo[8].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[8].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 8;
	p->iWeight = P_WpnInfo[8].misc_weight;
	return 1;
}

int CMP44::AddToPlayer( CBasePlayer *pPlayer )
{
	return CBasePlayerWeapon::AddToPlayer( pPlayer ) != 0;
}

BOOL CMP44::Deploy( void )
{
	ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[8].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[8].vmodel, P_WpnInfo[8].pmodel, MP44_DRAW, P_WpnInfo[8].szAnimExt,
						  P_WpnInfo[8].szAnimReloadExt, 0 );
}

BOOL CMP44::CanHolster( void )
{
	return TRUE;
}

void CMP44::PrimaryAttack( void )
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

		flSpread = P_WpnInfo[8].base_accuracy;

		if ( m_pPlayer->pev->velocity.Length() > 45.0f )
			flSpread += P_WpnInfo[8].accuracy_penalty;
		Vector vecSrc = m_pPlayer->GetGunPosition();

		Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, ( Vector ) gpGlobals->v_forward, flSpread, 8192, BULLET_DOD_MP44, 3, 0, m_pPlayer->pev,
												  m_pPlayer->random_seed );

		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireMP44, 0.0f, g_vecZero, g_vecZero, vecDir.x,
							 vecDir.y, 0, 0, m_iClip == 0, 0 );

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[8].anim_firedelay;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1f;
		m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0f, 15.0f ) + UTIL_WeaponTimeBase();
		RemoveStamina( 2.0f, m_pPlayer );
	}
}

void CMP44::SecondaryAttack( void )
{
	// Nothing.
}

void CMP44::Reload( void )
{
	DefaultReload( P_WpnInfo[8].ammo_maxclip, MP44_RELOAD, P_WpnInfo[8].anim_reloadtime );
}

void CMP44::WeaponIdle( void )
{
	ResetEmptySound();

	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( MP44_IDLE );
		m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0f, 15.0f ) + UTIL_WeaponTimeBase();
	}
}

int CMP44::Classify( void )
{
	return CLASS_AUTO_RIFLE;
}

class CMP44AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	BOOL AddAmmo( CBaseEntity *pOther )
	{
		int bResult = ( pOther->GiveAmmo( P_WpnInfo[8].ammo_maxclip, "ammo_55mm", P_WpnInfo[8].ammo_maxcarry ) != -1 );
		if ( bResult )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, 0, 100 );
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_mp44, CMP44AmmoClip )
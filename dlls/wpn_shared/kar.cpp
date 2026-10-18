// Kar98k - Axis bolt rifle (dod/dlls/wpn_shared/kar.cpp)
// | Param | Value |
// | Name | weapon_kar / CKAR / id 10 |
// | Damage | 160 (gSkillData.plrDmgKAR via BULLET_DOD_KAR (13)) |
// | Mag | 5 / carry 65 / default 65 |
// | FireDelay | 1.6 | Reload | 3.12 | Draw | 1.0 |
// | Spread hip | 0.014 +0.15 move | Ani | bolt |
// | Models | p_k98 / v_98k / w_98k / p_k98_l |
// | Ammo | ammo_16mm | Slot | 2/0 flags 0x80 weight 20 |
// | Sound | weapons/kar_shoot.wav | Event | events/weapons/kar.sc |
// | Secondary | RifleMeleeAttack butt id 10, delay 0.8 |
// preserved .text markers from server:
// .text:00178940 weapon_kar (wpn_shared/kar.cpp:19)
// .text:001781e0 CKAR::Precache (wpn_shared/kar.cpp:39)
// .text:00178540 CKAR::Spawn (wpn_shared/kar.cpp:29)
// .text:001782e0 CKAR::GetItemInfo (wpn_shared/kar.cpp:63)
// .text:00178610 CKAR::PrimaryAttack (wpn_shared/kar.cpp:104)
// .text:001783b0 CKAR::SecondaryAttack (wpn_shared/kar.cpp:167)
// .text:00178490 CKAR::Deploy (wpn_shared/kar.cpp:89)
// .text:00178380 CKAR::Reload (wpn_shared/kar.cpp:180)
// .text:00178590 CKAR::WeaponIdle (wpn_shared/kar.cpp:186)
// .text:00178a40 CKarAmmoClip::Spawn (wpn_shared/kar.cpp)
// .text:00178a70 CKarAmmoClip::AddAmmo (wpn_shared/kar.cpp)
// .text:001789b0 ammo_kar (wpn_shared/kar.cpp)
//
// kar.cpp
//
// implementation of CKAR class
//
// KAR_e moved to dod_shared.h (identical 1:1) — local duplicate removed.

//
// kar.cpp
//
// implementation of CKAR class
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS( weapon_kar, CKAR )

void CKAR::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[10].wmodel );
	m_iId = 10;
	m_iDefaultAmmo = P_WpnInfo[10].ammo_default;
	FallInit();
}

void CKAR::Precache( void )
{
	PRECACHE_MODEL( P_WpnInfo[10].vmodel );
	PRECACHE_MODEL( P_WpnInfo[10].wmodel );

	PRECACHE_SOUND( "weapons/kar_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/boltforward.wav" );
	PRECACHE_SOUND( "weapons/boltback.wav" );
	PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );
	PRECACHE_SOUND( "weapons/knifeswing.wav" );
	PRECACHE_SOUND( "weapons/knifeswing2.wav" );
	PRECACHE_SOUND( "weapons/knife_slash1.wav" );
	PRECACHE_SOUND( "weapons/knife_slash2.wav" );
	PRECACHE_SOUND( "weapons/knife_hit1.wav" );
	PRECACHE_SOUND( "weapons/knife_hit2.wav" );
	PRECACHE_SOUND( "weapons/knife_hit3.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit2.wav" );

	m_usFireKar = PRECACHE_EVENT( 1, "events/weapons/kar.sc" );
}

int CKAR::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_16mm";
	p->iMaxAmmo1 = P_WpnInfo[10].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[10].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 10;
	p->iWeight = P_WpnInfo[10].misc_weight;
	return 1;
}

int CKAR::AddToPlayer( CBasePlayer *pPlayer )
{
	return CBasePlayerWeapon::AddToPlayer( pPlayer ) != 0;
}

BOOL CKAR::Deploy( void )
{
	ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[10].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[10].vmodel, P_WpnInfo[10].pmodel, KAR_DRAW, P_WpnInfo[10].szAnimExt,
						  P_WpnInfo[10].szAnimReloadExt, 0 );
}

BOOL CKAR::CanHolster( void )
{
	return TRUE;
}

void CKAR::PrimaryAttack( void )
{
	float flSpread;

	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4f;
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

			flSpread = P_WpnInfo[10].base_accuracy;

			if ( m_pPlayer->pev->velocity.Length() > 45.0f )
				flSpread += P_WpnInfo[10].accuracy_penalty;

			Vector vecSrc = m_pPlayer->GetGunPosition();
			Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, ( Vector ) gpGlobals->v_forward, flSpread, 8192, BULLET_DOD_KAR, 3, 0, m_pPlayer->pev,
													  m_pPlayer->random_seed );

			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireKar, 0.0f, g_vecZero, g_vecZero, vecDir.x,
								 vecDir.y, 0, 0, m_iClip == 0, 0 );

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[10].anim_firedelay;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[10].anim_firedelay;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1f;
			m_fInAttack = TRUE;
			m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0f, 15.0f ) + UTIL_WeaponTimeBase();
			RemoveStamina( 2.0f, m_pPlayer );
		}
	}
}

void CKAR::SecondaryAttack( void )
{
	RifleMeleeAttack( m_pPlayer, 10 );
	m_pPlayer->SetAnimation( PLAYER_ATTACK2 );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireKar, 0.0f, g_vecZero, g_vecZero, 0, 0, 1, 0,
						 m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.8f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.8f;
}

void CKAR::Reload( void )
{
	DefaultReload( P_WpnInfo[10].ammo_maxclip, KAR_RELOAD, P_WpnInfo[10].anim_reloadtime );
}

void CKAR::WeaponIdle( void )
{
	ResetEmptySound();

	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( KAR_IDLE );
		m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0f, 15.0f ) + UTIL_WeaponTimeBase();
	}
}

int CKAR::Classify( void )
{
	return CLASS_RIFLE;
}

class CKarAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	BOOL AddAmmo( CBaseEntity *pOther )
	{
		int bResult =
			( pOther->GiveAmmo( P_WpnInfo[10].ammo_maxclip, "ammo_16mm", P_WpnInfo[10].ammo_maxcarry ) != -1 );
		if ( bResult )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, 0, 100 );
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_kar, CKarAmmoClip )

// Webley MkVI - British revolver (CWEBLEY)
// PARAMS: dmg 40 | clip 6 carry 18 | reload 4.0s | spread 0.035 | delay 0.10s draw 0.90s | snd webley_shoot | mdl v_webley p_webley w allied_ammo | evt events/weapons/webley.sc | ammo ammo_12mm id 28 slot 1 weight 10
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

// webley_e: WEBLEY_RELOAD removed (==2 in both, collides with dod_shared.h;
// unused — code uses literal 2). Rest kept.
// webley_e: WEBLEY_RELOAD/DRAW removed (collide with dod_shared.h WEBLEY_e;
// unused — code uses literals 2 (RELOAD) and 3 (DRAW)). Rest kept.
enum webley_e
{
	WEBLEY_IDLE1 = 0,
	WEBLEY_FIRE
};
LINK_ENTITY_TO_CLASS( weapon_webley, CWEBLEY );
// .text:001740b0 weapon_webley (wpn_shared/webley.cpp:20) link 001740b0 Ghidra 001840b0
// .text:00173ce0 CWEBLEY::Spawn (wpn_shared/webley.cpp:27) link 00173ce0
void CWEBLEY::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[28].wmodel );
	m_iId = 28;
	m_iDefaultAmmo = P_WpnInfo[28].ammo_default;
	FallInit();
}
// .text:00173cb0 CWEBLEY::AddToPlayer (wpn_shared/webley.cpp:36) link 00173cb0
int CWEBLEY::AddToPlayer( CBasePlayer *p )
{
	return CBasePlayerWeapon::AddToPlayer( p ) != 0;
}
// .text:00173ab0 CWEBLEY::Precache (wpn_shared/webley.cpp:45) link 00173ab0
void CWEBLEY::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[28].vmodel );
    PRECACHE_MODEL( P_WpnInfo[28].wmodel );

    PRECACHE_SOUND( "weapons/webley_shoot.wav" );
    PRECACHE_SOUND( "weapons/357_cock1.wav" );
    PRECACHE_SOUND( "weapons/colt_reload_clipin.wav" );
    PRECACHE_SOUND( "weapons/colt_reload_clipout.wav" );
    PRECACHE_SOUND( "weapons/colt_draw_pullslide.wav" );
    
    m_usFireWebley = PRECACHE_EVENT( 1, "events/weapons/webley.sc" );
}
// .text:00173b40 CWEBLEY::GetItemInfo (wpn_shared/webley.cpp:59) link 00173b40
int CWEBLEY::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_12mm";
	p->iMaxAmmo1 = P_WpnInfo[28].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[28].ammo_maxclip;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0x40;
	p->iId = m_iId = 28;
	p->iWeight = P_WpnInfo[28].misc_weight;
	return 1;
}
// .text:00173c10 CWEBLEY::Deploy (wpn_shared/webley.cpp:76) link 00173c10
BOOL CWEBLEY::Deploy()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
	m_pPlayer->CheckPlayerSpeed();
	return TimedDeploy( P_WpnInfo[28].vmodel, P_WpnInfo[28].pmodel, 3, P_WpnInfo[28].szAnimExt,
						P_WpnInfo[28].szAnimReloadExt, P_WpnInfo[28].anim_drawtime, 1.0, ( UseDecrement() != 0 ) );
}
// .text:00173bc0 CWEBLEY::CanHolster (wpn_shared/webley.cpp:88) link 00173bc0
int CWEBLEY::CanHolster()
{
	return 1;
}
// .text:00173d90 CWEBLEY::PrimaryAttack (wpn_shared/webley.cpp:91) link 00173d90 - 6.8KB, revolver double-action, punchangle kick
void CWEBLEY::PrimaryAttack()
{
	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
	}
	else if ( !m_fInAttack )
	{
		if ( m_iClip < 1 )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
			m_fInAttack = 1;
		}
		else
		{
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
			m_iClip--;
			float flSpread = P_WpnInfo[m_iId].base_accuracy;
			if ( m_pPlayer->pev->velocity.Length() > 45.0f )
				flSpread += P_WpnInfo[m_iId].accuracy_penalty;
			Vector vecSrc = m_pPlayer->GetGunPosition();
			Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, gpGlobals->v_forward, flSpread, 8192, BULLET_DOD_WEBLEY,
													  3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireWebley, 0, g_vecZero, g_vecZero, vecDir.x,
								 vecDir.y, 0, 0, ( m_iClip == 0 ), 0 );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[28].anim_firedelay;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
			m_fInAttack = 1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
			RemoveStamina( 1.0f, m_pPlayer );
		}
	}
}
// .text:00173bd0 CWEBLEY::SecondaryAttack (wpn_shared/webley.cpp:148) link 00173bd0
void CWEBLEY::SecondaryAttack()
{
	return;
}
// .text:00173be0 CWEBLEY::Reload (wpn_shared/webley.cpp:151) link 00173be0
void CWEBLEY::Reload()
{
	DefaultReload( P_WpnInfo[28].ammo_maxclip, 2, P_WpnInfo[28].anim_reloadtime );
}
// .text:00173d30 CWEBLEY::WeaponIdle (wpn_shared/webley.cpp:160) link 00173d30
void CWEBLEY::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
	SendWeaponAnim( 0, 1 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
}
// .text:00174190 CWebleyAmmoClip::Spawn link 00174190 Ghidra 00184190
class CWebleyAmmoClip : public CBasePlayerAmmo
{
  public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:00174190 CWebleyAmmoClip::Spawn (wpn_shared/webley.cpp)
void CWebleyAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:001741c0 CWebleyAmmoClip::AddAmmo (wpn_shared/webley.cpp)
int CWebleyAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult =
		( (CBasePlayer *) pOther )->GiveAmmo( P_WpnInfo[28].ammo_maxclip, "ammo_12mm", P_WpnInfo[28].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:00174120 ammo_webley link 00174120 Ghidra 00184120
LINK_ENTITY_TO_CLASS( ammo_webley, CWebleyAmmoClip );

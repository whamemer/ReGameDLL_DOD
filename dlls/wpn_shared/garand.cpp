// Garand M1 - Allied semi-auto rifle
// PARAMS: dmg 120 | clip 8 carry 88 | reload 1.8s | spread 0.016 +0.10 move | delay 0.40s draw 1.25s | snd garand_shoot reloadgarand clipding clipin butt_hit 357_cock1 | mdl p_garand v_garand w_garand p_garand_l | evt events/weapons/garand.sc | ammo ammo_16mm id 5 slot 2 pos 0 weight 20 flags 0x80 | BULLET_DOD_GARAND (8)
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

// garand_e: GARAND_RELOAD/IDLE_EMPTY removed (collide with dod_shared.h GARAND_e;
// RELOAD unused — code uses literal 5 == client GARAND_RELOAD; IDLE_EMPTY == 7
// in both, now resolves to client). Rest kept (unique names, same values).
enum garand_e
{
	GARAND_IDLE1 = 0,
	GARAND_FIRE1,
	GARAND_FIRE2,
	GARAND_DEPLOY = 6,
};
LINK_ENTITY_TO_CLASS( weapon_garand, CGarand );
// .text:00175070 weapon_garand (wpn_shared/garand.cpp:19)
// .text:00174950 CGarand::Precache (wpn_shared/garand.cpp:36)
void CGarand::Precache()
{
	PRECACHE_MODEL( "models/v_garand.mdl" );
	PRECACHE_MODEL( "models/w_garand.mdl" );
	PRECACHE_SOUND( "weapons/reloadgarand.wav" );
	PRECACHE_SOUND( "weapons/garand_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/garand_reload_clipding.wav" );
	PRECACHE_SOUND( "weapons/garand_reload_clipin.wav" );
	PRECACHE_SOUND( "weapons/butt_hit.wav" );
	m_usFireGarand = PRECACHE_EVENT( 1, "events/weapons/garand.sc" );
}
// .text:001749e0 CGarand::GetItemInfo (wpn_shared/garand.cpp:52)
int CGarand::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_16mm";
	p->iMaxAmmo1 = P_WpnInfo[5].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[5].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 5;
	p->iWeight = P_WpnInfo[5].misc_weight;
	return 1;
}
// .text:00174c10 CGarand::Spawn (wpn_shared/garand.cpp:26)
void CGarand::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[5].wmodel );
	m_iId = 5;
	m_iDefaultAmmo = P_WpnInfo[5].ammo_default;
	FallInit();
}
// .text:00174be0 CGarand::AddToPlayer (wpn_shared/garand.cpp:69)
int CGarand::AddToPlayer( CBasePlayer *p )
{
	return CBasePlayerWeapon::AddToPlayer( p ) != 0;
}
// .text:00174b60 CGarand::Deploy (wpn_shared/garand.cpp:78)
BOOL CGarand::Deploy()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[5].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[5].vmodel, P_WpnInfo[5].pmodel, GARAND_DEPLOY, P_WpnInfo[5].szAnimExt,
						  0, 0 ); // skiplocal is int here (recomputed in SendWeaponAnim); older SDKs passed a pointer
}
// .text:00174a60 CGarand::CanHolster (wpn_shared/garand.cpp:89)
int CGarand::CanHolster()
{
	return 1;
}
// .text:00174d40 CGarand::PrimaryAttack (wpn_shared/garand.cpp:92)
void CGarand::PrimaryAttack()
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
			m_pPlayer->m_iWeaponVolume = 600;
			m_iClip--;
			float flSpread = P_WpnInfo[m_iId].base_accuracy;
			if ( m_pPlayer->pev->velocity.Length() > 45.0f )
				flSpread += P_WpnInfo[m_iId].accuracy_penalty;
			Vector vecSrc = m_pPlayer->GetGunPosition();
			Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, gpGlobals->v_forward, flSpread, 8192, BULLET_DOD_GARAND,
													  3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireGarand, 0, g_vecZero, g_vecZero,
								 vecDir.x, vecDir.y, (int) m_pPlayer->pev->punchangle.x, 0, m_iClip, 0 );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[5].anim_firedelay;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[5].anim_firedelay;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
			m_fInAttack = 1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
			RemoveStamina( 2.0f, m_pPlayer );
		}
	}
}
// .text:00174a80 CGarand::SecondaryAttack (wpn_shared/garand.cpp:147)
void CGarand::SecondaryAttack()
{
	RifleMeleeAttack( m_pPlayer, 5 );
	m_pPlayer->SetAnimation( PLAYER_BUT_SWING );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireGarand, 0, g_vecZero, g_vecZero, 0, 0, 1, 0,
						 ( m_iClip == 0 ), 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.1;
}
// .text:00174cf0 CGarand::Reload (wpn_shared/garand.cpp:159)
void CGarand::Reload()
{
	if ( m_iClip > 0 )
		return;
	DefaultReload( P_WpnInfo[5].ammo_maxclip, 5, P_WpnInfo[5].anim_reloadtime );
}
// .text:00174c60 CGarand::WeaponIdle (wpn_shared/garand.cpp:167)
void CGarand::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
	int anim = ( m_iClip > 0 ) ? GARAND_IDLE1 : GARAND_IDLE_EMPTY;
	SendWeaponAnim( anim, 1 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
}
// .text:00174a70 CGarand::Classify (wpn_shared/garand.cpp:180)
int CGarand::Classify()
{
	return CLASS_RIFLE;
}
// .text:00165170 CGarandAmmoClip::Spawn link 00165170 Ghidra 00175170
class CGarandAmmoClip : public CBasePlayerAmmo
{
  public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:00165170 CGarandAmmoClip::Spawn (wpn_shared/garand.cpp)
void CGarandAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:001651a0 CGarandAmmoClip::AddAmmo (wpn_shared/garand.cpp)
int CGarandAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult =
		( (CBasePlayer *) pOther )->GiveAmmo( P_WpnInfo[5].ammo_maxclip, "ammo_16mm", P_WpnInfo[5].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:001650e0 ammo_garand link 001650e0 Ghidra 001750e0
LINK_ENTITY_TO_CLASS( ammo_garand, CGarandAmmoClip );

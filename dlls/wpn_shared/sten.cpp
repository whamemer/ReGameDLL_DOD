// Sten Mk II - Allied auto SMG
// PARAMS: dmg 40 | clip 30 carry 210 | reload 2.9s | spread 0.025 +0.10 | delay 0.10s draw 0.75s | snd reloadgarand sten_shoot 357_cock1 tommy_draw_slideback tommy_reload_clipout tommy_reload_clipin | mdl v_sten w_sten p_sten | evt events/weapons/sten.sc | ammo ammo_44mm id 26 slot 2 weight 20 flags 0x80 | BULLET_DOD_STEN (24) auto
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

// sten_e: STEN_RELOAD removed (==1 in both, collides with dod_shared.h STEN_e;
// unused — code uses literal 1 == client STEN_RELOAD). Rest kept.
enum sten_e
{
	STEN_IDLE1 = 0,
	STEN_DEPLOY = 2
};
class CSTENAmmoClip : public CBasePlayerAmmo
{
  public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( weapon_sten, CSTEN );
// .text:00181640 weapon_sten (wpn_shared/sten.cpp:22)
// .text:00181030 CSTEN::Precache (wpn_shared/sten.cpp:38)
void CSTEN::Precache()
{
	PRECACHE_MODEL( "models/v_sten.mdl" );
	PRECACHE_MODEL( "models/w_sten.mdl" );
	PRECACHE_SOUND( "weapons/reloadgarand.wav" );
	PRECACHE_SOUND( "weapons/sten_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/tommy_draw_slideback.wav" );
	PRECACHE_SOUND( "weapons/tommy_reload_clipout.wav" );
	PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );
	m_usFireSten = PRECACHE_EVENT( 1, "events/weapons/sten.sc" );
}
// .text:001810c0 CSTEN::GetItemInfo (wpn_shared/sten.cpp:53)
int CSTEN::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_44mm";
	p->iMaxAmmo1 = P_WpnInfo[26].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[26].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 26;
	p->iWeight = P_WpnInfo[26].misc_weight;
	return 1;
}
// .text:00181250 CSTEN::Spawn (wpn_shared/sten.cpp:28)
void CSTEN::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[26].wmodel );
	m_iId = 26;
	m_iDefaultAmmo = P_WpnInfo[26].ammo_default;
	FallInit();
}
// .text:00181220 CSTEN::AddToPlayer (wpn_shared/sten.cpp:70)
int CSTEN::AddToPlayer( CBasePlayer *p )
{
	if ( CBasePlayerWeapon::AddToPlayer( p ) )
		return 1;
	return 0;
}
// .text:001811a0 CSTEN::Deploy (wpn_shared/sten.cpp:79)
BOOL CSTEN::Deploy()
{
	ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75f;
	return DefaultDeploy( P_WpnInfo[26].vmodel, P_WpnInfo[26].pmodel, STEN_DEPLOY, P_WpnInfo[26].szAnimExt,
						  P_WpnInfo[26].szAnimReloadExt, 0 );
}
// .text:00181140 CSTEN::CanHolster (wpn_shared/sten.cpp:90)
int CSTEN::CanHolster()
{
	return 1;
}
// .text:00181330 CSTEN::PrimaryAttack (wpn_shared/sten.cpp:93)
void CSTEN::PrimaryAttack()
{
	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
	}
	else if ( m_iClip <= 0 )
	{
		if ( !m_fInAttack )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
			m_fInAttack = 1;
		}
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
		Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, ( Vector ) gpGlobals->v_forward, flSpread, 8192, BULLET_DOD_STEN, 3, 0, m_pPlayer->pev,
												  m_pPlayer->random_seed );
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireSten, 0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0,
							 ( m_iClip == 0 ), 0 );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[26].anim_firedelay;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10.0f, 15.0f );
		RemoveStamina( 2.0f, m_pPlayer );
	}
}
// .text:00181150 CSTEN::SecondaryAttack (wpn_shared/sten.cpp:146)
void CSTEN::SecondaryAttack()
{
	return;
}
// .text:00181170 CSTEN::Reload (wpn_shared/sten.cpp:149)
void CSTEN::Reload()
{
	DefaultReload( P_WpnInfo[26].ammo_maxclip, 1, P_WpnInfo[26].anim_reloadtime );
}
// .text:001812a0 CSTEN::WeaponIdle (wpn_shared/sten.cpp:154)
void CSTEN::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
	SendWeaponAnim( STEN_IDLE1, UseDecrement() != 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10.0f, 15.0f );
}
// .text:00181160 CSTEN::Classify (wpn_shared/sten.cpp:168)
int CSTEN::Classify()
{
	return CLASS_AUTO_RIFLE;
}
// .text:00181740 CSTENAmmoClip::Spawn (wpn_shared/sten.cpp:172)
void CSTENAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:00181770 CSTENAmmoClip::AddAmmo (wpn_shared/sten.cpp:177)
int CSTENAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult =
		( (CBasePlayer *) pOther )->GiveAmmo( P_WpnInfo[26].ammo_maxclip, "ammo_44mm", P_WpnInfo[26].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:001816b0 ammo_sten (wpn_shared/sten.cpp:188)
LINK_ENTITY_TO_CLASS( ammo_sten, CSTENAmmoClip )

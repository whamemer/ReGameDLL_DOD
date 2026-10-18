// Thompson M1928 - Allied auto SMG
// PARAMS: dmg 40 | clip 30 carry 210 | reload 2.8s | spread 0.025 +0.10 | delay 0.085s draw 1.10s | snd thompson_shoot tommy_draw_slideback tommy_reload_clipin tommy_reload_clipout | mdl p_tommy v_tommy w_tommy p_tommy_l | evt events/weapons/thompson.sc | ammo ammo_44mm id 7 slot 2 weight 20 flags 0x80 | BULLET_DOD_THOMPSON (10) auto
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

// thompson_e: THOMPSON_RELOAD removed (server 2 vs client 1, collides;
// unused — code uses literal 1 == client THOMPSON_RELOAD). Rest kept.
enum thompson_e
{
	THOMPSON_IDLE1 = 0,
	THOMPSON_FIRE1,
	THOMPSON_DEPLOY = 2
};
LINK_ENTITY_TO_CLASS( weapon_thompson, CThompson );
// .text:001838f0 weapon_thompson (wpn_shared/thompson.cpp:19)
// .text:001832c0 CThompson::Precache (wpn_shared/thompson.cpp:35)
void CThompson::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[7].vmodel );
    PRECACHE_MODEL( P_WpnInfo[7].wmodel );

    PRECACHE_SOUND( "weapons/reloadgarand.wav" );
    PRECACHE_SOUND( "weapons/thompson_shoot.wav" );
    PRECACHE_SOUND( "weapons/357_cock1.wav" );
    PRECACHE_SOUND( "weapons/tommy_draw_slideback.wav" );
    PRECACHE_SOUND( "weapons/tommy_reload_clipout.wav" );
    PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );

    m_usFireThompson = PRECACHE_EVENT( 1, "events/weapons/thompson.sc" );
}
// .text:00183350 CThompson::GetItemInfo (wpn_shared/thompson.cpp:50)
int CThompson::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_44mm";
	p->iMaxAmmo1 = P_WpnInfo[7].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[7].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 7;
	p->iWeight = P_WpnInfo[7].misc_weight;
	return 1;
}
// .text:001834e0 CThompson::Spawn (wpn_shared/thompson.cpp:25)
void CThompson::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[7].wmodel );
	m_iId = 7;
	m_iDefaultAmmo = P_WpnInfo[7].ammo_default;
	FallInit();
}
// .text:001834b0 CThompson::AddToPlayer (wpn_shared/thompson.cpp:67)
int CThompson::AddToPlayer( CBasePlayer *p )
{
	return CBasePlayerWeapon::AddToPlayer( p ) != 0;
}
// .text:00183430 CThompson::Deploy (wpn_shared/thompson.cpp:76)
BOOL CThompson::Deploy()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[7].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[7].vmodel, P_WpnInfo[7].pmodel, THOMPSON_DEPLOY, P_WpnInfo[7].szAnimExt,
						  P_WpnInfo[7].szAnimReloadExt, 0 );
}
// .text:001833d0 CThompson::CanHolster (wpn_shared/thompson.cpp:86)
int CThompson::CanHolster()
{
	return 1;
}
// .text:001835e0 CThompson::PrimaryAttack (wpn_shared/thompson.cpp:89)
void CThompson::PrimaryAttack()
{
	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
	}
	else
	{
		if ( m_iClip < 1 )
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
			Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, gpGlobals->v_forward, flSpread, 8192,
													  BULLET_DOD_THOMPSON, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireThompson, 0, g_vecZero, g_vecZero,
								 vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ), 0 );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[7].anim_firedelay;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
			RemoveStamina( 2.0f, m_pPlayer );
		}
	}
}
// .text:001833e0 CThompson::SecondaryAttack (wpn_shared/thompson.cpp:142)
void CThompson::SecondaryAttack()
{
	return;
}
// .text:00183400 CThompson::Reload (wpn_shared/thompson.cpp:145)
void CThompson::Reload()
{
	DefaultReload( P_WpnInfo[7].ammo_maxclip, 1, P_WpnInfo[7].anim_reloadtime );
}
// .text:00183530 CThompson::WeaponIdle (wpn_shared/thompson.cpp:150)
void CThompson::WeaponIdle()
{
	ResetEmptySound();

	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( m_iClip == 0 ? 5 : 0, UseDecrement() != 0 );
		m_flTimeWeaponIdle = RANDOM_FLOAT( 10, 15 ) + UTIL_WeaponTimeBase();
	}
}
// .text:001833f0 CThompson::Classify (wpn_shared/thompson.cpp:171)
int CThompson::Classify()
{
	return CLASS_AUTO_RIFLE;
}
// .text:001739f0 CThompsonAmmoClip::Spawn link 001739f0 Ghidra 001839f0
class CThompsonAmmoClip : public CBasePlayerAmmo
{
  public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:001739f0 CThompsonAmmoClip::Spawn (wpn_shared/thompson.cpp)
void CThompsonAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:00173a20 CThompsonAmmoClip::AddAmmo (wpn_shared/thompson.cpp)
int CThompsonAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult =
		( (CBasePlayer *) pOther )->GiveAmmo( P_WpnInfo[7].ammo_maxclip, "ammo_44mm", P_WpnInfo[7].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:00173960 ammo_thompson link 00173960 Ghidra 00183960
LINK_ENTITY_TO_CLASS( ammo_thompson, CThompsonAmmoClip );

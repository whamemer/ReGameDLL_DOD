// Greasegun M3 - Allied auto SMG
// PARAMS: dmg 40 | clip 30 carry 210 | reload 4.1s | spread 0.025 +0.10 | delay 0.15s | snd greasegun_shoot | mdl v_greasegun p_grease w_greasegun p_grease_l | evt events/weapons/greasegun.sc | ammo ammo_44mm id 22 slot 2 weight 20 | BULLET_DOD_GREASEGUN (20) auto
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum grease_e
{
	GREASE_IDLE1 = 0,
	GREASE_FIRE,
	GREASE_RELOAD,
	GREASE_IDLE_EMPTY = 5,
	GREASE_FAST_DRAW
};
LINK_ENTITY_TO_CLASS( weapon_greasegun, CGreaseGun );
// .text:00165a10 weapon_greasegun (wpn_shared/greasegun.cpp:19)
// .text:001653b0 CGreaseGun::Precache (wpn_shared/greasegun.cpp:35)
void CGreaseGun::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[22].vmodel );
    PRECACHE_MODEL( P_WpnInfo[22].wmodel );

    PRECACHE_SOUND( "weapons/greasegun_shoot.wav" );
    PRECACHE_SOUND( "weapons/357_cock1.wav" );

    m_usFireGreaseGun = PRECACHE_EVENT( 1, "events/weapons/greasegun.sc" );
}
// .text:00165410 CGreaseGun::GetItemInfo (wpn_shared/greasegun.cpp:46)
int CGreaseGun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_44mm";
	p->iMaxAmmo1 = P_WpnInfo[22].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[22].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 22;
	p->iWeight = P_WpnInfo[22].misc_weight;
	return 1;
}
// .text:00165620 CGreaseGun::Spawn (wpn_shared/greasegun.cpp:25)
void CGreaseGun::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[22].wmodel );
	m_iId = 22;
	m_iDefaultAmmo = P_WpnInfo[22].ammo_default;
	FallInit();
}
// .text:001655f0 CGreaseGun::AddToPlayer (wpn_shared/greasegun.cpp:63)
int CGreaseGun::AddToPlayer( CBasePlayer *p )
{
	return CBasePlayerWeapon::AddToPlayer( p ) != 0;
}
// .text:00165570 CGreaseGun::Deploy (wpn_shared/greasegun.cpp:72)
BOOL CGreaseGun::Deploy()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	return DefaultDeploy( P_WpnInfo[22].vmodel, P_WpnInfo[22].pmodel, GREASE_FAST_DRAW, P_WpnInfo[22].szAnimExt,
						  P_WpnInfo[22].szAnimReloadExt, 0 );
}
// .text:001654e0 CGreaseGun::SpawnDeploy (wpn_shared/greasegun.cpp:81)
BOOL CGreaseGun::SpawnDeploy()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
	return TimedDeploy( P_WpnInfo[22].vmodel, P_WpnInfo[22].pmodel, GREASE_RELOAD, P_WpnInfo[22].szAnimExt,
						P_WpnInfo[22].szAnimReloadExt, 1.6, 1.6, ( UseDecrement() != 0 ) );
}
// .text:00165490 CGreaseGun::CanHolster (wpn_shared/greasegun.cpp:91)
int CGreaseGun::CanHolster()
{
	return 1;
}
// .text:00165700 CGreaseGun::PrimaryAttack (wpn_shared/greasegun.cpp:94)
void CGreaseGun::PrimaryAttack()
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
													  BULLET_DOD_GREASEGUN, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireGreaseGun, 0, g_vecZero, g_vecZero,
								 vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ), 0 );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[22].anim_firedelay;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
			RemoveStamina( 2.0f, m_pPlayer );
		}
	}
}
// .text:001654b0 CGreaseGun::Reload (wpn_shared/greasegun.cpp:143)
void CGreaseGun::Reload()
{
	DefaultReload( P_WpnInfo[22].ammo_maxclip, 1, P_WpnInfo[22].anim_reloadtime );
}
// .text:00165670 CGreaseGun::WeaponIdle (wpn_shared/greasegun.cpp:148)
void CGreaseGun::WeaponIdle()
{
	ResetEmptySound();

	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( m_iClip < 1 ? GREASE_IDLE_EMPTY : GREASE_IDLE1, 1 );
		m_flTimeWeaponIdle = RANDOM_FLOAT( 10, 15 ) + UTIL_WeaponTimeBase();
	}
}
// .text:001654a0 CGreaseGun::Classify link 0x1654a0 size 0x6 Ghidra 0x1754a0: return 0x40 CLASS_AUTO_RIFLE exact (548B attribution bogus)
int CGreaseGun::Classify()
{
	return CLASS_AUTO_RIFLE;
}
// .text:00165b10 CGreaseGunAmmoClip::Spawn link 00165b10 Ghidra 00175b10
class CGreaseGunAmmoClip : public CBasePlayerAmmo
{
  public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:00165b10 CGreaseGunAmmoClip::Spawn (wpn_shared/greasegun.cpp)
void CGreaseGunAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:00165b40 CGreaseGunAmmoClip::AddAmmo (wpn_shared/greasegun.cpp)
int CGreaseGunAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult =
		( (CBasePlayer *) pOther )->GiveAmmo( P_WpnInfo[22].ammo_maxclip, "ammo_44mm", P_WpnInfo[22].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:00165a80 ammo_greasegun link 00165a80 Ghidra 00175a80
LINK_ENTITY_TO_CLASS( ammo_greasegun, CGreaseGunAmmoClip );

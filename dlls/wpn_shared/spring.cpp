// Springfield M1903 - Allied bolt sniper
// PARAMS: dmg 160 | clip 5 carry 55 | reload 2.7s | spread hip 0.06 scoped 0.004 | delay 1.85s draw 1.33s | snd spring_shoot boltback boltforward | mdl p_spring v_spring w_spring p_spring_l (+binoculars v for spotter id 33/34) | evt events/weapons/spring.sc | ammo ammo_22mm id 9 slot 2 weight 20 | BULLET_DOD_SPRING (12) | zoom FOV 20
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

// spring_e: SPRING_RELOAD removed (server 2 vs client 3, collides;
// unused — code uses literal 3 == client SPRING_RELOAD). Rest kept.
// spring_e: SPRING_RELOAD/DRAW removed (collide with dod_shared.h SPRING_e;
// unused — code uses literals 3 (RELOAD) and 4 (DRAW)). Rest kept.
enum spring_e
{
	SPRING_IDLE1 = 0,
	SPRING_FIRE
};
LINK_ENTITY_TO_CLASS( weapon_spring, CSPRING );
// .text:00170e70 weapon_spring (wpn_shared/spring.cpp:27) link 00170e70 Ghidra 00180e70
// .text:00170a00 CSPRING::Spawn (wpn_shared/spring.cpp:33) link 00170a00
void CSPRING::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[9].wmodel );
	m_iId = 9;
	m_iDefaultAmmo = P_WpnInfo[9].ammo_default;
	FallInit();
}
// .text:00170610 CSPRING::Precache (wpn_shared/spring.cpp:43) link 00170610
void CSPRING::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[9].vmodel );
    PRECACHE_MODEL( P_WpnInfo[9].wmodel );

    PRECACHE_SOUND( "weapons/reloadgarand.wav" );
    PRECACHE_SOUND( "weapons/spring_shoot.wav" );
    PRECACHE_SOUND( "weapons/357_cock1.wav" );
    PRECACHE_SOUND( "weapons/garand_reload_clipin.wav" );

    m_usFireSpring = PRECACHE_EVENT( 1, "events/weapons/spring.sc" );
}
// .text:00170690 CSPRING::GetItemInfo (wpn_shared/spring.cpp:55) link 00170690
int CSPRING::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_22mm";
	p->iMaxAmmo1 = P_WpnInfo[9].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[9].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 9;
	p->iWeight = P_WpnInfo[9].misc_weight;
	return 1;
}
// .text:001709d0 CSPRING::AddToPlayer (wpn_shared/spring.cpp:72) link 001709d0
int CSPRING::AddToPlayer( CBasePlayer *p )
{
	return CBasePlayerWeapon::AddToPlayer( p ) != 0;
}
// .text:00170950 CSPRING::Deploy (wpn_shared/spring.cpp:81) link 00170950
BOOL CSPRING::Deploy()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[9].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[9].vmodel, P_WpnInfo[9].pmodel, 4, P_WpnInfo[9].szAnimExt,
						  P_WpnInfo[9].szAnimReloadExt, 0 );
}
// .text:00170710 CSPRING::CanHolster (wpn_shared/spring.cpp:92) link 00170710
int CSPRING::CanHolster()
{
	return 1;
}
// .text:00170ad0 CSPRING::PrimaryAttack (wpn_shared/spring.cpp:95) link 00170ad0 - 7.9KB
void CSPRING::PrimaryAttack()
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
			float flSpread;
			if ( GetFOV() < 1 )
				flSpread = P_WpnInfo[9].base_accuracy;
			else
				flSpread = P_WpnInfo[9].base_accuracy2;
			if ( m_pPlayer->pev->velocity.Length() > 45.0f )
				flSpread += P_WpnInfo[9].accuracy_penalty;
			Vector vecSrc = m_pPlayer->GetGunPosition();
			Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, gpGlobals->v_forward, flSpread, 8192, BULLET_DOD_SPRING,
													  3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireSpring, 0, g_vecZero, g_vecZero, vecDir.x,
								 vecDir.y, 0, 0, ( m_iClip == 0 ), 0 );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[9].anim_firedelay;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[9].anim_firedelay;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
			m_fInAttack = 1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[9].anim_firedelay;
			if ( GetFOV() > 0 )
			{
				if ( m_iClip < 1 )
					SetThink( &CSPRING::ThinkZoomOut );
				else
					SetThink( &CSPRING::ThinkZoomOutIn );
				pev->nextthink = gpGlobals->time + 0.5;
			}
			RemoveStamina( 2.0f, m_pPlayer );
		}
	}
}
// .text:001707e0 CSPRING::SecondaryAttack (wpn_shared/spring.cpp:177) link 001707e0 - zoom toggle FOV 20
void CSPRING::SecondaryAttack()
{
	int fov = GetZoomFOVToggle( 20 );
	m_pPlayer->m_iFOV = fov;
	UpdateZoomSpeed();
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.9;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2;
}
// .text:001708d0 CSPRING::Reload (wpn_shared/spring.cpp:186) link 001708d0
void CSPRING::Reload()
{
	if ( GetFOV() > 0 )
	{
		m_pPlayer->m_iFOV = GetZoomFOV();
		UpdateZoomSpeed();
	}
	SetThink( NULL );
	DefaultReload( P_WpnInfo[9].ammo_maxclip, 3, P_WpnInfo[9].anim_reloadtime );
}
// .text:00170720 CSPRING::ThinkZoomOutIn (wpn_shared/spring.cpp:200) link 00170720
void CSPRING::ThinkZoomOutIn()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
	SetThink( &CBasePlayerWeapon::ThinkZoomIn );
	pev->nextthink = gpGlobals->time + 1.4;
}
// .text:00170790 CSPRING::ThinkZoomOut (wpn_shared/spring.cpp:211) link 00170790
void CSPRING::ThinkZoomOut()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
}
// .text:00170860 CSPRING::Holster (wpn_shared/spring.cpp:217) link 00170860
void CSPRING::Holster( int i )
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
	SetThink( NULL );
	CBasePlayerWeapon::Holster( i );
}
// .text:00170a50 CSPRING::WeaponIdle (wpn_shared/spring.cpp:226) link 00170a50
void CSPRING::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
	SendWeaponAnim( 0, 1 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
}
// .text:001707d0 CSPRING::Classify (wpn_shared/spring.cpp:239) link 001707d0
int CSPRING::Classify()
{
	return CLASS_SCOPE_RIFLE;
}
// .text:00170f70 CSpringAmmoClip::Spawn link 00170f70 Ghidra 00180f70
class CSpringAmmoClip : public CBasePlayerAmmo
{
  public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:00170f70 CSpringAmmoClip::Spawn (wpn_shared/spring.cpp)
void CSpringAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:00170fa0 CSpringAmmoClip::AddAmmo (wpn_shared/spring.cpp)
int CSpringAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult =
		( (CBasePlayer *) pOther )->GiveAmmo( P_WpnInfo[9].ammo_maxclip, "ammo_22mm", P_WpnInfo[9].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:00170ee0 ammo_spring link 00170ee0 Ghidra 00180ee0
LINK_ENTITY_TO_CLASS( ammo_spring, CSpringAmmoClip );

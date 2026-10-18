// Enfield No4 + ScopedEnfield - British bolt rifle, shared CENFIELD class
// PARAMS normal: dmg 160 | clip 5 carry 60 | reload 3.66s | spread 0.014 +0.15 move | delay 1.65s draw 0.80s | scoped: dmg 160 | reload 3.54s | spread scoped 0.004 base 0.06 | mdl p_enfield v_enfield w_enfield p_enfield_l / scoped p_enfields v_enfield_scoped w_enfield_scoped p_enfields_l | snd enfield_shoot enfieldsniper_shoot | evt events/weapons/enfield.sc scopedenfield.sc | ammo ammo_16mm id 25 scoped 38 slot 2 weight 20 | BULLET_DOD_ENFIELD (23)
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

// enfield_e: ENFIELD_RELOAD removed (==2 in both, collides with dod_shared.h
// ENFIELD_e; unused — Reload() uses m_iId). Rest kept (FIRE/SHOOT, DEPLOY/SWING
// share values 1/5 with client under different names — no macro collision).
// enfield_e: ENFIELD_RELOAD/DRAW removed (==2/3, collide with dod_shared.h
// ENFIELD_e; unused — code uses literals 2 and 3). Rest kept.
enum enfield_e
{
	ENFIELD_IDLE1 = 0,
	ENFIELD_FIRE
};
LINK_ENTITY_TO_CLASS( weapon_enfield, CENFIELD );
LINK_ENTITY_TO_CLASS( weapon_scopedenfield, CENFIELD );
// .text:00163a00 weapon_enfield (wpn_shared/enfield.cpp:34) link 00163a00 Ghidra 00173a00
// .text:00163a70 weapon_scopedenfield (wpn_shared/enfield.cpp:35) link 00163a70 Ghidra 00173a70 - same class, m_iWeaponState bit1 selects scoped P_WpnInfo[38]
// .text:00163280 CENFIELD::Spawn (wpn_shared/enfield.cpp:41) link 00163280
void CENFIELD::Spawn()
{
	Precache();
	if ( m_iWeaponState & 1 )
		SET_MODEL( ENT( pev ), P_WpnInfo[38].wmodel );
	else
		SET_MODEL( ENT( pev ), P_WpnInfo[25].wmodel );
	m_iId = 25;
	m_iDefaultAmmo = P_WpnInfo[25].ammo_default;
	FallInit();
}
// .text:00162ef0 CENFIELD::Precache (wpn_shared/enfield.cpp:56) link 00162ef0
void CENFIELD::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[25].vmodel );
    PRECACHE_MODEL( P_WpnInfo[25].wmodel );

    m_usFireEnfield = PRECACHE_EVENT( 1, "events/weapons/enfield.sc" );

    PRECACHE_SOUND( "weapons/enfield_shoot.wav" );
    PRECACHE_SOUND( "weapons/357_cock1.wav" );
    PRECACHE_SOUND( "weapons/boltforward.wav" );
    PRECACHE_SOUND( "weapons/boltback.wav" );
    PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );

    PRECACHE_MODEL( P_WpnInfo[38].vmodel );
    PRECACHE_MODEL( P_WpnInfo[38].wmodel );

    PRECACHE_SOUND( "weapons/enfieldsniper_shoot.wav" );

    m_usFireScopedEnfield = PRECACHE_EVENT( 1, "events/weapons/scopedenfield.sc" );
}
// .text:00162fc0 CENFIELD::GetItemInfo (wpn_shared/enfield.cpp:79) link 00162fc0
int CENFIELD::GetItemInfo( ItemInfo *p )
{
    p->pszName = STRING( pev->classname );
    p->pszAmmo1 = "ammo_16mm";
    p->iMaxAmmo1 = P_WpnInfo[25].ammo_maxcarry;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iMaxClip = P_WpnInfo[25].ammo_maxclip;
    p->iSlot = 2;
    p->iPosition = 0;
    p->iFlags = 0x80;
    p->iId = m_iId = 25;
    p->iWeight = P_WpnInfo[25].misc_weight;
    return 1;
}
// .text:00163250 CENFIELD::AddToPlayer (wpn_shared/enfield.cpp:96) link 00163250
int CENFIELD::AddToPlayer( CBasePlayer *p )
{
	return CBasePlayerWeapon::AddToPlayer( p ) != 0;
}
// .text:00163490 CENFIELD::Deploy (wpn_shared/enfield.cpp:105) link 00163490
BOOL CENFIELD::Deploy()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
	if ( m_iWeaponState & 1 )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[38].anim_drawtime;
		return DefaultDeploy( P_WpnInfo[38].vmodel, P_WpnInfo[38].pmodel, 3, P_WpnInfo[38].szAnimExt,
							  P_WpnInfo[38].szAnimReloadExt, 0 );
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[25].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[25].vmodel, P_WpnInfo[25].pmodel, 3, P_WpnInfo[25].szAnimExt,
						  P_WpnInfo[25].szAnimReloadExt, 0 );
}
// .text:00163040 CENFIELD::CanHolster (wpn_shared/enfield.cpp:126) link 00163040
int CENFIELD::CanHolster()
{
	return 1;
}
// .text:00163580 CENFIELD::PrimaryAttack (wpn_shared/enfield.cpp:129) link 00163580 - 8.9KB, bolt-action, uses scoped spread when zoomed
void CENFIELD::PrimaryAttack()
{
	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;
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
			float flSpread = P_WpnInfo[25].base_accuracy;
			if ( m_iWeaponState & 1 )
			{
				flSpread = P_WpnInfo[38].base_accuracy;
				if ( GetFOV() > 0 )
					flSpread = P_WpnInfo[38].base_accuracy2;
			}
			if ( m_pPlayer->pev->velocity.Length() > 45.0f )
				flSpread += P_WpnInfo[25].accuracy_penalty;
			Vector vecSrc = m_pPlayer->GetGunPosition();
			Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, gpGlobals->v_forward, flSpread, 8192,
													  BULLET_DOD_ENFIELD, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			if ( ( m_iWeaponState & 1 ) == 0 )
			{
				PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireEnfield, 0, g_vecZero, g_vecZero,
									 vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ), 0 );
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[25].anim_firedelay;
			}
			else
			{
				int zoomed = GetFOV();
				PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireScopedEnfield, 0, g_vecZero, g_vecZero,
									 vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ), ( zoomed > 0 ) );
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[38].anim_firedelay;
			}
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[25].anim_firedelay;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
			m_fInAttack = 1;
			if ( ( m_iWeaponState & 1 ) && GetFOV() > 0 )
			{
				if ( m_iClip < 1 )
					SetThink( &CBasePlayerWeapon::ThinkZoomOut );
				else
					SetThink( &CENFIELD::ThinkZoomOutIn );
				pev->nextthink = gpGlobals->time + 0.5;
			}
			RemoveStamina( 2.0f, m_pPlayer );
		}
	}
}
// .text:001630e0 CENFIELD::SecondaryAttack (wpn_shared/enfield.cpp:228) link 001630e0 - melee if hip, zoom toggle if scoped
void CENFIELD::SecondaryAttack()
{
	if ( ( m_iWeaponState & 1 ) == 0 )
	{
		RifleMeleeAttack( m_pPlayer, 25 );
		m_pPlayer->SetAnimation( PLAYER_ATTACK2 );
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireEnfield, 0, g_vecZero, g_vecZero, 0, 0, 1, 0,
							 ( m_iClip == 0 ), 0 );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.1;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.1;
	}
	else
	{
		int fov = GetZoomFOVToggle( 20 );
		m_pPlayer->m_iFOV = fov;
		UpdateZoomSpeed();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.9;
	}
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2;
}
// .text:001633f0 CENFIELD::Reload (wpn_shared/enfield.cpp:252) link 001633f0
void CENFIELD::Reload()
{
	float flReloadTime = P_WpnInfo[25].anim_reloadtime;
	if ( m_iWeaponState & 1 )
	{
		SetThink( NULL );
		flReloadTime = P_WpnInfo[38].anim_reloadtime;
		if ( GetFOV() > 0 )
		{
			m_pPlayer->m_iFOV = GetZoomFOV();
			UpdateZoomSpeed();
			flReloadTime = P_WpnInfo[38].anim_reloadtime;
		}
	}
	DefaultReload( P_WpnInfo[25].ammo_maxclip, 2, flReloadTime );
}
// .text:00163300 CENFIELD::ThinkZoomOutIn (wpn_shared/enfield.cpp:270) link 00163300
void CENFIELD::ThinkZoomOutIn()
{
	if ( m_iWeaponState & 1 )
	{
		m_pPlayer->m_iFOV = GetZoomFOV();
		UpdateZoomSpeed();
		SetThink( &CBasePlayerWeapon::ThinkZoomIn );
		pev->nextthink = gpGlobals->time + 1.1;
	}
}
// .text:00163070 CENFIELD::Holster (wpn_shared/enfield.cpp:283) link 00163070
void CENFIELD::Holster( int i )
{
	if ( m_iWeaponState & 1 )
	{
		m_pPlayer->m_iFOV = GetZoomFOV();
		UpdateZoomSpeed();
	}
	SetThink( NULL );
	CBasePlayerWeapon::Holster( i );
}
// .text:00163370 CENFIELD::WeaponIdle (wpn_shared/enfield.cpp:296) link 00163370
void CENFIELD::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
	SendWeaponAnim( 0, 1 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
}
// .text:00163050 CENFIELD::Classify (wpn_shared/enfield.cpp:321) link 00163050
int CENFIELD::Classify()
{
	return ( m_iWeaponState & 1 ) ? CLASS_SCOPE_RIFLE : CLASS_RIFLE;
}
// .text:00163b70 CEnfieldAmmoClip::Spawn link 00163b70 Ghidra 00173b70
class CEnfieldAmmoClip : public CBasePlayerAmmo
{
  public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:00163b70 CEnfieldAmmoClip::Spawn (wpn_shared/enfield.cpp)
void CEnfieldAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:00163ba0 CEnfieldAmmoClip::AddAmmo (wpn_shared/enfield.cpp)
int CEnfieldAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult =
		( (CBasePlayer *) pOther )->GiveAmmo( P_WpnInfo[25].ammo_maxclip, "ammo_16mm", P_WpnInfo[25].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:00163ae0 ammo_enfield link 00163ae0 Ghidra 00173ae0
LINK_ENTITY_TO_CLASS( ammo_enfield, CEnfieldAmmoClip );

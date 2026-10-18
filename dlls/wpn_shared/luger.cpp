// Luger P08 - Axis pistol, CPistol child (dod/dlls/wpn_shared/luger.cpp)
// | Param | Value |
// | Name | weapon_luger / CLUGER / id 4, base CPistol |
// | Damage | 40 (plrDmgLUGER via BULLET_DOD_LUGER (7)) |
// | Mag | 8 / carry 24 |
// | FireDelay | 0.10 | Reload | 2.10 / 2.10 | Draw | 0.66 |
// | Spread | 0.035 +0.10 | Ani | pistol |
// | Models | p_luger / v_luger / w axis_ammo |
// | Ammo | ammo_12mm | Slot | 1/0 flags 0x40 weight 10 |
// | Sound | weapons/luger_shoot.wav | Event | events/weapons/luger.sc |
// | Secondary | empty |
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
// luger_e moved to dod_shared.h as LUGER_e (identical values 1:1) — local duplicate removed.
// .text:0016b220 weapon_luger (wpn_shared/luger.cpp:16)
LINK_ENTITY_TO_CLASS( weapon_luger, CLUGER );
// .text:0016ac90 CLUGER::Precache (wpn_shared/luger.cpp:29)
void CLUGER::Precache()
{
	PRECACHE_MODEL( "models/v_luger.mdl" );
	PRECACHE_MODEL( "models/axis_ammo.mdl" );
	PRECACHE_SOUND( "weapons/luger_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	m_usFireLuger = PRECACHE_EVENT( 1, "events/weapons/luger.sc" );
}
// .text:0016ae50 CLUGER::Spawn (wpn_shared/luger.cpp:20)
void CLUGER::Spawn()
{
	Precache();
	m_iId = 4;
	SET_MODEL( ENT( pev ), P_WpnInfo[4].wmodel );
	m_iDefaultAmmo = P_WpnInfo[4].ammo_default;
	FallInit();
}
// .text:0016acf0 CLUGER::GetItemInfo (wpn_shared/luger.cpp:41)
int CLUGER::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_12mm";
	p->iMaxAmmo1 = P_WpnInfo[4].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[4].ammo_maxclip;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0x40;
	p->iId = m_iId = 4;
	p->iWeight = P_WpnInfo[4].misc_weight;
	return 1;
}
// .text:0016af10 CLUGER::PrimaryAttack (wpn_shared/luger.cpp:81)
void CLUGER::PrimaryAttack()
{
	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}
	if ( m_fInAttack )
		return;
	if ( m_iClip < 1 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		m_fInAttack = 1;
		return;
	}
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_iClip--;
	float flSpread = P_WpnInfo[m_iId].base_accuracy;
	if ( m_pPlayer->pev->velocity.Length() > 45.0f )
		flSpread += P_WpnInfo[m_iId].accuracy_penalty;
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, ( Vector ) gpGlobals->v_forward, flSpread, 8192, BULLET_DOD_LUGER, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireLuger, 0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ), 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[4].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_fInAttack = 1;
	m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0, 15.0 ) + UTIL_WeaponTimeBase();
	RemoveStamina( 1.0, m_pPlayer );
}
// .text:0016add0 CLUGER::Deploy (wpn_shared/luger.cpp:58)
BOOL CLUGER::Deploy()
{
	ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[4].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[4].vmodel, P_WpnInfo[4].pmodel, LUGER_DRAW, P_WpnInfo[4].szAnimExt, P_WpnInfo[4].szAnimReloadExt, 0 );
}
// .text:0016ad90 CLUGER::Reload (wpn_shared/luger.cpp:131)
void CLUGER::Reload()
{
	int iAnim = 6 - ( m_iClip == 0 );
	DefaultReload( P_WpnInfo[4].ammo_maxclip, iAnim, P_WpnInfo[4].anim_reloadtime );
}
// .text:0016aea0 CLUGER::WeaponIdle (wpn_shared/luger.cpp:152)
void CLUGER::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( ( m_iClip < 1 ) << 3, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
	}
}
// .text:0016ad80 CLUGER::SecondaryAttack (wpn_shared/luger.cpp:136)
void CLUGER::SecondaryAttack() {}
// .text:0016ad70 CLUGER::CanHolster (wpn_shared/luger.cpp:136)
int CLUGER::CanHolster()
{
	return 1;
}
// .text:0016b330 CLugerAmmoClip::Spawn (wpn_shared/luger.cpp:160)
class CLugerAmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:0016b330 CLugerAmmoClip::Spawn (wpn_shared/luger.cpp)
void CLugerAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:0016b360 CLugerAmmoClip::AddAmmo (wpn_shared/luger.cpp)
int CLugerAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult = ( (CBasePlayer *)pOther )->GiveAmmo( P_WpnInfo[4].ammo_maxclip, "ammo_12mm", P_WpnInfo[4].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:0016b290 ammo_luger (wpn_shared/luger.cpp:180)
LINK_ENTITY_TO_CLASS( ammo_luger, CLugerAmmoClip );

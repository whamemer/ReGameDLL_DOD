// M1 Carbine - Allied semi carbine
// PARAMS: dmg 40 | clip 15 carry 165 | reload 3.14s | spread 0.019 +0.10 | delay 0.10s | snd carbine_shoot boltback boltforward | mdl v_m1carbine p_m1carb w_m1carb p_m1carb_l | evt events/weapons/m1carbine.sc | ammo ammo_66mm id 20 slot 2 weight 20 | BULLET_DOD_CARBINE (19) semi
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "dod_gamerules.h"

enum m1carbine_e
{
	M1CARB_IDLE1 = 0,
	M1CARB_RELOAD,
	M1CARB_DEPLOY,
	M1CARB_FIRE,
	M1CARB_FAST_DEPLOY
};
LINK_ENTITY_TO_CLASS( weapon_m1carbine, CM1Carbine );
// .text:0016bbb0 weapon_m1carbine (wpn_shared/m1carbine.cpp:20)
// .text:0016b3f0 CM1Carbine::Precache (wpn_shared/m1carbine.cpp:57)
void CM1Carbine::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[20].vmodel );
    PRECACHE_MODEL( P_WpnInfo[20].wmodel );
    PRECACHE_MODEL( P_WpnInfo[WEAPON_FOLDINGCARBINE].vmodel );
    PRECACHE_MODEL( P_WpnInfo[WEAPON_FOLDINGCARBINE].wmodel );

    PRECACHE_SOUND( "weapons/carbine_shoot.wav" );
    PRECACHE_SOUND( "weapons/tommy_draw_slideback.wav" );
    PRECACHE_SOUND( "weapons/tommy_reload_clipout.wav" );
    PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );

    m_usFireM1Carbine = PRECACHE_EVENT( 1, "events/weapons/m1carbine.sc" );
}
// .text:0016b480 CM1Carbine::GetItemInfo (wpn_shared/m1carbine.cpp:73)
int CM1Carbine::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_66mm";
	p->iMaxAmmo1 = P_WpnInfo[20].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[20].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 20;
	p->iWeight = P_WpnInfo[20].misc_weight;
	return 1;
}
// .text:0016b750 CM1Carbine::Spawn (wpn_shared/m1carbine.cpp:27)
void CM1Carbine::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[20].wmodel );
	m_iId = 20;
	m_iDefaultAmmo = P_WpnInfo[20].ammo_default;
	FallInit();
}
// .text:0016b820 CM1Carbine::AddToPlayer (wpn_shared/m1carbine.cpp:36)
int CM1Carbine::AddToPlayer( CBasePlayer *p )
{
	if ( CBasePlayerWeapon::AddToPlayer( p ) )
	{
		if ( g_pGameRules != NULL && ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesPara() )
			SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_FOLDINGCARBINE].wmodel );
		else
			SET_MODEL( ENT( pev ), P_WpnInfo[20].wmodel );
		return 1;
	}
	return 0;
}
// .text:0016b650 CM1Carbine::SpawnDeploy (wpn_shared/m1carbine.cpp:90)
BOOL CM1Carbine::SpawnDeploy()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
	if ( g_pGameRules != NULL && ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesPara() )
	{
		return TimedDeploy( P_WpnInfo[WEAPON_FOLDINGCARBINE].vmodel, P_WpnInfo[WEAPON_FOLDINGCARBINE].pmodel,
							M1CARB_DEPLOY, P_WpnInfo[WEAPON_FOLDINGCARBINE].szAnimExt,
							P_WpnInfo[WEAPON_FOLDINGCARBINE].szAnimReloadExt, 1.66, 1.66, ( UseDecrement() != 0 ) );
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	return DefaultDeploy( P_WpnInfo[20].vmodel, P_WpnInfo[20].pmodel, M1CARB_DEPLOY, P_WpnInfo[20].szAnimExt,
						  P_WpnInfo[20].szAnimReloadExt, 0 );
}
// .text:0016b560 CM1Carbine::Deploy (wpn_shared/m1carbine.cpp:114)
BOOL CM1Carbine::Deploy()
{
	m_pPlayer->m_iFOV = GetZoomFOV();
	UpdateZoomSpeed();
	if ( g_pGameRules != NULL && ( (CDoDTeamPlay *) g_pGameRules )->IsAlliesPara() )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
		return DefaultDeploy( P_WpnInfo[WEAPON_FOLDINGCARBINE].vmodel, P_WpnInfo[WEAPON_FOLDINGCARBINE].pmodel,
							  M1CARB_FAST_DEPLOY, P_WpnInfo[WEAPON_FOLDINGCARBINE].szAnimExt,
							  P_WpnInfo[WEAPON_FOLDINGCARBINE].szAnimReloadExt, 0 );
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	return DefaultDeploy( P_WpnInfo[20].vmodel, P_WpnInfo[20].pmodel, M1CARB_DEPLOY, P_WpnInfo[20].szAnimExt,
						  P_WpnInfo[20].szAnimReloadExt, 0 );
}
// .text:0016b500 CM1Carbine::CanHolster (wpn_shared/m1carbine.cpp:141)
int CM1Carbine::CanHolster()
{
	return 1;
}
// .text:0016b890 CM1Carbine::PrimaryAttack (wpn_shared/m1carbine.cpp:144)
void CM1Carbine::PrimaryAttack()
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
			Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, gpGlobals->v_forward, flSpread, 8192,
													  BULLET_DOD_CARBINE, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireM1Carbine, 0, g_vecZero, g_vecZero,
								 vecDir.x, vecDir.y, (int) m_pPlayer->pev->punchangle.x, 0, m_iClip, 0 );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[20].anim_firedelay;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.05;
			m_fInAttack = 1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
			RemoveStamina( 2.0f, m_pPlayer );
		}
	}
}
// .text:0016b510 CM1Carbine::SecondaryAttack (wpn_shared/m1carbine.cpp:203)
void CM1Carbine::SecondaryAttack()
{
	return;
}
// .text:0016b530 CM1Carbine::Reload (wpn_shared/m1carbine.cpp:206)
void CM1Carbine::Reload()
{
	DefaultReload( P_WpnInfo[20].ammo_maxclip, 1, P_WpnInfo[20].anim_reloadtime );
}
// .text:0016b7a0 CM1Carbine::WeaponIdle (wpn_shared/m1carbine.cpp:211)
void CM1Carbine::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
	SendWeaponAnim( 0, 1 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
}
// .text:0016b520 CM1Carbine::Classify (wpn_shared/m1carbine.cpp:224)
int CM1Carbine::Classify()
{
	return CLASS_RIFLE;
}
// .text:0016bcb0 CM1CarbineAmmoClip::Spawn link 0016bcb0 Ghidra 0017bcb0
class CM1CarbineAmmoClip : public CBasePlayerAmmo
{
  public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:0016bcb0 CM1CarbineAmmoClip::Spawn (wpn_shared/m1carbine.cpp)
void CM1CarbineAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:0016bce0 CM1CarbineAmmoClip::AddAmmo (wpn_shared/m1carbine.cpp)
int CM1CarbineAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult =
		( (CBasePlayer *) pOther )->GiveAmmo( P_WpnInfo[20].ammo_maxclip, "ammo_66mm", P_WpnInfo[20].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:0016bc20 ammo_m1carbine link 0016bc20 Ghidra 0017bc20
LINK_ENTITY_TO_CLASS( ammo_m1carbine, CM1CarbineAmmoClip );

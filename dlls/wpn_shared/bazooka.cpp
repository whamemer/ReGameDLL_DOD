// Bazooka M1 - Allied rocket launcher + CBazookaShell
// PARAMS: dmg 100 blast | clip 1 carry 5 | reload 3.0s | delay 1.0s draw 1.0s up 1.0s down 0.8s | slow walk while deployed ReSlow/UnSlow | snd ususebazooka gerbazooka rocketfire reloadgetrocket rocketin shovehome | mdl p_bazooka v_bazooka w_bazooka p_bazooka_l w_bazooka_rocket | evt events/weapons/bazooka.sc | ammo rockets id 29 slot 4? flags 0x282 weight 20
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "items.h"
#include "dod_gamerules.h"
extern DLL_GLOBAL CGameRules *g_pGameRules;
#include "skill.h"

// bazooka_e: server FIRE/RELOAD unused (code uses literals 6/7 == client
// RELOAD_AIMED/RELOAD_IDLE); IDLE/LOWER == client values 0/5 now resolve to
// dod_shared.h. DEPLOY=4 kept (unique name, client has RAISE=4 same value
// different name — no collision). RAISE=6 differs from client RAISE=4, so
// renamed to preserve server anim 6.
// bazooka_e: names collide with dod_shared.h BAZOOKA_e; code uses literals
// 1 DRAW, 4 RAISE, 5 LOWER, 6 RELOAD_AIMED, 7 RELOAD_IDLE.
LINK_ENTITY_TO_CLASS( weapon_bazooka, CBazooka );
LINK_ENTITY_TO_CLASS( shell_bazooka, CBazookaShell );
// .text:00171530 weapon_bazooka (wpn_shared/bazooka.cpp:40) link 00161530 Ghidra 00171530
// .text:00170c30 CBazooka::Spawn? check: link 00160c30? Actually CBazooka::Spawn 00160c30 (wpn_shared/bazooka.cpp:157) Ghidra 00170c30
void CBazooka::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[29].wmodel );
	m_iId = 29;
	m_iDefaultAmmo = P_WpnInfo[29].ammo_default;
	FallInit();
	m_iClip = P_WpnInfo[29].ammo_maxclip;
}
// CBazookaShell::Spawn/CreateRocket/RocketTouch/Precache - link Spawn 00160d00 size 0x1e8 Ghidra 00170d00, CreateRocket 00161610 size 0x15a Ghidra 00171610
void CBazookaShell::Spawn()
{
	Precache();
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/w_bazooka_rocket.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );
	pev->movetype = MOVETYPE_TOSS;
	UTIL_MakeVectors( pev->angles );
	pev->velocity = gpGlobals->v_forward * 1300.0f;
	pev->gravity = 0.1f;
	pev->dmg = gSkillData.plrDmgBazookaShell;
	pev->angles.x = -pev->angles.x - 180.0f;
	pev->flags |= 0x800000;
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav", 1.0, 0.5, 0, 100 );
	SetTouch( &CGrenade::RocketExplodeTouch );
	pev->classname = MAKE_STRING( "shell_bazooka" );
}
void CBazookaShell::Precache()
{
	PRECACHE_MODEL( "models/w_bazooka_rocket.mdl" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
}
CBazookaShell *CBazookaShell::CreateRocket( Vector vecSrc, Vector vecDir, CBaseEntity *pOwner )
// link 00161610 size 0x15a Ghidra 00171610: same as PIAT/Pschreck - origin+angles, Spawn, Touch, owner/team, director 135, StartTrail
{
	CBazookaShell *p = GetClassPtr( (CBazookaShell *) NULL );
	UTIL_SetOrigin( p->pev, vecSrc );
	p->pev->angles = vecDir;
	p->Spawn();
	p->SetTouch( &CBazookaShell::RocketTouch );
	p->pev->owner = ENT( pOwner->pev );
	p->pev->team = pOwner->pev->team;
	UTIL_SendDirectorMessage( p, NULL, 135 );
	PLAYBACK_EVENT_FULL( FEV_GLOBAL, p->edict(), ((CDoDTeamPlay *)g_pGameRules)->StartTrail(), 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );
	return p;
}
void CBazookaShell::RocketTouch( CBaseEntity *pOther )
// link 00160c90 size 0x6a Ghidra 00170c90: rocket1.wav + RocketExplodeTouch (matches PIAT/Pschreck pattern)
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav", 0.0, 0.0, 0x20, 100 );
	CGrenade::RocketExplodeTouch( pOther );
}
// .text:001709b0 CBazooka::Precache (wpn_shared/bazooka.cpp:173) link 001609b0 Ghidra 001709b0
void CBazooka::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[29].wmodel );
    PRECACHE_MODEL( P_WpnInfo[29].vmodel );

    PRECACHE_SOUND( "items/ammopickup.wav" );
    PRECACHE_SOUND( "weapons/rocketfire1.wav" );
    PRECACHE_SOUND( "weapons/glauncher.wav" );
    PRECACHE_SOUND( "weapons/bazookareloadgetrocket.wav" );
    PRECACHE_SOUND( "weapons/bazookareloadrocketin.wav" );
    PRECACHE_SOUND( "weapons/bazookareloadshovehome.wav" );

    m_usFireBazooka = PRECACHE_EVENT( 1, "events/weapons/bazooka.sc" );
}
// .text:00170a40 CBazooka::GetItemInfo (wpn_shared/bazooka.cpp:191) link 00160a40
int CBazooka::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "rockets";
	p->iMaxAmmo1 = P_WpnInfo[29].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 1;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x282;
	p->iId = m_iId = 29;
	p->iWeight = 20;
	return 1;
}
// .text:00170bc0 CBazooka::Deploy (wpn_shared/bazooka.cpp:208) link 00160bc0
BOOL CBazooka::Deploy()
{
	m_iWeaponState &= 1;
	return TimedDeploy( P_WpnInfo[29].vmodel, P_WpnInfo[29].pmodel, 1, P_WpnInfo[29].szAnimExt,
						P_WpnInfo[29].szAnimReloadExt, P_WpnInfo[29].anim_drawtime, P_WpnInfo[29].anim_drawtime,
						( UseDecrement() != 0 ) );
}
// .text:00170ac0 CBazooka::CanHolster (wpn_shared/bazooka.cpp:234) link 00160ac0
int CBazooka::CanHolster()
{
	return ( ( m_iWeaponState & 1 ) ^ 1 );
}
// .text:00170b10 CBazooka::Holster (wpn_shared/bazooka.cpp:238) link 00160b10
void CBazooka::Holster( int i )
{
	m_pPlayer->m_bBazookaDeployed = FALSE;
	m_pPlayer->CheckPlayerSpeed();
	m_iWeaponState &= ~3;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	CBasePlayerWeapon::Holster( i );
}
// .text:00171040 CBazooka::PrimaryAttack (wpn_shared/bazooka.cpp:260) link 00161040 - fires rocket, consumes clip, punchangle kick, event
void CBazooka::PrimaryAttack()
{
	if ( m_iClip < 1 )
	{
		if ( !m_fInAttack )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
			m_fInAttack = 1;
		}
		return;
	}

	if ( m_pPlayer->pev->waterlevel >= 2 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
		return;
	}

	if ( ( m_iWeaponState & 1 ) == 0 )
	{
		UTIL_ShowMessage( "#game_shoulder_bazooka", m_pPlayer, TRUE );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[29].anim_reloadtime;
		return;
	}

	m_pPlayer->m_iWeaponVolume = 1000;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16;
	CBazookaShell::CreateRocket( vecSrc, gpGlobals->v_forward, m_pPlayer );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireBazooka, 0, g_vecZero, g_vecZero, 0, 0, 0, 0,
						 ( m_iClip == 0 ), 0 );
	m_iClip--;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[29].anim_reloadtime;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
}
// .text:00171410 CBazooka::SecondaryAttack (wpn_shared/bazooka.cpp:311) link 00161410 - toggles Lower/Raise (shoulder)
void CBazooka::SecondaryAttack()
{
	if ( m_pPlayer->pev->waterlevel < 2 )
	{
		if ( ( m_iWeaponState & 1 ) == 0 )
		{
			Raise();
			return;
		}
	}
	else if ( ( m_iWeaponState & 1 ) == 0 )
	{
		return;
	}

	Lower();
}
// .text:00170ef0 CBazooka::WeaponIdle (wpn_shared/bazooka.cpp:332) link 00160ef0
void CBazooka::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
	if ( ( m_iWeaponState & 1 ) == 0 )
		SendWeaponAnim( 0 );
	else
		SendWeaponAnim( 2, 1 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.0;
}
// .text:00170f70 CBazooka::Reload (wpn_shared/bazooka.cpp:131) link 00160f70
void CBazooka::Reload()
{
	BOOL bResult = 0;
	if ( m_iClip <= 0 )
	{
		if ( m_iWeaponState & 1 )
			bResult = DefaultReload( P_WpnInfo[29].ammo_maxclip, 6, P_WpnInfo[29].anim_reloadtime );
		else
			bResult = DefaultReload( P_WpnInfo[29].ammo_maxclip, 7, P_WpnInfo[29].anim_reloadtime );
	}
	if ( bResult )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[29].anim_reloadtime;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[29].anim_reloadtime;
		if ( m_iWeaponState & 1 )
		{
			UnSlow();
			m_iWeaponState = 2;
		}
	}
}
// .text:00171770 CBazooka::Raise (wpn_shared/bazooka.cpp:352) link 00161770
void CBazooka::Raise()
{
	SendWeaponAnim( 4, 1 );
	ReSlow();
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[29].anim_down2uptime;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[29].anim_down2uptime;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[29].anim_down2uptime;
}
// .text:001717e0 CBazooka::Lower (wpn_shared/bazooka.cpp:365) link 001617e0
void CBazooka::Lower()
{
	SendWeaponAnim( 5, 1 );
	UnSlow();
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[29].anim_up2downtime;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[29].anim_up2downtime;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[29].anim_up2downtime;
}
// .text:00171870 CBazooka::UnSlow (wpn_shared/bazooka.cpp:376) link 00161870
void CBazooka::UnSlow()
{
	m_iWeaponState = 0;
	m_pPlayer->m_bBazookaDeployed = FALSE;
	m_pPlayer->CheckPlayerSpeed();
}
// .text:00170ae0 CBazooka::ReSlow (wpn_shared/bazooka.cpp:389) link 00160ae0
void CBazooka::ReSlow()
{
	m_iWeaponState = 1;
	m_pPlayer->m_bBazookaDeployed = TRUE;
	m_pPlayer->CheckPlayerSpeed();
}
// .text:00161980 CBazookaAmmo::Spawn link 00161980 Ghidra 00171980
class CBazookaAmmo : public CBasePlayerAmmo
{
  public:
	void Spawn();
	void Precache();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:00161980 CBazookaAmmo::Spawn (wpn_shared/bazooka.cpp)
void CBazookaAmmo::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_bazooka_rocket.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:00161950 CBazookaAmmo::Precache (wpn_shared/bazooka.cpp)
void CBazookaAmmo::Precache()
{
	PRECACHE_MODEL( "models/w_bazooka_rocket.mdl" );
	PRECACHE_SOUND( "items/ammopickup.wav" );
}
// .text:001619c0 CBazookaAmmo::AddAmmo (wpn_shared/bazooka.cpp)
int CBazookaAmmo::AddAmmo( CBaseEntity *pOther )
{
	int bResult = ( (CBasePlayer *) pOther )->GiveAmmo( 2, "rockets", 5 );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:001618a0 ammo_bazooka link 001618a0 Ghidra 001718a0
LINK_ENTITY_TO_CLASS( ammo_bazooka, CBazookaAmmo );

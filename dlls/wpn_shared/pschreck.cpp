// Panzerschreck - Axis rocket, CRocketWeapon (dod/dlls/wpn_shared/pschreck.cpp)
// | Param | Value |
// | Name | weapon_pschreck / CPschreck / id 30 (0x1e) |
// | Damage | 100 (plrDmgPschreckShell) gravity 0.1 velocity 1300 |
// | Mag | 1 / carry 5 | FireDelay | 1.5 | Reload | 3.0 / 3.5 |
// | Draw | 1.0 | UpDown | 1.0 / 0.8 | Ani | pschreck |
// | Models | p_pschreck v_panzerschreck w_pschreck + w_pschreck_rocket |
// | Ammo | rockets | Slot | 2/0 flags 0x282 weight 20 |
// | Event | events/weapons/pschreck.sc | Sound | rocket1.wav like bazooka |
// | Shoulder | same Raise Lower as PIAT bazooka |
// | Touch | rocket1.wav vs PIAT rocket2.wav, same explode |
// | Vs bazooka | id30 vs 29, same 100dmg, leftreload p_pschreck_l |
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
// .text:0016f5d0 weapon_pschreck (wpn_shared/pschreck.cpp:41)
LINK_ENTITY_TO_CLASS( weapon_pschreck, CPschreck )
// .text:0016f950 ammo_pschreck (wpn_shared/pschreck.cpp:443)
// .text:0016f640 shell_pschreck (wpn_shared/pschreck.cpp:47)
LINK_ENTITY_TO_CLASS( shell_pschreck, CPschreckShell )
// .text:0016ea50 CPschreck::Precache (wpn_shared/pschreck.cpp:116)
void CPschreck::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[30].wmodel );
    PRECACHE_MODEL( P_WpnInfo[30].vmodel );

    PRECACHE_SOUND( "items/ammopickup.wav" );
    PRECACHE_SOUND( "weapons/rocketfire1.wav" );
    PRECACHE_SOUND( "weapons/glauncher.wav" );

    m_usFirePschreck = PRECACHE_EVENT( 1, "events/weapons/pschreck.sc" );
}
// .text:0016ecb0 CPschreck::Spawn (wpn_shared/pschreck.cpp:164)
void CPschreck::Spawn()
{
	Precache();
	m_iId = 30;
	SET_MODEL( ENT( pev ), P_WpnInfo[30].wmodel );
	m_iDefaultAmmo = P_WpnInfo[30].ammo_default;
	FallInit();
	m_iClip = P_WpnInfo[30].ammo_maxclip;
}
// .text:0016eac0 CPschreck::GetItemInfo (wpn_shared/pschreck.cpp:194)
int CPschreck::GetItemInfo( ItemInfo *p )
{
    p->pszName = STRING( pev->classname );
    p->pszAmmo1 = "rockets";
    p->iMaxAmmo1 = P_WpnInfo[30].ammo_maxcarry;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iMaxClip = 1;
    p->iSlot = 2;
    p->iPosition = 0;
    p->iFlags = 0x282;
    p->iId = m_iId = 30;
    p->iWeight = 20;
    return 1;
}
// .text:0016f0d0 CPschreck::PrimaryAttack (wpn_shared/pschreck.cpp:262)
void CPschreck::PrimaryAttack( void )
{
    if( m_iClip < 1 )
    {
        if( !m_fInAttack )
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
            m_fInAttack = TRUE;
        }
    }
    else
    {
        if( m_pPlayer->pev->waterlevel > 1 )
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0f;
        }
        else if( m_iWeaponState & 1 )
        {
            m_pPlayer->m_iWeaponVolume = 1000;
            m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
            UTIL_MakeVectors( m_pPlayer->pev->v_angle );
            Vector vecSrc = m_pPlayer->GetGunPosition();
            Vector vecDir = m_pPlayer->pev->v_angle;
            CPschreckShell::CreateRocket( vecSrc, vecDir, m_pPlayer );
            PLAYBACK_EVENT_FULL( FEV_NOTHOST, ENT( m_pPlayer->pev ), m_usFirePschreck, 0.0f, g_vecZero, g_vecZero, 0, 0, 0, 0, m_iClip == 0, 0 );
            --m_iClip;

            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[30].anim_firedelay;
            m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.17f;
        }
        else
        {
            UTIL_ShowMessage( "#game_shoulder_pschreck", m_pPlayer, true );
            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[30].anim_firedelay;
        }
    }
}
// .text:0016f4a0 CPschreck::SecondaryAttack (wpn_shared/pschreck.cpp:312)
void CPschreck::SecondaryAttack( void )
{
    if( m_pPlayer->pev->waterlevel <= 1 )
    {
        if( !( m_iWeaponState & 1 ) )
        {
            Raise();
            return;
        }
    }
    else if( !( m_iWeaponState & 1 ) )
    {
        return;
    }

    Lower();
}
// .text:0016ed10 CPschreckShell::RocketTouch (wpn_shared/pschreck.cpp:110)
void CPschreckShell::RocketTouch( CBaseEntity *pOther )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav", 0.0, 0.0, 0x20, 100 );
	CGrenade::RocketExplodeTouch( pOther );
}

void CPschreck::Reload( void )
{
    BOOL bResult;

    if( m_iClip < 1 )
    {
        if( m_iWeaponState & 1 )
        {
            bResult = DefaultReload( P_WpnInfo[30].ammo_maxclip, 6, P_WpnInfo[30].anim_reloadtime );

            if( !bResult )
                return;
            
            m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[30].anim_reloadtime;
            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[30].anim_reloadtime;
        }
        else
        {
            bResult = DefaultReload( P_WpnInfo[30].ammo_maxclip, 7, P_WpnInfo[30].anim_reloadtime2 );

            if( !bResult )
                return;
            
            m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[30].anim_reloadtime2;
            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[30].anim_reloadtime2;
        }

        if( m_iWeaponState & 1 )
        {
            UnSlow();
            m_iWeaponState = 2;
        }
    }
}

BOOL CPschreck::Deploy( void )
{
    m_iWeaponState &= 1;
    float idleTime = P_WpnInfo[30].anim_drawtime;

    return TimedDeploy( P_WpnInfo[30].vmodel, P_WpnInfo[30].pmodel, 1, P_WpnInfo[30].szAnimExt, 
        P_WpnInfo[30].szAnimReloadExt, idleTime, idleTime, UseDecrement() != FALSE );
}

BOOL CPschreck::CanHolster( void )
{
    return ( m_iWeaponState & 1 ) ^ 1;
}

void CPschreck::Holster( int skiplocal )
{
    m_pPlayer->m_bBazookaDeployed = FALSE;
    m_pPlayer->CheckPlayerSpeed();
    m_iWeaponState &= ~( 1 | 2 );
    m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
    CBasePlayerWeapon::Holster( skiplocal );
}

void CPschreck::WeaponIdle( void )
{
    ResetEmptySound();

    if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
    {
        if( m_iWeaponState & 1 )
            SendWeaponAnim( 2, 1 );
        else
            SendWeaponAnim( 0 );
        
        m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.0f;
    }
}

void CPschreck::Raise( void )
{
    SendWeaponAnim( 4, 1 );
    ReSlow();

    m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[30].anim_down2uptime;
    m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[30].anim_down2uptime;
    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[30].anim_down2uptime;
}

void CPschreck::Lower( void )
{
    SendWeaponAnim( 5, 1 );
    UnSlow();

    m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[30].anim_up2downtime;
    m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[30].anim_up2downtime;
    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[30].anim_up2downtime;
}

void CPschreck::UnSlow( void )
{
    m_iWeaponState = 0;
    m_pPlayer->m_bBazookaDeployed = FALSE;
    m_pPlayer->CheckPlayerSpeed();
}

void CPschreck::ReSlow( void )
{
    m_iWeaponState = 1;
    m_pPlayer->m_bBazookaDeployed = TRUE;
    m_pPlayer->CheckPlayerSpeed();
}

class CPschreckAmmo : public CBasePlayerAmmo
{
    void Spawn( void )
    {
        Precache();
        SET_MODEL( ENT( pev ), "models/w_pschreck_rocket.mdl" );
        CBasePlayerAmmo::Spawn();
    }

    void Precache( void )
    {
        PRECACHE_MODEL( "models/w_pschreck_rocket.mdl" );
    }

    BOOL AddAmmo( CBaseEntity *pOther )
    {
        int bResult = ( pOther->GiveAmmo( 2, "rockets", 5 ) != -1 );
        if( bResult )
        {
            EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, 0, 100 );
        }
		return bResult;
    }
};

LINK_ENTITY_TO_CLASS( ammo_pschreck, CPschreckAmmo );

CPschreckShell *CPschreckShell::CreateRocket( Vector vecSrc, Vector vecDir, CBaseEntity *pOwner )
// link 0x16f6b0 size 0x15a Ghidra 0x17f6b0: origin+angles, Spawn, Touch=RocketTouch, owner/team, director 135, StartTrail event
{
	CPschreckShell *p = GetClassPtr( (CPschreckShell *)0 );
	UTIL_SetOrigin( p->pev, vecSrc );
	p->pev->angles = vecDir;
	p->Spawn();
	p->SetTouch( &CPschreckShell::RocketTouch );
	p->pev->owner = ENT( pOwner->pev );
	p->pev->team = pOwner->pev->team;
	UTIL_SendDirectorMessage( p, NULL, 135 );
	PLAYBACK_EVENT_FULL( FEV_GLOBAL, p->edict(), ((CDoDTeamPlay *)g_pGameRules)->StartTrail(), 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );
	return p;
}

void CPschreckShell::Spawn()
// link 0x16ed80 size 0x1bb Ghidra 0x17ed80: solid BBOX, model, size0, origin, TOSS, fwd*1300, grav 0.1, dmg, angles flip -x-180, snd, Touch=ExplodeTouch
{
	Precache();
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/w_pschreck_rocket.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );
	pev->movetype = MOVETYPE_TOSS;
	pev->velocity = gpGlobals->v_forward * 1300.0f;
	pev->gravity = 0.1f;
	pev->dmg = gSkillData.plrDmgPschreckShell;
	pev->angles.x = -pev->angles.x - 180.0f;
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav", 1.0, 0.5, 0, 100 );
	SetTouch( &CGrenade::RocketExplodeTouch );
	pev->classname = MAKE_STRING( "shell_pschreck" );
}

void CPschreckShell::Precache()
{
	PRECACHE_MODEL( "models/w_pschreck_rocket.mdl" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
}

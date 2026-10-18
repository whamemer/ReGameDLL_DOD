// PIAT - Brit spring launcher, CRocketWeapon (dod/dlls/wpn_shared/piat.cpp)
// Assigned to axis batch for bazooka comparison.
// | Param | Value |
// | Name | weapon_piat / CPIAT / id 31 (0x1f) |
// | Damage | 100 (plrDmgPiatShell) gravity 0.1 velocity 1300 |
// | Mag | 1 / carry 5 | FireDelay | 1.0 | Reload | 3.10 |
// | Draw | 1.0 | UpDown | 1.0 / 0.8 | Ani | piat |
// | Models | p_piat v_piat w_piat + w_piat_rocket shell_piat |
// | Ammo | rockets | Slot | 2/0 flags 0x282 weight 20 |
// | Event | events/weapons/piat.sc | Sound | rocket2.wav fire, rocketfire1 |
// | Shoulder | Secondary Raise Lower via weaponState bit1, m_bBazookaDeployed |
// | Touch | RocketTouch rocket2.wav + RocketExplodeTouch |
// | Vs bazooka | same 100dmg 1300vel 0.1grav, model sound event differ |
// | Vs pschreck | pschreck rocket1.wav, same ballistics |
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
// .text:0016dfb0 weapon_piat (wpn_shared/piat.cpp:42)
LINK_ENTITY_TO_CLASS( weapon_piat, CPIAT )
// .text:0016e330 ammo_piat (wpn_shared/piat.cpp:446)
// .text:0016e020 shell_piat (wpn_shared/piat.cpp:48)
LINK_ENTITY_TO_CLASS( shell_piat, CPIATShell )
// .text:0016d440 CPIAT::Precache (wpn_shared/piat.cpp:169)
void CPIAT::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[31].wmodel );
    PRECACHE_MODEL( P_WpnInfo[31].vmodel );

    PRECACHE_SOUND( "items/ammopickup.wav" );
    PRECACHE_SOUND( "weapons/rocketfire1.wav" );
    PRECACHE_SOUND( "weapons/glauncher.wav" );

    m_usFirePIAT = PRECACHE_EVENT( 1, "events/weapons/piat.sc" );
}
// .text:0016d6b0 CPIAT::Spawn (wpn_shared/piat.cpp:153)
void CPIAT::Spawn()
{
	Precache();
	m_iId = 31;
	SET_MODEL( ENT( pev ), P_WpnInfo[31].wmodel );
	m_iDefaultAmmo = P_WpnInfo[31].ammo_default;
	FallInit();
	m_iClip = P_WpnInfo[31].ammo_maxclip;
}
// .text:0016d4b0 CPIAT::GetItemInfo (wpn_shared/piat.cpp:183)
int CPIAT::GetItemInfo( ItemInfo *p )
{
    p->pszName = STRING( pev->classname );
    p->pszAmmo1 = "rockets";
    p->iMaxAmmo1 = P_WpnInfo[31].ammo_maxcarry;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iMaxClip = 1;
    p->iSlot = 2;
    p->iPosition = 0;
    p->iFlags = 0x282;
    p->iId = m_iId = 31;
    p->iWeight = 20;
    return 1;
}
// .text:0016d9e0 CPIAT::PrimaryAttack (wpn_shared/piat.cpp:259)
void CPIAT::PrimaryAttack( void )
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
            m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
            m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
            UTIL_MakeVectors( m_pPlayer->pev->v_angle );
            Vector vecSrc = m_pPlayer->GetGunPosition();
            Vector vecDir = m_pPlayer->pev->v_angle;
            CPIATShell::CreateRocket( vecSrc, vecDir, m_pPlayer );
            PLAYBACK_EVENT_FULL( FEV_NOTHOST, ENT( m_pPlayer->pev ), m_usFirePIAT, 0.0f, g_vecZero, g_vecZero, 0, 0, 0, 0, m_iClip == 0, 0 );
            --m_iClip;

            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[31].anim_firedelay;
            m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[31].anim_firedelay;
        }
        else
        {
            UTIL_ShowMessage( "#game_shoulder_piat", m_pPlayer, true );
            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[31].anim_firedelay;
        }
    }
}
// .text:0016de80 CPIAT::SecondaryAttack (wpn_shared/piat.cpp:309)
void CPIAT::SecondaryAttack( void )
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
// .text:0016d710 CPIATShell::RocketTouch (wpn_shared/piat.cpp:110)
void CPIATShell::RocketTouch( CBaseEntity *pOther )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "weapons/rocket2.wav", 0.0, 0.0, 0x20, 100 );
	CGrenade::RocketExplodeTouch( pOther );
}
// .text:0016e090 CPIATShell::CreateRocket link 0016e090 Ghidra 0017e090 size 0x15a: origin+angles, Spawn, Touch=RocketTouch, owner/team, director 135, StartTrail event
CPIATShell *CPIATShell::CreateRocket( Vector vecSrc, Vector vecDir, CBaseEntity *pOwner )
{
	CPIATShell *p = GetClassPtr( (CPIATShell *)0 );
	UTIL_SetOrigin( p->pev, vecSrc );
	p->pev->angles = vecDir;
	p->Spawn();
	p->SetTouch( &CPIATShell::RocketTouch );
	p->pev->owner = ENT( pOwner->pev );
	p->pev->team = pOwner->pev->team;
	UTIL_SendDirectorMessage( p, NULL, 135 );
	PLAYBACK_EVENT_FULL( FEV_GLOBAL, p->edict(), ((CDoDTeamPlay *)g_pGameRules)->StartTrail(), 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );
	return p;
}
// .text:0016d780 CPIATShell::Spawn link 0016d780 Ghidra 0017d780 size 0x1bb: solid BBOX, model, size0, origin, TOSS, fwd*1300, grav 0.1, dmg, angles flip -x-180, snd, Touch=ExplodeTouch
void CPIATShell::Spawn()
{
	Precache();
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/w_piat_rocket.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );
	pev->movetype = MOVETYPE_TOSS;
	pev->velocity = gpGlobals->v_forward * 1300.0f;
	pev->gravity = 0.1f;
	pev->dmg = gSkillData.plrDmgPiatShell;
	pev->angles.x = -pev->angles.x - 180.0f;
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "weapons/rocket2.wav", 1.0, 0.5, 0, 100 );
	SetTouch( &CGrenade::RocketExplodeTouch );
	pev->classname = MAKE_STRING( "shell_piat" );
}

class CPIATAmmo : public CBasePlayerAmmo
{

    void Spawn( void )
    {
        Precache();
        SET_MODEL( ENT( pev ), "models/w_piat_rocket.mdl" );
        CBasePlayerAmmo::Spawn();
    }

    void Precache( void )
    {
        PRECACHE_MODEL( "models/w_piat_rocket.mdl" );
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

LINK_ENTITY_TO_CLASS( ammo_piat, CPIATAmmo );

void CPIAT::Reload( void )
{
    BOOL bResult = FALSE;

    if( m_iClip < 1 )
    {
        if( m_iWeaponState & 1 )
            bResult = DefaultReload( P_WpnInfo[31].ammo_maxclip, 12, P_WpnInfo[31].anim_reloadtime );
        else
            bResult = DefaultReload( P_WpnInfo[31].ammo_maxclip, 11, P_WpnInfo[31].anim_reloadtime );

        if( bResult )
        {
            m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[31].anim_reloadtime;
            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[31].anim_reloadtime;

            if( m_iWeaponState & 1 )
            {
                UnSlow();
                m_iWeaponState = 2;
            }
        }
    }
}

BOOL CPIAT::Deploy( void )
{
    m_iWeaponState &= 1;
    float idleTime = P_WpnInfo[31].anim_drawtime;
    int iAnim = ( m_iClip != 1 ) + 2;

    return TimedDeploy( P_WpnInfo[31].vmodel, P_WpnInfo[31].pmodel, iAnim, P_WpnInfo[31].szAnimExt, 
        P_WpnInfo[31].szAnimReloadExt, idleTime, idleTime, ( UseDecrement() != 0 ) );
}

BOOL CPIAT::CanHolster( void )
{
    return ( m_iWeaponState & 1 ) ^ 1;
}

void CPIAT::Holster( int skiplocal )
{
    m_pPlayer->m_bBazookaDeployed = FALSE;
    m_pPlayer->CheckPlayerSpeed();
    m_iWeaponState &= ~( 1 | 2 );
    m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
    CBasePlayerWeapon::Holster( skiplocal );
}

void CPIAT::WeaponIdle( void )
{
    ResetEmptySound();

    if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
    {
        if( m_iWeaponState & 1 )
            SendWeaponAnim( ( m_iClip != 1 ) + 4, 1 );
        else
            SendWeaponAnim( m_iClip != 1, 1 );
        
        m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.0f;
    }
}

void CPIAT::Raise( void )
{
    SendWeaponAnim( ( m_iClip != 1 ) + 7, 1 );
    ReSlow();

    m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[31].anim_down2uptime;
    m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[31].anim_down2uptime;
    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[31].anim_down2uptime;
}

void CPIAT::Lower( void )
{
    SendWeaponAnim( ( m_iClip != 1 ) + 9, 1 );
    UnSlow();

    m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[31].anim_up2downtime;
    m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[31].anim_up2downtime;
    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[31].anim_up2downtime;
}

void CPIAT::UnSlow( void )
{
    m_iWeaponState = 0;
    m_pPlayer->m_bBazookaDeployed = FALSE;
    m_pPlayer->CheckPlayerSpeed();
}

void CPIAT::ReSlow( void )
{
    m_iWeaponState = 1;
    m_pPlayer->m_bBazookaDeployed = TRUE;
    m_pPlayer->CheckPlayerSpeed();
}

void CPIATShell::Precache()
{
	PRECACHE_MODEL( "models/w_piat_rocket.mdl" );
	PRECACHE_SOUND( "weapons/rocket2.wav" );
}

// FG42 - Axis auto rifle + bipod + scope variant (dod/dlls/wpn_shared/fg42.cpp)
// | Param | Value |
// | Name | weapon_fg42 + weapon_scopedfg42 / CFG42 / id 23 (0x17) |
// | Damage | 85 (plrDmgFG42 via BULLET_DOD_FG42 (21)) |
// | Mag | 20 / carry 180 / default 180 / give 20 |
// | FireDelay | 0.085 | Reload | 3.85 / 3.71 second |
// | Spread hip | 0.025 +0.10 | Deployed | 0.02 |
// | Ani | tommy / fg42 | Bipod | p_fg42bu p_fg42bd sr pr |
// | Models | p_fg42bu / v_fg42 / w_fg42 |
// | Ammo | ammo_55mm | Slot | 2/0 flags 0x80 weight 20 |
// | Event | events/weapons/fg42.sc | Sound | weapons/fg42_shoot.wav |
// | Secondary | scope zoom if scoped variant (FOV 20) else bipod deploy |
// | Note | scopedfg42 entity shares CFG42, m_iWeaponState bit1 = scoped |
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
// .text:00174700 weapon_fg42 (wpn_shared/fg42.cpp:42)
LINK_ENTITY_TO_CLASS( weapon_fg42, CFG42 )
// .text:00174770 weapon_scopedfg42 (wpn_shared/fg42.cpp:43)
LINK_ENTITY_TO_CLASS( weapon_scopedfg42, CFG42 )
// .text:00174800 ammo_fg42 (wpn_shared/fg42.cpp:348)
// .text:00173c30 CFG42::Precache (wpn_shared/fg42.cpp:67)
void CFG42::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[23].vmodel );
    PRECACHE_MODEL( P_WpnInfo[23].wmodel );
    PRECACHE_MODEL( P_WpnInfo[37].vmodel );
    PRECACHE_MODEL( P_WpnInfo[37].wmodel );

    PRECACHE_SOUND( "weapons/357_cock1.wav" );
    PRECACHE_SOUND( "weapons/fg42_shoot.wav" );

    m_usFireFG42 = PRECACHE_EVENT( 1, "events/weapons/fg42.sc" );
}
// .text:001740c0 CFG42::Spawn (wpn_shared/fg42.cpp:49)
void CFG42::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[23].wmodel );
	m_iId = 23;
	m_iDefaultAmmo = P_WpnInfo[23].ammo_default;
	FallInit();
}
// .text:00173cb0 CFG42::GetItemInfo (wpn_shared/fg42.cpp:80)
int CFG42::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_55mm";
	p->iMaxAmmo1 = P_WpnInfo[23].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[23].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 23;
	p->iWeight = P_WpnInfo[23].misc_weight;
	return 1;
}
// .text:001742f0 CFG42::PrimaryAttack (wpn_shared/fg42.cpp:110)
void CFG42::PrimaryAttack( void )
{
    float flSpread;
    if( PlayerIsWaterSniping() )
    {
        PlayEmptySound();
        m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
    }
    else if( m_iClip <= 0 )
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
        m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
        m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

        if( !( m_iWeaponState & 1 ) )
        {
            if( IsDeployed() )
                flSpread = P_WpnInfo[23].base_accuracy2;
            else
                flSpread = P_WpnInfo[23].base_accuracy;
        }
        else
        {
            if( m_pPlayer->m_iFOV > 0 )
                flSpread = 0.008f;
            else
                flSpread = P_WpnInfo[23].base_accuracy;
        }

        if( m_pPlayer->pev->velocity.Length() > 45.0f )
            flSpread += P_WpnInfo[23].accuracy_penalty;

        Vector vecSrc = m_pPlayer->GetGunPosition();
        Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, ( Vector ) gpGlobals->v_forward, flSpread, 8192.0f, BULLET_DOD_FG42, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
        --m_iClip;

        PLAYBACK_EVENT_FULL( FEV_NOTHOST, ENT( m_pPlayer->pev ), m_usFireFG42, 0.0f, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, m_iClip == 0, m_pPlayer->m_iFOV > 0 );

        if( m_pPlayer->m_iFOV <= 0 )
            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[23].anim_firedelay;
        else
            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[37].anim_firedelay;

        m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1f;
        m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0f, 15.0f ) + UTIL_WeaponTimeBase();

        if( m_pPlayer->m_iFOV > 0 && m_iClip <= 0 )
        {
            SetThink( &CBasePlayerWeapon::ThinkZoomOut );
            pev->nextthink = gpGlobals->time + 0.5f;
        }
        RemoveStamina( 2.0f, m_pPlayer );
    }
}
// .text:00173e10 CFG42::SecondaryAttack (wpn_shared/fg42.cpp:188)
void CFG42::SecondaryAttack( void )
{
    if( m_iWeaponState & 1 )
    {
        ChangeFOV( 20 );
        UpdateZoomSpeed();
        m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
        m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.2f;
        m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
        m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2f;
    }
    else
    {
        if( m_pPlayer->pev->iuser3 == 1 || m_pPlayer->pev->vuser1.x == 1.0f )
        {
            m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.2f;
            SendWeaponAnim( 8 );
        }
        else if( m_pPlayer->pev->iuser3 == 2 || m_pPlayer->pev->vuser1.x == 2.0f )
        {
            m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.98f;
            SendWeaponAnim( 2 );
        }
        m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.3f;
        m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.3f;
    }
}
// .text:001747e0 CFG42::IsDeployed (wpn_shared/fg42.cpp:322)
BOOL CFG42::IsDeployed( void )
{
    return (m_iWeaponState & 2);
}

class CFG42AmmoClip : public CBasePlayerAmmo
{

    void Spawn( void )
    {
        SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
        CBasePlayerAmmo::Spawn();
    }

    BOOL AddAmmo( CBaseEntity *pOther )
    {
		int bResult = ( pOther->GiveAmmo( P_WpnInfo[23].ammo_maxclip, "ammo_55mm", P_WpnInfo[23].ammo_maxcarry ) != -1 );
		if( bResult )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, 0, 100 );
		}
		return bResult;
    }
};

LINK_ENTITY_TO_CLASS( ammo_fg42, CFG42AmmoClip );

int CFG42::AddToPlayer( CBasePlayer *pPlayer )
{
    return CBasePlayerWeapon::AddToPlayer( pPlayer ) != 0;
}

BOOL CFG42::Deploy( void )
{
    ZoomOut();
    UpdateZoomSpeed();
    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.93f;

    if( !( m_iWeaponState & 1 ) )
        return DefaultDeploy( P_WpnInfo[23].vmodel, P_WpnInfo[23].pmodel, 2, P_WpnInfo[23].szAnimExt,
            P_WpnInfo[23].szAnimReloadExt, 0 );
    else
        return DefaultDeploy( P_WpnInfo[37].vmodel, P_WpnInfo[37].pmodel, 2, P_WpnInfo[37].szAnimExt,
            P_WpnInfo[37].szAnimReloadExt, 0 );
}

void CFG42::Reload( void )
{
    if( m_pPlayer->m_iFOV > 0 )
    {
        ZoomOut();
        UpdateZoomSpeed();
    }

    if( IsDeployed() )
        DefaultReload( P_WpnInfo[23].ammo_maxclip, 6, P_WpnInfo[23].anim_reloadtime );
    else
        DefaultReload( P_WpnInfo[23].ammo_maxclip, 1, P_WpnInfo[23].anim_reloadtime );
}

void CFG42::Holster( int skiplocal )
{
    if( IsDeployed() )
        CBasePlayerWeapon::Holster( skiplocal );
}

BOOL CFG42::CanHolster( void )
{
    return IsDeployed();
}

void CFG42::WeaponIdle( void )
{
    ResetEmptySound();

    if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
    {
        if( IsDeployed() )
            SendWeaponAnim( 5, 1 );
        else
            SendWeaponAnim( 0 );
        
        m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0f, 15.0f ) + UTIL_WeaponTimeBase();
    }
}

int CFG42::Classify( void )
{
    return m_iWeaponState & 1 ? CLASS_SCOPE_RIFLE : CLASS_MACHINEGUNS;
}


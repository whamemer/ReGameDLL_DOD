#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"


extern p_wpninfo_s P_WpnInfo[];

void CPistol::Spawn( int weapon_id )
// link 0x16e9b0 size 0x6d Ghidra 0x17e9b0: Precache, m_iId, SET_MODEL wmodel, ammo_default, FallInit (no crash guard; client guard omitted per binary arbiter)
{
    Precache();
    m_iId = weapon_id;
    SET_MODEL( ENT( pev ), P_WpnInfo[m_iId].wmodel );
    m_iDefaultAmmo = P_WpnInfo[m_iId].ammo_default;
    FallInit();
}

BOOL CPistol::Deploy( void )
{
    ZoomOut();
    UpdateZoomSpeed();

    m_pPlayer->CheckPlayerSpeed();

    float idleTime = P_WpnInfo[m_iId].anim_drawtime;
    int iAnim = GetDrawAnim();

    return TimedDeploy( P_WpnInfo[m_iId].vmodel, P_WpnInfo[m_iId].pmodel, iAnim, P_WpnInfo[m_iId].szAnimExt, P_WpnInfo[m_iId].szAnimReloadExt, idleTime, idleTime, ( UseDecrement() != 0 ) );
}

BOOL CPistol::CanHolster( void )
{
    return TRUE;
}

void CPistol::Reload( void )
{
    float fDelay = P_WpnInfo[m_iId].anim_reloadtime;
    int iAnim = GetReloadAnim();

    DefaultReload( P_WpnInfo[m_iId].ammo_maxclip, iAnim, fDelay );
}

void CPistol::PrimaryAttack( void )
{
    if( PlayerIsWaterSniping() )
    {
        PlayEmptySound();
        m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
    }
    else if( !m_fInAttack )
    {
        if( m_iClip <= 0 )
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
            m_fInAttack = TRUE;
        }
        else
        {
            m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
            --m_iClip;

            ItemInfo sz;
            GetItemInfo( &sz );
            int iBulletType = sz.iBulletId;

            float flSpread = P_WpnInfo[m_iId].base_accuracy;

            if( m_pPlayer->pev->velocity.Length() > 45.0f )
                flSpread += P_WpnInfo[m_iId].accuracy_penalty;

            Vector vecSrc = m_pPlayer->GetGunPosition();
            Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, ( Vector ) gpGlobals->v_forward, flSpread, 8192.0f, iBulletType, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );

            PLAYBACK_EVENT_FULL( FEV_NOTHOST, ENT( m_pPlayer->pev ), m_iFireEvent, 0.0f, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, m_iClip == 0, 0 );

            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[m_iId].anim_firedelay;
            m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + P_WpnInfo[m_iId].anim_firedelay;
            m_fInAttack = TRUE;
            m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f ) + UTIL_WeaponTimeBase();

            RemoveStamina( 1.0f, m_pPlayer );
        }
    }
}

void CPistol::WeaponIdle( void )
{
    ResetEmptySound();

    if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
    {
        int iAnim = GetIdleAnim();
        SendWeaponAnim( iAnim, 1 );
        m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.0f;
    }
}
// Scoped Kar98k - Axis sniper (dod/dlls/wpn_shared/scoped98k.cpp)
// | Param | Value |
// | Name | weapon_scopedkar / CScopedKar / id 6 |
// | Damage | 160 (plrDmgSCOPEDKAR via BULLET_DOD_SCOPEDKAR (9)) |
// | Mag | 5 / carry 65 |
// | FireDelay | 1.61 | Reload | 3.12 | Draw | 1.0 |
// | Spread hip | 0.06 | Scoped | 0.004 +0.10 move |
// | Ani | bolt | Models | p_k98s v_scoped98k w_scoped98k p_k98s_l |
// | Ammo | ammo_16mm | Slot | 2/0 flags 0x80 weight 20 |
// | Event | events/weapons/scopedkar.sc |
// | Zoom | Secondary FOV 20, ThinkZoomOutIn-ThinkZoomIn rezoom after shot |
// | Holster | unzoom, CanHolster blocks while zoomed |
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
// .text:001802e0 weapon_scopedkar (wpn_shared/scoped98k.cpp:28)
LINK_ENTITY_TO_CLASS( weapon_scopedkar, CScopedKar )
// .text:00180350 ammo_scopedkar (wpn_shared/scoped98k.cpp:256)
// .text:0017fa90 CScopedKar::Precache (wpn_shared/scoped98k.cpp:44)
void CScopedKar::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[6].vmodel );
    PRECACHE_MODEL( P_WpnInfo[6].wmodel );

    PRECACHE_SOUND( "weapons/kar_shoot.wav" );
    PRECACHE_SOUND( "weapons/boltforward.wav" );
    PRECACHE_SOUND( "weapons/boltback.wav" );
    PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );

    m_usFireScopedKar = PRECACHE_EVENT( 1, "events/weapons/scopedkar.sc" );
}
// .text:0017fe40 CScopedKar::Spawn (wpn_shared/scoped98k.cpp:34)
void CScopedKar::Spawn( void )
{
    Precache();
    SET_MODEL( ENT( pev ), P_WpnInfo[6].wmodel );
    m_iId = 6;
    m_iDefaultAmmo = P_WpnInfo[6].ammo_default;
    FallInit();
}
// .text:0017fb10 CScopedKar::GetItemInfo (wpn_shared/scoped98k.cpp:58)
int CScopedKar::GetItemInfo( ItemInfo *p )
{
    p->pszName = STRING( pev->classname );
    p->pszAmmo1 = "ammo_16mm";
    p->iMaxAmmo1 = P_WpnInfo[6].ammo_maxcarry;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iMaxClip = P_WpnInfo[6].ammo_maxclip;
    p->iSlot = 2;
    p->iPosition = 0;
    p->iFlags = 0x80;
    p->iId = m_iId = 6;
    p->iWeight = P_WpnInfo[6].misc_weight;
    return 1;
}
// .text:0017ff10 CScopedKar::PrimaryAttack (wpn_shared/scoped98k.cpp:98)
void CScopedKar::PrimaryAttack( void )
{
    float flSpread;

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
            m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
            --m_iClip;

            if( m_pPlayer->m_iFOV <= 0 )
                flSpread = P_WpnInfo[6].base_accuracy;
            else
                flSpread = P_WpnInfo[6].base_accuracy2;

            if( m_pPlayer->pev->velocity.Length() > 45.0f )
                flSpread += P_WpnInfo[6].accuracy_penalty;

            Vector vecSrc = m_pPlayer->GetGunPosition();
            Vector vecDir = m_pPlayer->FireBulletsNC( vecSrc, ( Vector ) gpGlobals->v_forward, flSpread, 8192.0f, BULLET_DOD_SCOPEDKAR, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );

            PLAYBACK_EVENT_FULL( FEV_NOTHOST, ENT( m_pPlayer->pev ), m_usFireScopedKar, 0.0f, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, m_iClip == 0, m_pPlayer->m_iFOV > 0 );

            m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[6].anim_firedelay;
            m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1f;
            m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[6].anim_firedelay;
            m_fInAttack = TRUE;
            m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0f, 15.0f ) + UTIL_WeaponTimeBase();

            if( m_pPlayer->m_iFOV > 0 )
            {
                if( m_iClip <= 0 )
                    SetThink( &CBasePlayerWeapon::ThinkZoomOut );
                else
                    SetThink( &CScopedKar::ThinkZoomOutIn );
                pev->nextthink = gpGlobals->time + 0.5f;
            }
            RemoveStamina( 2.0f, m_pPlayer );
        }
    }
}
// .text:0017fc20 CScopedKar::SecondaryAttack (wpn_shared/scoped98k.cpp:181)
void CScopedKar::SecondaryAttack( void )
{
    ChangeFOV( 20 );
    UpdateZoomSpeed();
    m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
    m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.2f;
    m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2f;
}
// .text:0017fba0 CScopedKar::ThinkZoomOutIn (wpn_shared/scoped98k.cpp:204)
void CScopedKar::ThinkZoomOutIn( void )
{
    ZoomOut();
    UpdateZoomSpeed();
    SetThink( &CBasePlayerWeapon::ThinkZoomIn );
    pev->nextthink = gpGlobals->time + 1.1f;
}
// .text:0017fca0 CScopedKar::Holster (wpn_shared/scoped98k.cpp:214)
void CScopedKar::Holster( int skiplocal )
{
    ZoomOut();
    UpdateZoomSpeed();
    SetThink( NULL );
    CBasePlayerWeapon::Holster( skiplocal );
}
// .text:0017fd10 CScopedKar::Reload (wpn_shared/scoped98k.cpp:190)
void CScopedKar::Reload( void )
{
    if( m_pPlayer->m_iFOV > 0 )
    {
        ZoomOut();
        UpdateZoomSpeed();
    }
    SetThink( NULL );

    DefaultReload( P_WpnInfo[6].ammo_maxclip, 2, P_WpnInfo[6].anim_reloadtime );
}

class CScopedKarAmmoClip : public CBasePlayerAmmo
{

    void Spawn( void )
    {
        SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
        CBasePlayerAmmo::Spawn();
    }

    BOOL AddAmmo( CBaseEntity *pOther )
    {
		int bResult = ( pOther->GiveAmmo( P_WpnInfo[6].ammo_maxclip, "ammo_22mm", P_WpnInfo[6].ammo_maxcarry ) != -1 );
		if( bResult )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, 0, 100 );
		}
		return bResult;
    }
};

LINK_ENTITY_TO_CLASS( ammo_scopedkar, CScopedKarAmmoClip )

int CScopedKar::AddToPlayer( CBasePlayer *pPlayer )
{
    return CBasePlayerWeapon::AddToPlayer( pPlayer ) != 0;
}

BOOL CScopedKar::Deploy( void )
{
    ZoomOut();
    UpdateZoomSpeed();
    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[6].anim_drawtime;
    return DefaultDeploy( P_WpnInfo[6].vmodel, P_WpnInfo[6].pmodel, 3, P_WpnInfo[6].szAnimExt, 
        P_WpnInfo[6].szAnimReloadExt, 0 );
}

void CScopedKar::WeaponIdle( void )
{
    ResetEmptySound();

    if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
    {
        SendWeaponAnim( 0 );
        m_flTimeWeaponIdle = RANDOM_FLOAT( 10.0f, 15.0f ) + UTIL_WeaponTimeBase();
    }
}


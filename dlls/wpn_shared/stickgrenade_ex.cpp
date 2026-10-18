// Stickgrenade_ex - Axis impact/extra (dod/dlls/wpn_shared/stickgrenade_ex.cpp)
// | Param | Value |
// | Name | weapon_stickgrenade_ex / CStickGrenadeEx / id 15 (0xf) |
// | Damage | 180 class |
// | Ammo | ammo_ggrensex max 1, clip -1 | Slot | 4/4 flags 0x18 weight 100 |
// | Models | same stick mdls | Ani | primstick / gren |
// | Draw | 0.75 | Diff vs base | pos4 weight100 max1, SetDmgTime, faster throw |
// | Funcs | Spawn Deploy DropGren StartThrow SetDmgTime CanDeploy CanHolster |
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
// .text:00173150 weapon_stickgrenade_ex (wpn_shared/stickgrenade_ex.cpp:19)
LINK_ENTITY_TO_CLASS( weapon_stickgrenade_ex, CStickGrenadeEx )
// .text:00172710 CStickGrenadeEx::Precache (wpn_shared/stickgrenade_ex.cpp:39)
void CStickGrenadeEx::Precache( void )
{
    PRECACHE_SOUND( "weapons/grenthrow.wav" );
}
// .text:00172b50 CStickGrenadeEx::Spawn (wpn_shared/stickgrenade_ex.cpp:23)
void CStickGrenadeEx::Spawn( void )
{
    Precache();
    m_iId = 15;
    SET_MODEL( ENT( pev ), P_WpnInfo[15].wmodel );
    pev->dmg = gSkillData.plrDmgStickGrenade;
    m_iDefaultAmmo = 1;
    FallInit();
}
// .text:00172720 CStickGrenadeEx::GetItemInfo (wpn_shared/stickgrenade_ex.cpp:43)
int CStickGrenadeEx::GetItemInfo( ItemInfo *p )
{
    p->pszName = STRING( pev->classname );
    p->pszAmmo1 = "ammo_ggrensex";
    p->iMaxAmmo1 = 1;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iMaxClip = -1;
    p->iSlot = 4;
    p->iPosition = 4;
    p->iFlags = 0x18;
    p->iId = m_iId = 15;
    p->iWeight = 100;
    return 1;
}
// .text:00173050 CStickGrenadeEx::PrimaryAttack (wpn_shared/stickgrenade_ex.cpp:92)
void CStickGrenadeEx::PrimaryAttack( void )
{
    StartThrow( FALSE );
}
// .text:00172f50 CStickGrenadeEx::SecondaryAttack (wpn_shared/stickgrenade_ex.cpp:97)
void CStickGrenadeEx::SecondaryAttack( void )
{
    StartThrow( TRUE );
}
// .text:001731c0 CStickGrenadeEx::StartThrow (wpn_shared/stickgrenade_ex.cpp:92)
void CStickGrenadeEx::StartThrow( BOOL bUnderhand )
{
    if( m_flTimeToExplode < gpGlobals->time )
        DropGren();

    if( m_flStartThrow == 0.0f && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
    {
        m_flReleaseThrow = 0.0f;
        m_flStartThrow = 1.0f;
        m_bUnderhand = bUnderhand;
        SendWeaponAnim( STICKGRENADE_EX_PINPULL, UseDecrement() != 0 );

        if( m_flTimeToExplode >= gpGlobals->time )
            m_flReleaseThrow = gpGlobals->time + 0.01f;
    }
}
// .text:001727d0 CStickGrenadeEx::SetDmgTime (wpn_shared/stickgrenade_ex.cpp:77)
void CStickGrenadeEx::SetDmgTime( float t )
{
	m_flTimeToExplode = t;
}

BOOL CStickGrenadeEx::Deploy( void )
{
    UTIL_SendDirectorMessage( m_pPlayer, this, 0x26 );
    m_pPlayer->m_iFOV = ZoomOut();
    m_flReleaseThrow = -1.0f;
    SendWeaponAnim( 1, UseDecrement() != 0 );
    return DefaultDeploy( P_WpnInfo[15].vmodel, P_WpnInfo[15].pmodel, STICKGRENADE_EX_DRAW,
                          P_WpnInfo[15].szAnimExt, P_WpnInfo[15].szAnimReloadExt, 0 );
}

BOOL CStickGrenadeEx::CanHolster( void )
{
    return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0;
}

void CStickGrenadeEx::Holster( int skiplocal )
{
}

void CStickGrenadeEx::DropGren( void )
{
    vec3_t angThrow, eyes, vecThrow, vecSrc;
    float time, friction, flVel;
    TraceResult tr;

    if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
    {
        SendWeaponAnim( STICKGRENADE_EX_THROW, UseDecrement() != 0 );

        angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

        if( angThrow.x < 0.0f )
            angThrow.x = angThrow.x * 0.89f - 10.0f;
        else
            angThrow.x = angThrow.x * 1.11 - 10.0f;

        flVel = ( 90.0f - angThrow.x ) * 4.0f;

        if( flVel > 600.0f )
            flVel = 600.0f;

        UTIL_MakeVectors( angThrow );

        eyes = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
        vecSrc = eyes + gpGlobals->v_forward * 16.0f;
        time = m_flTimeToExplode - gpGlobals->time;

        if( time < 0.0f )
            time = 0.0f;

        if( m_bUnderhand )
        {
            friction = 0.6f;
            vecSrc = eyes + gpGlobals->v_forward * 16.0f;
            vecThrow = gpGlobals->v_forward * 300.0f;
        }
        else
        {
            friction = 0.8f;
            float flPlayerSpeedBonus = DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );
            vecThrow = gpGlobals->v_forward * ( flPlayerSpeedBonus + flVel );
        }

        UTIL_TraceLine( eyes, vecSrc, ignore_monsters, m_pPlayer->pev->pContainingEntity, &tr );

        if( tr.flFraction < 1.0f )
            vecSrc = tr.vecEndPos;

        CGrenade::ShootPickup( m_pPlayer->pev, vecSrc, vecThrow, time, 2, friction );

        m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
        m_flStartThrow = 0.0f;
    }
}

void CStickGrenadeEx::WeaponIdle( void )
{
    if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
    {
        RetireWeapon();
        SetThink( &CBasePlayerItem::DestroyItem );
        pev->nextthink = gpGlobals->time + 0.1f;
        m_flStartThrow = 0.0f;
        return;
    }

    if( m_flTimeToExplode < gpGlobals->time )
        DropGren();

    if( m_flTimeWeaponIdle < gpGlobals->time )
    {
        if( m_flReleaseThrow == -1.0f )
        {
            m_flTimeWeaponIdle = gpGlobals->time + 1.0f;
            SendWeaponAnim( STICKGRENADE_EX_IDLE, UseDecrement() != 0 );
        }
    }

    if( m_flReleaseThrow <= gpGlobals->time )
    {
        if( m_flStartThrow == 1.0f )
        {
            if( m_flReleaseThrow != -1.0f && m_flTimeToExplode >= gpGlobals->time )
            {
                if( m_bUnderhand )
                    m_pPlayer->SetAnimation( PLAYER_ROLLGRENADE );
                else
                    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

                m_flStartThrow = 2.0f;
                m_flReleaseThrow = gpGlobals->time + 0.3f;
            }
        }
        else if( m_flStartThrow == 2.0f && m_flReleaseThrow != -1.0f && m_flTimeToExplode >= gpGlobals->time )
        {
            DropGren();
        }
    }
}

BOOL CStickGrenadeEx::CanDeploy( void )
{
    return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != FALSE;
}

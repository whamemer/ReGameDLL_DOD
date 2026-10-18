// Stickgrenade M24 - Axis frag (dod/dlls/wpn_shared/stickgrenade.cpp)
// | Param | Value |
// | Name | weapon_stickgrenade / CStickGrenade / id 14 (0xe) |
// | Damage | 180 (plrDmgStickGrenade) |
// | Ammo | ammo_ggrens max 5, clip -1 | Slot | 4/0 flags 0x18 weight 1 |
// | Models | p_stick v_stick w_stick |
// | Draw | 0.75 | Base | CDoDGrenade m_flStartThrow m_flReleaseThrow m_bUnderhand |
// | Primary | overhand throw | Secondary | underhand throw StartThrow |
// | Funcs | Spawn Deploy Holster DropGren StartThrow CanDeploy CanHolster |
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
// .text:001725e0 weapon_stickgrenade (wpn_shared/stickgrenade.cpp:20)
LINK_ENTITY_TO_CLASS( weapon_stickgrenade, CStickGrenade )
// .text:00171800 CStickGrenade::Precache (wpn_shared/stickgrenade.cpp:39)
void CStickGrenade::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[14].vmodel );
    PRECACHE_MODEL( P_WpnInfo[14].wmodel );

    PRECACHE_SOUND( "weapons/grenthrow.wav" );
}
// .text:00172170 CStickGrenade::Spawn (wpn_shared/stickgrenade.cpp:24)
void CStickGrenade::Spawn( void )
{
    Precache();
    m_iId = 14;
    SET_MODEL( ENT( pev ), P_WpnInfo[14].wmodel );
    m_iDefaultAmmo = 1;
    FallInit();
}
// .text:00171830 CStickGrenade::GetItemInfo (wpn_shared/stickgrenade.cpp:47)
int CStickGrenade::GetItemInfo( ItemInfo *p )
{
    p->pszName = STRING( pev->classname );
    p->pszAmmo1 = "ammo_ggrens";
    p->iMaxAmmo1 = 5;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iMaxClip = -1;
    p->iSlot = 4;
    p->iPosition = 0;
    p->iFlags = 0x18;
    p->iId = m_iId = 14;
    p->iWeight = 1;
    return 1;
}
// .text:00172520 CStickGrenade::PrimaryAttack (wpn_shared/stickgrenade.cpp:145)
void CStickGrenade::PrimaryAttack( void )
{
    StartThrow( FALSE );
}
// .text:00172460 CStickGrenade::SecondaryAttack (wpn_shared/stickgrenade.cpp:150)
void CStickGrenade::SecondaryAttack( void )
{
    StartThrow( TRUE );
}
// .text:00172650 CStickGrenade::StartThrow (wpn_shared/stickgrenade.cpp:155)
void CStickGrenade::StartThrow( BOOL bUnderhand )
{
    if( m_flStartThrow == 0.0f )
    {
        m_flReleaseThrow = 0.0f;
        m_flStartThrow = 1.0f;
        m_bUnderhand = bUnderhand;
        SendWeaponAnim( STICKGRENADE_PINPULL, UseDecrement() != 0 );
        m_flTimeWeaponIdle = gpGlobals->time + 0.1f;
    }
}
// .text:001721d0 CStickGrenade::DropGren (wpn_shared/stickgrenade.cpp:88)
void CStickGrenade::DropGren( void )
{
    vec3_t eyes, vecSrc;
    TraceResult tr;

    if( m_flStartThrow != 0.0f && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
    {
        SendWeaponAnim( STICKGRENADE_THROW, UseDecrement() != 0 );

        if( m_bUnderhand )
            m_pPlayer->SetAnimation( PLAYER_ROLLGRENADE );
        else
            m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

        eyes = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
        vecSrc = eyes + gpGlobals->v_forward * 16.0f;

        UTIL_TraceLine( eyes, vecSrc, ignore_monsters, m_pPlayer->pev->pContainingEntity, &tr );

		if( tr.flFraction < 1.0f )
            vecSrc = tr.vecEndPos;

        m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
        m_flStartThrow = 0.0f;
        m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
        m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
        m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
    }
}

BOOL CStickGrenade::Deploy( void )
{
    m_pPlayer->m_iFOV = ZoomOut();
    m_flReleaseThrow = -1.0f;
    SendWeaponAnim( STICKGRENADE_DRAW, UseDecrement() != 0 );
    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[14].anim_drawtime;
    return DefaultDeploy( P_WpnInfo[14].vmodel, P_WpnInfo[14].pmodel, 1, P_WpnInfo[14].szAnimExt,
                          P_WpnInfo[14].szAnimReloadExt, 0 );
}

BOOL CStickGrenade::CanHolster( void )
{
    return m_flStartThrow == 0.0f;
}

void CStickGrenade::Holster( int skiplocal )
{
    m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

    if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
    {
        SendWeaponAnim( STICKGRENADE_HOLSTER, UseDecrement() != 0 );
    }
    else
    {
        SetThink( &CBasePlayerItem::DestroyItem );
        pev->nextthink = gpGlobals->time + 0.1f;
    }
    EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM, 0, 100 );
}

void CStickGrenade::WeaponIdle( void )
{
	vec3_t eyes;
	float time;
	TraceResult tr;

	time = gpGlobals->time;

	if( m_flReleaseThrow == 0.0f )
		m_flReleaseThrow = time;

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		if( m_flReleaseThrow > 0.0f && m_flStartThrow == 0.0f && time > m_flTimeWeaponIdle )
		{
			m_flStartThrow = 0.0f;
			RetireWeapon();
		}
		return;
	}

	if( m_flTimeWeaponIdle > time )
		return;

	if( m_flTimeWeaponIdle != time && m_flStartThrow == 1.0f && m_flReleaseThrow != 0.0f )
	{
		m_flStartThrow = time + 0.5f;

		if( m_bUnderhand )
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		else
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if( angThrow.x >= 0.0f )
			angThrow.x = angThrow.x * 1.11f - 10.0f;
		else
			angThrow.x = angThrow.x * 0.89f - 10.0f;

		flVel = ( 90.0f - angThrow.x ) * 4.0f;

		if( flVel > 600.0f )
			flVel = 600.0f;

		UTIL_MakeVectors( angThrow );

		float flPlayerSpeedBonus = DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );
		vecThrow = gpGlobals->v_forward * ( flPlayerSpeedBonus + flVel );
		return;
	}

	if( time > m_flStartThrow && m_flStartThrow != 0.0f && m_flStartThrow != 1.0f )
	{
		angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if( angThrow.x < 0.0f )
			angThrow.x = angThrow.x * 0.89 - 10.0f;
		else
			angThrow.x = angThrow.x * 1.11 - 10.0f;

		UTIL_MakeVectors( angThrow );

		eyes = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
		vecSrc = eyes + gpGlobals->v_forward * 16.0f;

		if( m_bUnderhand )
		{
			vecSrc = eyes + gpGlobals->v_forward * 16.0f;
			vecThrow = gpGlobals->v_forward * 300.0f;
		}

		UTIL_TraceLine( eyes, vecSrc, ignore_monsters, m_pPlayer->pev->pContainingEntity, &tr );

		if( tr.flFraction < 1.0f )
			vecSrc = tr.vecEndPos;

		float friction = m_bUnderhand ? 0.6f : 0.8f;
		CGrenade::ShootPickup( m_pPlayer->pev, vecSrc, vecThrow, 5.0f, G_STICKGRENADE, friction );

		EMIT_SOUND( m_pPlayer->pev->pContainingEntity, CHAN_WEAPON, "weapons/grenthrow.wav", 0.8f, ATTN_NORM );
		SendWeaponAnim( STICKGRENADE_THROW, UseDecrement() != 0 );

		m_flStartThrow = 0.0f;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
		return;
	}

	if( m_flStartThrow == 0.0f )
	{
		if( m_flReleaseThrow < 0.0f )
		{
			m_flTimeWeaponIdle = time + RANDOM_FLOAT( 5.0f, 7.0f );

			SendWeaponAnim( STICKGRENADE_DRAW, UseDecrement() != 0 );
			return;
		}

		if( m_flReleaseThrow > 0.0f && time > m_flTimeWeaponIdle )
		{
			m_flTimeWeaponIdle = time + RANDOM_FLOAT( 5.0f, 7.0f );;

				SendWeaponAnim( STICKGRENADE_IDLE, UseDecrement() != 0 );
				m_flReleaseThrow = -1.0f;
		}
	}
}

BOOL CStickGrenade::CanDeploy( void )
{
    return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != FALSE;
}

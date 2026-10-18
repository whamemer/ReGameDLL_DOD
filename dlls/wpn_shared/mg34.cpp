// MG34 - Axis bipod MG, CBipodWeapon child (dod/dlls/wpn_shared/mg34.cpp)
// | Param | Value |
// | Name | weapon_mg34 / CMG34 / id 21 (0x15) |
// | Damage | 85 (plrDmgMG34 via BULLET_DOD_MG34 (17)) |
// | Mag | 75 / carry 450 / default 450 / give 75 |
// | FireDelay | 0.10 | Reload | 5.70 | Up2Down | 0.60 |
// | Spread hip | 0.20 | Deployed | 0.02 via GetBipodSpread |
// | Ani | mg / mg34 | Recoil | 15.0 | BulletId | 17 |
// | Models | p_mg34bu v_mg34 w_mg34 + bd sr pr variants |
// | Ammo | ammo_66mm | Slot | 2/0 flags 0x82 weight 40 |
// | Event | events/weapons/mg34.sc | Sound | mg34_shoot mgbolt mgoverheat |
// | Base | CBipodWeapon bipod.cpp: Primary Secondary Deploy Reload |
// | Base | CoolThink m_flWeaponHeat, Hint mg_overheat, IsInMGDeploy |
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
// .text:0016bf80 weapon_mg34 (wpn_shared/mg34.cpp:36)
LINK_ENTITY_TO_CLASS( weapon_mg34, CMG34 )
// .text:0016bff0 ammo_mg34 (wpn_shared/mg34.cpp:132)
// .text:0016bd70 CMG34::Precache (wpn_shared/mg34.cpp:48)
void CMG34::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[21].vmodel );
    PRECACHE_MODEL( P_WpnInfo[21].wmodel );

    PRECACHE_SOUND( "weapons/357_cock1.wav" );
    PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );
    PRECACHE_SOUND( "weapons/tommy_reload_clipout.wav" );
    PRECACHE_SOUND( "weapons/clampdown.wav" );
    PRECACHE_SOUND( "weapons/mgbolt.wav" );
    PRECACHE_SOUND( "weapons/mg34_shoot.wav" );

    m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/mg34.sc" );
}
// .text:0016bf60 CMG34::Spawn (wpn_shared/mg34.cpp:42)
void CMG34::Spawn()
{
	CBipodWeapon::Spawn( 21 );
}
// .text:0016be10 CMG34::GetItemInfo (wpn_shared/mg34.cpp:63)
int CMG34::GetItemInfo( ItemInfo *p )
{
    p->pszName = STRING( pev->classname );
    p->pszAmmo1 = "ammo_66mm";
    p->iMaxAmmo1 = P_WpnInfo[21].ammo_maxcarry;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iMaxClip = P_WpnInfo[21].ammo_maxclip;
    p->iSlot = 2;
    p->iPosition = 0;
    p->iFlags = 0x82;
    p->iId = m_iId = 21;
    p->iBulletId = BULLET_DOD_MG34;
    p->flRecoil = 15.0f;
    p->iWeight = P_WpnInfo[21].misc_weight;
    return 1;
}
// Bipod base shared with MG42:
// .text:00171f30 CBipodWeapon::PrimaryAttack (wpn_shared/bipod.cpp:58)
// .text:00171d20 CBipodWeapon::SecondaryAttack (wpn_shared/bipod.cpp:207)
// .text:00171c10 CBipodWeapon::CoolThink (wpn_shared/bipod.cpp:194)
// .text:00172750 CBipodWeapon::GetBipodSpread (wpn_shared/bipod.cpp:326)
// .text:00172730 CBipodWeapon::IsDeployed (wpn_shared/bipod.cpp:322)

class CMG34AmmoClip : public CBasePlayerAmmo
{

    void Spawn( void )
    {
        SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
        CBasePlayerAmmo::Spawn();
    }

    BOOL AddAmmo( CBaseEntity *pOther )
    {
		int bResult = ( pOther->GiveAmmo( P_WpnInfo[21].ammo_maxclip, "ammo_66mm", P_WpnInfo[21].ammo_maxcarry ) != -1 );
		if( bResult )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, 0, 100 );
		}
		return bResult;
    }
};

LINK_ENTITY_TO_CLASS( ammo_mg34, CMG34AmmoClip );

int CMG34::Classify( void )
{
    return CLASS_MACHINEGUNS;
}

int CMG34::GetReloadAnim( void )
{
    return 12;
}

int CMG34::GetDrawAnim( void )
{
    return GetDownToUpAnim();
}

int CMG34::GetIdleAnim( void )
{
    if( IsDeployed() )
        return (m_iClip < 1);
    else
        return (m_iClip < 1) + 6;
}

int CMG34::GetUpToDownAnim( void )
{
    return (m_iClip < 1) + 8;
}

int CMG34::GetDownToUpAnim( void )
{
    return (m_iClip < 1) + 2;
}


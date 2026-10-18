// MG42 - Axis bipod MG high capacity (dod/dlls/wpn_shared/mg42.cpp)
// | Param | Value |
// | Name | weapon_mg42 / CMG42 / id 17 (0x11) |
// | Damage | 85 (plrDmgMG42 via BULLET_DOD_MG42 (16)) |
// | Mag | 250 / carry 500 / default 500 / give 250 |
// | FireDelay | 0.10 | Reload | 6.50 | Up2Down | 0.60 |
// | Spread hip | 0.20 | Deployed | 0.025 |
// | Ani | mg / mg42 | Recoil | 15.0 | BulletId | 16 |
// | Models | p_mg42bu v_mg42 w_mg42 + bd sr pr |
// | Ammo | ammo_66mm_ger | Slot | 2/0 flags 0x882 weight 40 |
// | Event | events/weapons/mg42.sc |
// | Diff vs MG34 | mag 250 vs 75, deployed spread 0.025 vs 0.02, flags 0x882 |
// | Base | same CBipodWeapon overheat deploy as MG34 |
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
// .text:0016c320 weapon_mg42 (wpn_shared/mg42.cpp:36)
LINK_ENTITY_TO_CLASS( weapon_mg42, CMG42 )
// .text:0016c390 ammo_mg42 (wpn_shared/mg42.cpp:205)
// .text:0016c240 CMG42::Precache (wpn_shared/mg42.cpp:48)
void CMG42::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[17].vmodel );
    PRECACHE_MODEL( P_WpnInfo[17].wmodel );

    PRECACHE_SOUND( "weapons/weaponempty.wav" );
    PRECACHE_SOUND( "weapons/chainpull.wav" );
    PRECACHE_SOUND( "weapons/bulletchain.wav" );
    PRECACHE_SOUND( "weapons/clampdown.wav" );
    PRECACHE_SOUND( "weapons/mgbolt.wav" );
    PRECACHE_SOUND( "weapons/mg42_shoot.wav" );
    PRECACHE_SOUND( "weapons/tommy_draw_slideback.wav" );
    PRECACHE_SOUND( "weapons/mgoverheat.wav" );

    m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/mg42.sc" );
    CBipodWeapon::Precache();
}
// .text:0016c300 CMG42::Spawn (wpn_shared/mg42.cpp:42)
void CMG42::Spawn()
{
	CBipodWeapon::Spawn( 17 );
}
// .text:0016c130 CMG42::GetItemInfo (wpn_shared/mg42.cpp:67)
int CMG42::GetItemInfo( ItemInfo *p )
{
    p->pszName = STRING( pev->classname );
    p->pszAmmo1 = "ammo_66mm_ger";
    p->iMaxAmmo1 = P_WpnInfo[17].ammo_maxcarry;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iMaxClip = P_WpnInfo[17].ammo_maxclip;
    p->iSlot = 2;
    p->iPosition = 0;
    p->iFlags = 0x882;
    p->iId = m_iId = 17;
    p->iBulletId = BULLET_DOD_MG42;
    p->flRecoil = 15.0f;
    p->iWeight = P_WpnInfo[17].misc_weight;
    return 1;
}

class CMG42AmmoClip : public CBasePlayerAmmo
{

    void Spawn( void )
    {
        SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
        CBasePlayerAmmo::Spawn();
    }

    BOOL AddAmmo( CBaseEntity *pOther )
    {
		int bResult = ( pOther->GiveAmmo( P_WpnInfo[17].ammo_maxclip, "ammo_66mm_ger", P_WpnInfo[17].ammo_maxcarry ) != -1 );
		if( bResult )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, 0, 100 );
		}
		return bResult;
    }
};

LINK_ENTITY_TO_CLASS( ammo_mg42, CMG42AmmoClip );

int CMG42::Classify( void )
{
    return CLASS_MACHINEGUNS;
}

int CMG42::GetReloadAnim( void )
{
    return 58;
}

int CMG42::GetDrawAnim( void )
{
    return GetDownToUpAnim();
}

int CMG42::GetIdleAnim( void )
{
    return -1;
}

int CMG42::GetUpToDownAnim( void )
{
    return m_iClip < 9 ? 39 - m_iClip : 30;
}

int CMG42::GetDownToUpAnim( void )
{
    return m_iClip < 9 ? 29 - m_iClip : 20;
}


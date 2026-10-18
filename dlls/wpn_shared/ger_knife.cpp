// German knife - CMeleeWeapon child (dod/dlls/wpn_shared/ger_knife.cpp)
// | Param | Value |
// | Name | weapon_gerknife / CGerKnife / id 2 |
// | Damage | 60 slash/stab (plrDmgBayonet/Crowbar class) |
// | Mag | none -1 | Slot | 0/1 weight 0 |
// | Ani | knife | Models | p_paraknife v_paraknife w_paraknife |
// | Base | CMeleeWeapon in knife.cpp: Swing Stab Smack SwingAgain |
// | Sounds | knife_slash knife_hit knife_hitwall knifeswing |
// preserved .text markers from server:
// .text:00165340 weapon_gerknife (wpn_shared/ger_knife.cpp:20)
// .text:001652e0 CGerKnife::Precache (wpn_shared/ger_knife.cpp:29)
// .text:00165320 CGerKnife::Spawn (wpn_shared/ger_knife.cpp:24)
// .text:00165230 CGerKnife::GetItemInfo (wpn_shared/ger_knife.cpp:41)
// .text:001652c0 CGerKnife::GetDrawAnim (wpn_shared/ger_knife.cpp:74)
// .text:00165290 CGerKnife::GetSlashAnim (wpn_shared/ger_knife.cpp:74)
//
// ger_knife.cpp
//
// implementation of CGerKnife class
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

// KNIFE_e moved to dod_shared.h (identical) — local duplicate removed.
LINK_ENTITY_TO_CLASS( weapon_gerknife, CGerKnife )

void CGerKnife::Spawn( void )
{
	CMeleeWeapon::Spawn( 2 );
}

void CGerKnife::Precache( void )
{
	PRECACHE_MODEL( P_WpnInfo[2].vmodel );
	PRECACHE_MODEL( P_WpnInfo[2].wmodel );
	PRECACHE_MODEL( P_WpnInfo[35].vmodel );

	CMeleeWeapon::Precache();
}

int CGerKnife::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = 0;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = -1;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = m_iId = 2;
	p->iWeight = 0;
	return 1;
}

int CGerKnife::GetSlashAnim( int m_iSwing )
{
	if ( m_iSwing % KNIFE_SLASH2 == KNIFE_SLASH1 )
		return KNIFE_SLASH2;

	return KNIFE_SLASH1;
}

int CGerKnife::GetDrawAnim( void )
{
	return KNIFE_DRAW;
}

int CGerKnife::GetIdleAnim( void )
{
	return -1;
}
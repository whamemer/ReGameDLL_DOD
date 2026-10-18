// Spade shovel - CMeleeWeapon child (dod/dlls/wpn_shared/spade.cpp)
// | Param | Value |
// | Name | weapon_spade / CSpade / id 19 (0x13) |
// | Damage | 60 class melee |
// | Mag | none -1 | Slot | 0/2 weight 0 |
// | Ani | spade / knife reload | Models | p_spade v_spade w axis_ammo |
// | Base | CMeleeWeapon Swing/Stab, anim 5 reloadtime, 0.1 firedelay table |
// | Note | P_WpnInfo[19] ammo fields are dummy 300/150, unused by melee |
// preserved .text markers from server:
// .text:001705a0 weapon_spade (wpn_shared/spade.cpp:17)
// .text:00170550 CSpade::Precache (wpn_shared/spade.cpp:27)
// .text:00170580 CSpade::Spawn (wpn_shared/spade.cpp:21)
// .text:001704a0 CSpade::GetItemInfo (wpn_shared/spade.cpp:36)
//
// spade.cpp
//
// implementation of CSpade class
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

// KNIFE_e moved to dod_shared.h (identical) — local duplicate removed.
LINK_ENTITY_TO_CLASS( weapon_spade, CSpade )

void CSpade::Spawn( void )
{
	CMeleeWeapon::Spawn( 19 );
}

void CSpade::Precache( void )
{
	PRECACHE_MODEL( P_WpnInfo[19].vmodel );
	PRECACHE_MODEL( P_WpnInfo[19].wmodel );

	CMeleeWeapon::Precache();
}

int CSpade::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = 0;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = -1;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = m_iId = 19;
	p->iWeight = 0;
	return 1;
}

int CSpade::GetSlashAnim( int m_iSwing )
{
	if ( m_iSwing % KNIFE_SLASH2 == KNIFE_SLASH1 )
		return KNIFE_SLASH2;

	return KNIFE_SLASH1;
}

int CSpade::GetDrawAnim( void )
{
	return KNIFE_DRAW;
}

int CSpade::GetIdleAnim( void )
{
	return -1;
}
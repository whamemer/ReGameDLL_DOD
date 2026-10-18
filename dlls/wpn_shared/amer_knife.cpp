// AmerKnife Ka-Bar - Allied melee (CAmerKnife inherits CMeleeWeapon knife.cpp)
// PARAMS: dmg slash 60 stab 60 | no ammo | anims draw/slash/idle | snd knife_slash knife_hit knife_hitwall knifeswing | mdl p_amerk? v_amerk w_amerk (P_WpnInfo[1]) | evt events/weapons/melee.sc | id 1
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS( weapon_amerknife, CAmerKnife );
// .text:00160550 weapon_amerknife (wpn_shared/amer_knife.cpp:28) link 00160550 Ghidra 00170550
// .text:00160530 CAmerKnife::Spawn (wpn_shared/amer_knife.cpp:32) link 00160530
void CAmerKnife::Spawn()
{
	CMeleeWeapon::Spawn( 1 );
}
// .text:001604e0 CAmerKnife::Precache (wpn_shared/amer_knife.cpp:37) link 001604e0
void CAmerKnife::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[1].vmodel );
    PRECACHE_MODEL( P_WpnInfo[1].wmodel );
    PRECACHE_MODEL( P_WpnInfo[40].vmodel );
    PRECACHE_MODEL( P_WpnInfo[40].wmodel );

    CMeleeWeapon::Precache();
}
// .text:00160430 CAmerKnife::GetItemInfo (wpn_shared/amer_knife.cpp:48) link 00160430
int CAmerKnife::GetItemInfo( ItemInfo *p )
{
    p->pszName = STRING( pev->classname );
    p->pszAmmo1 = 0;
    p->iMaxAmmo1 = -1;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iMaxClip = -1;
    p->iSlot = 0;
    p->iPosition = 0;
    p->iId = m_iId = 1;
    p->iWeight = 0;
    return 1;
}
// .text:00160490 CAmerKnife::GetSlashAnim (wpn_shared/amer_knife.cpp:62) link 00160490
int CAmerKnife::GetSlashAnim( int stab )
{
	if ( stab % 2 == 1 )
		return 2;
	return 1;
}
// .text:001604c0 CAmerKnife::GetDrawAnim (wpn_shared/amer_knife.cpp:81) link 001604c0
int CAmerKnife::GetDrawAnim()
{
	return 3;
}
// .text:001604d0 CAmerKnife::GetIdleAnim (wpn_shared/amer_knife.cpp:86) link 001604d0
int CAmerKnife::GetIdleAnim()
{
	return -1;
}
// NOTE: Swing/Stab/Smack/Deploy/Idle in CMeleeWeapon base (knife.cpp) with dmg gSkillData.plrDmgBayonet=60. Overrides only models/anims.

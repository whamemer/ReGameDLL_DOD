// BAR M1918 - Allied bipod auto rifle
// PARAMS: dmg 85 | clip 20 carry 260 | reload 3.72s bipod 3.86s | spread 0.025 bipod 0.02 | delay 0.12s draw 1.5s updown 1.5s | snd bar_shoot | mdl v_bar w_bar p_barbu p_barbd | evt events/weapons/bar.sc | ammo ammo_55mm id 11 slot 3? weight 20 | BULLET_DOD_BAR (14) bipod CBipodWeapon
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum bar_e
{
	BAR_IDLE1 = 0,
	BAR_FIRE,
	BAR_RELOAD,
	BAR_DEPLOY = 5
};
LINK_ENTITY_TO_CLASS( weapon_bar, CBAR );
// .text:001607d0 weapon_bar (wpn_shared/bar.cpp:33)
// .text:001607b0 CBAR::Spawn (wpn_shared/bar.cpp:38)
void CBAR::Spawn()
{
	CBipodWeapon::Spawn( 11 );
}
// .text:00160610 CBAR::Precache (wpn_shared/bar.cpp:44)
void CBAR::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[11].vmodel );
    PRECACHE_MODEL( P_WpnInfo[11].wmodel );

    PRECACHE_SOUND( "weapons/357_cock1.wav" );
    PRECACHE_SOUND( "weapons/bar_shoot.wav" );

    m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/bar.sc" );
}
// .text:00160670 CBAR::GetItemInfo (wpn_shared/bar.cpp:55)
int CBAR::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_55mm";
	p->iMaxAmmo1 = P_WpnInfo[11].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[11].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 11;
	p->iWeight = P_WpnInfo[11].misc_weight;
	p->iBulletId = BULLET_DOD_BAR;
	p->flRecoil = 2.0;
	return 1;
}
// .text:00160700 CBAR::Classify (wpn_shared/bar.cpp:76)
int CBAR::Classify()
{
	return CLASS_MACHINEGUNS;
}
// .text:00160790 CBAR::GetReloadAnim (wpn_shared/bar.cpp:79)
int CBAR::GetReloadAnim( void )
{
    return !CBipodWeapon::IsDeployed() ? BAR_UP_RELOAD : BAR_DOWN_RELOAD;
}
// .text:00160710 CBAR::GetDrawAnim (wpn_shared/bar.cpp:89)
int CBAR::GetDrawAnim()
{
	return 2;
}
// .text:00160740 CBAR::GetIdleAnim (wpn_shared/bar.cpp:92)
int CBAR::GetIdleAnim( void )
{
    ResetEmptySound();

    if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
        return CBipodWeapon::IsDeployed() ? BAR_DOWN_IDLE : BAR_UP_IDLE;
    
    return -1;
}
// .text:00160720 CBAR::GetUpToDownAnim (wpn_shared/bar.cpp:108)
int CBAR::GetUpToDownAnim()
{
	return 4;
}
// .text:00160730 CBAR::GetDownToUpAnim (wpn_shared/bar.cpp:113)
int CBAR::GetDownToUpAnim()
{
	return 8;
}
// NOTE: fire/idle/reload/deploy via CBipodWeapon base (bipod.cpp) using P_WpnInfo[11], BULLET_DOD_BAR (14), event m_usFireBAR
// .text:001608c0 CBARAmmoClip::Spawn link 001608c0 Ghidra 001708c0
class CBARAmmoClip : public CBasePlayerAmmo
{
  public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:001608c0 CBARAmmoClip::Spawn (wpn_shared/bar.cpp)
void CBARAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:001608f0 CBARAmmoClip::AddAmmo (wpn_shared/bar.cpp)
int CBARAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult =
		( (CBasePlayer *) pOther )->GiveAmmo( P_WpnInfo[11].ammo_maxclip, "ammo_55mm", P_WpnInfo[11].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:00160840 ammo_bar link 00160840 Ghidra 00170840
LINK_ENTITY_TO_CLASS( ammo_bar, CBARAmmoClip );

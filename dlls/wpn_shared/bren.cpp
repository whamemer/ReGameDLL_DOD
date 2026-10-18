// Bren MkII - British bipod LMG
// PARAMS: dmg 85 | clip 30 carry 180 | reload 3.6s bipod 4.2s fire2 0.30s | spread 0.025 bipod 0.02 | delay 0.12s draw 1.0s up 1.29s down 1.57s | snd bren_shoot (Bweapons prefix bug) | mdl v_bren w_bren p_brenbu p_brenbd p_brensr p_brenbr p_bren_l | evt events/weapons/bren.sc | ammo ammo_55mm id 27 slot 3 weight 20 | BULLET_DOD_BREN (25)
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS( weapon_bren, CBREN );
// .text:001629c0 weapon_bren (wpn_shared/bren.cpp:34)
// .text:001629a0 CBREN::Spawn (wpn_shared/bren.cpp:40)
void CBREN::Spawn()
{
	CBipodWeapon::Spawn( 27 );
}
// .text:00162800 CBREN::Precache (wpn_shared/bren.cpp:45)
void CBREN::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[27].vmodel );
    PRECACHE_MODEL( P_WpnInfo[27].wmodel );

    PRECACHE_SOUND( "weapons/357_cock1.wav" );
    PRECACHE_SOUND( "weapons/bren_shoot.wav" );

    m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/bren.sc" );
}
// .text:00162860 CBREN::GetItemInfo (wpn_shared/bren.cpp:56)
int CBREN::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_55mm";
	p->iMaxAmmo1 = P_WpnInfo[27].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[27].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0x80;
	p->iId = m_iId = 27;
	p->iWeight = P_WpnInfo[27].misc_weight;
	p->iBulletId = BULLET_DOD_BREN;
	p->flRecoil = 2.0;
	return 1;
}
// .text:001628f0 CBREN::Classify (wpn_shared/bren.cpp:77)
int CBREN::Classify()
{
	return CLASS_MACHINEGUNS;
}
// .text:00162980 CBREN::GetReloadAnim (wpn_shared/bren.cpp:80)
int CBREN::GetReloadAnim( void )
{
    return !CBipodWeapon::IsDeployed() ? BREN_UP_RELOAD : BREN_DOWN_RELOAD;
}
// .text:00162900 CBREN::GetDrawAnim (wpn_shared/bren.cpp:90)
int CBREN::GetDrawAnim()
{
	return 2;
}
// .text:00162930 CBREN::GetIdleAnim (wpn_shared/bren.cpp:93)
int CBREN::GetIdleAnim( void )
{
    ResetEmptySound();

    if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
        return CBipodWeapon::IsDeployed() ? BREN_DOWN_IDLE : BREN_UP_IDLE;
    
    return -1;
}
// .text:00162910 CBREN::GetUpToDownAnim (wpn_shared/bren.cpp:110)
int CBREN::GetUpToDownAnim()
{
	return 4;
}
// .text:00162920 CBREN::GetDownToUpAnim (wpn_shared/bren.cpp:115)
int CBREN::GetDownToUpAnim()
{
	return 8;
}
// NOTE: FINDING sound string has stray B prefix in binary: Bweapons/bren_shoot.wav - engine precache will fail, silent shot on some builds
// .text:00162ab0 CBRENAmmoClip::Spawn link 00162ab0 Ghidra 00172ab0
class CBRENAmmoClip : public CBasePlayerAmmo
{
  public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:00162ab0 CBRENAmmoClip::Spawn (wpn_shared/bren.cpp)
void CBRENAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:00162ae0 CBRENAmmoClip::AddAmmo (wpn_shared/bren.cpp)
int CBRENAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult =
		( (CBasePlayer *) pOther )->GiveAmmo( P_WpnInfo[27].ammo_maxclip, "ammo_55mm", P_WpnInfo[27].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:00162a30 ammo_bren link 00162a30 Ghidra 00172a30
LINK_ENTITY_TO_CLASS( ammo_bren, CBRENAmmoClip );

// 30cal M1919 - Allied bipod HMG
// PARAMS: dmg 85 | clip 150 carry 300 | reload 5.0s | spread 0.20 bipod 0.02 | delay 0.10s | snd 30cal_shoot bulletchain mgbolt | mdl v_30cal w_30cal p_30cal p_30calsr p_30calpr | evt events/weapons/30cal.sc | ammo ammo_66mm_us id 18 slot 3 weight 40 | BULLET_DOD_30CAL (18) bipod
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS( weapon_30cal, C30CAL );
// .text:00160260 weapon_30cal (wpn_shared/30cal.cpp:39)
// .text:00160240 C30CAL::Spawn (wpn_shared/30cal.cpp:45)
void C30CAL::Spawn()
{
	CBipodWeapon::Spawn( 18 );
}
// .text:00160090 C30CAL::Precache (wpn_shared/30cal.cpp:51)
void C30CAL::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[18].vmodel );
    PRECACHE_MODEL( P_WpnInfo[18].wmodel );

    PRECACHE_SOUND( "weapons/357_cock1.wav" );
    PRECACHE_SOUND( "weapons/chainpull.wav" );
    PRECACHE_SOUND( "weapons/bulletchain.wav" );
    PRECACHE_SOUND( "weapons/clampdown.wav" );
    PRECACHE_SOUND( "weapons/mgbolt.wav" );
    PRECACHE_SOUND( "weapons/30cal_shoot.wav" );
    PRECACHE_SOUND( "weapons/tommy_draw_slideback.wav" );

    m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/30cal.sc" );
}
// .text:00160130 C30CAL::GetItemInfo (wpn_shared/30cal.cpp:70)
int C30CAL::GetItemInfo( ItemInfo *p )
{
    p->pszName = STRING( pev->classname );
    p->pszAmmo1 = "ammo_66mm_us";
    p->iMaxAmmo1 = P_WpnInfo[18].ammo_maxcarry;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iMaxClip = P_WpnInfo[18].ammo_maxclip;
    p->iSlot = 2;
    p->iPosition = 0;
    p->iFlags = 0x82;
    p->iId = m_iId = 18;
    p->iBulletId = BULLET_DOD_30CAL;
    p->flRecoil = 15.0f;
    p->iWeight = P_WpnInfo[18].misc_weight;
    return 1;
}
// .text:001601c0 C30CAL::Classify (wpn_shared/30cal.cpp:90)
int C30CAL::Classify()
{
	return CLASS_MACHINEGUNS;
}
// .text:001601d0 C30CAL::GetReloadAnim (wpn_shared/30cal.cpp:95)
int C30CAL::GetReloadAnim()
{
	return 58;
}
// .text:001601e0 C30CAL::GetDrawAnim (wpn_shared/30cal.cpp:100)
int C30CAL::GetDrawAnim()
{
	return 20;
}
// .text:001601f0 C30CAL::GetIdleAnim (wpn_shared/30cal.cpp:105)
int C30CAL::GetIdleAnim()
{
	return -1;
}
// .text:00160200 C30CAL::GetUpToDownAnim (wpn_shared/30cal.cpp:111)
int C30CAL::GetUpToDownAnim()
{
	return m_iClip < 9 ? 39 - m_iClip : 30;
}
// .text:00160220 C30CAL::GetDownToUpAnim (wpn_shared/30cal.cpp:152)
int C30CAL::GetDownToUpAnim()
{
	return m_iClip < 9 ? 29 - m_iClip : 20;
}
// NOTE: fire logic in CBipodWeapon::PrimaryAttack with BULLET_DOD_30CAL (18), overheat CoolThink
// .text:00160370 C30CALAmmoClip::Spawn link 00160370 Ghidra 00170370
class C30CALAmmoClip : public CBasePlayerAmmo
{
  public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:00160370 C30CALAmmoClip::Spawn (wpn_shared/30cal.cpp)
void C30CALAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:001603a0 C30CALAmmoClip::AddAmmo (wpn_shared/30cal.cpp)
int C30CALAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult =
		( (CBasePlayer *) pOther )->GiveAmmo( P_WpnInfo[18].ammo_maxclip, "ammo_66mm_us", P_WpnInfo[18].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:001602d0 ammo_30cal link 001602d0 Ghidra 001702d0
LINK_ENTITY_TO_CLASS( ammo_30cal, C30CALAmmoClip );

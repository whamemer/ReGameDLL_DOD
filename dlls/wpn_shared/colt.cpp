// Colt M1911 - Allied pistol (CCOLT inherits CPistol base pistol.cpp)
// PARAMS: dmg 40 | clip 7 carry 21 | reload 2.75s | spread 0.035 | delay 0.10s draw 0.68s | snd colt_shoot colt_reload_clipin colt_reload_clipout colt_draw_pullslide | mdl p_colt v_colt w allied_ammo | evt events/weapons/colt.sc | ammo ammo_12mm id 3 slot 1 weight 10 | BULLET_DOD_COLT (6)
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS( weapon_colt, CCOLT );
// .text:00162d00 weapon_colt (wpn_shared/colt.cpp:14) link 00162d00 Ghidra 00172d00
// .text:00162ce0 CCOLT::Spawn (wpn_shared/colt.cpp:21) link 00162ce0 Ghidra 00172ce0
void CCOLT::Spawn()
{
	CPistol::Spawn( 3 );
}
// .text:00162b70 CCOLT::Precache (wpn_shared/colt.cpp:26) link 00162b70
void CCOLT::Precache( void )
{
    PRECACHE_MODEL( P_WpnInfo[3].vmodel );
    PRECACHE_MODEL( P_WpnInfo[3].wmodel );

    PRECACHE_SOUND( "weapons/colt_shoot.wav" );
    PRECACHE_SOUND( "weapons/357_cock1.wav" );
    PRECACHE_SOUND( "weapons/colt_reload_clipin.wav" );
    PRECACHE_SOUND( "weapons/colt_reload_clipout.wav" );
    PRECACHE_SOUND( "weapons/colt_draw_pullslide.wav" );

    m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/colt.sc" );
}
// .text:00162c00 CCOLT::GetItemInfo (wpn_shared/colt.cpp:40) link 00162c00
int CCOLT::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_12mm";
	p->iMaxAmmo1 = P_WpnInfo[3].ammo_maxcarry;
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[3].ammo_maxclip;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0x40;
	p->iId = m_iId = 3;
	p->iWeight = P_WpnInfo[3].misc_weight;
	p->iBulletId = BULLET_DOD_COLT;
	p->flSpread = 0.035;
	return 1;
}
// .text:00162c90 CCOLT::GetReloadAnim (wpn_shared/colt.cpp:61) link 00162c90
int CCOLT::GetReloadAnim()
{
	return m_iClip == 0 ? 3 : 4;
}
// .text:00162cb0 CCOLT::GetDrawAnim (wpn_shared/colt.cpp:69) link 00162cb0
int CCOLT::GetDrawAnim()
{
	return 5;
}
// .text:00162cc0 CCOLT::GetIdleAnim (wpn_shared/colt.cpp:74) link 00162cc0
int CCOLT::GetIdleAnim()
{
	return m_iClip == 0 ? 7 : 0;
}
// NOTE: fire/reload/deploy/idle in CPistol::PrimaryAttack etc (pistol.cpp, not in scope) with BULLET_DOD_COLT (6), P_WpnInfo[3], event m_iFireEvent. CCOLT only overrides models/anims.
// .text:00162e30 CColtAmmoClip::Spawn link 00162e30 Ghidra 00172e30
class CColtAmmoClip : public CBasePlayerAmmo
{
  public:
	void Spawn();
	int AddAmmo( CBaseEntity *pOther );
};
// .text:00162e30 CColtAmmoClip::Spawn (wpn_shared/colt.cpp)
void CColtAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}
// .text:00162e60 CColtAmmoClip::AddAmmo (wpn_shared/colt.cpp)
int CColtAmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult =
		( (CBasePlayer *) pOther )->GiveAmmo( P_WpnInfo[3].ammo_maxclip, "ammo_12mm", P_WpnInfo[3].ammo_maxcarry );
	if ( bResult != -1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, 100 );
	return bResult != -1;
}
// .text:00162d70 ammo_colt link 00162d70 Ghidra 00172d70
LINK_ENTITY_TO_CLASS( ammo_colt, CColtAmmoClip );

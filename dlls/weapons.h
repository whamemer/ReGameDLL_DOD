/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef WEAPONS_H
#define WEAPONS_H

#include "effects.h"
#include "dod_shared.h"

// DoD weapon ids 1-31, slots 0/1/2/4 (restore/weapons/weapons.h + NOTES.md)
// 1 amerknife 0 | 2 gerknife 0 | 3 colt 1 | 4 luger 1 | 5 garand 2 | 6 scopedkar 2
// 7 thompson 2 | 8 mp44 2 | 9 spring 2 | 10 kar 2 | 11 bar 2 | 12 mp40 2
// 13 handgrenade 4 | 14 stickgrenade 4 | 15 stickgrenade_ex 4 | 16 handgrenade_ex 4
// 17 mg42 2 | 18 30cal 2 | 19 spade 0 | 20 m1carbine 2 | 21 mg34 2 | 22 greasegun 2
// 23 fg42 2 | 24 k43 2 | 25 enfield 2 | 26 sten 2 | 27 bren 2 | 28 webley 1
// 29 bazooka 2 | 30 pschreck 2 | 31 piat 2 ; scoped variants share IDs
enum DoDWeaponId
{
	WEAPON_NONE = 0,
	WEAPON_AMERKNIFE = 1,
	WEAPON_GERKNIFE = 2,
	WEAPON_COLT = 3,
	WEAPON_LUGER = 4,
	WEAPON_GARAND = 5,
	WEAPON_SCOPEDKAR = 6,
	WEAPON_THOMPSON = 7,
	WEAPON_MP44 = 8,
	WEAPON_SPRING = 9,
	WEAPON_KAR = 10,
	WEAPON_BAR = 11,
	WEAPON_MP40 = 12,
	WEAPON_HANDGRENADE = 13,
	WEAPON_STICKGRENADE = 14,
	WEAPON_STICKGRENADE_EX = 15,
	WEAPON_HANDGRENADE_EX = 16,
	WEAPON_MG42 = 17,
	WEAPON_30CAL = 18,
	WEAPON_SPADE = 19,
	WEAPON_M1CARBINE = 20,
	WEAPON_MG34 = 21,
	WEAPON_GREASEGUN = 22,
	WEAPON_FG42 = 23,
	WEAPON_K43 = 24,
	WEAPON_ENFIELD = 25,
	WEAPON_STEN = 26,
	WEAPON_BREN = 27,
	WEAPON_WEBLEY = 28,
	WEAPON_BAZOOKA = 29,
	WEAPON_PSCHRECK = 30,
	WEAPON_PIAT = 31,
	WEAPON_MORTAR = 32,
	WEAPON_BINOC = 33,
	WEAPON_BINOCULARS = 34,
	WEAPON_GERPARAKNIFE = 35,
	WEAPON_MILLSGRENADE = 36,
	WEAPON_SCOPED_FG42 = 37,
	WEAPON_SCOPEDENFIELD = 38,
	WEAPON_FOLDINGCARBINE = 39,
	WEAPON_FAIRBAIRN = 40,
};
// NOTE: the former separate DoDBulletId enum (BULLET_DOD_*) has been merged into the
// single `Bullet` enum below (the binary has ONE anonymous Bullet enum; BULLET_DOD_*
// are server-code aliases with identical numeric values). See `Bullet`.
#define WEAPON_ALLWEAPONS ( ~( 1 << 31 ) )
#define SLOT_MELEE 0
#define SLOT_PISTOL 1
#define SLOT_PRIMARY 2
#define SLOT_GRENADE 4
#undef MAX_WEAPONS
#define MAX_WEAPONS 64
#undef MAX_AMMO_SLOTS
#define MAX_AMMO_SLOTS 64
#define MAX_ITEM_SLOTS MAX_ITEM_TYPES

class CBasePlayer;
extern int gmsgWeapPickup;

void DeactivateSatchels( CBasePlayer *pOwner );

// DoD grenade type (weapons.h:27, DWARF CGrenade::g_GrenType)
typedef enum
{
	G_MILLSGRENADE = 0,
	G_HANDGRENADE,
	G_STICKGRENADE
} grenType;

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseMonster
{
  public:
	grenType g_GrenType;   // DWARF offset 940
	int m_flClipTime;	   // DWARF offset 944
	int m_iOldMoveType;	   // DWARF offset 948
	int m_iBodyGibs;	   // DWARF offset 952
	BOOL m_fRegisteredSound;  // whether or not this grenade has issued its DANGER sound to the world sound list yet. (offset 956)

	typedef enum
	{
		SATCHEL_DETONATE = 0,
		SATCHEL_RELEASE
	} SATCHELCODE;

	static CGrenade *ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time );
	static CGrenade *ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	static CGrenade *ShootSatchelCharge( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void UseSatchelCharges( entvars_t *pevOwner, SATCHELCODE code );
	static CGrenade *ShootPickup( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, int grenType,
								  float fDelay );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual int ObjectCaps( void ) { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void Explode( Vector vecSrc, Vector vecAim );
	virtual void Explode( TraceResult *pTrace, int bitsDamageType );
	void EXPORT Smoke( void );

	void EXPORT BounceTouch( CBaseEntity *pOther );
	void EXPORT SlideTouch( CBaseEntity *pOther );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );
	void EXPORT DangerSoundThink( void );
	void EXPORT PreDetonate( void );
	void EXPORT Detonate( void );
	void EXPORT DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TumbleThink( void );
	void EXPORT RemoveMe( void );
	void EXPORT PickupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	// DoD grenade touches (restore/weapons/ggrenade.cpp) - new vs SDK
	void EXPORT MortarShellExplodeTouch( CBaseEntity *pOther );
	void EXPORT RocketExplodeTouch( CBaseEntity *pOther );

	virtual void BounceSound( void );
	virtual int BloodColor( void ) { return DONT_BLEED; }
	virtual void Killed( entvars_t *pevAttacker, int iGib );
};

// constant items
#define ITEM_HEALTHKIT 1
#define ITEM_ANTIDOTE 2
#define ITEM_SECURITY 3
#define ITEM_BATTERY 4

#define MAX_NORMAL_BATTERY 100

// weapon weight factors (for auto-switching)   (-1 = noswitch)
#define CROWBAR_WEIGHT 0
#define GLOCK_WEIGHT 10
#define PYTHON_WEIGHT 15
#define MP5_WEIGHT 15
#define SHOTGUN_WEIGHT 15
#define CROSSBOW_WEIGHT 10
#define RPG_WEIGHT 20
#define GAUSS_WEIGHT 20
#define EGON_WEIGHT 20
#define HORNETGUN_WEIGHT 15
#define HANDGRENADE_WEIGHT 5
#define SNARK_WEIGHT 5
#define SATCHEL_WEIGHT -10
#define TRIPMINE_WEIGHT -10

// weapon clip/carry ammo capacities
#define URANIUM_MAX_CARRY 100
#define _9MM_MAX_CARRY 250
#define _357_MAX_CARRY 36
#define BUCKSHOT_MAX_CARRY 125
#define BOLT_MAX_CARRY 50
#define ROCKET_MAX_CARRY 5
#define HANDGRENADE_MAX_CARRY 10
#define SATCHEL_MAX_CARRY 5
#define TRIPMINE_MAX_CARRY 5
#define SNARK_MAX_CARRY 15
#define HORNET_MAX_CARRY 8
#define M203_GRENADE_MAX_CARRY 10

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP -1

//#define CROWBAR_MAX_CLIP		WEAPON_NOCLIP
#define GLOCK_MAX_CLIP 17
#define PYTHON_MAX_CLIP 6
#define MP5_MAX_CLIP 50
#define MP5_DEFAULT_AMMO 25
#define SHOTGUN_MAX_CLIP 8
#define CROSSBOW_MAX_CLIP 5
#define RPG_MAX_CLIP 1
#define GAUSS_MAX_CLIP WEAPON_NOCLIP
#define EGON_MAX_CLIP WEAPON_NOCLIP
#define HORNETGUN_MAX_CLIP WEAPON_NOCLIP
#define HANDGRENADE_MAX_CLIP WEAPON_NOCLIP
#define SATCHEL_MAX_CLIP WEAPON_NOCLIP
#define TRIPMINE_MAX_CLIP WEAPON_NOCLIP
#define SNARK_MAX_CLIP WEAPON_NOCLIP

// the default amount of ammo that comes with each gun when it spawns
#define GLOCK_DEFAULT_GIVE 17
#define PYTHON_DEFAULT_GIVE 6
#define MP5_DEFAULT_GIVE 25
#define MP5_DEFAULT_AMMO 25
#define MP5_M203_DEFAULT_GIVE 0
#define SHOTGUN_DEFAULT_GIVE 12
#define CROSSBOW_DEFAULT_GIVE 5
#define RPG_DEFAULT_GIVE 1
#define GAUSS_DEFAULT_GIVE 20
#define EGON_DEFAULT_GIVE 20
#define HANDGRENADE_DEFAULT_GIVE 5
#define SATCHEL_DEFAULT_GIVE 1
#define TRIPMINE_DEFAULT_GIVE 1
#define SNARK_DEFAULT_GIVE 5
#define HIVEHAND_DEFAULT_GIVE 8

// The amount of ammo given to a player by an ammo item.
#define AMMO_URANIUMBOX_GIVE 20
#define AMMO_GLOCKCLIP_GIVE GLOCK_MAX_CLIP
#define AMMO_357BOX_GIVE PYTHON_MAX_CLIP
#define AMMO_MP5CLIP_GIVE MP5_MAX_CLIP
#define AMMO_CHAINBOX_GIVE 200
#define AMMO_M203BOX_GIVE 2
#define AMMO_BUCKSHOTBOX_GIVE 12
#define AMMO_CROSSBOWCLIP_GIVE CROSSBOW_MAX_CLIP
#define AMMO_RPGCLIP_GIVE RPG_MAX_CLIP
#define AMMO_URANIUMBOX_GIVE 20
#define AMMO_SNARKBOX_GIVE 5

// bullet types
// Reconstructed EXACT from binary DWARF (gdb -batch -ex "ptype Bullet" ~/rev/dod.so):
// 6 legacy HL bullets (0-5), then the 21 DoD player bullets are inserted at 6-26
// (binary names BULLET_PLAYER_*), so the monster bullets start at 27. The server
// code (MapBullet, wpn_shared barrels, combat.cpp) refers to the DoD player bullets
// through the BULLET_DOD_* aliases, which are kept here with identical values.
// BULLET_MONSTER_9MM == 27 (verified against sarge.cpp/func_tank.cpp/turret.cpp).
typedef enum
{
	BULLET_NONE = 0,		// BULLET_DOD_NONE alias below
	BULLET_DOD_NONE = 0,
	BULLET_PLAYER_9MM,		 // glock
	BULLET_PLAYER_MP5,		 // mp5
	BULLET_PLAYER_357,		 // python
	BULLET_PLAYER_BUCKSHOT,	 // shotgun
	BULLET_PLAYER_CROWBAR,	 // crowbar swipe

	// DoD player bullets (binary canonical names) + server BULLET_DOD_* aliases
	BULLET_PLAYER_COLT = 6,			BULLET_DOD_COLT = 6,
	BULLET_PLAYER_LUGER = 7,			BULLET_DOD_LUGER = 7,
	BULLET_PLAYER_GARAND = 8,		BULLET_DOD_GARAND = 8,
	BULLET_PLAYER_SCOPEDKAR = 9,		BULLET_DOD_SCOPEDKAR = 9,
	BULLET_PLAYER_THOMPSON = 10,		BULLET_DOD_THOMPSON = 10,
	BULLET_PLAYER_MP44 = 11,			BULLET_DOD_MP44 = 11,
	BULLET_PLAYER_SPRING = 12,		BULLET_DOD_SPRING = 12,
	BULLET_PLAYER_KAR = 13,			BULLET_DOD_KAR = 13,
	BULLET_PLAYER_BAR = 14,			BULLET_DOD_BAR = 14,
	BULLET_PLAYER_MP40 = 15,			BULLET_DOD_MP40 = 15,
	BULLET_PLAYER_MG42 = 16,			BULLET_DOD_MG42 = 16,
	BULLET_PLAYER_MG34 = 17,			BULLET_DOD_MG34 = 17,
	BULLET_PLAYER_30CAL = 18,		BULLET_DOD_30CAL = 18,
	BULLET_PLAYER_M1CARBINE = 19,	BULLET_DOD_CARBINE = 19,
	BULLET_PLAYER_GREASEGUN = 20,	BULLET_DOD_GREASEGUN = 20,
	BULLET_PLAYER_FG42 = 21,			BULLET_DOD_FG42 = 21,
	BULLET_PLAYER_K43 = 22,			BULLET_DOD_K43 = 22,
	BULLET_PLAYER_ENFIELD = 23,		BULLET_DOD_ENFIELD = 23,
	BULLET_PLAYER_STEN = 24,			BULLET_DOD_STEN = 24,
	BULLET_PLAYER_BREN = 25,			BULLET_DOD_BREN = 25,
	BULLET_PLAYER_WEBLEY = 26,		BULLET_DOD_WEBLEY = 26,

	BULLET_MONSTER_9MM = 27,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,
} Bullet;

#define ITEM_FLAG_SELECTONEMPTY 1
#define ITEM_FLAG_NOAUTORELOAD 2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY 4
#define ITEM_FLAG_LIMITINWORLD 8
#define ITEM_FLAG_EXHAUSTIBLE 16  // A player can totally exhaust their ammo supply and lose this weapon
#define ITEM_FLAG_NOAUTOSWITCHTO 32

#define WEAPON_IS_ONTARGET 0x40

typedef struct
{
	int iSlot;
	int iPosition;
	const char *pszAmmo1;  // ammo 1 type
	int iMaxAmmo1;		   // max ammo 1
	const char *pszAmmo2;  // ammo 2 type
	int iMaxAmmo2;		   // max ammo 2
	const char *pszName;
	int iMaxClip;
	int iId;
	int iFlags;
	int iWeight;  // this value used to determine this weapon's importance in autoselection.
	int iBulletId;
	float flSpread;
	float flRecoil;
} ItemInfo;

typedef struct
{
	const char *pszName;
	int iId;
} AmmoInfo;

// Items that the player has in their inventory that they can use
class CBasePlayerItem : public CBaseAnimating
{
	void SpawnReplacement( void );
  public:
	BOOL m_bTriggerSpawnOnDie;  // DoD CBasePlayerItem+412
	virtual void SetObjectCollisionBox( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	virtual int
	AddToPlayer( CBasePlayer *pPlayer );  // return TRUE if the item you want the item added to the player inventory
	virtual int AddDuplicate( CBasePlayerItem *pItem )
	{
		return FALSE;
	}  // return TRUE if you want your duplicate removed from world
	void EXPORT DestroyItem( void );
	void EXPORT DefaultTouch( CBaseEntity *pOther );  // default weapon touch
	void EXPORT FallThink(
		void );	 // when an item is first spawned, this think is run to determine when the object has hit the ground.
	void EXPORT Materialize( void );  // make a weapon visible and tangible
	void EXPORT
	AttemptToMaterialize( void );  // the weapon desires to become visible and tangible, if the game rules allow for it
	CBaseEntity *Respawn( void );  // copy a weapon
	void FallInit( void );
	void CheckRespawn( void );
	virtual int GetItemInfo( ItemInfo *p ) { return 0; };  // returns 0 if struct not filled out
	virtual unsigned int GetItemClass( void ) { return 0; }
	virtual BOOL CanDeploy( void ) { return TRUE; };
	virtual BOOL Deploy()  // returns is deploy was successful
	{
		return TRUE;
	};

	virtual BOOL CanHolster( void ) { return TRUE; };  // can this weapon be put away right now?
	virtual void Holster( int skiplocal = 0 );
	virtual void UpdateItemInfo( void ) { return; };

	virtual void ItemPreFrame( void ) { return; }	// called each frame by the player PreThink
	virtual void ItemPostFrame( void ) { return; }	// called each frame by the player PostThink

	virtual void Drop( void );
	virtual void Kill( void );
	virtual void AttachToPlayer( CBasePlayer *pPlayer );
	void SetNextThink( float t ) { pev->nextthink = t; }

	virtual int PrimaryAmmoIndex() { return -1; };
	virtual int SecondaryAmmoIndex() { return -1; };

	virtual int UpdateClientData( CBasePlayer *pPlayer ) { return 0; }

	virtual CBasePlayerItem *GetWeaponPtr( void ) { return NULL; };

	static ItemInfo ItemInfoArray[MAX_WEAPONS];
	static AmmoInfo AmmoInfoArray[MAX_AMMO_SLOTS];

	CBasePlayer *m_pPlayer;
	CBasePlayerItem *m_pNext;
	int m_iId;	// WEAPON_???

	virtual int iItemSlot( void ) { return 0; }	 // return 0 to MAX_ITEMS_SLOTS, used in hud

	int iItemPosition( void ) { return ItemInfoArray[m_iId].iPosition; }
	const char *pszAmmo1( void ) { return ItemInfoArray[m_iId].pszAmmo1; }
	int iMaxAmmo1( void ) { return ItemInfoArray[m_iId].iMaxAmmo1; }
	const char *pszAmmo2( void ) { return ItemInfoArray[m_iId].pszAmmo2; }
	int iMaxAmmo2( void ) { return ItemInfoArray[m_iId].iMaxAmmo2; }
	const char *pszName( void ) { return ItemInfoArray[m_iId].pszName; }
	int iMaxClip( void ) { return ItemInfoArray[m_iId].iMaxClip; }
	int iWeight( void ) { return ItemInfoArray[m_iId].iWeight; }
	int iFlags( void ) { return ItemInfoArray[m_iId].iFlags; }

	// int		m_iIdPrimary;										// Unique Id for primary ammo
	// int		m_iIdSecondary;										// Unique Id for secondary ammo
	void RoundRespawn( void );
	BOOL m_bReadyToRespawn;
	void PickupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

// inventory items that
class CBasePlayerWeapon : public CBasePlayerItem
{
  public:
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	// generic weapon versions of CBasePlayerItem calls
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual int AddDuplicate( CBasePlayerItem *pItem );

	virtual int ExtractAmmo(
		CBasePlayerWeapon
			*pWeapon );	 //{ return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up
	virtual int ExtractClipAmmo(
		CBasePlayerWeapon
			*pWeapon );	 // { return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up

	virtual int AddWeapon( void )
	{
		ExtractAmmo( this );
		return TRUE;
	};	// Return TRUE if you want to add yourself to the player

	// generic "shared" ammo handlers
	BOOL AddPrimaryAmmo( int iCount, char *szName, int iMaxClip, int iMaxCarry );
	BOOL AddSecondaryAmmo( int iCount, char *szName, int iMaxCarry );

	virtual void UpdateItemInfo( void ) {};	 // updates HUD state

	int m_iPlayEmptySound;
	int m_fFireOnEmpty;	 // True when the gun is empty and the player is still holding down the
						 // attack key(s)
	virtual BOOL PlayEmptySound( void );
	virtual void ResetEmptySound( void );

	virtual void SendWeaponAnim( int iAnim, int skiplocal = 1,
								 int body = 0 );  // skiplocal is 1 if client is predicting weapon animations

	virtual BOOL CanDeploy( void );
	virtual BOOL IsUseable( void );
	BOOL DefaultDeploy( const char *szViewModel, const char *szWeaponModel, int iAnim, const char *szAnimExt,
						const char *szAnimReloadExt, int skiplocal = 0 );
 	BOOL TimedDeploy( const char *szViewModel, const char *szWeaponModel, int iAnim, const char *szAnimExt,
					  const char *szReloadExt, float idleTime, float attackTime, int skiplocal = 0 );
	int DefaultReload( int iClipSize, int iAnim, float fDelay, int body = 0 );

	virtual void ItemPostFrame( void );	 // called each frame by the player PostThink
	// called by CBasePlayerWeapons ItemPostFrame()
	virtual void PrimaryAttack( void ) { return; }		   // do "+ATTACK"
	virtual void SecondaryAttack( void ) { return; }	   // do "+ATTACK2"
	virtual void Reload( void ) { return; }				   // do "+RELOAD"
	virtual void Special( void ) { return; }			   // do "+SPECIAL" (binary+client, unused in wpn_shared)
	virtual void WeaponIdle( void ) { return; }			   // called when no buttons pressed
	virtual int UpdateClientData( CBasePlayer *pPlayer );  // sends hud info to client dll, if things have changed
	virtual void RetireWeapon( void );
	virtual BOOL ShouldWeaponIdle( void ) { return FALSE; };
	virtual void Holster( int skiplocal = 0 );
	virtual BOOL UseDecrement( void ) { return FALSE; };

	int PrimaryAmmoIndex();
	int SecondaryAmmoIndex();

	void PrintState( void );

	virtual CBasePlayerItem *GetWeaponPtr( void ) { return (CBasePlayerItem *) this; };
	virtual int Classify( void ) { return 0; };
	int GetRoundState( void );
	float GetNextAttackDelay( float delay );

	float m_flPumpTime;
	int m_fInSpecialReload;			// Are we in the middle of a reload for the shotguns
	float m_flNextPrimaryAttack;	// soonest time ItemPostFrame will call PrimaryAttack
	float m_flNextSecondaryAttack;	// soonest time ItemPostFrame will call SecondaryAttack
	float m_flTimeWeaponIdle;		// soonest time ItemPostFrame will call WeaponIdle
	int m_iPrimaryAmmoType;			// "primary" ammo index into players m_rgAmmo[]
	int m_iSecondaryAmmoType;		// "secondary" ammo index into players m_rgAmmo[]
	int m_iClip;					// number of shots left in the primary weapon clip, -1 it not used
	int m_iClientClip;				// the last version of m_iClip sent to hud dll
	int m_iClientWeaponState;  // the last version of the weapon state sent to hud dll (is current weapon, is on target)
	int m_fInReload;		   // Are we in the middle of a reload;

	int m_iDefaultAmmo;	 // how much ammo you get when you pick up this weapon as placed by a level designer.
	int m_fInAttack;
	int *current_ammo;
	int m_iWeaponState;
	float m_flWeaponHeat;
	virtual void UpdateZoomSpeed( void );
	virtual int ZoomOut( void );
	virtual int ZoomIn( void );
	virtual int ChangeFOV( int fov );
	void ThinkZoomOut( void );
	void ThinkZoomIn( void );
	virtual void RemoveStamina( float s, CBasePlayer *p );
	virtual bool PlayerIsWaterSniping( void );
	virtual Vector Aim( float accuracyFactor, CBasePlayer *pOther, unsigned int shared_rand );
	virtual float flAim( float accuracyFactor, CBasePlayer *pOther );
	virtual int GetFOV( void );
	int GetZoomFOV( void ) { return GetFOV(); }
	int GetZoomFOVToggle( int fov ) { return fov; }
	BOOL IsEmptyOrZoomed( void );
	BOOL IsEmptyOrZoomedNoFire( void );
	void DeployIdle( void ) {}

	void PostMortarValue( float v );
	void SendMortarFireCommand( char *c );

	// hle time creep vars
	float m_flPrevPrimaryAttack;
	float m_flLastFireTime;
};

class CBasePlayerAmmo : public CBaseEntity
{
  public:
	virtual void Spawn( void );
	void EXPORT DefaultTouch( CBaseEntity *pOther );  // default weapon touch
	virtual BOOL AddAmmo( CBaseEntity *pOther ) { return TRUE; };
	bool GiveGenericAmmo( CBaseEntity *pOther );

	CBaseEntity *Respawn( void );
	void EXPORT Materialize( void );
};

extern DLL_GLOBAL short g_sModelIndexLaser;	 // holds the index for the laser beam
extern DLL_GLOBAL const char *g_pModelNameLaser;

extern DLL_GLOBAL short g_sModelIndexLaserDot;	  // holds the index for the laser beam dot
extern DLL_GLOBAL short g_sModelIndexFireball;	  // holds the index for the fireball
extern DLL_GLOBAL short g_sModelIndexSmoke;		  // holds the index for the smoke cloud
extern DLL_GLOBAL short g_sModelIndexWExplosion;  // holds the index for the underwater explosion
extern DLL_GLOBAL short g_sModelIndexBubbles;	  // holds the index for the bubbles model
extern DLL_GLOBAL short g_sModelIndexBloodDrop;	  // holds the sprite index for blood drops
extern DLL_GLOBAL short g_sModelIndexBloodSpray;  // holds the sprite index for blood spray (bigger)

extern void ClearMultiDamage( void );
extern void ApplyMultiDamage( entvars_t *pevInflictor, entvars_t *pevAttacker );
extern void AddMultiDamage( entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType );

extern void DecalGunshot( TraceResult *pTrace, int iBulletType );
extern void SpawnBlood( Vector vecSpot, int bloodColor, float flDamage );
extern int DamageDecal( CBaseEntity *pEntity, int bitsDamageType );
extern void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage,
						  float flRadius, int iClassIgnore, int bitsDamageType );

typedef struct
{
	CBaseEntity *pEntity;
	float amount;
	int type;
} MULTIDAMAGE;

extern MULTIDAMAGE gMultiDamage;

#define LOUD_GUN_VOLUME 1000
#define NORMAL_GUN_VOLUME 600
#define QUIET_GUN_VOLUME 200

#define BRIGHT_GUN_FLASH 512
#define NORMAL_GUN_FLASH 256
#define DIM_GUN_FLASH 128

#define BIG_EXPLOSION_VOLUME 2048
#define NORMAL_EXPLOSION_VOLUME 1024
#define SMALL_EXPLOSION_VOLUME 512

#define WEAPON_ACTIVITY_VOLUME 64

#define VECTOR_CONE_1DEGREES Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES Vector( 0.06105, 0.06105, 0.06105 )
#define VECTOR_CONE_8DEGREES Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES Vector( 0.17365, 0.17365, 0.17365 )

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo.
//=========================================================
class CWeaponBox : public CBaseEntity
{
	void EXPORT ActivateThink( void );
	BOOL m_bTouch;  // DoD CWeaponBox+640
	void Precache( void );
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	BOOL IsEmpty( void );
	int GiveAmmo( int iCount, const char *szName, int iMax, int *pIndex = NULL );
	void SetObjectCollisionBox( void );

  public:
	void EXPORT Kill( void );
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	BOOL HasWeapon( CBasePlayerItem *pCheckItem );
	BOOL PackWeapon( CBasePlayerItem *pWeapon );
	BOOL PackAmmo( int iszName, int iCount );

	CBasePlayerItem *m_rgpPlayerItems[MAX_ITEM_TYPES];	// one slot for each

	int m_rgiszAmmo[MAX_AMMO_SLOTS];  // ammo names
	int m_rgAmmo[MAX_AMMO_SLOTS];	  // ammo quantities

	int m_cAmmoTypes;  // how many ammo types packed into this box (if packed by a level designer)
	void RoundRespawn( void );
};

#ifdef CLIENT_DLL
bool bIsMultiplayer( void );
void LoadVModel( char *szViewModel, CBasePlayer *m_pPlayer );
#endif

class CShotgun : public CBasePlayerWeapon
{
  public:
#ifndef CLIENT_DLL
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn( void );
	void Precache( void );
	int iItemSlot() { return 3; }
	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy();
	void Reload( void );
	void WeaponIdle( void );
	void ItemPostFrame( void );
	int m_fInReload;
	float m_flNextReload;
	int m_iShell;

	virtual BOOL UseDecrement( void )
	{
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}

  private:
	unsigned short m_usDoubleFire;
	unsigned short m_usSingleFire;
};

class CLaserSpot : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );

	int ObjectCaps( void ) { return FCAP_DONT_SAVE; }

  public:
	void Suspend( float flSuspendTime );
	void EXPORT Revive( void );

	static CLaserSpot *CreateSpot( void );
};

class CRpg : public CBasePlayerWeapon
{
  public:
#ifndef CLIENT_DLL
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn( void );
	void Precache( void );
	void Reload( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	void UpdateSpot( void );
	BOOL ShouldWeaponIdle( void ) { return TRUE; };

	CLaserSpot *m_pSpot;
	int m_fSpotActive;
	int m_cActiveRockets;  // how many missiles in flight from this launcher right now?

	virtual BOOL UseDecrement( void )
	{
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}

  private:
	unsigned short m_usRpg;
};

class CRpgRocket : public CGrenade
{
  public:
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
	void Spawn( void );
	void Precache( void );
	void EXPORT FollowThink( void );
	void EXPORT IgniteThink( void );
	void EXPORT RocketTouch( CBaseEntity *pOther );

	virtual void Explode( TraceResult *pTrace, int bitsDamageType );

	static CRpgRocket *CreateRpgRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CRpg *pLauncher );

	CRpg *GetLauncher();

	int m_iTrail;
	float m_flIgniteTime;

	EHANDLE m_hLauncher;  // pointer back to the launcher that fired me.
};

class CGauss : public CBasePlayerWeapon
{
  public:
#ifndef CLIENT_DLL
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	void StartFire( void );
	void Fire( Vector vecOrigSrc, Vector vecDirShooting, float flDamage );
	float GetFullChargeTime( void );
	int m_iBalls;
	int m_iGlow;
	int m_iBeam;
	int m_iSoundState;	// don't save this

	// was this weapon just fired primary or secondary?
	// we need to know so we can pick the right set of effects.
	BOOL m_fPrimaryFire;

	virtual BOOL UseDecrement( void )
	{
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}

  private:
	unsigned short m_usGaussFire;
	unsigned short m_usGaussSpin;
};

class CEgon : public CBasePlayerWeapon
{
  public:
#ifndef CLIENT_DLL
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend );

	void CreateEffect( void );
	void DestroyEffect( void );

	void EndAttack( void );
	void Attack( void );
	void PrimaryAttack( void );
	void WeaponIdle( void );

	float m_flAmmoUseTime;	// since we use < 1 point of ammo per update, we subtract ammo on a timer.

	float GetPulseInterval( void );
	float GetDischargeInterval( void );

	void Fire( const Vector &vecOrigSrc, const Vector &vecDir );

	BOOL HasAmmo( void );
	BOOL CanHolster();

	void UseAmmo( int count );

	enum EGON_FIREMODE
	{
		FIRE_NARROW,
		FIRE_WIDE
	};

	CBeam *m_pBeam;
	CBeam *m_pNoise;
	CSprite *m_pSprite;

	virtual BOOL UseDecrement( void )
	{
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}

	unsigned short m_usEgonStop;

  private:
	float m_shootTime;
	EGON_FIREMODE m_fireMode;
	float m_shakeTime;
	BOOL m_deployed;

	unsigned short m_usEgonFire;
};

class CSatchel : public CBasePlayerWeapon
{
  public:
#ifndef CLIENT_DLL
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddDuplicate( CBasePlayerItem *pOriginal );
	BOOL CanDeploy( void );
	BOOL Deploy( void );
	BOOL IsUseable( void );

	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );
	void Throw( void );

	virtual BOOL UseDecrement( void )
	{
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}
};

// DoD weapon base classes (restore/weapons/weapons.h) - foundation for wpn_shared barrel agents
// Per-weapon iItemSlot values in comments; barrels override iItemSlot, core does not touch.
class CBipodWeapon : public CBasePlayerWeapon
{
  protected:
	int m_iFireEvent;
	int m_iOverheatEvent;

  public:
	void Spawn( int id );
	virtual void Precache();
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void Holster( int skiplocal = 0 );
	void ForceUndeploy();
	virtual int iItemSlot();
	virtual BOOL UseDecrement();
	virtual BOOL CanDrop();
	virtual int GetIdleAnim();
	virtual int GetDrawAnim();
	virtual int GetReloadAnim();
	virtual int GetUpToDownAnim();
	virtual int GetDownToUpAnim();
	float GetBipodSpread();
	bool IsDeployed();
	void CoolThink();
	int GetFireEvent() { return m_iFireEvent; }
	void SetCoolThink() { CoolThink(); }
	void DeployBipod() { m_iWeaponState |= 2; }
	void DeployIdle() {}
};
class CMeleeWeapon : public CBasePlayerWeapon
{
  protected:
	int m_iSwing;
	TraceResult m_trHit;
	int m_iFireEvent;

  public:
	void Spawn( int id );
	virtual void Precache();
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual void WeaponIdle();
	void SwingAgain();
	void Smack();
	int Swing( int fFirst );
	int Stab( int fStab );
	virtual void Holster( int skiplocal = 0 );
	virtual int iItemSlot();
	virtual BOOL UseDecrement();
	virtual int Classify();
	virtual BOOL CanDrop();
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual int GetIdleAnim();
	virtual int GetDrawAnim();
	virtual int GetSlashAnim( int stab );
};
class CPistol : public CBasePlayerWeapon
{
  protected:
	int m_iFireEvent;

  public:
	void Spawn( int id );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int iItemSlot();
	virtual BOOL UseDecrement();
	virtual int Classify();
	virtual BOOL CanDrop();
	virtual int GetIdleAnim();
	virtual int GetDrawAnim();
	virtual int GetReloadAnim();
};
class CRocketWeapon : public CBasePlayerWeapon
{
  public:
	virtual int iItemSlot();
	virtual BOOL UseDecrement();
	virtual int Classify();
	virtual void ReSlow();
};
class CDoDGrenade : public CBasePlayerWeapon
{
  protected:
	float m_flStartThrow;
	float m_flReleaseThrow;

  public:
	virtual int iItemSlot();
	virtual BOOL UseDecrement();
	virtual int Classify();
	virtual BOOL CanDrop();
	virtual BOOL ShouldWeaponIdle();
};
class CDoDGrenadeEx : public CDoDGrenade
{
  public:
	virtual int Classify();
};
class CDodAmmoBox : public CBaseEntity
{
	EHANDLE m_hOwner;

  public:
	virtual void Precache();
	virtual void Spawn();
	virtual void Touch( CBaseEntity *pOther );
	void SetOwner( CBaseEntity *pOwner );
	void KillThink();
};

// DoD weapon classes - ground truth dod.so DWARF via gdb ptype
class CAmerKnife : public CMeleeWeapon
{
  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int GetSlashAnim( int stab );
	virtual int GetDrawAnim();
	virtual int GetIdleAnim();
};
class CGerKnife : public CMeleeWeapon
{
  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int GetSlashAnim( int stab );
	virtual int GetDrawAnim();
	virtual int GetIdleAnim();
};
class CSpade : public CMeleeWeapon
{
  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int GetSlashAnim( int stab );
	virtual int GetDrawAnim();
	virtual int GetIdleAnim();
};
class CBAR : public CBipodWeapon
{
	unsigned short m_usFireBAR;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int Classify();
	virtual int GetIdleAnim();
	virtual int GetDrawAnim();
	virtual int GetReloadAnim();
	virtual int GetUpToDownAnim();
	virtual int GetDownToUpAnim();
};
class CBREN : public CBipodWeapon
{
	unsigned short m_usFireBren;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int Classify();
	virtual int GetIdleAnim();
	virtual int GetDrawAnim();
	virtual int GetReloadAnim();
	virtual int GetUpToDownAnim();
	virtual int GetDownToUpAnim();
};
class C30CAL : public CBipodWeapon
{
	unsigned short m_usFire30CAL;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int Classify();
	virtual int GetIdleAnim();
	virtual int GetDrawAnim();
	virtual int GetReloadAnim();
	virtual int GetUpToDownAnim();
	virtual int GetDownToUpAnim();
};
class CMG34 : public CBipodWeapon
{
	unsigned short m_usFireMG34;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int Classify();
	virtual int GetIdleAnim();
	virtual int GetDrawAnim();
	virtual int GetReloadAnim();
	virtual int GetUpToDownAnim();
	virtual int GetDownToUpAnim();
};
class CMG42 : public CBipodWeapon
{
	unsigned short m_usFireMG42;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int Classify();
	virtual int GetIdleAnim();
	virtual int GetDrawAnim();
	virtual int GetReloadAnim();
	virtual int GetUpToDownAnim();
	virtual int GetDownToUpAnim();
};
class CBazooka : public CRocketWeapon
{
	unsigned short m_usFireBazooka;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int GetItemInfo( ItemInfo *p );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Holster( int skiplocal = 0 );
	virtual void Reload();
	virtual void WeaponIdle();
	void Raise();
	void Lower();
	void UnSlow();
	virtual void ReSlow();
};
class CBazookaShell : public CGrenade
{
  public:
	virtual void Spawn();
	virtual void Precache();
	void RocketTouch( CBaseEntity *pOther );
	static CBazookaShell *CreateRocket( Vector vecSrc, Vector vecDir, CBaseEntity *pOwner );
};
class CPIAT : public CRocketWeapon
{
	unsigned short m_usFirePIAT;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int GetItemInfo( ItemInfo *p );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Holster( int skiplocal = 0 );
	virtual void Reload();
	virtual void WeaponIdle();
	void Raise();
	void Lower();
	void UnSlow();
	virtual void ReSlow();
};
class CPIATShell : public CGrenade
{
  public:
	virtual void Spawn();
	virtual void Precache();
	void RocketTouch( CBaseEntity *pOther );
	static CPIATShell *CreateRocket( Vector vecSrc, Vector vecDir, CBaseEntity *pOwner );
};
class CPschreck : public CRocketWeapon
{
	unsigned short m_usFirePschreck;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int GetItemInfo( ItemInfo *p );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Holster( int skiplocal = 0 );
	virtual void Reload();
	virtual void WeaponIdle();
	void Raise();
	void Lower();
	void UnSlow();
	virtual void ReSlow();
};
class CPschreckShell : public CGrenade
{
  public:
	virtual void Spawn();
	virtual void Precache();
	void RocketTouch( CBaseEntity *pOther );
	static CPschreckShell *CreateRocket( Vector vecSrc, Vector vecDir, CBaseEntity *pOwner );
};
class CCOLT : public CPistol
{
  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int GetReloadAnim();
	virtual int GetDrawAnim();
	virtual int GetIdleAnim();
};
class CLUGER : public CPistol
{
	unsigned short m_usFireLuger;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int GetItemInfo( ItemInfo *p );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
};
class CWEBLEY : public CPistol
{
	unsigned short m_usFireWebley;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
};

class CENFIELD : public CBasePlayerWeapon
{
	unsigned short m_usFireEnfield;
	unsigned short m_usFireScopedEnfield;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	void ThinkZoomOutIn();
	virtual void Holster( int skiplocal = 0 );
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
};
class CSTEN : public CBasePlayerWeapon
{
	unsigned short m_usFireSten;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
};
class CFG42 : public CBasePlayerWeapon
{
	unsigned short m_usFireFG42;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void Holster( int skiplocal = 0 );
	virtual BOOL CanHolster();
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
	BOOL IsDeployed();
};
class CGarand : public CBasePlayerWeapon
{
	unsigned short m_usFireGarand;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
};
class CGreaseGun : public CBasePlayerWeapon
{
	unsigned short m_usFireGreaseGun;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual BOOL Deploy();
	virtual BOOL SpawnDeploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
};
class CK43 : public CBasePlayerWeapon
{
	unsigned short m_usFireK43;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
};
class CKAR : public CBasePlayerWeapon
{
	unsigned short m_usFireKar;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
};
class CM1Carbine : public CBasePlayerWeapon
{
	unsigned short m_usFireM1Carbine;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL SpawnDeploy();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
};
class CMP40 : public CBasePlayerWeapon
{
	unsigned short m_usFireMP40;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
};
class CMP44 : public CBasePlayerWeapon
{
	unsigned short m_usFireMP44;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
};
class CSPRING : public CBasePlayerWeapon
{
	unsigned short m_usFireSpring;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	void ThinkZoomOutIn();
	void ThinkZoomOut();
	virtual void Holster( int skiplocal = 0 );
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
};
class CScopedKar : public CBasePlayerWeapon
{
	unsigned short m_usFireScopedKar;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	void ThinkZoomOutIn();
	virtual void Reload();
	virtual void Holster( int skiplocal = 0 );
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
};
class CThompson : public CBasePlayerWeapon
{
	unsigned short m_usFireThompson;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int iItemSlot();
	virtual int GetItemInfo( ItemInfo *p );
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Reload();
	virtual void WeaponIdle();
	virtual int Classify();
	virtual BOOL UseDecrement();
};
class CHandGrenade : public CDoDGrenade
{
	Vector angThrow;
	Vector vecSrc;
	Vector vecThrow;
	float flVel;
	BOOL m_bUnderhand;

  public:
	virtual void Spawn();
	virtual int GetItemInfo( ItemInfo *p );
	virtual void Precache();
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	void StartThrow( BOOL underhand );
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Holster( int skiplocal = 0 );
	virtual void WeaponIdle();
	virtual void DropGren();
	virtual BOOL CanDeploy();
};
class CHandGrenadeEx : public CDoDGrenadeEx
{
	float m_flTimeToExplode;
	BOOL m_bUnderhand;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int GetItemInfo( ItemInfo *p );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	void StartThrow( BOOL underhand );
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Holster( int skiplocal = 0 );
	virtual void WeaponIdle();
	virtual BOOL CanDeploy();
	virtual void DropGren();
	virtual void SetDmgTime( float t );
};
class CStickGrenade : public CDoDGrenade
{
	Vector angThrow;
	Vector vecSrc;
	Vector vecThrow;
	float flVel;
	float m_flLeaveHand;
	BOOL m_bUnderhand;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int GetItemInfo( ItemInfo *p );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	void StartThrow( BOOL underhand );
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Holster( int skiplocal = 0 );
	virtual void WeaponIdle();
	virtual void DropGren();
	virtual BOOL CanDeploy();
};
class CStickGrenadeEx : public CDoDGrenadeEx
{
	float m_flTimeToExplode;
	BOOL m_bUnderhand;

  public:
	virtual void Spawn();
	virtual void Precache();
	virtual int GetItemInfo( ItemInfo *p );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	void StartThrow( BOOL underhand );
	virtual BOOL Deploy();
	virtual BOOL CanHolster();
	virtual void Holster( int skiplocal = 0 );
	virtual void WeaponIdle();
	virtual void DropGren();
	virtual void SetDmgTime( float t );
	virtual BOOL CanDeploy();
};

// DoD P_WpnInfo runtime table link 0x1d4b20 Ghidra 0x1e4b20 size 41 (0-40)
// struct p_wpninfo_s now from dod_shared.h (client copy, layout verified 1:1
// via gdb ptype p_wpninfo_s vs binary) — local duplicate removed to avoid
// redefinition once weapons.h includes dod_shared.h.
extern p_wpninfo_s P_WpnInfo[41];
// MapBullet exact: binary bipod 0x161f30 loads ItemInfoArray bullet, pistol 0x16e640 same; link 0xcdff0
inline int MapBullet( int id )
{
	switch ( id )
	{
	case 3:
		return BULLET_DOD_COLT;
	case 4:
		return BULLET_DOD_LUGER;
	case 5:
		return BULLET_DOD_GARAND;
	case 6:
		return BULLET_DOD_SCOPEDKAR;
	case 7:
		return BULLET_DOD_THOMPSON;
	case 8:
		return BULLET_DOD_MP44;
	case 9:
		return BULLET_DOD_SPRING;
	case 10:
		return BULLET_DOD_KAR;
	case 11:
		return BULLET_DOD_BAR;
	case 12:
		return BULLET_DOD_MP40;
	case 17:
		return BULLET_DOD_MG42;
	case 18:
		return BULLET_DOD_30CAL;
	case 20:
		return BULLET_DOD_CARBINE;
	case 21:
		return BULLET_DOD_MG34;
	case 22:
		return BULLET_DOD_GREASEGUN;
	case 23:
		return BULLET_DOD_FG42;
	case 24:
		return BULLET_DOD_K43;
	case 25:
		return BULLET_DOD_ENFIELD;
	case 26:
		return BULLET_DOD_STEN;
	case 27:
		return BULLET_DOD_BREN;
	case 28:
		return BULLET_DOD_WEBLEY;
	default:
		return id;
	}
}
void RifleMeleeAttack( CBasePlayer *p, int id );
// NOTE: removed bogus #define PLAYER_BUT_SWING 1 — collides with
// player.h enum PLAYER_ANIM { ..., PLAYER_BUT_SWING = 16 }. Macro forced
// value 1 and broke any TU including weapons.h before player.h
// (knife.cpp/pistol.cpp). Correct value is 16 (see k43.cpp literal).

#endif	// WEAPONS_H

// DoD weapon virtual helper definitions (client weapons.h inline parity; DWARF-verified ids).
// Declared as prototypes in the class bodies above; defined here to satisfy linking.

inline int CBipodWeapon::iItemSlot( void ) { return 2; }
inline BOOL CBipodWeapon::UseDecrement( void ) { return TRUE; }
inline BOOL CBipodWeapon::CanDrop( void ) { return CanHolster(); }
inline int CBipodWeapon::GetIdleAnim( void ) { return 0; }
inline int CBipodWeapon::GetDrawAnim( void ) { return 0; }
inline int CBipodWeapon::GetReloadAnim( void ) { return 0; }
inline int CBipodWeapon::GetUpToDownAnim( void ) { return 0; }
inline int CBipodWeapon::GetDownToUpAnim( void ) { return 0; }

inline int CMeleeWeapon::iItemSlot( void ) { return 0; }
inline int CMeleeWeapon::Classify( void ) { return CLASS_MELEE; }
inline BOOL CMeleeWeapon::CanDrop( void ) { return FALSE; }
inline BOOL CMeleeWeapon::UseDecrement( void ) { return TRUE; }
inline void CMeleeWeapon::SecondaryAttack( void ) { return; }
inline int CMeleeWeapon::GetIdleAnim( void ) { return 0; }
inline int CMeleeWeapon::GetDrawAnim( void ) { return 0; }
inline int CMeleeWeapon::GetSlashAnim( int stab ) { return 0; }

inline int CPistol::iItemSlot( void ) { return 1; }
inline int CPistol::Classify( void ) { return CLASS_PISTOL; }
inline BOOL CPistol::CanDrop( void ) { return FALSE; }
inline BOOL CPistol::UseDecrement( void ) { return TRUE; }
inline void CPistol::SecondaryAttack( void ) { return; }
inline int CPistol::GetIdleAnim( void ) { return 0; }
inline int CPistol::GetDrawAnim( void ) { return 0; }
inline int CPistol::GetReloadAnim( void ) { return 0; }

inline int CRocketWeapon::iItemSlot( void ) { return 2; }
inline int CRocketWeapon::Classify( void ) { return CLASS_ROCKET; }
inline BOOL CRocketWeapon::UseDecrement( void ) { return TRUE; }

inline int CDoDGrenade::iItemSlot( void ) { return 4; }
inline int CDoDGrenade::Classify( void ) { return CLASS_GRENADE; }
inline BOOL CDoDGrenade::CanDrop( void ) { return FALSE; }
inline BOOL CDoDGrenade::ShouldWeaponIdle( void ) { return TRUE; }
inline BOOL CDoDGrenade::UseDecrement( void ) { return TRUE; }

inline int CDoDGrenadeEx::Classify( void ) { return CLASS_GRENADE_EX; }

inline int C30CAL::iItemSlot( void ) { return 2; }
inline int CBAR::iItemSlot( void ) { return 2; }
inline int CBREN::iItemSlot( void ) { return 2; }
inline int CMG34::iItemSlot( void ) { return 2; }
inline int CMG42::iItemSlot( void ) { return 2; }

inline int CM1Carbine::iItemSlot( void ) { return 2; }
inline BOOL CM1Carbine::UseDecrement( void ) { return TRUE; }
inline int CGarand::iItemSlot( void ) { return 2; }
inline BOOL CGarand::UseDecrement( void ) { return TRUE; }
inline int CThompson::iItemSlot( void ) { return 2; }
inline BOOL CThompson::UseDecrement( void ) { return TRUE; }
inline int CMP44::iItemSlot( void ) { return 2; }
inline BOOL CMP44::UseDecrement( void ) { return TRUE; }
inline int CMP40::iItemSlot( void ) { return 2; }
inline BOOL CMP40::UseDecrement( void ) { return TRUE; }
inline int CKAR::iItemSlot( void ) { return 2; }
inline BOOL CKAR::UseDecrement( void ) { return TRUE; }
inline int CK43::iItemSlot( void ) { return 2; }
inline BOOL CK43::UseDecrement( void ) { return TRUE; }
inline int CFG42::iItemSlot( void ) { return 2; }
inline BOOL CFG42::UseDecrement( void ) { return TRUE; }
inline int CGreaseGun::iItemSlot( void ) { return 2; }
inline BOOL CGreaseGun::UseDecrement( void ) { return TRUE; }
inline int CSTEN::iItemSlot( void ) { return 2; }
inline BOOL CSTEN::UseDecrement( void ) { return TRUE; }
inline int CENFIELD::iItemSlot( void ) { return 2; }
inline BOOL CENFIELD::UseDecrement( void ) { return TRUE; }
inline int CScopedKar::iItemSlot( void ) { return 2; }
inline BOOL CScopedKar::UseDecrement( void ) { return TRUE; }
inline int CScopedKar::Classify( void ) { return CLASS_SCOPE_RIFLE; }
inline BOOL CScopedKar::CanHolster( void ) { return TRUE; }
inline int CSPRING::iItemSlot( void ) { return 2; }
inline BOOL CSPRING::UseDecrement( void ) { return TRUE; }

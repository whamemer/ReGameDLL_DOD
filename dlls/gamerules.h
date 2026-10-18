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
//=========================================================
// GameRules
//=========================================================

#pragma once
//#include "weapons.h"
//#include "items.h"
class CBasePlayerItem;
class CBasePlayer;
class CItem;
class CBasePlayerAmmo;

// weapon respawning return codes
enum
{
	GR_NONE = 0,

	GR_WEAPON_RESPAWN_YES,
	GR_WEAPON_RESPAWN_NO,

	GR_AMMO_RESPAWN_YES,
	GR_AMMO_RESPAWN_NO,

	GR_ITEM_RESPAWN_YES,
	GR_ITEM_RESPAWN_NO,

	GR_PLR_DROP_GUN_ALL,
	GR_PLR_DROP_GUN_ACTIVE,
	GR_PLR_DROP_GUN_NO,

	GR_PLR_DROP_AMMO_ALL,
	GR_PLR_DROP_AMMO_ACTIVE,
	GR_PLR_DROP_AMMO_NO,
};

// Player relationship return codes
enum
{
	GR_NOTTEAMMATE = 0,
	GR_TEAMMATE,
	GR_ENEMY,
	GR_ALLY,
	GR_NEUTRAL,
};

// Virtual declaration order below is the EXACT binary vtable order of the original
// DoD CGameRules (verified via `gdb ptype CGameRules` + `_ZTV10CGameRules`/`_ZTV12CDoDTeamPlay`
// relocations). Slots with `= 0` are __cxa_pure_virtual in the reference binary.
class CGameRules
{
  public:
	virtual void Init( void ) = 0;								// vtbl+0x08
	virtual void RefreshSkillData( void );						// vtbl+0x0c
	virtual void Think( void ) = 0;								// vtbl+0x10
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity ) = 0;	// vtbl+0x14
	virtual BOOL FAllowFlashlight( void ) = 0;					// vtbl+0x18
	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ) = 0;	// vtbl+0x1c
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon ) = 0;	// vtbl+0x20
	virtual BOOL IsMultiplayer( void ) = 0;						// vtbl+0x24
	virtual BOOL IsDeathmatch( void ) = 0;						// vtbl+0x28
	virtual BOOL IsTeamplay( void ) { return FALSE; };			// vtbl+0x2c
	virtual BOOL IsCoOp( void ) = 0;							// vtbl+0x30
	virtual const char *GetGameDescription( void ) { return "Half-Life"; }	// vtbl+0x34
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress,
								  char *szRejectReason ) = 0;	// vtbl+0x38
	virtual void InitHUD( CBasePlayer *pl ) = 0;				// vtbl+0x3c
	virtual void ClientDisconnected( edict_t *pClient ) = 0;		// vtbl+0x40
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer ) = 0;	// vtbl+0x44
	virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker ) { return TRUE; }	// vtbl+0x48
	virtual BOOL ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target ) { return TRUE; }	// vtbl+0x4c
	virtual void PlayerSpawn( CBasePlayer *pPlayer ) = 0;		// vtbl+0x50
	virtual void PlayerThink( CBasePlayer *pPlayer ) = 0;		// vtbl+0x54
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer ) = 0;	// vtbl+0x58
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer ) = 0;	// vtbl+0x5c
	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );	// vtbl+0x60
	void GetInitialSpawnSpot( CBasePlayer *pPlayer );			// non-virtual
	virtual BOOL AllowAutoTargetCrosshair( void ) { return TRUE; };	// vtbl+0x64
	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd ) { return FALSE; };	// vtbl+0x68
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer ) {}	// vtbl+0x6c
	virtual void PlayerDeathThink( CBasePlayer *pPlayer ) = 0;	// vtbl+0x70
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled ) = 0;	// vtbl+0x74
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags = 0 ) = 0;	// vtbl+0x78
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags = 0 ) = 0;	// vtbl+0x7c
	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );	// vtbl+0x80
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ) = 0;	// vtbl+0x84
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon ) = 0;	// vtbl+0x88
	virtual float FlWeaponRespawnTime( CBasePlayerItem *pWeapon ) = 0;	// vtbl+0x8c
	virtual float FlWeaponTryRespawn( CBasePlayerItem *pWeapon ) = 0;	// vtbl+0x90
	virtual Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon ) = 0;	// vtbl+0x94
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem ) = 0;	// vtbl+0x98
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem ) = 0;	// vtbl+0x9c
	virtual int ItemShouldRespawn( CItem *pItem ) = 0;			// vtbl+0xa0
	virtual float FlItemRespawnTime( CItem *pItem ) = 0;		// vtbl+0xa4
	virtual Vector VecItemRespawnSpot( CItem *pItem ) = 0;		// vtbl+0xa8
	virtual BOOL CanHaveAmmo( CBasePlayer *pPlayer, const char *pszAmmoName, int iMaxCarry );	// vtbl+0xac
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount ) = 0;	// vtbl+0xb0
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo ) = 0;	// vtbl+0xb4
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo ) = 0;	// vtbl+0xb8
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo ) = 0;	// vtbl+0xbc
	virtual float FlHealthChargerRechargeTime( void ) = 0;		// vtbl+0xc0
	virtual float FlHEVChargerRechargeTime( void ) { return 0; }	// vtbl+0xc4
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer ) = 0;	// vtbl+0xc8
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer ) = 0;		// vtbl+0xcc
	virtual const char *GetTeamID( CBaseEntity *pEntity ) = 0;	// vtbl+0xd0
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget ) = 0;	// vtbl+0xd4
	virtual int GetTeamIndex( const char *pTeamName ) { return -1; }	// vtbl+0xd8
	virtual const char *GetIndexedTeamName( int teamIndex ) { return ""; }	// vtbl+0xdc
	virtual BOOL IsValidTeam( const char *pTeamName ) { return TRUE; }	// vtbl+0xe0
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib ) {}	// vtbl+0xe4
	virtual const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer ) { return ""; }	// vtbl+0xe8
	virtual BOOL PlayTextureSounds( void ) { return TRUE; }		// vtbl+0xec
	virtual BOOL PlayFootstepSounds( CBasePlayer *pl, float fvol ) { return TRUE; }	// vtbl+0xf0
	virtual BOOL FAllowMonsters( void ) = 0;					// vtbl+0xf4
	virtual void EndMultiplayerGame( void ) {}					// vtbl+0xf8
	virtual int RoundState( void ) = 0;							// vtbl+0xfc
	virtual void VoteKick( CBasePlayer *pPlayer, int vote, bool bAdmin ) {}	// vtbl+0x100
	virtual void SetRoundState( int state ) {}					// vtbl+0x104
	virtual int SetRoundState( void ) { return 0; }				// vtbl+0x108
	virtual void SetRestartTime( float f_Time ) {}				// vtbl+0x10c
	virtual void SendTeamScores( int iTeam ) {}					// vtbl+0x110
	virtual float GetMaxWaveTime( int nTeam ) = 0;				// vtbl+0x114
	virtual float getWaveTime( int iTeam ) = 0;					// vtbl+0x118
	virtual void setWaveTime( int iTeam, float fTime ) {}		// vtbl+0x11c
	virtual int TeamScores( int team ) = 0;						// vtbl+0x120
	virtual void SetTeamScores( int iTeam, int iScore ) {}		// vtbl+0x124
	virtual void AddToTeamScores( int team, int points ) {}		// vtbl+0x128
	virtual void setMapTime( void ) {}							// vtbl+0x12c
	virtual void RestartRound( void ) {}						// vtbl+0x130
	virtual float getMapTime( void ) = 0;						// vtbl+0x134
	virtual bool IsRoundFrozen( void ) { return false; }		// vtbl+0x138
	virtual void SetClanMatch( bool b ) {}						// vtbl+0x13c
	virtual bool IsClanMatch( void ) { return false; }			// vtbl+0x140
	virtual bool IsClanMatchActive( void ) { return false; }	// vtbl+0x144
	virtual unsigned short Camera( void ) = 0;					// vtbl+0x148
	virtual unsigned short usVoice( void ) = 0;					// vtbl+0x14c
	virtual unsigned short bodyDamage( void ) = 0;				// vtbl+0x150
	virtual unsigned short roundRestartSound( void ) = 0;		// vtbl+0x154
	virtual unsigned short gerVoice( void ) = 0;				// vtbl+0x158
	virtual unsigned short Pain( void ) = 0;					// vtbl+0x15c
	virtual unsigned short Smoke( void ) = 0;					// vtbl+0x160
	virtual unsigned short Prone( void ) = 0;					// vtbl+0x164
	virtual unsigned short BloodSprite( void ) = 0;				// vtbl+0x168
	virtual unsigned short BloodStream( void ) = 0;				// vtbl+0x16c
	virtual unsigned short BulletTracers( void ) = 0;			// vtbl+0x170
	virtual unsigned short SparkShower( void ) = 0;				// vtbl+0x174
	virtual unsigned short ScreenFades( void ) = 0;				// vtbl+0x178
	virtual unsigned short BubbleTrails( void ) = 0;			// vtbl+0x17c
	virtual unsigned short Bubbles( void ) = 0;					// vtbl+0x180
	virtual unsigned short Shards( void ) = 0;					// vtbl+0x184
	virtual unsigned short Explosion( void ) = 0;				// vtbl+0x188
	virtual unsigned short PopHelmet( void ) = 0;				// vtbl+0x18c
	virtual unsigned short RoundReset( void ) = 0;				// vtbl+0x190
	virtual unsigned short StartTrail( void ) = 0;				// vtbl+0x194
	virtual void SetCamera( short s ) {}						// vtbl+0x198
	virtual void SetusVoice( short s ) {}						// vtbl+0x19c
	virtual void SetbodyDamage( short s ) {}					// vtbl+0x1a0
	virtual void SetroundRestartSound( short s ) {}				// vtbl+0x1a4
	virtual void SetgerVoice( short s ) {}						// vtbl+0x1a8
	virtual void SetPain( short s ) {}							// vtbl+0x1ac
	virtual void SetSmoke( short s ) {}							// vtbl+0x1b0
	virtual void SetProne( short s ) {}							// vtbl+0x1b4
	virtual void SetBloodSprite( short s ) {}					// vtbl+0x1b8
	virtual void SetBloodStream( short s ) {}					// vtbl+0x1bc
	virtual void SetBulletTracers( short s ) {}					// vtbl+0x1c0
	virtual void SetSparkShower( short s ) {}					// vtbl+0x1c4
	virtual void SetScreenFades( short s ) {}					// vtbl+0x1c8
	virtual void SetBubbleTrails( short s ) {}					// vtbl+0x1cc
	virtual void SetBubbles( short s ) {}						// vtbl+0x1d0
	virtual void SetShards( short s ) {}						// vtbl+0x1d4
	virtual void SetExplosion( short s ) {}						// vtbl+0x1d8
	virtual void SetPopHelmet( short s ) {}						// vtbl+0x1dc
	virtual void SetRoundReset( short s ) {}					// vtbl+0x1e0
	virtual void SetRocketTrail( short s ) {}					// vtbl+0x1e4
	virtual int ReturnGameRules( void ) = 0;					// vtbl+0x1e8
	virtual void SetMapChangeFromTime( int iTime ) {}			// vtbl+0x1ec

	// ---- extras kept for this restoration (non-virtual: absent from the binary vtable) ----
	void Precache() {}
	void UpdateGameMode( CBasePlayer *pPlayer ) {}
	BOOL IsFreezePeriod( void ) { return FALSE; }
	BOOL IsRoundRestarting( void ) { return FALSE; }
	void RecheckScores( void ) {}
	BOOL IsAlliesBrit( void ) { return FALSE; }
};

extern CGameRules *InstallGameRules( void );

//=========================================================
// CHalfLifeRules - rules for the single player Half-Life
// game.
//=========================================================
class CHalfLifeRules : public CGameRules
{
  public:
	CHalfLifeRules( void );

	// GR_Think
	virtual void Think( void );
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity );
	virtual BOOL FAllowFlashlight( void ) { return TRUE; };

	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon );

	// Functions to verify the single/multiplayer status of a game
	virtual BOOL IsMultiplayer( void );
	virtual BOOL IsDeathmatch( void );
	virtual BOOL IsCoOp( void );

	// Client connection/disconnection
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress,
								  char szRejectReason[128] );
	virtual void InitHUD( CBasePlayer *pl );  // the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient );

	// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );

	// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );

	virtual BOOL AllowAutoTargetCrosshair( void );

	// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );

	// Weapon retrieval
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );

	// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon );
	virtual float FlWeaponRespawnTime( CBasePlayerItem *pWeapon );
	virtual float FlWeaponTryRespawn( CBasePlayerItem *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon );

	// Item retrieval
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );

	// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );

	// Ammo retrieval
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount );

	// Ammo spawn/respawn control
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo );
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo );
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo );

	// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void );

	// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );

	// What happens to a dead player's ammo
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );

	// Monsters
	virtual BOOL FAllowMonsters( void );

	// Teamplay stuff
	virtual const char *GetTeamID( CBaseEntity *pEntity ) { return ""; };
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual int RoundState( void ) { return 0; }
	virtual BOOL IsFreezePeriod( void ) { return FALSE; }
	virtual BOOL IsRoundRestarting( void ) { return FALSE; }
	virtual void RecheckScores( void ) {}
	virtual BOOL IsAlliesBrit( void ) { return FALSE; }
};

//=========================================================
// CHalfLifeMultiplay - rules for the basic half life multiplayer
// competition
//=========================================================
class CHalfLifeMultiplay : public CGameRules
{
  public:
	CHalfLifeMultiplay();

	// GR_Think
	virtual void Think( void );
	virtual void RefreshSkillData( void );
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity );
	virtual BOOL FAllowFlashlight( void );

	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon );

	// Functions to verify the single/multiplayer status of a game
	virtual BOOL IsMultiplayer( void );
	virtual BOOL IsDeathmatch( void );
	virtual BOOL IsCoOp( void );

	// Client connection/disconnection
	// If ClientConnected returns FALSE, the connection is rejected and the user is provided the reason specified in
	//  svRejectReason
	// Only the client's name and remote address are provided to the dll for verification.
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress,
								  char szRejectReason[128] );
	virtual void InitHUD( CBasePlayer *pl );  // the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient );
	virtual void UpdateGameMode( CBasePlayer *pPlayer );  // the client needs to be informed of the current game mode

	// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );

	// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );
	virtual void GetInitialSpawnSpot( CBasePlayer *pPlayer );
	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );

	virtual BOOL AllowAutoTargetCrosshair( void );
	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd );
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer );

	// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );

	// Weapon retrieval
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL
	CanHavePlayerItem( CBasePlayer *pPlayer,
					   CBasePlayerItem *pWeapon );	// The player is touching an CBasePlayerItem, do I give it to him?

	// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon );
	virtual float FlWeaponRespawnTime( CBasePlayerItem *pWeapon );
	virtual float FlWeaponTryRespawn( CBasePlayerItem *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon );

	// Item retrieval
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );

	// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );

	// Ammo retrieval
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount );

	// Ammo spawn/respawn control
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo );
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo );
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo );

	// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void );
	virtual float FlHEVChargerRechargeTime( void );

	// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );

	// What happens to a dead player's ammo
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );

	// Teamplay stuff
	virtual const char *GetTeamID( CBaseEntity *pEntity ) { return ""; }
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );

	virtual BOOL PlayTextureSounds( void ) { return FALSE; }
	virtual BOOL PlayFootstepSounds( CBasePlayer *pl, float fvol );

	// Monsters
	virtual BOOL FAllowMonsters( void );

	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame( void ) { GoToIntermission(); }

  protected:
	virtual void ChangeLevel( void );
	virtual void GoToIntermission( void );
	float m_flIntermissionEndTime;
	BOOL m_iEndIntermissionButtonHit;
	void SendMOTDToClient( edict_t *client );
};

//=========================================================
// CMultiplayBusters
// Rules for a multiplayer mode that makes you feel good
//=========================================================
class CMultiplayBusters : public CHalfLifeMultiplay
{
  public:
	CMultiplayBusters();

	virtual void Think( void );
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon );
	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );

	void SetPlayerModel( CBasePlayer *pPlayer );

  protected:
	float m_flEgonBustingCheckTime = -1.0f;
	void CheckForEgons( void );
};

extern DLL_GLOBAL CGameRules *g_pGameRules;
class CDoDTeamPlay;
class CSPDoDRules;
class CDoDStatMgr;

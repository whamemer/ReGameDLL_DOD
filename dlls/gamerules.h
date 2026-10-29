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

// the state of the round, CGameRules::RoundState
#define ROUNDSTATE_PREROUND 0	// the players wait for the start of the round
#define ROUNDSTATE_RUNNING 1
#define ROUNDSTATE_ALLIES_WON 3	// the round is over, the next one comes after the pause
#define ROUNDSTATE_AXIS_WON 4
#define ROUNDSTATE_DRAW 5

class CGameRules
{
public:
	virtual void Init( void ) = 0;
	virtual void RefreshSkillData( void );
	virtual void Think( void ) = 0;
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity ) = 0;
	virtual BOOL FAllowFlashlight( void ) = 0;
	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ) = 0;
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon ) = 0;
	virtual BOOL IsMultiplayer( void ) = 0;
	virtual BOOL IsDeathmatch( void ) = 0;
	virtual BOOL IsTeamplay( void ) { return FALSE; };
	virtual BOOL IsCoOp( void ) = 0;
	virtual const char *GetGameDescription( void ) { return "Day of Defeat"; }
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress,
								  char *szRejectReason ) = 0;
	virtual void InitHUD( CBasePlayer *pl ) = 0;
	virtual void ClientDisconnected( edict_t *pClient ) = 0;
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer ) = 0;
	virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker ) { return TRUE; }
	virtual BOOL ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target ) { return TRUE; }
	virtual void PlayerSpawn( CBasePlayer *pPlayer ) = 0;
	virtual void PlayerThink( CBasePlayer *pPlayer ) = 0;
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer ) = 0;
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer ) = 0;
	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	void GetInitialSpawnSpot( CBasePlayer *pPlayer );			// non-virtual
	virtual BOOL AllowAutoTargetCrosshair( void ) { return TRUE; };
	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd ) { return FALSE; };
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer ) {}
	virtual void PlayerDeathThink( CBasePlayer *pPlayer ) = 0;
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled ) = 0;
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags = 0 ) = 0;
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags = 0 ) = 0;
	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ) = 0;
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon ) = 0;
	virtual float FlWeaponRespawnTime( CBasePlayerItem *pWeapon ) = 0;
	virtual float FlWeaponTryRespawn( CBasePlayerItem *pWeapon ) = 0;
	virtual Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon ) = 0;
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem ) = 0;
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem ) = 0;
	virtual int ItemShouldRespawn( CItem *pItem ) = 0;
	virtual float FlItemRespawnTime( CItem *pItem ) = 0;
	virtual Vector VecItemRespawnSpot( CItem *pItem ) = 0;
	virtual BOOL CanHaveAmmo( CBasePlayer *pPlayer, const char *pszAmmoName, int iMaxCarry );
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount ) = 0;
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo ) = 0;
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo ) = 0;
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo ) = 0;
	virtual float FlHealthChargerRechargeTime( void ) = 0;
	virtual float FlHEVChargerRechargeTime( void ) { return 0; }
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer ) = 0;
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer ) = 0;
	virtual const char *GetTeamID( CBaseEntity *pEntity ) = 0;
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget ) = 0;
	virtual int GetTeamIndex( const char *pTeamName ) { return -1; }
	virtual const char *GetIndexedTeamName( int teamIndex ) { return ""; }
	virtual BOOL IsValidTeam( const char *pTeamName ) { return TRUE; }
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib ) {}
	virtual const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer ) { return ""; }
	virtual BOOL PlayTextureSounds( void ) { return TRUE; }
	virtual BOOL PlayFootstepSounds( CBasePlayer *pl, float fvol ) { return TRUE; }
	virtual BOOL FAllowMonsters( void ) = 0;
	virtual void EndMultiplayerGame( void ) {}
	virtual int RoundState( void ) = 0;
	virtual void VoteKick( CBasePlayer *pPlayer, int vote, bool bAdmin ) {}
	virtual void SetRoundState( int state ) {}
	virtual int SetRoundState( void ) { return 0; }
	virtual void SetRestartTime( float f_Time ) {}
	virtual void SendTeamScores( int iTeam ) {}
	virtual float GetMaxWaveTime( int nTeam ) = 0;
	virtual float getWaveTime( int iTeam ) = 0;
	virtual void setWaveTime( int iTeam, float fTime ) {}
	virtual int TeamScores( int team ) = 0;
	virtual void SetTeamScores( int iTeam, int iScore ) {}
	virtual void AddToTeamScores( int team, int points ) {}
	virtual void setMapTime( void ) {}
	virtual void RestartRound( void ) {}
	virtual float getMapTime( void ) = 0;
	virtual bool IsRoundFrozen( void ) { return false; }
	virtual void SetClanMatch( bool b ) {}
	virtual bool IsClanMatch( void ) { return false; }
	virtual bool IsClanMatchActive( void ) { return false; }
	virtual unsigned short Camera( void ) = 0;
	virtual unsigned short usVoice( void ) = 0;
	virtual unsigned short bodyDamage( void ) = 0;
	virtual unsigned short roundRestartSound( void ) = 0;
	virtual unsigned short gerVoice( void ) = 0;
	virtual unsigned short Pain( void ) = 0;
	virtual unsigned short Smoke( void ) = 0;
	virtual unsigned short Prone( void ) = 0;
	virtual unsigned short BloodSprite( void ) = 0;
	virtual unsigned short BloodStream( void ) = 0;
	virtual unsigned short BulletTracers( void ) = 0;
	virtual unsigned short SparkShower( void ) = 0;
	virtual unsigned short ScreenFades( void ) = 0;
	virtual unsigned short BubbleTrails( void ) = 0;
	virtual unsigned short Bubbles( void ) = 0;
	virtual unsigned short Shards( void ) = 0;
	virtual unsigned short Explosion( void ) = 0;
	virtual unsigned short PopHelmet( void ) = 0;
	virtual unsigned short RoundReset( void ) = 0;
	virtual unsigned short StartTrail( void ) = 0;
	virtual void SetCamera( short s ) {}
	virtual void SetusVoice( short s ) {}
	virtual void SetbodyDamage( short s ) {}
	virtual void SetroundRestartSound( short s ) {}
	virtual void SetgerVoice( short s ) {}
	virtual void SetPain( short s ) {}
	virtual void SetSmoke( short s ) {}
	virtual void SetProne( short s ) {}
	virtual void SetBloodSprite( short s ) {}
	virtual void SetBloodStream( short s ) {}
	virtual void SetBulletTracers( short s ) {}
	virtual void SetSparkShower( short s ) {}
	virtual void SetScreenFades( short s ) {}
	virtual void SetBubbleTrails( short s ) {}
	virtual void SetBubbles( short s ) {}
	virtual void SetShards( short s ) {}
	virtual void SetExplosion( short s ) {}
	virtual void SetPopHelmet( short s ) {}
	virtual void SetRoundReset( short s ) {}
	virtual void SetRocketTrail( short s ) {}
	virtual int ReturnGameRules( void ) = 0;
	virtual void SetMapChangeFromTime( int iTime ) {}
};

extern CGameRules *InstallGameRules( void );

extern DLL_GLOBAL CGameRules *g_pGameRules;
class CDoDTeamPlay;
class CSPDoDRules;
class CDoDStatMgr;

class CSPDoDRules : public CGameRules
{
public:
	CSPDoDRules( void );
	virtual void Think( void );
	void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags );
	void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags );
	void PlayerDeathThink( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *szRejectReason );
	virtual void InitHUD( CBasePlayer *pPlayer );
	virtual void ClientDisconnected( edict_t *pClient );
	virtual BOOL IsMultiplayer( void );
	virtual BOOL IsDeathmatch( void );
	virtual BOOL IsCoOp( void );
	virtual BOOL FAllowFlashlight( void ) { return TRUE; }
	virtual BOOL AllowAutoTargetCrosshair( void );
	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon );
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity );
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon );
	virtual float FlWeaponRespawnTime( CBasePlayerItem *pWeapon );
	virtual float FlWeaponTryRespawn( CBasePlayerItem *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon );
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount );
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo );
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo );
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo );
	virtual float FlHealthChargerRechargeTime( void );
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );
	virtual const char *GetTeamID( CBaseEntity *pEntity ) { return ""; }
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual BOOL FAllowMonsters( void );

	virtual unsigned short Camera( void );
	virtual unsigned short usVoice( void );
	virtual unsigned short bodyDamage( void );
	virtual unsigned short roundRestartSound( void );
	virtual unsigned short gerVoice( void );
	virtual unsigned short Pain( void );
	virtual unsigned short Smoke( void );
	virtual unsigned short Prone( void );
	virtual unsigned short BloodSprite( void );
	virtual unsigned short BloodStream( void );
	virtual unsigned short BulletTracers( void );
	virtual unsigned short SparkShower( void );
	virtual unsigned short ScreenFades( void );
	virtual unsigned short BubbleTrails( void );
	virtual unsigned short Bubbles( void );
	virtual unsigned short Shards( void );
	virtual unsigned short Explosion( void );
	virtual unsigned short PopHelmet( void );
	virtual unsigned short RoundReset( void );
	virtual unsigned short StartTrail( void );
	virtual void SetCamera( short s );
	virtual void SetusVoice( short s );
	virtual void SetbodyDamage( short s );
	virtual void SetroundRestartSound( short s );
	virtual void SetgerVoice( short s );
	virtual void SetPain( short s );
	virtual void SetSmoke( short s );
	virtual void SetProne( short s );
	virtual void SetBloodSprite( short s );
	virtual void SetBloodStream( short s );
	virtual void SetBulletTracers( short s );
	virtual void SetSparkShower( short s );
	virtual void SetScreenFades( short s );
	virtual void SetBubbleTrails( short s );
	virtual void SetBubbles( short s );
	virtual void SetShards( short s );
	virtual void SetExplosion( short s );
	virtual void SetPopHelmet( short s );
	virtual void SetRoundReset( short s );
	virtual void SetRocketTrail( short s );
	int RoundState( void );
	int TeamScores( int iTeam );
	float getWaveTime( int iTeam );
	void setWaveTime( int iTeam, float fTime );
	float GetMaxWaveTime( int nTeam );
	void setMapTime( void );
	float getMapTime( void ) { return 0.0f; }
	bool IsRoundFrozen( void ) { return false; }
	void SetClanMatch( bool bMatchOn ) {}
	bool IsClanMatch( void ) { return false; }
	bool IsClanMatchActive( void ) { return false; }
	void SetRoundState( int iState );
	void SetTeamScores( int iTeam, int iScore );
	void SendTeamScores( int iTeam );
	void SetRestartTime( float fTime );
	void AddToTeamScores( int iTeam, int iScore );
	void SetMapChangeFromTime( int iTime );
	int ReturnGameRules( void );
	void Init( void );
	void VoteKick( CBasePlayer *pPlayer, int id, bool b );

protected:
	unsigned short m_Camera;
	unsigned short m_usVoice;
	unsigned short m_bodyDamage;
	unsigned short m_roundRestartSound;
	unsigned short m_gerVoice;
	unsigned short m_Pain;
	unsigned short m_Smoke;
	unsigned short m_BloodSprite;
	unsigned short m_Prone;
	unsigned short m_BloodStream;
	unsigned short m_BulletTracers;
	unsigned short m_SparkShower;
	unsigned short m_ScreenFades;
	unsigned short m_BubbleTrails;
	unsigned short m_Bubbles;
	unsigned short m_Shards;
	unsigned short m_Explosion;
	unsigned short m_DeadBodies;
	unsigned short m_PopHelmet;
	unsigned short m_RoundReset;
	unsigned short m_RocketTrail;
};

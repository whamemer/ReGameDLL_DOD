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
#ifndef PLAYER_H
#define PLAYER_H

#include "pm_materials.h"

#include "hintmessage.h"
#include "unisignals.h"

#define PLAYER_FATAL_FALL_SPEED 1024	// approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED 500	// approx 20 feet
#define DAMAGE_FOR_FALL_SPEED                                                                                          \
	(float) 100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )	// damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED 200
#define PLAYER_FALL_PUNCH_THRESHHOLD                                                                                   \
	(float) 280	 // won't punch player's screen/make scrape noise unless player falling at least this fast.

//
// Player PHYSICS FLAGS bits
//
#define PFLAG_ONLADDER ( 1 << 0 )
#define PFLAG_ONSWING ( 1 << 0 )
#define PFLAG_ONTRAIN ( 1 << 1 )
#define PFLAG_ONBARNACLE ( 1 << 2 )
#define PFLAG_DUCKING ( 1 << 3 )   // In the process of ducking, but totally squatted yet
#define PFLAG_USING ( 1 << 4 )	   // Using a continuous entity
#define PFLAG_OBSERVER ( 1 << 5 )  // player is locked in stationary cam mode. Spectators can move, observers can't.

//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
#define CSUITPLAYLIST 4	 // max of 4 suit sentences queued up at any time

#define SUIT_GROUP TRUE
#define SUIT_SENTENCE FALSE

#define SUIT_REPEAT_OK 0
#define SUIT_NEXT_IN_30SEC 30
#define SUIT_NEXT_IN_1MIN 60
#define SUIT_NEXT_IN_5MIN 300
#define SUIT_NEXT_IN_10MIN 600
#define SUIT_NEXT_IN_30MIN 1800
#define SUIT_NEXT_IN_1HOUR 3600

#define CSUITNOREPEAT 32

#define SOUND_FLASHLIGHT_ON "items/flashlight1.wav"
#define SOUND_FLASHLIGHT_OFF "items/flashlight1.wav"

#define TEAM_NAME_LENGTH 16

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
	PLAYER_PRONE1,
	PLAYER_PRONE2,
	PLAYER_PRONE3,
	PLAYER_RELOAD,
	PLAYER_SIGNAL,
	PLAYER_ATTACK2,
	PLAYER_ROLLGRENADE,
	PLAYER_RPGSHOULDER,
	PLAYER_EXPLODE_BACK,
	PLAYER_EXPLODE_FORWARD,
	PLAYER_BUT_SWING,
} PLAYER_ANIM;

#define MAX_ID_RANGE 2048
#define SBAR_STRING_SIZE 128

enum JoinState
{
	JUSTJOINED = 0,
	READYTOGO = 1,
	JOINED = 2,
};

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_END,
};

#define CHAT_INTERVAL 1.0f

// the stance of the player, pev->iuser3 (STANCE_PRONE, STANCE_PRONE_DEPLOYED and STANCE_MORTAR are in dod_shared.h)
#define STANCE_STANDING 0
// the player lies on the ground, with the bipod deployed or not
#define STANCE_IS_PRONE( iStance ) ( (unsigned int)( ( iStance ) - STANCE_PRONE ) <= ( STANCE_PRONE_DEPLOYED - STANCE_PRONE ) )

// the state of the sandbag, pev->vuser1.x (SANDBAG_DEPLOYED is in dod_shared.h)
#define SANDBAG_NONE 0
#define SANDBAG_ZONE 1	// the player stands in a sandbag zone

// the areas the player stands in, bits of CUnifiedSignals (HandleSignals reports the changes)
#define SIGNAL_SANDBAG 0x01		// a dod_trigger_sandbag
#define SIGNAL_CAPTUREAREA 0x02	// a dod_capture_area
#define SIGNAL_OBJECTAREA 0x04	// a dod_object_goal or a breakable that waits for an object

// the weapon classes (CLASS_* of dod_shared.h) a killed player drops, 0x3f0
#define CLASS_DROPPED_ON_DEATH ( CLASS_RIFLE | CLASS_SCOPED_RIFLE | CLASS_SMG | CLASS_UNUSED_128 | CLASS_MACHINEGUN | CLASS_ROCKET )

// the hints that were shown to the player (bits of m_flDisplayHistory), each one is shown only once
#define HINT_SPOTTED_FRIEND 0x00002
#define HINT_SPOTTED_ENEMY 0x00004
#define HINT_INJURED_TEAMMATE 0x00008
#define HINT_CAREFUL_AROUND_TEAMMATES 0x00010
#define HINT_KILLING_ENEMIES 0x00020
#define HINT_TOUCHED_AREA_CAPTURE 0x00040
#define HINT_TOUCHED_CONTROL_POINT 0x00080
#define HINT_PICKED_UP_OBJECT 0x00100
#define HINT_MG_FIRE_BETTER_DEPLOYED 0x00200
#define HINT_OUT_OF_AMMO 0x01000
#define HINT_PRONE 0x02000
#define HINT_LOW_STAMINA 0x04000
#define HINT_AREA_REQUIRES_OBJECT 0x08000
#define HINT_KILLED_WAVETIME_FIRST 0x10000
#define HINT_KILLED_WAVETIME_AGAIN 0x20000
#define HINT_MG_OVERHEAT 0x40000
#define HINT_KEPT_AFTER_RESTORE 0xFFF80031	// the hints that stay marked as shown after a restored game, the others come again

class CObject;
class CBasePlayerWeapon;
class CBasePlayer : public CBaseMonster
{
public:
	int m_iPlayerSpeed;
	int random_seed;
	int m_iPlayerSound;
	int m_iTargetVolume;
	int m_iWeaponVolume;
	int m_iExtraSoundTypes;
	float m_flStopExtraSoundTime;
	int m_afButtonLast;
	int m_afButtonPressed;
	int m_afButtonReleased;
	edict_t *m_pentSndLast;
	float m_flSndRoomtype;
	float m_flSndRange;
	float m_flFallVelocity;
	int m_fKnownItem;
	int m_fNewAmmo;
	unsigned int m_afPhysicsFlags;
	float m_fNextSuicideTime;
	float m_flTimeWeaponIdle;
	char m_szTextureName[CBTEXTURENAMEMAX];
	char m_chTextureType;
	int m_idrowndmg;
	int m_idrownrestored;
	int m_bitsHUDDamage;
	BOOL m_fInitHUD;
	BOOL m_fGameHUDInitialized;
	int m_iTrain;
	BOOL m_fWeapon;
	enum JoinState m_iJoiningState;
	EHANDLE m_pTank;
	float m_fDeadTime;
	int m_iUpdateTime;
	int m_iHideHUD;
	float m_iRespawnFrames;
	int m_iClientHealth;
	int m_iClientHideHUD;
	int m_iClientFOV;
	int m_nCustomSprayFrames;
	float m_flNextDecalTime;
	CBasePlayerItem *m_rgpPlayerItems[6];
	CBasePlayerItem *m_pActiveItem;
	CBasePlayerItem *m_pClientActiveItem;
	CBasePlayerItem *m_pLastItem;
	int m_rgAmmo[32];
	int m_rgAmmoLast[32];
	int m_rgItems[5];
	char m_szTeamName[TEAM_NAME_LENGTH];
	static TYPEDESCRIPTION m_playerSaveData[22];
	int m_irdytospawn;
	int m_imissedwave;
	int m_ilastteam;
	int m_ijustjoined;
	int m_chatMsg;
	int m_deathMsg;
	int m_handSignal;
	float m_nextRunThink;
	float m_flIdleTimer;
	float m_inextprone;
	int m_voteid;
	int m_iFOV;
	float m_fGunDropTime;
	int m_iNextClass;
	bool m_bIsRandomClass;
	EHANDLE m_hObserverTarget;
	float m_flNextObserverInput;
	int m_iObserverWeapon;
	int m_iObserverLastMode;
	int m_bIsObserver;
	BOOL HasObject;
	CObject *m_pObject;
	char *m_sPlayerModel;
	float m_fNextStamina;
	int m_nMGAmmoCount;
	int m_nGenericAmmoCount;
	int m_iClassSpeed;
	float m_fSpeedFactor;
	bool m_bSlowedByHit;
	float m_flUnslowTime;
	float m_flNextVoice;
	float m_flNextHand;
	int lastMenuSelected;
	int m_izSBarState[SBAR_END];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[SBAR_STRING_SIZE];
	char m_SbarString1[SBAR_STRING_SIZE];
	EHANDLE m_hLastIDTarget;
	float m_flLastTalkTime;
	int i_cutScenes;
	int i_seenAllieSpawnScene;
	int i_seenAxisSpawnScene;
	int i_seenAllieWinScene;
	int i_seenAxisWinScene;
	int i_seenAllieLooseScene;
	int i_seenAxisLooseScene;
	int i_seenAllieDrawScene;
	int i_seenAxisDrawScene;
	CUnifiedSignals m_signals;
	int m_iCapAreaIndex;
	int m_iCapAreaIconIndex;
	int m_iObjectAreaIndex;
	float m_fHandleSignalsTime;
	int m_iObjScore;
	int m_iDeaths;
	int m_iNumTKs;
	BOOL m_bBazookaDeployed;
	int m_iMinimap;
	bool m_bShowHints;
	bool m_bAutoReload;
	CHintMessageQueue m_hintMessageQueue;
	long m_flDisplayHistory;
	char m_szAnimExtention[32];
	char m_szAnimReloadExt[32];
	int m_iGaitsequence;
	float m_flGaitframe;
	float m_flGaityaw;
	Vector m_prevgaitorigin;
	float m_flPitch;
	float m_flYaw;
	float m_flGaitMovement;
	float m_flYawModifier;
	int m_iCurrentAnimationState;
	int m_iCurrentAnimationSequence;
	int m_pszSavedWeaponModel;
	int m_ianimupdate;
	float m_flNextMapMarkerTime;
	int m_iMapMarker;
	int m_iWeapons2;

	// Spectator camera
	void Observer_FindNextPlayer( bool bReverse );
	void Observer_HandleButtons();
	void Observer_SetMode( int iMode );
	void Observer_CheckTarget();
	void Observer_CheckProperties();
	bool Observer_PlayerValidTarget( CBaseEntity *pEnt, bool bSameTeam );
	void SendObserverTargetWeaponAnim();
	void StopObserver();
	void Observer_SpectatePlayer( int idx );
	int IsObserver() { return pev->iuser1; };
	void StartObserver( Vector vecPosition, Vector vecViewAngle );
	void StartDeathCam( void );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void PreThink( void );
	virtual void PostThink( void );
	virtual void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr,
							  int bitsDamageType );
	virtual int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	virtual int TakeHealth( float flHealth, int bitsDamageType );
	virtual void Killed( entvars_t *pevAttacker, int iGib );
	virtual void Jump( void );
	virtual void Duck( void );
	void WaterMove( void );
	virtual void UpdateClientData( void );
	void SetCustomDecalFrames( int nFrames );
	int GetCustomDecalFrames( void );
	virtual int AddPlayerItem( CBasePlayerItem *pItem );
	virtual int RemovePlayerItem( CBasePlayerItem *pItem );
	void DropPlayerItem( char *pszItemName, bool bForceDrop );
	BOOL HasPlayerItem( CBasePlayerItem *pCheckItem );
	BOOL HasNamedPlayerItem( const char *pszItemName );
	BOOL HasPlayerItemFromID( int nID );
	BOOL HasWeapons( void );  // do I have ANY weapons?
	bool HasWeapons( int weaponId );
	void SelectPrevItem( int iItem );
	void SelectNextItem( int iItem );
	void SelectLastItem( void );
	void SelectItem( const char *pstr );
	void ItemPreFrame( void );
	void ItemPostFrame( void );
	virtual Vector GetGunPosition( void );
	edict_t *GiveNamedItem( const char *szName );
	static int GetAmmoIndex( const char *psz );
	int AmmoInventory( int iAmmoIndex );
	void TabulateAmmo( void );
	virtual int GiveAmmo( int iAmount, const char *szName, int iMax );
	void SendAmmoUpdate( void );
	virtual Vector BodyTarget( const Vector &posSrc )
	{
		return Center() + pev->view_ofs * RANDOM_FLOAT( 0.5, 1.1 );
	};

	virtual BOOL IsAlive( void ) { return ( pev->deadflag == DEAD_NO ) && pev->health > 0; }
	virtual BOOL ShouldFadeOnDeath( void ) { return FALSE; }
	virtual BOOL IsPlayer( void ) { return TRUE; }
	virtual BOOL IsNetClient( void ) { return TRUE; }
	BOOL IsOnLadder( void ) { return ( pev->movetype == MOVETYPE_FLY ); }
	virtual int Classify( void ) { return CLASS_PLAYER; }
	virtual int ObjectCaps( void ) { return CBaseMonster ::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual const char *TeamID( void )
	{
		if ( pev == NULL )	// Not fully connected yet
			return "";

		// return their team name
		return m_szTeamName;
	}

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	CBasePlayerWeapon *GetPrimaryWeapon();
	void PackDeadPlayerItems( void );
	void RemoveAllItems( BOOL removeSuit );
	BOOL SwitchWeapon( CBasePlayerItem *pWeapon, BOOL bJustSpawned );
	void DelayedSpawn( void );
	void EXPORT SpawnThink( void );
	void EXPORT ServerSpawnCheck();
	int PlayerStatus();
	void EXPORT ClientDisconnect();
	void UTIL_EdictScreenFade( edict_t *pEdict, const Vector &color, float fadeTime, float fadeHold, int alpha,
							   int flags );
	void PushPlayer( void );
	void UpdatePlayerSound( void );
	virtual void DeathSound( void );
	virtual void ImpulseCommands( void );
	void EnableControl( BOOL fControl );
	void PlayerUse( void );
	void EXPORT PlayerDeathThink( void );
	virtual BOOL FBecomeProne( void );
	void ForceClientDllUpdate( void );
	void DeathMessage( entvars_t *pevKiller );
	void VoteMenu( int i, int j );
	void ForceZoomOut();
	void UpdateScope( int weapon_id );
	bool ShouldGunLower();
	void AddObjectIcon( char *szIconName );
	void RemoveObjectIcon();
	bool PlayerHasExplodingGrenades();
	BOOL FlashlightIsOn( void );
	void FlashlightTurnOn( void );
	void FlashlightTurnOff( void );
	void GetObject( CObject *pObject );
	void DropObject();
	void Stamina( void );
	float GetStamina() { return pev->fuser4; }
	void SetStamina( float s ) { pev->fuser4 = s; }
	void DropGenericAmmo();
	BOOL DropItemByClass( int iClass );
	bool CheckPlayerSpeed( void );
	bool SetSpeed( int speed );
	void ShowMenu( int slots, int time, char *text, int bitsvalid, int menu );
	void VoiceCom( int v );
	void HandSignal( int v );
	void ClearVoiceMenu();
	void VoiceMenu( int i);
	void InitStatusBar( void );
	void UpdateStatusBar( void );
	void PopHelmet( Vector *vecSrc );
	void SetClientAreaIcon( int areaIndex, bool state );
	void HandleSignals();
	void AddFrags( int num, BOOL bSendMsgToAll );
	void AddObjScore( int num, BOOL bSendMsgToAll );
	virtual void AddPoints( int score, BOOL bAllowNegativeScore );
	virtual void AddPointsToTeam( int score, BOOL bAllowNegativeScore );
	bool HintMessage( const char *pMessage, BOOL bOverride );
	// Not in the original (suit system removed); kept because items.cpp still calls it.
	void SetPrefsFromUserinfo( char *infobuffer );
	void StudioEstimateGait( void );
	void StudioPlayerBlend( int *pBlend, float *pflBlend );
	void CalculatePitchBlend( void );
	void CalculateYawBlend( void );
	void StudioProcessGait( void );
	Activity SetIdealActivity( Activity NewActivity );
	void SetAnimation( PLAYER_ANIM playerAnim );
	void SetWeaponAnimType( const char *szExtention );
	void FireMapMarker();
	void NextMapMarker();
	void PrevMapMarker();
	void SetMapMarker( int marker );
	void SpawnClientSideCorpse();
	void SetHasWeapon( int weaponId );
	void ClearHasWeapon( int weaponId );
	void RemoveAllWeaponFlags();
	int GetWeapons2() { return m_iWeapons2; }
	bool IsInMGDeploy();
	bool IsProneDeployed();
	bool IsInSandbagZone();
	bool IsSandbagDeployed();
	bool IsProne();
	void SetProneState( int state );
	void SetSandbagState( int state );
	int GetProneState() { return pev->iuser3; }
	void MortarDeploy( void );
	void MortarUnDeploy();
	bool IsInMortarDeploy();
	void GoProne( void );
	void UnProne();
	bool IsDucking();
	bool IsParatrooper();
	bool IsBritish();
	void MortarFireCommand( byte command );

	// DoD hint / object helpers used tree-wide (no standalone symbols: inlined).
	static BOOL IsProne( CBasePlayer *pPlayer );
	static BOOL IsSandbagDeployed( CBasePlayer *pPlayer );
};

#if defined( __GNUC__ ) && defined( __i386__ )  // the layout was taken from the 32-bit gcc build of the original, the MSVC ABI differs
static_assert( sizeof( CBasePlayer ) == 2132, "CBasePlayer size must be 2132 (dod.so DWARF)" );
#endif

#define AUTOAIM_2DEGREES 0.0348994967025
#define AUTOAIM_5DEGREES 0.08715574274766
#define AUTOAIM_8DEGREES 0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669

extern int iVoiceToHandSignal[];
extern int gmsgHudText;
extern BOOL gInitHUD;

extern int gmsgScope;
extern int gmsgFrags;
extern int gmsgObjScore;
extern int gmsgObject;
extern int gmsgReloadDone;
extern int gmsgClientAreas;
extern int gmsgYouDied;
extern int gmsgBloodPuff;
extern int gmsgResetSens;
extern int gmsgPStatus;
extern int gmsgPClass;
extern int gmsgMapMarker;
extern int gmsgHandSignal;
extern int gmsgSendCorpse;
extern int gmsgWeather;
extern int gmsgCapMsg;

edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer, BOOL bJustChecking );

#endif	// PLAYER_H

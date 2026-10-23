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

#define PLAYER_FATAL_FALL_SPEED 1024	// approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED 580	// approx 20 feet
#define DAMAGE_FOR_FALL_SPEED                                                                                          \
	(float) 100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )	// damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED 200
#define PLAYER_FALL_PUNCH_THRESHHOLD                                                                                   \
	(float) 350	 // won't punch player's screen/make scrape noise unless player falling at least this fast.

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

// DoD DWARF: enum JoinState { JUSTJOINED, READYTOGO, JOINED }
enum JoinState
{
	JUSTJOINED = 0,
	READYTOGO = 1,
	JOINED = 2,
};

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

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_END,
};

#define CHAT_INTERVAL 1.0f

// DoD DWARF player.h: CUnifiedSignals (methods are inlined in dod.so - no symbols).
class CUnifiedSignals
{
  public:
	int m_flSignal;
	int m_flState;

	CUnifiedSignals()
	{
		m_flSignal = 0;
		m_flState = 0;
	}
	int Update()
	{
		int old = m_flState;
		int sig = m_flSignal;
		m_flSignal = 0;
		m_flState = sig;
		return sig ^ old;
	}
	void Signal( int s ) { m_flSignal |= s; }
	int GetState() { return m_flState; }
};

class CObject;
class CBasePlayerWeapon;
class CBasePlayer : public CBaseMonster
{
  public:
	// DoD DWARF CBasePlayer offset 940 (0x3ac). Member order/sizes are exact.
	int m_iPlayerSpeed;	 // 940
	int random_seed;	 // 944
	int m_iPlayerSound;	 // 948
	int m_iTargetVolume;	 // 952
	int m_iWeaponVolume;	 // 956
	int m_iExtraSoundTypes;	 // 960
	float m_flStopExtraSoundTime;  // 964
	int m_afButtonLast;	 // 968
	int m_afButtonPressed;	 // 972
	int m_afButtonReleased;	 // 976
	edict_t *m_pentSndLast;	 // 980
	float m_flSndRoomtype;	 // 984
	float m_flSndRange;	 // 988
	float m_flFallVelocity;	 // 992
	int m_fKnownItem;	 // 996
	int m_fNewAmmo;		 // 1000
	unsigned int m_afPhysicsFlags;	// 1004
	float m_fNextSuicideTime;	// 1008
	float m_flTimeWeaponIdle;	// 1012
	char m_szTextureName[CBTEXTURENAMEMAX];	 // 1016
	char m_chTextureType;			 // 1029
	int m_idrowndmg;	 // 1032
	int m_idrownrestored;	 // 1036
	int m_bitsHUDDamage;	 // 1040
	BOOL m_fInitHUD;	 // 1044
	BOOL m_fGameHUDInitialized;  // 1048
	int m_iTrain;		 // 1052
	BOOL m_fWeapon;		 // 1056
	enum JoinState m_iJoiningState;	 // 1060
	EHANDLE m_pTank;	 // 1064
	float m_fDeadTime;	 // 1072
	int m_iUpdateTime;	 // 1076
	int m_iHideHUD;		 // 1080
	float m_iRespawnFrames;	 // 1084
	int m_iClientHealth;	 // 1088
	int m_iClientHideHUD;	 // 1092
	int m_iClientFOV;	 // 1096
	int m_nCustomSprayFrames;  // 1100
	float m_flNextDecalTime;   // 1104
	CBasePlayerItem *m_rgpPlayerItems[6];	 // 1108
	CBasePlayerItem *m_pActiveItem;		 // 1132
	CBasePlayerItem *m_pClientActiveItem;	 // 1136
	CBasePlayerItem *m_pLastItem;		 // 1140
	int m_rgAmmo[32];			 // 1144
	int m_rgAmmoLast[32];			 // 1272
	int m_rgItems[5];			 // 1400
	char m_szTeamName[TEAM_NAME_LENGTH];	 // 1420
	static TYPEDESCRIPTION m_playerSaveData[22];
	int m_irdytospawn;	 // 1436
	int m_imissedwave;	 // 1440
	int m_ilastteam;	 // 1444
	int m_ijustjoined;	 // 1448
	int m_chatMsg;		 // 1452
	int m_deathMsg;		 // 1456
	int m_handSignal;	 // 1460
	float m_nextRunThink;	 // 1464
	float m_flIdleTimer;	 // 1468
	float m_inextprone;	 // 1472
	int m_voteid;		 // 1476
	int m_iFOV;		 // 1480
	float m_fGunDropTime;	 // 1484
	int m_iNextClass;	 // 1488
	bool m_bIsRandomClass;	 // 1492
	EHANDLE m_hObserverTarget;	  // 1496
	float m_flNextObserverInput;	  // 1504
	int m_iObserverWeapon;		  // 1508
	int m_iObserverLastMode;	  // 1512
	int m_bIsObserver;		  // 1516
	BOOL HasObject;			  // 1520
	CObject *m_pObject;		  // 1524
	char *m_sPlayerModel;		  // 1528
	float m_fNextStamina;		  // 1532
	int m_nMGAmmoCount;		  // 1536
	int m_nGenericAmmoCount;	  // 1540
	int m_iClassSpeed;		  // 1544
	float m_fSpeedFactor;		  // 1548
	bool m_bSlowedByHit;		  // 1552
	float m_flUnslowTime;		  // 1556
	float m_flNextVoice;		  // 1560
	float m_flNextHand;		  // 1564
	int lastMenuSelected;		  // 1568
	int m_izSBarState[SBAR_END];	  // 1572
	float m_flNextSBarUpdateTime;	  // 1588
	float m_flStatusBarDisappearDelay;  // 1592
	char m_SbarString0[SBAR_STRING_SIZE];  // 1596
	char m_SbarString1[SBAR_STRING_SIZE];  // 1724
	EHANDLE m_hLastIDTarget;	  // 1852
	float m_flLastTalkTime;		  // 1860
	int i_cutScenes;		  // 1864
	int i_seenAllieSpawnScene;	  // 1868
	int i_seenAxisSpawnScene;	  // 1872
	int i_seenAllieWinScene;	  // 1876
	int i_seenAxisWinScene;		  // 1880
	int i_seenAllieLooseScene;	  // 1884
	int i_seenAxisLooseScene;	  // 1888
	int i_seenAllieDrawScene;	  // 1892
	int i_seenAxisDrawScene;	  // 1896
	CUnifiedSignals m_signals;	  // 1900
	int m_iCapAreaIndex;		  // 1908
	int m_iCapAreaIconIndex;	  // 1912
	int m_iObjectAreaIndex;		  // 1916
	float m_fHandleSignalsTime;	  // 1920
	int m_iObjScore;		  // 1924
	int m_iDeaths;			  // 1928
	int m_iNumTKs;			  // 1932
	BOOL m_bBazookaDeployed;	  // 1936
	int m_iMinimap;			  // 1940
	bool m_bShowHints;		  // 1944
	bool m_bAutoReload;		  // 1945
	CHintMessageQueue m_hintMessageQueue;  // 1948
	long m_flDisplayHistory;	  // 1996
	char m_szAnimExtention[32];	  // 2000
	char m_szAnimReloadExt[32];	  // 2032
	int m_iGaitsequence;		  // 2064
	float m_flGaitframe;		  // 2068
	float m_flGaityaw;		  // 2072
	Vector m_prevgaitorigin;	  // 2076
	float m_flPitch;		  // 2088
	float m_flYaw;			  // 2092
	float m_flGaitMovement;		  // 2096
	float m_flYawModifier;		  // 2100
	int m_iCurrentAnimationState;	  // 2104
	int m_iCurrentAnimationSequence;  // 2108
	int m_pszSavedWeaponModel;	  // 2112
	int m_ianimupdate;		  // 2116
	float m_flNextMapMarkerTime;	  // 2120
	int m_iMapMarker;		  // 2124
	int m_iWeapons2;		  // 2128

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
	BOOL IsOnLadder( void );
	virtual int Classify( void );
	virtual int ObjectCaps( void ) { return CBaseMonster ::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual const char *TeamID( void );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	CBasePlayerWeapon *GetPrimaryWeapon();
	void PackDeadPlayerItems( void );
	void RemoveAllItems( BOOL removeSuit );
	BOOL SwitchWeapon( CBasePlayerItem *pWeapon, BOOL bSetActive );
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
	void CheatImpulseCommands( int iImpulse );
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
	BOOL HasObjectOfGroup( const char *grp );
	BOOL IsCarryingObject( void );
	bool PlayerHasExplodingGrenades();
	BOOL FlashlightIsOn( void );
	void FlashlightTurnOn( void );
	void FlashlightTurnOff( void );
	void GetObject( CObject *pObject );
	void DropObject();
	void Stamina( void );
	float GetStamina();
	void SetStamina( float );
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
	bool HintMessageOnce( const char *pMessage );
	// No symbol in dod.so (suit system removed); kept because items.cpp still calls it.
	void SetSuitUpdate( const char *name, int fgroup, int iNoRepeat );
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
	static bool HintMessage( CBasePlayer *pPlayer, const char *pMessage, BOOL bOverride );
	static BOOL IsProne( CBasePlayer *pPlayer );
	static BOOL IsSandbagDeployed( CBasePlayer *pPlayer );
};

#if defined( __GNUC__ ) && defined( __i386__ )  // the layout was taken from the 32-bit gcc build of dod.so, the MSVC ABI differs
static_assert( sizeof( CBasePlayer ) == 2132, "CBasePlayer size must be 2132 (dod.so DWARF)" );
#endif

#define AUTOAIM_2DEGREES 0.0348994967025
#define AUTOAIM_5DEGREES 0.08715574274766
#define AUTOAIM_8DEGREES 0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669

extern int iVoiceToHandSignal[];
extern int gmsgHudText;
extern BOOL gInitHUD;

// DoD user messages owned by player.cpp (DWARF: gmsg* decl_file player.cpp).
extern int gmsgScope;
extern int gmsgFrags;
extern int gmsgObjScore;
extern int gmsgObject;
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

// DoD spawn helper (2-arg per binary; player.cpp owns it).
edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer, BOOL bJustChecking );

#endif	// PLAYER_H

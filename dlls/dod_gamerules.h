#ifndef DOD_GAMERULES_H
#define DOD_GAMERULES_H
class CBasePlayer;
class CBasePlayerItem;
class CDodParaRoundTimer : public CBaseEntity
{
  public:
	float m_fUpdateTimer;  // 344, 30.0 s
	float m_fRoundTimer;   // 348, default 240.0 on reset
	float m_fLastThinkTime;// 352
	bool m_bTimer;		   // 356
	void Spawn( void );
	void StartTimer( void );
	void StopTimer( void );
	void ResetTimer( void );
	void SendStatus( void );
	void SendStatus( class CBasePlayer *p );
	void EXPORT TimerThink( void );
};

typedef struct
{
	bool m_bAlliesInfiniteLives;
	bool m_bAxisInfiniteLives;
	bool m_bAlliesArePara;
	bool m_bAxisArePara;
	bool m_bAlliesAreBrit;
	int m_iPointsAlliesElim;
	int m_iPointsAxisElim;
	string_t m_sAlliesElimTarget;
	string_t m_sAxisElimTarget;
	bool m_bUseTimer;
	string_t m_sTimerExpireTarget;
	int m_iTimerTeam;
	int m_iPointsTimerExpires;
	float m_fAlliesRespawnFactor;
	float m_fAxisRespawnFactor;
} gameplay_rules_t;
// info_doddetect (dod_gamerules.cpp) - DWARF-canonical (gdb ptype /o CDoDDetect)
// legacy trailing ints kept for exact layout parity (unused by the binary body)
class CDoDDetect : public CBaseEntity
{
  public:
	gameplay_rules_t m_GamePlayRules;   // 344 (48)
	int m_iAlliesInfiniteLives;			// 392
	int m_iAxisInfiniteLives;			// 396
	int m_iAlliesPara;					// 400
	int m_iAxisPara;					// 404
	int m_iAlliesElimPoints;			// 408
	int m_iAxisElimPoints;				// 412
	string_t m_sMaster;					// 416
	bool IsMasteredOn( void );
	CDoDDetect( void );
	void Spawn( void );
	virtual void KeyValue( KeyValueData *pkvd );
};

class CDoDPreRound : public CPointEntity
{
  public:
	void Launch( int state );
	void KeyValue( KeyValueData *pkvd );
	int m_roundstate;
	string_t m_allieswin;
	string_t m_alliesloose;
	string_t m_axiswin;
	string_t m_axisloose;
	string_t m_draw;
};
class CDoDTeamPlay : public CGameRules
{
  public:
	virtual void RefreshSkillData( void );
	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pItem );
	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags = 0 );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pevKiller, entvars_t *pevInflictor, int flags = 0 );
	virtual void Think( void );
	void RestartRound( void );
	void TeamWon( int iTeamWon );
	void EnumerateTeams( int *aA, int *nA, int *aX, int *nX );
	void TimerExpired( void );
	void DetectGameRules( void );
	void GoodToGo( void );
	virtual void InitHUD( CBasePlayer *pPlayer );
	virtual void ClientDisconnected( edict_t *pClient );
	void RoundUnfreeze( void );
	void RoundFreeze( void );
	void SendClientGamerules( CBasePlayer *pPlayer );
	void SendTeamScores( int m_iTeam );
	void SendObjectiveStatus( CBasePlayer *pPlayer );
	void SetNextMap( void );
	void StartClanMatch( void );
	void CheckClanRestart( void );
	void VoiceCommSelect( CBasePlayer *pPlayer, int v );
	int GetWeaponId( char *wn );
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress,
								  char *szRejectReason );
	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd );
	// verified vtable slots (binary CGameRules order): RoundState vtbl+0xfc, BloodSprite vtbl+0x168,
	// BulletTracers vtbl+0x170, SparkShower vtbl+0x174, Explosion vtbl+0x188, etc.
	virtual int RoundState( void );
	virtual bool IsClanMatch( void );
	virtual bool IsClanMatchActive( void );
	virtual BOOL PlayTextureSounds( void ) { return FALSE; }
	virtual const char *GetGameDescription( void ) { return "Day of Defeat"; }
	virtual void EndMultiplayerGame( void ) { GoToIntermission(); }
	virtual float getMapTime( void ) { return m_flDoDMapTime; }
	virtual void setMapTime( void );
	virtual bool IsRoundFrozen( void );
	virtual int ReturnGameRules( void );
	virtual void SetMapChangeFromTime( int iTime );
	const char *GetIndexedTeamName( int teamIndex );
	float getWaveTime( int iTeam );
	void SetTeamScores( int iTeam, int iScore );
	bool IsAlliesBrit( void );
	unsigned short Camera( void );
	unsigned short usVoice( void );
	unsigned short bodyDamage( void );
	unsigned short roundRestartSound( void );
	unsigned short gerVoice( void );
	unsigned short Pain( void );
	unsigned short Smoke( void );
	unsigned short Prone( void );
	virtual unsigned short BloodSprite( void );
	unsigned short BloodStream( void );
	virtual unsigned short BulletTracers( void );
	virtual unsigned short SparkShower( void );
	unsigned short ScreenFades( void );
	unsigned short BubbleTrails( void );
	unsigned short Bubbles( void );
	unsigned short Shards( void );
	unsigned short Explosion( void );
	unsigned short PopHelmet( void );
	unsigned short RoundReset( void );
	void SetusVoice( short s );
	void SetbodyDamage( short s );
	void SetroundRestartSound( short s );
	void SetgerVoice( short s );
	void SetPain( short s );
	void SetSmoke( short s );
	void SetProne( short s );
	void SetBloodSprite( short s );
	void SetBloodStream( short s );
	void SetBulletTracers( short s );
	void SetSparkShower( short s );
	void SetScreenFades( short s );
	void SetBubbleTrails( short s );
	void SetBubbles( short s );
	void SetShards( short s );
	void SetExplosion( short s );
	void SetPopHelmet( short s );
	void SetRoundReset( short s );
	void SetRocketTrail( short s );
	unsigned short StartTrail( void );
	void SetCamera( short s );
	void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer );
	BOOL PlayFootstepSounds( CBasePlayer *pl, float fvol );
	float GetMaxWaveTime( int nTeam );
	void setWaveTime( int iTeam, float fTime );
	int GetClassLimit( int cls );
	BOOL IsTeamPara( int team );
	BOOL IsTeamInfinite( int team );
	BOOL IsAlliesInfinite( void );
	BOOL IsAxisInfinite( void );
	BOOL IsAlliesPara( void );
	BOOL IsAxisPara( void );
	void Init( void );
	char *COM_Parse( char *data );
	BOOL COM_TokenWaiting( char *buffer );
	void PreparePlayerForSpawn( CBasePlayer *pPlayer );
	int CountClasses( int team );
	BOOL ReachedClassLimit( int cls );
	BOOL CanPlayerJoinClass( int playerclass );
	void ChooseRandomClass( CBasePlayer *pPlayer );
	void UpdateData( CBasePlayer *pPlayer );
	void LevelChangeResets( CBasePlayer *pPlayer );
	void ResetForDoDCamera( void );
	void StartClanRestart( void );
	int GetClassMask( int team );
	void ShowClassSelectMenu( CBasePlayer *pPlayer );
	BOOL AllowSpawnableBazookas( void );
	void AddToTeamScores( int team, int points );
	void SetRoundState( int state );
	void SetRestartTime( float f_Time );
	void TeamSelect( CBasePlayer *pPlayer, int team );
	void ClassSelect( CBasePlayer *pPlayer, int cls );
	virtual void VoteKick( CBasePlayer *pPlayer, int vote, bool bAdmin );
	void VoteKickTimeUp( void );
	void CheckOneLifeRoundConditions( void );
	const char *GetPlayerClassName( int cls );
	virtual BOOL IsMultiplayer( void );
	virtual BOOL IsDeathmatch( void );
	virtual BOOL IsTeamplay( void );
	virtual BOOL IsCoOp( void );
	virtual BOOL FAllowFlashlight( void );
	virtual BOOL AllowAutoTargetCrosshair( void );
	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon );
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity );
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual void PlayerDeathThink( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
	virtual BOOL ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target );
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
	virtual float FlHEVChargerRechargeTime( void );
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );
	virtual const char *GetTeamID( CBaseEntity *pEntity );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer );
	virtual BOOL FAllowMonsters( void );
	int m_RoundDoesCount;
	int m_DoDCameraState;
	int m_RoundParaCanJoin;
	float m_flGoodToGoTime;
	float m_flRoundTime;
	float m_flCanJoinTime;
	float m_flRestartTime;
	float m_flFinalCheckTime;
	float m_flDoDMapTime;
	float m_flAlliesWaveTime;
	float m_flAxisWaveTime;
	float m_flAlliesRespawn;
	float m_flAxisRespawn;
	int m_iTeamScores[32];
	char *m_szTeamNames[32];
	float m_flSpamResetTime;
	bool m_bClanMatch;
	bool m_bClanMatchActive;
	char m_szAuthID[32];
	class CBasePlayer *m_vKickENT;
	int m_vKick;
	int m_vKickVotes;
	int m_vKickSessionState;
	float m_vKickTime;
	float m_rRoundStartCount;
	int m_rReadyToUnFreeze;
	int gr_gameRules;
	gameplay_rules_t m_GamePlayRules;
	int m_iNumAlliesAlive;
	int m_iNumAxisAlive;
	int m_iNumAlliesOnTeam;
	int m_iNumAxisOnTeam;
	unsigned short m_Camera;
	unsigned short m_usVoice;
	unsigned short m_bodyDamage;
	unsigned short m_roundRestartSound;
	unsigned short m_gerVoice;
	unsigned short m_Pain;
	unsigned short m_Smoke;
	unsigned short m_Prone;
	unsigned short m_BloodSprite;
	unsigned short m_BloodStream;
	unsigned short m_BulletTracers;
	unsigned short m_SparkShower;
	unsigned short m_ScreenFades;
	unsigned short m_BubbleTrails;
	unsigned short m_Bubbles;
	unsigned short m_Shards;
	unsigned short m_Explosion;
	unsigned short m_PopHelmet;
	unsigned short m_RoundReset;
	unsigned short m_RocketTrail;
	char sNextMap[32];
	int MapChangeFromTime;
	bool m_bIsRoundFrozen;
	BOOL m_DisableDeathMessages;
	BOOL m_DisableDeathPenalty;
	class CDodParaRoundTimer *m_pParaTimer;
	int m_RoundState;
	BOOL m_bRoundRestarting;
	BOOL m_bAwaitingReadyRestart;
	BOOL m_bHeardAlliesReady;
	BOOL m_bHeardAxisReady;
	float m_flRestartRoundTime;
	float m_fLogScoresTime;
	float m_flIntermissionEndTime;
	BOOL m_iEndIntermissionButtonHit;
	int TeamScores( int team );
	virtual void SetClanMatch( bool b );
	void ReturnToWarmup( void );
	void SendWaveStatus( CBasePlayer *pPlayer, int wave );
	void SendMOTDToClient( edict_t *pClient );
	void CopyGamePlayLogic( const gameplay_rules_t &rules );
	// ---- new virtuals introduced by CDoDTeamPlay (binary vtable idx 122..124) ----
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, int iTeam, int bKill, int bGib );
	virtual void GoToIntermission( void );
	virtual void ChangeLevel( void );
	BOOL IsValidTeam( const char *pTeamName );
	int GetTeamIndex( const char *pTeamName );
	void ReadInClassInfo( void );
	CDoDTeamPlay( void );

};

static const char g_szTeamNames[4][128] = { "", "Allies", "Axis", "Spectators" };
#endif

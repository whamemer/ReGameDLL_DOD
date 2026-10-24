#ifndef DOD_GAMERULES_H
#define DOD_GAMERULES_H
class CBasePlayer;
class CBasePlayerItem;
struct GamePlayRules_t
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
};
typedef GamePlayRules_t gameplay_rules_t;
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
	virtual int RoundState( void ) { return m_RoundState; }
	virtual bool IsClanMatch( void );
	virtual bool IsClanMatchActive( void );
	virtual BOOL PlayTextureSounds( void ) { return FALSE; }
	virtual const char *GetGameDescription( void ) { return "Day of Defeat"; }
	virtual void EndMultiplayerGame( void ) { GoToIntermission(); }
	virtual float getMapTime( void ) { return m_flDoDMapTime; }
	virtual void setMapTime( void );
	virtual bool IsRoundFrozen( void ) { return m_bIsRoundFrozen; }
	virtual int ReturnGameRules( void ) { return gr_gameRules; }
	virtual void SetMapChangeFromTime( int iTime );
	const char *GetIndexedTeamName( int teamIndex );
	float getWaveTime( int iTeam );
	void SetTeamScores( int iTeam, int iScore );
	bool IsAlliesBrit( void );
	unsigned short Camera( void ) { return m_Camera; }
	unsigned short usVoice( void ) { return m_usVoice; }
	unsigned short bodyDamage( void ) { return m_bodyDamage; }
	unsigned short roundRestartSound( void ) { return m_roundRestartSound; }
	unsigned short gerVoice( void ) { return m_gerVoice; }
	unsigned short Pain( void ) { return m_Pain; }
	unsigned short Smoke( void ) { return m_Smoke; }
	unsigned short Prone( void ) { return m_Prone; }
	virtual unsigned short BloodSprite( void ) { return m_BloodSprite; }
	unsigned short BloodStream( void ) { return m_BloodStream; }
	virtual unsigned short BulletTracers( void ) { return m_BulletTracers; }
	virtual unsigned short SparkShower( void ) { return m_SparkShower; }
	unsigned short ScreenFades( void ) { return m_ScreenFades; }
	unsigned short BubbleTrails( void ) { return m_BubbleTrails; }
	unsigned short Bubbles( void ) { return m_Bubbles; }
	unsigned short Shards( void ) { return m_Shards; }
	unsigned short Explosion( void ) { return m_Explosion; }
	unsigned short PopHelmet( void ) { return m_PopHelmet; }
	unsigned short RoundReset( void ) { return m_RoundReset; }
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
	unsigned short StartTrail( void ) { return m_RocketTrail; }
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
	virtual BOOL IsMultiplayer( void ) { return TRUE; }
	virtual BOOL IsDeathmatch( void ) { return FALSE; }
	virtual BOOL IsTeamplay( void ) { return TRUE; }
	virtual BOOL IsCoOp( void ) { return FALSE; }
	virtual BOOL FAllowFlashlight( void ) { return FALSE; }
	virtual BOOL AllowAutoTargetCrosshair( void );
	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon );
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity );
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual void PlayerDeathThink( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer ) { return TRUE; }
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
	virtual BOOL ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target );
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon );
	virtual float FlWeaponRespawnTime( CBasePlayerItem *pWeapon );
	virtual float FlWeaponTryRespawn( CBasePlayerItem *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon );
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem ) { return TRUE; }
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem ) {}
	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount ) {}
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
	virtual BOOL FAllowMonsters( void ) { return FALSE; }
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
	int TeamScores( int team ) { return m_iTeamScores[team]; }
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
#endif

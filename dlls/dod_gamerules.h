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
struct DoDPlayerStats_t
{
	int stat[4];
	float starttime;
};
class CDoDStatMgr : public CBaseEntity
{
  public:
	CDoDStatMgr( void );
	void InitPlayer( int index );
	DoDPlayerStats_t *GetPlayer( int index );
	void PrintPlayerStats( int index );
	DoDPlayerStats_t m_PlayerStats[32];
};
class CDoDPreRound : public CBaseEntity
{
  public:
	void Launch( int state );
	void KeyValue( KeyValueData *pkvd );
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
	unsigned short m_Camera;
	BOOL m_bIsRoundFrozen;
	float m_flDoDMapTime;
	GamePlayRules_t m_GamePlayRules;
	BOOL m_RoundDoesCount;
	BOOL m_DisableDeathPenalty;
	int m_RoundState;
	int m_iTeamScores[3];
	BOOL m_RoundParaCanJoin;
	float m_rRoundStartCount;
	int m_rReadyToUnFreeze;
	float m_flAlliesWaveTime;
	float m_flAxisWaveTime;
	float m_flRoundTime;
	float m_flCanJoinTime;
	float m_flAlliesRespawn;
	float m_flAxisRespawn;
	float m_flRestartTime;
	float m_flGoodToGoTime;
	float m_fLogScoresTime;
	BOOL m_bClanMatch;
	BOOL m_bClanMatchActive;
	int m_vKickSessionState;
	int m_vKick;
	BOOL m_bAwaitingReadyRestart;
	BOOL m_bHeardAlliesReady;
	BOOL m_bHeardAxisReady;
	BOOL m_bRoundRestarting;
	float m_flRestartRoundTime;
	int m_DoDCameraState;
	char m_szNextMap[64];
	int m_iTracerEvent;
	int gr_gameRules;
	int MapChangeFromTime;
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
	BOOL IsValidTeam( char *pTeamName );
	int GetTeamIndex( char *pTeamName );
	void ReadInClassInfo( void );
	CDoDTeamPlay( void );

	int m_iNumAlliesAlive;
	int m_iNumAxisAlive;
	int m_iNumAlliesOnTeam;
	int m_iNumAxisOnTeam;
	int m_DisableDeathMessages;
	class CDodParaRoundTimer *m_pParaTimer;
	float m_vKickTime;
	float m_flSpamResetTime;
	class CBasePlayer *m_vKickENT;
};
class CSPDoDRules : public CGameRules
{
  public:
	CSPDoDRules( void );
	virtual void Think( void );
	void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags );
	void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor ) {}
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor ) {}
	void PlayerDeathThink( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	BOOL ClientConnected( edict_t *pEntity, char *pszName, char *pszAddress, char *szRejectReason );
	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress,
								  char szRejectReason[128] )
	{
		return TRUE;
	}
	virtual void InitHUD( CBasePlayer *pPlayer );
	virtual void ClientDisconnected( edict_t *pClient );
	virtual BOOL IsMultiplayer( void ) { return FALSE; }
	virtual BOOL IsDeathmatch( void ) { return FALSE; }
	virtual BOOL IsCoOp( void ) { return FALSE; }
	virtual BOOL FAllowFlashlight( void ) { return TRUE; }
	virtual BOOL AllowAutoTargetCrosshair( void );
	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon ) { return FALSE; }
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity ) { return TRUE; }
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer ) { return TRUE; }
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer ) { return 0; }
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled ) { return 1; }
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ) {}
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon ) { return 0; }
	virtual float FlWeaponRespawnTime( CBasePlayerItem *pWeapon ) { return 0; }
	virtual float FlWeaponTryRespawn( CBasePlayerItem *pWeapon ) { return 0; }
	virtual Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon );
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem ) { return TRUE; }
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem ) {}
	virtual int ItemShouldRespawn( CItem *pItem ) { return 0; }
	virtual float FlItemRespawnTime( CItem *pItem ) { return 0; }
	virtual Vector VecItemRespawnSpot( CItem *pItem );
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount ) {}
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo ) { return 0; }
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo ) { return 0; }
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo );
	virtual float FlHealthChargerRechargeTime( void ) { return 0; }
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer ) { return 0; }
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer ) { return 0; }
	virtual const char *GetTeamID( CBaseEntity *pEntity ) { return ""; }
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget ) { return 0; }
	virtual BOOL FAllowMonsters( void ) { return TRUE; }

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
	void SetRoundState( int iState );
	void SetTeamScores( int iTeam, int iScore );
	void SendTeamScores( int iTeam );
	void SetRestartTime( float fTime );
	void AddToTeamScores( int iTeam, int iScore );
	void SetMapChangeFromTime( int iTime );
	int ReturnGameRules( void );
	void Init( void );
	void VoteKick( CBasePlayer *pPlayer, int id, bool b );
};
#endif

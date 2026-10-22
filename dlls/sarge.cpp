// DoD sarge.cpp restore - GoldSrc 32-bit dod.so - Ghidra base +0x10000
// Class CSarge : CTalkMonster, allied instructor. SDK barney.cpp port with team logic.
// Model models/barney.mdl, health 1000, FOV -0.7, team 1, capability 0x7c0, gun holstered, no Use.
// Fire: BarneyFirePistol with barney/ba_attack2.wav, dist 1024. Precache ba_attack1/2 + pain/die.
// Talk groups fixed BA_* pitch 99 (no body variants). BestVisibleEnemy filters pev->team.
// RunAI custom squad-leader-ish loop, Touch + GiveAdvice for training, GetEnemy wrapper.
// No gamerules wave spawn: single monster_sarge_allies entity, RoundRespawn alloc 0x454.
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )

#define	BARNEY_BODY_GUNHOLSTERED	0
#define	BARNEY_BODY_GUNDRAWN		1
#define BARNEY_BODY_GUNGONE			2

class CSarge : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void BarneyFirePistol( void );
	void AlertSound( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	virtual int	ObjectCaps( void ) { return 10; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	
	void DeclineFollowing( void );
	int IRelationship( CBaseEntity *pTarget );
	CBaseEntity *BestVisibleEnemy( void );
	BOOL GetEnemy( void );
	void RunAI( void );
	void Touch( CBaseEntity *pOther );
	void GiveAdvice( CBaseEntity *pPlayer );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );
	
	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;
};


TYPEDESCRIPTION	CSarge::m_SaveData[] = 
{
	DEFINE_FIELD( CSarge, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSarge, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CSarge, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CSarge, m_lastAttackCheck, FIELD_BOOLEAN ),
	};

IMPLEMENT_SAVERESTORE( CSarge, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlBaFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slBaFollow[] =
{
	{
		tlBaFollow,
		ARRAYSIZE ( tlBaFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// BarneyDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t	tlBarneyEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slBarneyEnemyDraw[] = 
{
	{
		tlBarneyEnemyDraw,
		ARRAYSIZE ( tlBarneyEnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	}
};

Task_t	tlBaFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slBaFaceTarget[] =
{
	{
		tlBaFaceTarget,
		ARRAYSIZE ( tlBaFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


static Task_t	tlIdleBaStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

static Schedule_t	slIdleBaStand[] =
{
	{ 
		tlIdleBaStand,
		ARRAYSIZE ( tlIdleBaStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

DEFINE_CUSTOM_SCHEDULES( CSarge )
{
	slBaFollow,
	slBarneyEnemyDraw,
	slBaFaceTarget,
	slIdleBaStand,
};


IMPLEMENT_CUSTOM_SCHEDULES( CSarge, CTalkMonster );

// .text:000c2100 CSarge::StartTask (bot_barney.cpp:184)
// .text:00139fe0 CSarge::StartTask (sarge.cpp:229)
void CSarge :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );
}

// .text:000c2300 CSarge::RunTask (bot_barney.cpp:188)
// .text:0013a1a0 CSarge::RunTask (sarge.cpp:233)
void CSarge :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask( pTask );
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}




//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
// .text:000c1a10 CSarge::ISoundMask (bot_barney.cpp:221)
// .text:00139990 CSarge::ISoundMask (sarge.cpp:265)
int CSarge :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
// .text:000c1a20 CSarge::Classify (bot_barney.cpp:229)
// .text:001399a0 CSarge::Classify (sarge.cpp:274)
int	CSarge :: Classify( void )
{
	return 11;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
// .text:000c21f0 CSarge::AlertSound (bot_barney.cpp:236)
// .text:0013a030 CSarge::AlertSound (sarge.cpp:280)
void CSarge :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak() )
		{
			PlaySentence( "BA_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		}
	}

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
// .text:000c1a40 CSarge::SetYawSpeed (bot_barney.cpp:261)
// .text:001399b0 CSarge::SetYawSpeed (sarge.cpp:295)
void CSarge :: SetYawSpeed ( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
// .text:000c25c0 CSarge::CheckRangeAttack1 (bot_barney.cpp:290)
// .text:0013a460 CSarge::CheckRangeAttack1 (sarge.cpp:324)
BOOL CSarge :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 1024 && flDot >= 0.5 )
	{
		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;
			
			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}


//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
// .text:000c2ab0 CSarge::BarneyFirePistol (bot_barney.cpp:324)
// .text:0013a9b0 CSarge::BarneyFirePistol (sarge.cpp:353)
void CSarge :: BarneyFirePistol( void )
{
	Vector vecShootOrigin;
	UTIL_MakeVectors( pev->angles );
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;
	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, (Bullet)27 ); // 27 = BULLET_MONSTER_9MM in the original DoD Bullet enum (binary-verified)
	int i = RANDOM_LONG( 0, 20 );
	int pitch = 100;
	if ( i < 11 )
		pitch = i + 95;
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "barney/ba_attack2.wav", 1.0, ATTN_NORM, 0, pitch );
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
	m_cAmmoLoaded--;
}
		
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
// .text:000c2c80 CSarge::HandleAnimEvent (bot_barney.cpp:360)
// .text:0013ab80 CSarge::HandleAnimEvent (sarge.cpp:388)
void CSarge :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_DRAW:
		pev->body = 1;
		m_fGunDrawn = TRUE;
		break;
	case BARNEY_AE_SHOOT:
		BarneyFirePistol();
		break;
	case BARNEY_AE_HOLSTER:
		pev->body = 0;
		m_fGunDrawn = FALSE;
		break;
	default:
		CTalkMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
// .text:000c1f80 CSarge::Spawn (bot_barney.cpp:389)
// .text:00139ec0 CSarge::Spawn (sarge.cpp:416)
void CSarge :: Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/barney.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = 247;
	pev->health = 1000;
	pev->view_ofs = Vector( 0, 0, 50 );
	m_MonsterState = MONSTERSTATE_NONE;
	m_flFieldOfView = -0.7;
	pev->team = 1;
	pev->body = 0;
	m_afCapability = 0x7c0;
	m_fGunDrawn = FALSE;
	MonsterInit();
	m_pfnUse = NULL;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
// .text:000c2e50 CSarge::Precache (bot_barney.cpp:431)
// .text:0013a7e0 CSarge::Precache (sarge.cpp:447)
void CSarge :: Precache( void )
{
	PRECACHE_MODEL( "models/barney.mdl" );
	PRECACHE_SOUND( "barney/ba_attack1.wav" );
	PRECACHE_SOUND( "barney/ba_attack2.wav" );
	PRECACHE_SOUND( "barney/ba_pain1.wav" );
	PRECACHE_SOUND( "barney/ba_pain2.wav" );
	PRECACHE_SOUND( "barney/ba_pain3.wav" );
	PRECACHE_SOUND( "barney/ba_die1.wav" );
	PRECACHE_SOUND( "barney/ba_die2.wav" );
	PRECACHE_SOUND( "barney/ba_die3.wav" );
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
// .text:000c2ce0 CSarge::TalkInit (bot_barney.cpp:452)
// .text:0013ac00 CSarge::TalkInit (sarge.cpp:471)
void CSarge :: TalkInit( void )
{
	CTalkMonster::TalkInit();
	// NOTE: talkmonster.h's TALKGROUPNAMES is missing TLK_DECLINE (index 6);
	// use the binary-verified numeric indices.
	m_szGrp[0] = "BA_ANSWER";
	m_szGrp[1] = "BA_QUESTION";
	m_szGrp[2] = "BA_IDLE";
	m_szGrp[3] = "BA_STARE";
	m_szGrp[4] = "BA_OK";
	m_szGrp[5] = "BA_WAIT";
	m_szGrp[7] = "BA_STOP";
	m_szGrp[8] = "BA_SCARED";
	m_szGrp[9] = "BA_HELLO";
	m_szGrp[10] = NULL;
	m_szGrp[11] = NULL;
	m_szGrp[12] = "BA_PQUEST";
	m_szGrp[13] = "!BA_CUREA";
	m_szGrp[14] = "!BA_CUREB";
	m_szGrp[15] = "!BA_CUREC";
	m_szGrp[16] = "BA_SMELL";
	m_szGrp[17] = "BA_WOUND";
	m_szGrp[18] = "BA_MORTAL";
	m_voicePitch = 99;
}


static BOOL IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );
	// He's facing me, he meant it
	if ( DotProduct( forward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}


// .text:000c2770 CSarge::TakeDamage (bot_barney.cpp:526)
// .text:00139e60 CSarge::TakeDamage (sarge.cpp:524)
int CSarge :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	int ret = CTalkMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );

	if ( IsAlive() && pev->deadflag != DEAD_DYING )
		Remember( bits_MEMORY_PROVOKED );

	return ret;
}

	
//=========================================================
// PainSound
//=========================================================
// .text:000c24a0 CSarge::PainSound (bot_barney.cpp:570)
// .text:0013a340 CSarge::PainSound (sarge.cpp:581)
void CSarge :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
// .text:000c1ea0 CSarge::DeathSound (bot_barney.cpp:588)
// .text:00139d80 CSarge::DeathSound (sarge.cpp:599)
void CSarge :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


// .text:000c1dc0 CSarge::TraceAttack (bot_barney.cpp:599)
// .text:00139ca0 CSarge::TraceAttack (sarge.cpp:610)
void CSarge::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch( ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


// .text:000c1d10 CSarge::Killed (bot_barney.cpp:629)
// .text:00139c80 CSarge::Killed (sarge.cpp:640)
void CSarge :: Killed( entvars_t *pevAttacker, int iGib )
{
	SetUse( NULL );
	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// .text:000c2110 CSarge::GetScheduleOfType (bot_barney.cpp:651)
// .text:0013a0c0 CSarge::GetScheduleOfType (sarge.cpp:650)
Schedule_t* CSarge :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slBarneyEnemyDraw;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slBaFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slBaFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleBaStand;
		}
		else
			return psched;	
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CSarge :: GetSchedule ( void )
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
	{
		PlaySentence( "BA_KILL", 4, VOL_NORM, ATTN_NORM );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
				
			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if ( m_hEnemy == NULL && IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}
			else
			{
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

// .text:000c1ab0 CSarge::GetIdealState (bot_barney.cpp:782)
// .text:00139a20 CSarge::GetIdealState (sarge.cpp:781)
MONSTERSTATE CSarge :: GetIdealState( void )
{
	return CTalkMonster::GetIdealState();
}



// .text:000c1a70 CSarge::DeclineFollowing (bot_barney.cpp:788)
// .text:001399e0 CSarge::DeclineFollowing (sarge.cpp:785)
void CSarge :: DeclineFollowing( void )
{
	PlaySentence( "BA_POK", 2.0, VOL_NORM, ATTN_NORM );
}





//=========================================================
// DEAD BARNEY PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================


// .text:000c2380 CSarge::ScheduleFromName (bot_barney.cpp:180)

// .text:000c2ee0 CSarge::ResetAfterDoDMovie (bot_barney.cpp:794)

// .text:000c2440 CSarge::Save (bot_barney.cpp:66)
// .text:0013a2e0 CSarge::Save (sarge.cpp:111)

// .text:000c23e0 CSarge::Restore (bot_barney.cpp:66)



int CSarge :: IRelationship( CBaseEntity *pTarget )
{
	int rel = 0;
	if ( pTarget->pev->iuser1 == 0 && pTarget->pev->deadflag != 2 )
		rel = ( pTarget->pev->team != pev->team ) * 5 - 2;
	return rel;
}
CBaseEntity *CSarge :: BestVisibleEnemy( void )
{
	CBaseEntity *pReturn = NULL;
	CBaseEntity *pNextEnt = m_pLink;
	int iBestRel = 0;
	int iNearest = 8192;
	while ( pNextEnt )
	{
		if ( pNextEnt->IsAlive() && pNextEnt->pev->team != pev->team )
		{
			int rel = IRelationship( pNextEnt );
			if ( rel > iBestRel )
			{
				iBestRel = rel;
				Vector vecDiff = pNextEnt->pev->origin - pev->origin;
				iNearest = (int)vecDiff.Length();
				pReturn = pNextEnt;
			}
			else if ( rel == iBestRel )
			{
				Vector vecDiff = pNextEnt->pev->origin - pev->origin;
				int d = (int)vecDiff.Length();
				if ( d <= iNearest )
				{
					iNearest = d;
					pReturn = pNextEnt;
				}
			}
		}
		pNextEnt = pNextEnt->m_pLink;
	}
	return pReturn;
}
// .text:0013ace0 CSarge::GetEnemy (sarge.cpp:846)
BOOL CSarge :: GetEnemy( void )
{
	CBaseEntity *pNewEnemy;

	if ( HasConditions( bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_NEMESIS ) )
	{
		pNewEnemy = BestVisibleEnemy();

		if ( pNewEnemy != m_hEnemy && pNewEnemy != NULL )
		{
			// DO NOT mess with the monster's m_hEnemy pointer unless the schedule the monster is currently running will be interrupted
			// by COND_NEW_ENEMY.
			if ( m_pSchedule )
			{
				if ( m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY )
				{
					PushEnemy( m_hEnemy, m_vecEnemyLKP );
					SetConditions( bits_COND_NEW_ENEMY );
					m_hEnemy = pNewEnemy;
					m_vecEnemyLKP = m_hEnemy->pev->origin;
				}

				// if the new enemy has an owner, take that one as well
				if ( pNewEnemy->pev->owner != NULL )
				{
					CBaseEntity *pOwner = GetMonsterPointer( pNewEnemy->pev->owner );
					if ( pOwner && ( pOwner->pev->flags & FL_MONSTER ) && IRelationship( pOwner ) != R_NO )
						PushEnemy( pOwner, m_vecEnemyLKP );
				}
			}
		}
	}

	// remember old enemies
	if ( m_hEnemy == NULL && PopEnemy() )
	{
		if ( m_pSchedule )
		{
			if ( m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY )
			{
				SetConditions( bits_COND_NEW_ENEMY );
			}
		}
	}

	if ( m_hEnemy != NULL )
	{
		// monster has an enemy.
		return TRUE;
	}

	return FALSE;  // monster has no enemy
}
// .text:0013ae60 CSarge::RunAI (sarge.cpp:790)
void CSarge :: RunAI( void )
{
	// to test model's eye height
	UTIL_ParticleEffect ( pev->origin + pev->view_ofs, g_vecZero, 255, 10 );

	// IDLE sound permitted in ALERT state is because monsters were silent in ALERT state. Only play IDLE sound in IDLE state
	// once we have sounds for that state.
	if ( ( m_MonsterState == MONSTERSTATE_IDLE || m_MonsterState == MONSTERSTATE_ALERT ) && RANDOM_LONG( 0, 99 ) == 0 &&
		 !( pev->flags & SF_MONSTER_GAG ) )
	{
		IdleSound();
	}

	if ( m_MonsterState != MONSTERSTATE_NONE && m_MonsterState != MONSTERSTATE_PRONE &&
		 m_MonsterState != MONSTERSTATE_DEAD )	// don't bother with this crap if monster is prone.
	{
		// collect some sensory Condition information.
		if ( !FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) || ( m_MonsterState == MONSTERSTATE_COMBAT ) )
		{
			Look( m_flDistLook );
			Listen();  // check for audible sounds.

			// now filter conditions.
			ClearConditions( IgnoreConditions() );

			GetEnemy();
		}

		// do these calculations if monster has an enemy.
		if ( m_hEnemy != NULL )
		{
			CheckEnemy( m_hEnemy );
		}

		CheckAmmo();
	}

	FCheckAITrigger();

	PrescheduleThink();

	MaintainSchedule();
}
// .text:0013a610 CSarge::Touch (sarge.cpp:967)
void CSarge :: Touch( CBaseEntity *pOther )
{
	if ( pOther->IsPlayer() )
	{
		if ( IRelationship( pOther ) <= R_NO )
		{
			if ( !IsTalking() )
				GiveAdvice( pOther );
		}
	}
}
// .text:0013b000 CSarge::GiveAdvice (sarge.cpp:992)
void CSarge :: GiveAdvice( CBaseEntity *pPlayer )
{
}



// .text:0013a940 monster_sarge_allies (sarge.cpp:101)
LINK_ENTITY_TO_CLASS( monster_sarge_allies, CSarge );

// .text:0013a220 CSarge::ScheduleFromName (sarge.cpp:225)

// .text:0013a2e0 CSarge::Save (sarge.cpp:111)

// .text:0013a280 CSarge::Restore (sarge.cpp:111)


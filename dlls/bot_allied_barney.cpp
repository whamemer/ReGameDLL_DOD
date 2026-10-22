// DoD bot_allied_barney.cpp restore - GoldSrc 32-bit dod.so - Ghidra base +0x10000
// Class CAlliedBarney : CTalkMonster, SDK barney.cpp port. Support rifleman, not squad.
// Model models/allied_barney.mdl, health 25, FOV -0.7, capability 0x7c0, gun drawn.
// Weapon: BarneyFirePistol fires garand_shoot.wav via FireBullets dist 1024 (SDK uses 9mm, here rifle sound).
// Sounds: weapons/colt_shoot.wav precached but fire uses garand; barney/ba_pain1-3, ba_die1-3.
// Talk groups BA_* (see TalkInit), pitch by body 90/95/99.
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
#define		ALBARNEY_AE_DRAW		( 2 )
#define		ALBARNEY_AE_SHOOT		( 3 )
#define		ALBARNEY_AE_HOLSTER	( 4 )

#define	ALBARNEY_BODY_GUNHOLSTERED	0
#define	ALBARNEY_BODY_GUNDRAWN		1
#define ALBARNEY_BODY_GUNGONE			2

class CAlliedBarney : public CTalkMonster
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
	void ResetAfterDoDMovie( void );

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
	Vector	m_angles;
	Vector	m_origin;

	CUSTOM_SCHEDULES;
};


TYPEDESCRIPTION	CAlliedBarney::m_SaveData[] = 
{
	DEFINE_FIELD( CAlliedBarney, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAlliedBarney, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CAlliedBarney, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CAlliedBarney, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAlliedBarney, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CAlliedBarney, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlAlBaFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slAlBaFollow[] =
{
	{
		tlAlBaFollow,
		ARRAYSIZE ( tlAlBaFollow ),
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
Task_t	tlAlBarneyEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slAlBarneyEnemyDraw[] = 
{
	{
		tlAlBarneyEnemyDraw,
		ARRAYSIZE ( tlAlBarneyEnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	}
};

Task_t	tlAlBaFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slAlBaFaceTarget[] =
{
	{
		tlAlBaFaceTarget,
		ARRAYSIZE ( tlAlBaFaceTarget ),
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

DEFINE_CUSTOM_SCHEDULES( CAlliedBarney )
{
	slAlBaFollow,
	slAlBarneyEnemyDraw,
	slAlBaFaceTarget,
	slIdleBaStand,
};


IMPLEMENT_CUSTOM_SCHEDULES( CAlliedBarney, CTalkMonster );

// .text:000c2100 CAlliedBarney::StartTask (bot_allied_barney.cpp:184)
void CAlliedBarney :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );
}

// .text:000c2300 CAlliedBarney::RunTask (bot_allied_barney.cpp:188)
void CAlliedBarney :: RunTask( Task_t *pTask )
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
// .text:000c1a10 CAlliedBarney::ISoundMask (bot_allied_barney.cpp:221)
int CAlliedBarney :: ISoundMask ( void) 
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
// .text:000c1a20 CAlliedBarney::Classify (bot_allied_barney.cpp:229)
int	CAlliedBarney :: Classify ( void )
{
	if ( m_iClass == 0 )
		return 11;
	return m_iClass;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
// .text:000c21f0 CAlliedBarney::AlertSound (bot_allied_barney.cpp:236)
void CAlliedBarney :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak() )
		{
			if ( m_iszSpeakAs == 0 )
			{
				PlaySentence( "BA_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
			}
			else
			{
				char szBuf[64];
				strcpy( szBuf, STRING( m_iszSpeakAs ) );
				strcat( szBuf, "_ATTACK" );
				PlaySentence( szBuf, RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
			}
		}
	}

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
// .text:000c1a40 CAlliedBarney::SetYawSpeed (bot_allied_barney.cpp:261)
void CAlliedBarney :: SetYawSpeed ( void )
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
// .text:000c25c0 CAlliedBarney::CheckRangeAttack1 (bot_allied_barney.cpp:290)
BOOL CAlliedBarney :: CheckRangeAttack1 ( float flDot, float flDist )
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
// .text:000c2ab0 CAlliedBarney::BarneyFirePistol (bot_allied_barney.cpp:324)
void CAlliedBarney :: BarneyFirePistol ( void )
{
	Vector vecShootOrigin;
	UTIL_MakeVectors( pev->angles );
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;
	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM );
	int i = RANDOM_LONG( 0, 20 );
	int pitch = 100;
	if ( i < 11 )
		pitch = i + 95;
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/garand_shoot.wav", 1.0, ATTN_NORM, 0, pitch );
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
	m_cAmmoLoaded--;
}
		
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
// .text:000c2c80 CAlliedBarney::HandleAnimEvent (bot_allied_barney.cpp:360)
void CAlliedBarney :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ALBARNEY_AE_DRAW:
		m_fGunDrawn = TRUE;
		break;
	case ALBARNEY_AE_SHOOT:
		BarneyFirePistol();
		break;
	case ALBARNEY_AE_HOLSTER:
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
// .text:000c1f80 CAlliedBarney::Spawn (bot_allied_barney.cpp:389)
void CAlliedBarney :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/allied_barney.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 25;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	m_angles			= pev->angles;
	m_origin			= pev->origin;

	if ( pev->body == -1 )
		pev->body = RANDOM_LONG(0,3); // random body

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_fGunDrawn			= TRUE;

	MonsterInit();
	SetUse( &CAlliedBarney::FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
// .text:000c2e50 CAlliedBarney::Precache (bot_allied_barney.cpp:431)
void CAlliedBarney :: Precache()
{
	PRECACHE_MODEL("models/allied_barney.mdl");

	PRECACHE_SOUND("weapons/colt_shoot.wav" );

	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");
	
	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
// .text:000c2ce0 CAlliedBarney::TalkInit (bot_allied_barney.cpp:452)
void CAlliedBarney :: TalkInit( void )
{
	CTalkMonster::TalkInit();
	if ( m_iszSpeakAs == 0 )
	{
		// NOTE: talkmonster.h's TALKGROUPNAMES is missing TLK_DECLINE (index 6);
		// use the binary-verified numeric indices (matches CTalkMonster::TalkInit).
		m_szGrp[0] = "BA_ANSWER";
		m_szGrp[1] = "BA_QUESTION";
		m_szGrp[2] = "BA_IDLE";
		m_szGrp[3] = "BA_STARE";
		if ( pev->spawnflags & 0x100 )
			m_szGrp[4] = "BA_PFOLLOW";
		else
			m_szGrp[4] = "BA_OK";
		if ( pev->spawnflags & 0x100 )
			m_szGrp[5] = "BA_PWAIT";
		else
			m_szGrp[5] = "BA_WAIT";
		if ( pev->spawnflags & 0x100 )
			m_szGrp[6] = "BA_POK";
		else
			m_szGrp[6] = "BA_NOTOK";
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
	}
	int b = pev->body % 3;
	if ( b == 1 )
		m_voicePitch = 90;
	else if ( b == 2 )
		m_voicePitch = 85;
	else
		m_voicePitch = 95;
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


// .text:000c2770 CAlliedBarney::TakeDamage (bot_allied_barney.cpp:526)
int CAlliedBarney :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				PlaySentence( "BA_MAD", 4, VOL_NORM, ATTN_NORM );

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}

	
//=========================================================
// PainSound
//=========================================================
// .text:000c24a0 CAlliedBarney::PainSound (bot_allied_barney.cpp:570)
void CAlliedBarney :: PainSound ( void )
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
// .text:000c1ea0 CAlliedBarney::DeathSound (bot_allied_barney.cpp:588)
void CAlliedBarney :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


// .text:000c1dc0 CAlliedBarney::TraceAttack (bot_allied_barney.cpp:599)
void CAlliedBarney::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
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


// .text:000c1d10 CAlliedBarney::Killed (bot_allied_barney.cpp:629)
void CAlliedBarney :: Killed( entvars_t *pevAttacker, int iGib )
{
	if ( pev->body < 2 && !( pev->spawnflags & 0x400 ) )
	{
		pev->body = 2;
		Vector vecGunPos;
		Vector vecGunAngles;
		GetAttachment( 0, vecGunPos, vecGunAngles );
		DropItem( "weapon_garand", vecGunPos, vecGunAngles );
	}
	SetUse( NULL );
	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// .text:000c2110 CAlliedBarney::GetScheduleOfType (bot_allied_barney.cpp:651)
Schedule_t* CAlliedBarney :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slAlBarneyEnemyDraw;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slAlBaFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slAlBaFollow;

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
Schedule_t *CAlliedBarney :: GetSchedule ( void )
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

// .text:000c1ab0 CAlliedBarney::GetIdealState (bot_allied_barney.cpp:782)
MONSTERSTATE CAlliedBarney :: GetIdealState( void )
{
	return CTalkMonster::GetIdealState();
}



// .text:000c1a70 CAlliedBarney::DeclineFollowing (bot_allied_barney.cpp:788)
void CAlliedBarney :: DeclineFollowing( void )
{
	PlaySentence( m_szGrp[6], 2.0, VOL_NORM, ATTN_NORM );
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


// .text:000c2380 CAlliedBarney::ScheduleFromName (bot_allied_barney.cpp:180)

// .text:000c2ee0 CAlliedBarney::ResetAfterDoDMovie (bot_allied_barney.cpp:794)
void CAlliedBarney :: ResetAfterDoDMovie( void )
{
	pev->angles = m_angles;
	pev->origin = m_origin;
}

// .text:000c2a40 monster_allied_barney (bot_allied_barney.cpp:55)
LINK_ENTITY_TO_CLASS( monster_allied_barney, CAlliedBarney );

// .text:000c2440 CAlliedBarney::Save (bot_allied_barney.cpp:66)

// .text:000c23e0 CAlliedBarney::Restore (bot_allied_barney.cpp:66)


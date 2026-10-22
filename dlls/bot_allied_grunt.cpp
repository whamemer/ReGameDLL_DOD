// DoD bot_allied_grunt.cpp restore - GoldSrc 32-bit dod.so - Ghidra base +0x10000
// Class CAlliedGrunt : CSquadMonster : CTalkMonster-like squad AI, ported from SDK dlls/hgrunt.cpp
// pev->weapons bits: 2 = GARAND light clip 8, 4 = THOMPSON medium, 8 = BAR heavy clip 20
// DoDWeaponId cross-check weapons.h: GARAND=5 THOMPSON=7 BAR=11 COLT=3 (pistol fallback on death drop)
// Strings: models/allied_grunt.mdl, weapons/garand_shoot.wav, weapons/bar_shoot.wav,
//   weapons/thompson_shoot.wav, weapons/colt_shoot.wav, hgrunt/gr_die1-3.wav, hgrunt/gr_pain1-5.wav,
//   hgrunt/gr_reload1.wav, zombie/claw_miss2.wav, sentences ALLIEDG_* (see below)
// .text:000c5900 monster_allied_grunt (bot_allied_grunt.cpp:189)
#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"

// .text:000c6430 monster_alliedgrunt_dead (bot_allied_grunt.cpp:2218)

// Sentence table at 0x1d2bc0: index = ALLIEDGRUNT_SENT_*
// 0 ALLIEDG_GREN, 1 ALLIEDG_ALERT, 2 ALLIEDG_MONSTER, 3 ALLIEDG_COVER,
// 4 ALLIEDG_THROW, 5 ALLIEDG_CHARGE, 6 ALLIEDG_TAUNT
// .text:000c5970 CAlliedGrunt::SpeakSentence (bot_allied_grunt.cpp:245)
extern char *pAlliedGruntSentences[];
// .text:000c4af0 CAlliedGrunt::IdleSound (bot_allied_grunt.cpp:632)
// Idle uses ALLIEDG_ANSWER, ALLIEDG_CHECK, ALLIEDG_CLEAR, ALLIEDG_IDLE, ALLIEDG_QUEST
// .text:000c50e0 CAlliedGrunt::GetSchedule (bot_allied_grunt.cpp:1836)
// GetSchedule sets m_iSentence: 3 = COVER, 5 = CHARGE, plus ALLIEDG_ALERT/GREN/MONST/TAUNT/THROW

// Schedules: externs from .data, same layout as SDK hgrunt sl* / tl*
// slAlliedGruntTakeCover, slAlliedGruntGrenadeCover, slAlliedGruntTossGrenadeCover,
// slAlliedGruntTakeCoverFromBestSound, slAlliedGruntRangeAttack1A/B, slAlliedGruntRangeAttack2,
// slAlliedGruntCombatFail/Fail, slAlliedGruntSuppress/SignalSuppress, slAlliedGruntEstablishLineOfFire,
// slAlliedGruntHideReload, slAlliedGruntSweep, slAlliedGruntFoundEnemy, slAlliedGruntWaitInCover,
// slAlliedGruntCombatFace, slAlliedGruntVictoryDance
// Tasks: TASK_ALLIEDGRUNT_CHECK_FIRE, TASK_ALLIEDGRUNT_FACE_TOSS_DIR, TASK_ALLIEDGRUNT_SPEAK_SENTENCE



int g_fAlliedGruntQuestion;				// true if an idle grunt asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	GRUNT_CLIP_SIZE					36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define GRUNT_VOL						0.35		// volume of grunt sounds
#define ALLIEDGRUNT_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define HGRUNT_LIMP_HEALTH				20
#define HGRUNT_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.
#define HGRUNT_NUM_HEADS				2 // how many grunt heads are there? 
#define HGRUNT_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	ALLIEDGRUNT_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

#define HGRUNT_9MMAR				( 1 << 0)
#define HGRUNT_HANDGRENADE (1<<4)
#define HGRUNT_GRENADELAUNCHER		( 1 << 2)
#define HGRUNT_SHOTGUN				( 1 << 3)

#define HEAD_GROUP					1
#define HEAD_GRUNT					0
#define HEAD_COMMANDER				1
#define HEAD_SHOTGUN				2
#define HEAD_M203					3
#define GUN_GROUP					2
#define GUN_MP5						0
#define GUN_SHOTGUN					1
#define GUN_NONE					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ALLIEDGRUNT_AE_RELOAD		( 2 )
#define		ALLIEDGRUNT_AE_KICK			( 3 )
#define		ALLIEDGRUNT_AE_BURST1		( 4 )
#define		ALLIEDGRUNT_AE_BURST2		( 5 ) 
#define		ALLIEDGRUNT_AE_BURST3		( 6 ) 
#define		ALLIEDGRUNT_AE_GREN_TOSS		( 7 )
#define		ALLIEDGRUNT_AE_GREN_LAUNCH	( 8 )
#define		ALLIEDGRUNT_AE_GREN_DROP		( 9 )
#define		ALLIEDGRUNT_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		ALLIEDGRUNT_AE_DROP_GUN		( 11) // grunt (probably dead) is dropping his mp5.

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_ALLIEDGRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_ALLIEDGRUNT_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_ALLIEDGRUNT_COVER_AND_RELOAD,
	SCHED_ALLIEDGRUNT_SWEEP,
	SCHED_ALLIEDGRUNT_FOUND_ENEMY,
	SCHED_ALLIEDGRUNT_REPEL,
	SCHED_ALLIEDGRUNT_REPEL_ATTACK,
	SCHED_ALLIEDGRUNT_REPEL_LAND,
	SCHED_ALLIEDGRUNT_WAIT_FACE_ENEMY,
	SCHED_ALLIEDGRUNT_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_ALLIEDGRUNT_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_ALLIEDGRUNT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_ALLIEDGRUNT_SPEAK_SENTENCE,
	TASK_ALLIEDGRUNT_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_GRUNT_NOFIRE	( bits_COND_SPECIAL1 )

class CAlliedGrunt : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	int ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	void IdleSound ( void );
	Vector GetGunPosition( void );
	void Shoot ( void );
	void PrescheduleThink ( void );
	void GibMonster( void );
	void SpeakSentence( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
	CBaseEntity	*Kick( void );
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int IRelationship ( CBaseEntity *pTarget );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int		m_cClipSize;

	int m_voicePitch;

	int		m_iBrassShell;
	int		m_iShotgunShell;

	int		m_iSentence;

	static const char *pAlliedGruntSentences[];
};


TYPEDESCRIPTION	CAlliedGrunt::m_SaveData[] = 
{
	DEFINE_FIELD( CAlliedGrunt, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CAlliedGrunt, m_flNextPainTime, FIELD_TIME ),
//	DEFINE_FIELD( CAlliedGrunt, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CAlliedGrunt, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CAlliedGrunt, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAlliedGrunt, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAlliedGrunt, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAlliedGrunt, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CAlliedGrunt, m_voicePitch, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( CAlliedGrunt, m_iSentence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CAlliedGrunt, CSquadMonster );

const char *CAlliedGrunt::pAlliedGruntSentences[] = 
{
	"ALLIEDG_GREN", // grenade scared grunt
	"ALLIEDG_ALERT", // sees player
	"ALLIEDG_MONSTER", // sees monster
	"ALLIEDG_COVER", // running to cover
	"ALLIEDG_THROW", // about to throw grenade
	"ALLIEDG_CHARGE",  // running out to get the enemy
	"ALLIEDG_TAUNT", // say rude things
};

enum
{
	ALLIEDGRUNT_SENT_NONE = -1,
	ALLIEDGRUNT_SENT_GREN = 0,
	ALLIEDGRUNT_SENT_ALERT,
	ALLIEDGRUNT_SENT_MONSTER,
	ALLIEDGRUNT_SENT_COVER,
	ALLIEDGRUNT_SENT_THROW,
	ALLIEDGRUNT_SENT_CHARGE,
	ALLIEDGRUNT_SENT_TAUNT,
} ALLIEDGRUNT_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has 
// started moving.
//=========================================================
// .text:000c5970 CAlliedGrunt::SpeakSentence (bot_allied_grunt.cpp:245)
void CAlliedGrunt :: SpeakSentence( void )
{
	if ( m_iSentence == ALLIEDGRUNT_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pAlliedGruntSentences[ m_iSentence ], ALLIEDGRUNT_SENTENCE_VOLUME, ALLIEDGRUNT_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
// .text:000c4050 CAlliedGrunt::IRelationship (bot_allied_grunt.cpp:264)
int CAlliedGrunt :: IRelationship( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_axis_grunt" ) )
		return R_HT;
	return CSquadMonster::IRelationship( pTarget );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
// .text:000c3a40 CAlliedGrunt::GibMonster (bot_allied_grunt.cpp:277)
void CAlliedGrunt :: GibMonster( void )
{
	Vector vecGunPos;
	Vector vecGunAngles;
	if ( GetBodygroup( 2 ) != 2 && !( pev->spawnflags & 0x400 ) )
	{
		GetAttachment( 0, vecGunPos, vecGunAngles );
		CBaseEntity *pGun = NULL;
		if ( pev->weapons & 2 )
			pGun = DropItem( "weapon_garand", vecGunPos, vecGunAngles );
		else if ( pev->weapons & 8 )
			pGun = DropItem( "weapon_bar", vecGunPos, vecGunAngles );
		else
			pGun = DropItem( "weapon_colt", vecGunPos, vecGunAngles );
		if ( pGun )
		{
			pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
			pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}
	CBaseMonster::GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
// .text:000c30f0 CAlliedGrunt::ISoundMask (bot_allied_grunt.cpp:318)
int CAlliedGrunt :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
// .text:000c5b10 CAlliedGrunt::FOkToSpeak (bot_allied_grunt.cpp:326)
BOOL CAlliedGrunt :: FOkToSpeak( void )
{
// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return FALSE;
		}
	}

	// if player is not in pvs, don't speak
//	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
//		return FALSE;
	
	return TRUE;
}

//=========================================================
//=========================================================
// .text:000c5b50 CAlliedGrunt::JustSpoke (bot_allied_grunt.cpp:348)
void CAlliedGrunt :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = ALLIEDGRUNT_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
// .text:000c4830 CAlliedGrunt::PrescheduleThink (bot_allied_grunt.cpp:358)
void CAlliedGrunt :: PrescheduleThink ( void )
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds. 
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
// .text:000c3100 CAlliedGrunt::FCanCheckAttacks (bot_allied_grunt.cpp:393)
BOOL CAlliedGrunt :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
// .text:000c34a0 CAlliedGrunt::CheckMeleeAttack1 (bot_allied_grunt.cpp:406)
BOOL CAlliedGrunt :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7	&& 
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
// .text:000c3c90 CAlliedGrunt::CheckRangeAttack1 (bot_allied_grunt.cpp:437)
BOOL CAlliedGrunt :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients, but don't shoot at them.
			return FALSE;
		}

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
// .text:000c40c0 CAlliedGrunt::CheckRangeAttack2 (bot_allied_grunt.cpp:467)
BOOL CAlliedGrunt :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (! FBitSet(pev->weapons, 0x10))
	{
		return FALSE;
	}
	
	// if the grunt isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
	
	Vector vecTarget;

	if (FBitSet( pev->weapons, 0x10))
	{
		// find feet
		if (RANDOM_LONG(0,1))
		{
			// magically know where they are
			vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions( bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed) * m_hEnemy->pev->velocity;
	}

	// are any of my squad members near the intended grenade impact area?
	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}
	
	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

		
	if (FBitSet( pev->weapons, 0x10))
	{
		Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
		Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.hgruntGrenadeSpeed, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}

	

	return m_fThrowGrenade;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
// .text:000c3bb0 CAlliedGrunt::TraceAttack (bot_allied_grunt.cpp:552)
void CAlliedGrunt :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// make sure we're wearing one
		if (GetBodygroup( 0 ) != 1 && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		{
			// absorb damage
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
// .text:000c3320 CAlliedGrunt::TakeDamage (bot_allied_grunt.cpp:578)
int CAlliedGrunt :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
// .text:000c3120 CAlliedGrunt::SetYawSpeed (bot_allied_grunt.cpp:589)
void CAlliedGrunt :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:	
		ys = 150;		
		break;
	case ACT_RUN:	
		ys = 150;	
		break;
	case ACT_WALK:	
		ys = 180;		
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_RANGE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:	
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

// .text:000c4af0 CAlliedGrunt::IdleSound (bot_allied_grunt.cpp:632)
void CAlliedGrunt :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fAlliedGruntQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fAlliedGruntQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "ALLIEDG_CHECK", ALLIEDGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fAlliedGruntQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "ALLIEDG_QUEST", ALLIEDGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fAlliedGruntQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "ALLIEDG_IDLE", ALLIEDGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fAlliedGruntQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "ALLIEDG_CLEAR", ALLIEDGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(ENT(pev), "ALLIEDG_ANSWER", ALLIEDGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fAlliedGruntQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
// .text:000c3160 CAlliedGrunt::CheckAmmo (bot_allied_grunt.cpp:675)
void CAlliedGrunt :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
// .text:000c3dc0 CAlliedGrunt::Classify (bot_allied_grunt.cpp:687)
int CAlliedGrunt :: Classify( void )
{
	if ( m_iClass == 0 )
	{
		if ( pev->spawnflags & 0x800 )
			return 11;
		return 4;
	}
	return m_iClass;
}

//=========================================================
//=========================================================
CBaseEntity *CAlliedGrunt :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

// .text:000c3180 CAlliedGrunt::GetGunPosition (bot_allied_grunt.cpp:724)
Vector CAlliedGrunt :: GetGunPosition( )
{
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 48 );
	}
}

//=========================================================
// Shoot
//=========================================================
// .text:000c6400 CAlliedGrunt::Shoot (bot_allied_grunt.cpp:739)
// .text:000b43c0 CAlliedGrunt::Shoot part (bot_allied_grunt.cpp:738)
void CAlliedGrunt :: Shoot( void )
{
	if ( m_hEnemy == NULL )
		return;
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	UTIL_MakeVectors( pev->angles );
	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + gpGlobals->v_up * RANDOM_FLOAT( 75, 200 ) + gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	EjectBrass( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );
	if ( pev->weapons & 2 )
	{
		FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 2048, BULLET_PLAYER_GARAND );
		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/garand_shoot.wav", 1, ATTN_NORM );
		ALERT( at_console, "Shoot GARAND\n" );
	}
	else if ( pev->weapons & 8 )
	{
		FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BAR );
		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/bar_shoot.wav", 1, ATTN_NORM );
		ALERT( at_console, "Shoot BAR\n" );
	}
	else if ( pev->weapons & 4 )
	{
		FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_THOMPSON );
		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/thompson_shoot.wav", 1, ATTN_NORM );
		ALERT( at_console, "Shoot THOMPSON\n" );
	}
	pev->effects |= EF_MUZZLEFLASH;
	m_cAmmoLoaded--;
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// Shoot
//=========================================================

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
// .text:000c5c60 CAlliedGrunt::HandleAnimEvent (bot_allied_grunt.cpp:778)
void CAlliedGrunt :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector vecShootDir;
	Vector vecShootOrigin;
	switch( pEvent->event )
	{
		case ALLIEDGRUNT_AE_DROP_GUN:
		{
			Vector vecGunPos;
			Vector vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			if ( pev->weapons & 2 )
				DropItem( "weapon_garand", vecGunPos, vecGunAngles );
			else if ( pev->weapons & 8 )
				DropItem( "weapon_bar", vecGunPos, vecGunAngles );
			else
				DropItem( "weapon_colt", vecGunPos, vecGunAngles );
		}
		break;
		case ALLIEDGRUNT_AE_RELOAD:
			EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions( bits_COND_NO_AMMO_LOADED );
			break;
		case ALLIEDGRUNT_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );
			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;
		}
		break;
		case ALLIEDGRUNT_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );
		}
		break;
		case ALLIEDGRUNT_AE_BURST1:
		{
			if ( ( pev->weapons & 2 ) == 0 && ( pev->weapons & 8 ) != 0 )
				EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/bar_shoot.wav", 1, ATTN_NORM );
			if ( m_hEnemy != NULL )
				Shoot();
			CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;
		case ALLIEDGRUNT_AE_BURST2:
		case ALLIEDGRUNT_AE_BURST3:
			Shoot();
			break;
		case ALLIEDGRUNT_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();
			if ( pHurt )
			{
				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );
			}
		}
		break;
		case ALLIEDGRUNT_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz( ENT( pev ), "ALLIEDG_ALERT", ALLIEDGRUNT_SENTENCE_VOLUME, ALLIEDGRUNT_ATTN, 0, m_voicePitch );
				JustSpoke();
			}
		}
		break;
		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
// .text:000c37f0 CAlliedGrunt::Spawn (bot_allied_grunt.cpp:935)
void CAlliedGrunt::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/allied_grunt.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = 247;
	pev->effects = 0;
	pev->health = 100;
	m_flFieldOfView = 0.2;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1.0;
	m_flNextPainTime = gpGlobals->time;
	m_HackedGunPos = Vector( 0, 0, 55 );
	m_iSentence = -1;
	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;
	m_flDistTooFar = 2048.0;
	m_flDistLook = 2048.0;
	if ( pev->weapons == 0 )
		pev->weapons = 2;
	if ( pev->weapons & 2 )
	{
		SetBodygroup( 0, 0 );
		m_cClipSize = 8;
		ALERT( at_console, "Spawn new light grunt with GARAND" );
	}
	else if ( pev->weapons & 4 )
	{
		SetBodygroup( 0, 1 );
		ALERT( at_console, "Spawn new medium grunt with THOMPSON" );
	}
	else if ( pev->weapons & 8 )
	{
		SetBodygroup( 0, 2 );
		m_cClipSize = 20;
		ALERT( at_console, "Spawn new heavy grunt with BAR" );
	}
	CTalkMonster::g_talkWaitTime = 0;
	m_cAmmoLoaded = m_cClipSize;
	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
// .text:000c31e0 CAlliedGrunt::Precache (bot_allied_grunt.cpp:999)
void CAlliedGrunt::Precache( void )
{
	PRECACHE_MODEL( "models/allied_grunt.mdl" );
	PRECACHE_SOUND( "weapons/garand_shoot.wav" );
	PRECACHE_SOUND( "weapons/colt_shoot.wav" );
	PRECACHE_SOUND( "weapons/bar_shoot.wav" );
	PRECACHE_SOUND( "weapons/thompson_shoot.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain4.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain5.wav" );
	PRECACHE_SOUND( "hgrunt/gr_reload1.wav" );
	PRECACHE_SOUND( "zombie/claw_miss2.wav" );
	if ( RANDOM_LONG( 0, 1 ) == 0 )
		m_voicePitch = 100;
	else
		m_voicePitch = 90 + RANDOM_LONG( 0, 7 );
	m_iBrassShell = PRECACHE_MODEL( "models/shell.mdl" );
}	

//=========================================================
// start task
//=========================================================
// .text:000c5a30 CAlliedGrunt::StartTask (bot_allied_grunt.cpp:1034)
void CAlliedGrunt :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_ALLIEDGRUNT_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_GRUNT_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_ALLIEDGRUNT_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_ALLIEDGRUNT_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default: 
		CSquadMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
// .text:000c3f10 CAlliedGrunt::RunTask (bot_allied_grunt.cpp:1085)
void CAlliedGrunt :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ALLIEDGRUNT_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// PainSound
//=========================================================
// .text:000c49e0 CAlliedGrunt::PainSound (bot_allied_grunt.cpp:1112)
void CAlliedGrunt :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
#if 0
		if ( RANDOM_LONG(0,99) < 5 )
		{
			// pain sentences are rare
			if (FOkToSpeak())
			{
				SENTENCEG_PlayRndSz(ENT(pev), "ALLIEDG_PAIN", ALLIEDGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, PITCH_NORM);
				JustSpoke();
				return;
			}
		}
#endif 
		switch ( RANDOM_LONG(0,6) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain3.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain4.wav", 1, ATTN_NORM );	
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain5.wav", 1, ATTN_NORM );	
			break;
		case 3:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain1.wav", 1, ATTN_NORM );	
			break;
		case 4:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain2.wav", 1, ATTN_NORM );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
// .text:000c4900 CAlliedGrunt::DeathSound (bot_allied_grunt.cpp:1154)
void CAlliedGrunt :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die1.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die3.wav", 1, ATTN_IDLE );	
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t	tlAlliedGruntFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slAlliedGruntFail[] =
{
	{
		tlAlliedGruntFail,
		ARRAYSIZE ( tlAlliedGruntFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Grunt Fail"
	},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t	tlAlliedGruntCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slAlliedGruntCombatFail[] =
{
	{
		tlAlliedGruntCombatFail,
		ARRAYSIZE ( tlAlliedGruntCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Grunt Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlAlliedGruntVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slAlliedGruntVictoryDance[] =
{
	{ 
		tlAlliedGruntVictoryDance,
		ARRAYSIZE ( tlAlliedGruntVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"GruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlAlliedGruntEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_ALLIEDGRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_ALLIEDGRUNT_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slAlliedGruntEstablishLineOfFire[] =
{
	{ 
		tlAlliedGruntEstablishLineOfFire,
		ARRAYSIZE ( tlAlliedGruntEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntEstablishLineOfFire"
	},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlAlliedGruntFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slAlliedGruntFoundEnemy[] =
{
	{ 
		tlAlliedGruntFoundEnemy,
		ARRAYSIZE ( tlAlliedGruntFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlAlliedGruntCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_ALLIEDGRUNT_SWEEP	},
};

Schedule_t	slAlliedGruntCombatFace[] =
{
	{ 
		tlAlliedGruntCombatFace1,
		ARRAYSIZE ( tlAlliedGruntCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlAlliedGruntSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slAlliedGruntSignalSuppress[] =
{
	{ 
		tlAlliedGruntSignalSuppress,
		ARRAYSIZE ( tlAlliedGruntSignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlAlliedGruntSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slAlliedGruntSuppress[] =
{
	{ 
		tlAlliedGruntSuppress,
		ARRAYSIZE ( tlAlliedGruntSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlAlliedGruntWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slAlliedGruntWaitInCover[] =
{
	{ 
		tlAlliedGruntWaitInCover,
		ARRAYSIZE ( tlAlliedGruntWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"GruntWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlAlliedGruntTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_ALLIEDGRUNT_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_ALLIEDGRUNT_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_ALLIEDGRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slAlliedGruntTakeCover[] =
{
	{ 
		tlAlliedGruntTakeCover1,
		ARRAYSIZE ( tlAlliedGruntTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlAlliedGruntGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_ALLIEDGRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slAlliedGruntGrenadeCover[] =
{
	{ 
		tlAlliedGruntGrenadeCover1,
		ARRAYSIZE ( tlAlliedGruntGrenadeCover1 ), 
		0,
		0,
		"GrenadeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlAlliedGruntTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slAlliedGruntTossGrenadeCover[] =
{
	{ 
		tlAlliedGruntTossGrenadeCover1,
		ARRAYSIZE ( tlAlliedGruntTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlAlliedGruntTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slAlliedGruntTakeCoverFromBestSound[] =
{
	{ 
		tlAlliedGruntTakeCoverFromBestSound,
		ARRAYSIZE ( tlAlliedGruntTakeCoverFromBestSound ), 
		0,
		0,
		"GruntTakeCoverFromBestSound"
	},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlAlliedGruntHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slAlliedGruntHideReload[] = 
{
	{
		tlAlliedGruntHideReload,
		ARRAYSIZE ( tlAlliedGruntHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlAlliedGruntSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slAlliedGruntSweep[] =
{
	{ 
		tlAlliedGruntSweep,
		ARRAYSIZE ( tlAlliedGruntSweep ), 
		
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Grunt Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlAlliedGruntRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH },
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slAlliedGruntRangeAttack1A[] =
{
	{ 
		tlAlliedGruntRangeAttack1A,
		ARRAYSIZE ( tlAlliedGruntRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlAlliedGruntRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_ALLIEDGRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slAlliedGruntRangeAttack1B[] =
{
	{ 
		tlAlliedGruntRangeAttack1B,
		ARRAYSIZE ( tlAlliedGruntRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlAlliedGruntRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_ALLIEDGRUNT_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_ALLIEDGRUNT_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slAlliedGruntRangeAttack2[] =
{
	{ 
		tlAlliedGruntRangeAttack2,
		ARRAYSIZE ( tlAlliedGruntRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// repel 
//=========================================================



//=========================================================
// repel 
//=========================================================


//=========================================================
// repel land
//=========================================================



DEFINE_CUSTOM_SCHEDULES( CAlliedGrunt )
{
	slAlliedGruntFail,
	slAlliedGruntCombatFail,
	slAlliedGruntVictoryDance,
	slAlliedGruntEstablishLineOfFire,
	slAlliedGruntFoundEnemy,
	slAlliedGruntCombatFace,
	slAlliedGruntSignalSuppress,
	slAlliedGruntSuppress,
	slAlliedGruntWaitInCover,
	slAlliedGruntTakeCover,
	slAlliedGruntGrenadeCover,
	slAlliedGruntTossGrenadeCover,
	slAlliedGruntTakeCoverFromBestSound,
	slAlliedGruntHideReload,
	slAlliedGruntSweep,
	slAlliedGruntRangeAttack1A,
	slAlliedGruntRangeAttack1B,
	slAlliedGruntRangeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES( CAlliedGrunt, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
// .text:000c3530 CAlliedGrunt::SetActivity (bot_allied_grunt.cpp:1731)
void CAlliedGrunt :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		if (FBitSet( pev->weapons, HGRUNT_9MMAR))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_colt" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_colt" );
			}
		}
		else if (FBitSet( pev->weapons, 2 ))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_garand" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_garand" );
			}
		}
		break;
	case ACT_RANGE_ATTACK2:
		// grunt is going to a secondary long range attack. This may be a thrown 
		// grenade or fired grenade, we must determine which and pick proper sequence
		if ( pev->weapons & 0x10 )
		{
			// get toss anim
			iSequence = LookupSequence( "throwgrenade" );
		}
		else
		{
			ALERT( at_console, "No grenades available. " );
		}
		break;
	case ACT_RUN:
		if ( pev->health <= HGRUNT_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= HGRUNT_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CAlliedGrunt :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = ALLIEDGRUNT_SENT_NONE;

	// flying? If PRONE, barnacle has us.
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// don't take falling damage if we hit the ground
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType( SCHED_ALLIEDGRUNT_REPEL_LAND );
		}
		else
		{
			// see if we should fly
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_ALLIEDGRUNT_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_ALLIEDGRUNT_REPEL );
		}
	}

	// grunts place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				
				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "ALLIEDG_GREN", ALLIEDGRUNT_SENTENCE_VOLUME, ALLIEDGRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
					{
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					}
					else 
					{
						//!!!KELLY - the leader of a squad of grunts has just seen the player or a 
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight" 
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if ((m_hEnemy != NULL) && m_hEnemy->IsPlayer())
								// player
								SENTENCEG_PlayRndSz( ENT(pev), "ALLIEDG_ALERT", ALLIEDGRUNT_SENTENCE_VOLUME, ALLIEDGRUNT_ATTN, 0, m_voicePitch);
							else if ((m_hEnemy != NULL) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) && 
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) && 
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "ALLIEDG_MONST", ALLIEDGRUNT_SENTENCE_VOLUME, ALLIEDGRUNT_ATTN, 0, m_voicePitch);

							JustSpoke();
						}
						
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_ALLIEDGRUNT_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_ALLIEDGRUNT_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return GetScheduleOfType ( SCHED_ALLIEDGRUNT_COVER_AND_RELOAD );
			}
			
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != NULL )
				{
					// only try to take cover if we actually have an enemy!

					//!!!KELLY - this grunt was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "ALLIEDG_COVER", ALLIEDGRUNT_SENTENCE_VOLUME, ALLIEDGRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = ALLIEDGRUNT_SENT_COVER;
						//JustSpoke();
					}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a 
					// little time and give the player a chance to turn.
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_ALLIEDGRUNT_FOUND_ENEMY );
					}
				}

				if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					// throw a grenade if can and no engage slots are available
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else
				{
					// hide!
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "ALLIEDG_THROW", ALLIEDGRUNT_SENTENCE_VOLUME, ALLIEDGRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					//!!!KELLY - grunt cannot see the enemy and has just decided to 
					// charge the enemy's position. 
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "ALLIEDG_CHARGE", ALLIEDGRUNT_SENTENCE_VOLUME, ALLIEDGRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = ALLIEDGRUNT_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_ALLIEDGRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - grunt is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// grunt's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "ALLIEDG_TAUNT", ALLIEDGRUNT_SENTENCE_VOLUME, ALLIEDGRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_ALLIEDGRUNT_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
// .text:000c4d50 CAlliedGrunt::GetScheduleOfType (bot_allied_grunt.cpp:2069)
Schedule_t* CAlliedGrunt :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( InSquad() )
			{
				if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "ALLIEDG_THROW", ALLIEDGRUNT_SENTENCE_VOLUME, ALLIEDGRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return slAlliedGruntTossGrenadeCover;
				}
				else
				{
					return &slAlliedGruntTakeCover[ 0 ];
				}
			}
			else
			{
				if ( RANDOM_LONG(0,1) )
				{
					return &slAlliedGruntTakeCover[ 0 ];
				}
				else
				{
					return &slAlliedGruntGrenadeCover[ 0 ];
				}
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slAlliedGruntTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_ALLIEDGRUNT_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_ALLIEDGRUNT_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_ALLIEDGRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			return &slAlliedGruntEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// randomly stand or crouch
			if (RANDOM_LONG(0,4) == 0)
				m_fStanding = RANDOM_LONG(0,1);
		 
			if (m_fStanding)
				return &slAlliedGruntRangeAttack1B[ 0 ];
			else
				return &slAlliedGruntRangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slAlliedGruntRangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slAlliedGruntCombatFace[ 0 ];
		}
	case SCHED_ALLIEDGRUNT_WAIT_FACE_ENEMY:
		{
			return &slAlliedGruntWaitInCover[ 0 ];
		}
	case SCHED_ALLIEDGRUNT_SWEEP:
		{
			return &slAlliedGruntSweep[ 0 ];
		}
	case SCHED_ALLIEDGRUNT_COVER_AND_RELOAD:
		{
			return &slAlliedGruntHideReload[ 0 ];
		}
	case SCHED_ALLIEDGRUNT_FOUND_ENEMY:
		{
			return &slAlliedGruntFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slAlliedGruntFail[ 0 ];
				}
			}

			return &slAlliedGruntVictoryDance[ 0 ];
		}
	case SCHED_ALLIEDGRUNT_SUPPRESS:
		{
			if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slAlliedGruntSignalSuppress[ 0 ];
			}
			else
			{
				return &slAlliedGruntSuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slAlliedGruntCombatFail[ 0 ];
			}

			return &slAlliedGruntFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}


//=========================================================
// CAlliedGruntRepel - when triggered, spawns a monster_allied_grunt
// repelling down a line.
//=========================================================

class CDeadAlliedGrunt : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
 	static const char *m_szPoses[3];
};

const char *CDeadAlliedGrunt::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

// .text:000c3fc0 CDeadAlliedGrunt::KeyValue (bot_allied_grunt.cpp:2208)
void CDeadAlliedGrunt::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}


//=========================================================
// ********** DeadHGrunt SPAWN **********
//=========================================================
// .text:000c3330 CDeadAlliedGrunt::Spawn (bot_allied_grunt.cpp:2224)
void CDeadAlliedGrunt :: Spawn( void )
{
	PRECACHE_MODEL("models/allied_grunt.mdl");
	SET_MODEL(ENT(pev), "models/allied_grunt.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead hgrunt with bad pose\n" );
	}

	// Corpses have less health
	pev->health			= 8;

	// map old bodies onto new bodies
	switch( pev->body )
	{
	case 0:
		pev->skin = 0;
		SetBodygroup( 0, 0 );
		break;
	case 1:
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( 0, 1 );
		break;
	case 2:
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( 0, 2 );
		break;
	}

	MonsterInitDead();
}


// .text:000c5900 monster_allied_grunt (bot_allied_grunt.cpp:189)
LINK_ENTITY_TO_CLASS( monster_allied_grunt, CAlliedGrunt );

// .text:000c6430 monster_alliedgrunt_dead (bot_allied_grunt.cpp:2218)
LINK_ENTITY_TO_CLASS( monster_alliedgrunt_dead, CDeadAlliedGrunt );

// .text:000c3df0 CAlliedGrunt::ScheduleFromName (bot_allied_grunt.cpp:1725)

// .text:000c3eb0 CAlliedGrunt::Save (bot_allied_grunt.cpp:207)

// .text:000c3e50 CAlliedGrunt::Restore (bot_allied_grunt.cpp:207)


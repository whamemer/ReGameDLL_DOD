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
/*

===== combat.cpp ========================================================

  functions dealing with damage infliction & death

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "soundent.h"
#include "decals.h"
#include "animation.h"
#include "weapons.h"
#include "func_break.h"
#include "player.h"
#include "gamerules.h"

extern DLL_GLOBAL Vector g_vecAttackDir;
extern DLL_GLOBAL int g_iSkillLevel;

extern Vector VecBModelOrigin( entvars_t *pevBModel );
extern entvars_t *g_pevLastInflictor;

float MapWallMaterialModFromTexture( char texture );

#define GERMAN_GIB_COUNT 4
#define HUMAN_GIB_COUNT 6
#define ALIEN_GIB_COUNT 4

// HACKHACK -- The gib velocity equations don't work
void CGib ::LimitVelocity( void )
{
	float length = pev->velocity.Length();

	// ceiling at 1500.  The gib velocity equation is not bounded properly.  Rather than tune it
	// in 3 separate places again, I'll just limit it here.
	if ( length > 1500.0 )
		pev->velocity = pev->velocity.Normalize() * 1500;  // This should really be sv_maxvelocity * 0.75 or something
}

void CGib ::SpawnStickyGibs( entvars_t *pevVictim, Vector vecOrigin, int cGibs )
{
	int i;

	if ( g_Language == LANGUAGE_GERMAN )
	{
		// no sticky gibs in germany right now!
		return;
	}

	for ( i = 0; i < cGibs; i++ )
	{
		CGib *pGib = GetClassPtr( (CGib *) NULL );

		pGib->Spawn( "models/stickygib.mdl" );
		pGib->pev->body = RANDOM_LONG( 0, 2 );

		if ( pevVictim )
		{
			pGib->pev->origin.x = vecOrigin.x + RANDOM_FLOAT( -3, 3 );
			pGib->pev->origin.y = vecOrigin.y + RANDOM_FLOAT( -3, 3 );
			pGib->pev->origin.z = vecOrigin.z + RANDOM_FLOAT( -3, 3 );

			/*
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * (RANDOM_FLOAT ( 0 , 1 ) );
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * (RANDOM_FLOAT ( 0 , 1 ) );
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * (RANDOM_FLOAT ( 0 , 1 ) );
			*/

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT( -0.15, 0.15 );
			pGib->pev->velocity.y += RANDOM_FLOAT( -0.15, 0.15 );
			pGib->pev->velocity.z += RANDOM_FLOAT( -0.15, 0.15 );

			pGib->pev->velocity = pGib->pev->velocity * 900;

			pGib->pev->avelocity.x = RANDOM_FLOAT( 250, 400 );
			pGib->pev->avelocity.y = RANDOM_FLOAT( 250, 400 );

			// copy owner's blood color
			pGib->m_bloodColor = ( CBaseEntity::Instance( pevVictim ) )->BloodColor();

			if ( pevVictim->health > -50 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7;
			}
			else if ( pevVictim->health > -200 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 2;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 4;
			}

			pGib->pev->movetype = MOVETYPE_TOSS;
			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize( pGib->pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
			pGib->SetTouch( &CGib::StickyGibTouch );
			pGib->SetThink( NULL );
		}
		pGib->LimitVelocity();
	}
}
void CGib::SpawnHeadGib( entvars_t *pevVictim, int allies )
{
	CGib *pGib = GetClassPtr( (CGib *) NULL );
	if ( !allies )
	{
		pGib->Spawn( "models/axis_gibs.mdl" );
		pGib->pev->body = 0;
	}
	else
	{
		pGib->Spawn( "models/allied_gibs.mdl" );
		pGib->pev->body = 0;
	}
	if ( pevVictim )
	{
		pGib->pev->origin = pevVictim->origin + pevVictim->view_ofs;
		edict_t *pentPlayer = FIND_CLIENT_IN_PVS( pGib->edict() );
		if ( RANDOM_LONG( 0, 100 ) < 6 && pentPlayer )
		{
			entvars_t *pevPlayer = VARS( pentPlayer );
			pGib->pev->velocity = ( ( pevPlayer->origin + pevPlayer->view_ofs ) - pGib->pev->origin ).Normalize() * 300;
			pGib->pev->velocity.z += 100;
		}
		else
			pGib->pev->velocity =
				Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
		pGib->pev->avelocity.x = RANDOM_FLOAT( 100, 200 );
		pGib->pev->avelocity.y = RANDOM_FLOAT( 100, 300 );
		pGib->m_bloodColor = ( CBaseEntity::Instance( pevVictim ) )->BloodColor();
		if ( pevVictim->health > -50 )
			pGib->pev->velocity = pGib->pev->velocity * 0.7;
		else if ( pevVictim->health > -200 )
			pGib->pev->velocity = pGib->pev->velocity * 2;
		else
			pGib->pev->velocity = pGib->pev->velocity * 4;
	}
	pGib->LimitVelocity();
}
void CGib::SpawnRandomGibs( entvars_t *pevVictim, int cGibs, int allies )
{
	return;
}
BOOL CBaseMonster::HasHumanGibs( void )
{
	if ( Classify() > 1 && m_bloodColor == 0xF7 )  // BLOOD_COLOR_RED
		return TRUE;
	return FALSE;
}
BOOL CBaseMonster::HasAlienGibs( void )
{
	if ( Classify() > 1 && m_bloodColor == 0xC3 )  // BLOOD_COLOR_YELLOW
		return TRUE;
	return FALSE;
}

void CBaseMonster::FadeMonster( void )
{
	StopAnimation();
	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->avelocity = g_vecZero;
	pev->animtime = gpGlobals->time;
	pev->effects |= EF_NOINTERP;
	SUB_StartFadeOut();
}

//=========================================================
// GibMonster - create some gore and get rid of a monster's
// model.
//=========================================================
void CBaseMonster::GibMonster( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "common/bodysplat.wav", 1.0, ATTN_NORM, 0, 100 );
	CGib::SpawnHeadGib( pev, ( pev->team == TEAM_ALLIES ) );	// 1 = allies
	if ( !IsPlayer() )
	{
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
}

//=========================================================
// GetDeathActivity - determines the best type of death
// anim to play.
//=========================================================
Activity CBaseMonster::GetDeathActivity( void )
{
	Activity deathActivity = DOD_BUTTSTOCK;
	Vector vecSrc = Center();	// binary: virtual Center() (vtable+0x38) called before UTIL_MakeVectors @000d8a70
	UTIL_MakeVectors( pev->angles );
	BOOL bDucking = FALSE;	// binary: bDucking only set for players; stays FALSE for monsters
	if ( IsPlayer() )
	{
		if ( ( (CBasePlayer *) this )->IsProne() )
			return DOD_PRONE1;
		bDucking = ( (CBasePlayer *) this )->IsDucking();
	}
	(void) vecSrc;
	if ( ( m_bitsDamageType & ( DMG_NERVEGAS | DMG_DOD_GIB6 ) ) ==
		 0 )  // binary: TEST [ESI+0x364],0x2010000 @000d8aea (ghidra 000d8aea, link 0xc8aea)
	{
		float flDot = DotProduct( gpGlobals->v_forward, g_vecAttackDir * -1 );
		BOOL fromFront =
			flDot >
			-0.3;  // binary: _DAT_00195220 double -0.3 (link 0x185220, ghidra 0x195220, FLD [0x195220] @000d8b1d)
		if ( fromFront || !bDucking )
		{
			if (
				flDot <= 0.3 ||
				!bDucking )  // binary: _DAT_00195228 double +0.3 (link 0x185228, ghidra 0x195228, FLD [0x195228] @000d8b33)
			{
				switch ( m_LastHitGroup )
				{
				case HITGROUP_CHEST:
					deathActivity = fromFront ? ACT_DIEFORWARD : ACT_DIEBACKWARD;
					break;
				case HITGROUP_STOMACH:
					deathActivity =
						fromFront
							? ACT_DIEFORWARD
							: ACT_DIE_GUTSHOT;	// binary: front 38 FORWARD / back 68 GUTSHOT (SBB+AND 0xFFFFFFE2+ADD 0x44 @000d8bd0, decompile ~-(fromFront)+38 / (-(fromFront)&-30)+68)
					break;
				default:
					deathActivity = DOD_SIMPLE;
					if ( m_LastHitGroup == HITGROUP_HEAD )
						deathActivity = ACT_DIE_HEADSHOT;
					break;
				}
			}
			else
				deathActivity = DOD_CROUCH_HEADSHOT;
		}
		else
			deathActivity = DOD_CROUCH_BACKWARD;
	}
	return deathActivity;
}

//=========================================================
// GetSmallFlinchActivity - determines the best type of flinch
// anim to play.
//=========================================================
Activity CBaseMonster::GetSmallFlinchActivity( void )
{
	static Activity flinchMap[7] = {
		ACT_FLINCH_HEAD,	  // 70: head (same as SDK)
		ACT_SMALL_FLINCH,	  // 26: chest (SDK: ACT_FLINCH_CHEST) <-- DoD DIFF
		ACT_FLINCH_STOMACH,	  // 72
		ACT_FLINCH_LEFTARM,	  // 73
		ACT_FLINCH_RIGHTARM,  // 74
		ACT_FLINCH_LEFTLEG,	  // 75
		ACT_FLINCH_RIGHTLEG,  // 76
	};
	Activity flinchActivity = ACT_SMALL_FLINCH;
	UTIL_MakeVectors( pev->angles );	// binary: UTIL_MakeVectors @000c8be0
	if ( (unsigned) ( m_LastHitGroup - 1 ) < 7 )
		flinchActivity = flinchMap[m_LastHitGroup - 1];
	if ( LookupActivity( flinchActivity ) == -1 )
		flinchActivity = ACT_SMALL_FLINCH;
	return flinchActivity;
}

void CBaseMonster::BecomeDead( void )
{
	pev->takedamage = DAMAGE_YES;  // don't let autoaim aim at corpses.

	// give the corpse half of the monster's original maximum health.
	pev->health = pev->max_health / 2;
	pev->max_health = 5;  // max_health now becomes a counter for how many blood decals the corpse can place.

	// make the corpse fly away from the attack vector
	pev->movetype = MOVETYPE_TOSS;
	//pev->flags &= ~FL_ONGROUND;
	//pev->origin.z += 2;
	//pev->velocity = g_vecAttackDir * -1;
	//pev->velocity = pev->velocity * RANDOM_FLOAT( 300, 400 );
}
BOOL CBaseMonster::ShouldGibMonster( int iGib )
{
	return FALSE;
}

void CBaseMonster::CallGibMonster( void )
{
	BOOL fade = FALSE;

	if ( HasHumanGibs() )
	{
		if ( CVAR_GET_FLOAT( "violence_hgibs" ) == 0 )
			fade = TRUE;
	}
	else if ( HasAlienGibs() )
	{
		if ( CVAR_GET_FLOAT( "violence_agibs" ) == 0 )
			fade = TRUE;
	}

	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;	 // do something with the body. while monster blows up

	if ( fade )
	{
		FadeMonster();
	}
	else
	{
		pev->effects = EF_NODRAW;  // make the model invisible.
		GibMonster();
	}

	pev->deadflag = DEAD_DEAD;
	FCheckAITrigger();

	// don't let the status bar glitch for players.with <0 health.
	if ( pev->health < -99 )
	{
		pev->health = 0;
	}

	if ( ShouldFadeOnDeath() && !fade )
		UTIL_Remove( this );
}

/*
============
Killed
============
*/
void CBaseMonster::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( HasMemory( bits_MEMORY_KILLED ) )
		return;
	Remember( bits_MEMORY_KILLED );	 // binary: OR m_afMemory,0x80 @000d8dd0 (SET, not Forget)
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM, 0, 100 );
	m_IdealMonsterState = MONSTERSTATE_DEAD;
	SetConditions( bits_COND_LIGHT_DAMAGE );
	m_IdealMonsterState = MONSTERSTATE_DEAD;
	CBaseEntity *pOwner = NULL;
	if ( pev->owner )
		pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner )
		pOwner->DeathNotice( pev );	 // owner gets death notice (vtbl +0x38)
	if ( pev->flags & FL_MONSTER )
	{
		SetTouch( NULL );
		Touch( NULL );	// vtbl +0x48 hook, then
	}
	if ( pev->health < -99 )
		pev->health = 0;
	m_IdealMonsterState = MONSTERSTATE_DEAD;
}

//
// fade out - slowly fades a entity out, then removes it.
//
// DON'T USE ME FOR GIBS AND STUFF IN MULTIPLAYER!
// SET A FUTURE THINK AND A RENDERMODE!!
void CBaseEntity ::SUB_StartFadeOut( void )
{
	if ( pev->rendermode == kRenderNormal )
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}

	pev->solid = SOLID_NOT;
	pev->avelocity = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink( &CBaseEntity::SUB_FadeOut );
}

void CBaseEntity ::SUB_FadeOut( void )
{
	if ( pev->renderamt > 7 )
	{
		pev->renderamt -= 7;
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		pev->renderamt = 0;
		pev->nextthink = gpGlobals->time + 0.2;
		SetThink( &CBaseEntity::SUB_Remove );
	}
}

//=========================================================
// WaitTillLand - in order to emit their meaty scent from
// the proper location, gibs should wait until they stop
// bouncing to emit their scent. That's what this function
// does.
//=========================================================
void CGib ::WaitTillLand( void )
{
	if ( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	if ( pev->velocity == g_vecZero )
	{
		SetThink( &CGib::SUB_StartFadeOut );
		pev->nextthink = gpGlobals->time + m_lifeTime;

		// If you bleed, you stink!
		if ( m_bloodColor != DONT_BLEED )
		{
			// ok, start stinkin!
			CSoundEnt::InsertSound( bits_SOUND_MEAT, pev->origin, 384, 25 );
		}
	}
	else
	{
		// wait and check again in another half second.
		pev->nextthink = gpGlobals->time + 0.5;
	}
}

//
// Gib bounces on the ground or wall, sponges some blood down, too!
//
void CGib ::BounceGibTouch( CBaseEntity *pOther )
{
	Vector vecSpot;
	TraceResult tr;

	//if ( RANDOM_LONG(0,1) )
	//	return;// don't bleed everytime

	if ( pev->flags & FL_ONGROUND )
	{
		pev->velocity = pev->velocity * 0.9;
		pev->angles.x = 0;
		pev->angles.z = 0;
		pev->avelocity.x = 0;
		pev->avelocity.z = 0;
	}
	else
	{
		if ( g_Language != LANGUAGE_GERMAN && m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED )
		{
			vecSpot = pev->origin + Vector( 0, 0, 8 );	//move up a bit, and trace down.
			UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -24 ), ignore_monsters, ENT( pev ), &tr );

			UTIL_BloodDecalTrace( &tr, m_bloodColor );

			m_cBloodDecals--;
		}

		if ( m_material != matNone && RANDOM_LONG( 0, 2 ) == 0 )
		{
			float volume;
			float zvel = fabs( pev->velocity.z );

			volume = 0.8 * min( 1.0, ( (float) zvel ) / 450.0 );

			CBreakable::MaterialSoundRandom( edict(), (Materials) m_material, volume );
		}
	}
}

//
// Sticky gib puts blood on the wall and stays put.
//
void CGib ::StickyGibTouch( CBaseEntity *pOther )
{
	Vector vecSpot;
	TraceResult tr;

	SetThink( &CGib::SUB_Remove );
	pev->nextthink = gpGlobals->time + 10;

	if ( !FClassnameIs( pOther->pev, "worldspawn" ) )
	{
		pev->nextthink = gpGlobals->time;
		return;
	}

	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 32, ignore_monsters, ENT( pev ), &tr );

	UTIL_BloodDecalTrace( &tr, m_bloodColor );

	pev->velocity = tr.vecPlaneNormal * -1;
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
}

//
// Throw a chunk
//
void CGib ::Spawn( const char *szGibModel )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55;  // deading the bounce a bit

	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or rendermode! bad!
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_SLIDEBOX;  /// hopefully this will fix the VELOCITY TOO LOW crap
	if ( pev->classname )
		RemoveEntityHashValue( pev, STRING( pev->classname ), CLASSNAME );	// binary: RemoveEntityHashValue @CGib::Spawn 000d8f59
	pev->classname = MAKE_STRING( "gib" );
	AddEntityHashValue( pev, STRING( pev->classname ), CLASSNAME );	 // binary: AddEntityHashValue @CGib::Spawn 000d8f93

	SET_MODEL( ENT( pev ), szGibModel );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->nextthink = gpGlobals->time + 4;
	m_lifeTime = 25;
	SetThink( &CGib::WaitTillLand );
	SetTouch( &CGib::BounceGibTouch );

	m_material = matNone;
	m_cBloodDecals = 5;	 // how many blood decals this gib can place (1 per bounce until none remain).
}

// take health
int CBaseMonster::TakeHealth( float flHealth, int bitsDamageType )
{
	if ( !pev->takedamage )
		return 0;
	m_bitsDamageType &= ~(
		bitsDamageType &
		DMG_DOD_TAKEHEALTH_MASK );	// binary: AND 0x3fbf @000d9a00 (link 0xc9a00), =0x3fff & ~DMG_BLAST (drops BLAST 0x40 vs SDK 0x3fff)
	return CBaseEntity::TakeHealth( flHealth, bitsDamageType );
}

/*
============
TakeDamage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
bitsDamageType indicates the type of damage sustained, ie: DMG_SHOCK

Time-based damage: only occurs while the monster is within the trigger_hurt.
When a monster is poisoned via an arrow etc it takes all the poison damage at once.



GLOBALS ASSUMED SET:  g_iSkillLevel
============
*/
int CBaseMonster::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	float flTake;
	Vector vecDir;

	if ( !pev->takedamage )
		return 0;

	if ( !IsAlive() )
		return DeadTakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );

	if ( pev->deadflag == DEAD_NO )
		PainSound();

	flTake = flDamage;

	// DoD: plain assign, SDK does |=
	m_bitsDamageType = bitsDamageType;

	vecDir = Vector( 0, 0, 0 );
	if ( !FNullEnt( pevInflictor ) )
	{
		CBaseEntity *pInflictor = CBaseEntity::Instance( pevInflictor );
		if ( pInflictor )
		{
			vecDir = ( pInflictor->Center() - Vector( 0, 0, 10 ) - Center() ).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

	if ( IsPlayer() )
	{
		if ( pevInflictor )
			pev->dmg_inflictor = ENT( pevInflictor );
		pev->dmg_take += flTake;
		if ( pev->flags & FL_GODMODE )
			return 0;
	}

	pev->health -= flTake;

	if ( m_MonsterState == MONSTERSTATE_SCRIPT )
	{
		SetConditions( bits_COND_LIGHT_DAMAGE );
		return 0;
	}

	if ( pev->health <= 0 )
	{
		g_pevLastInflictor = pevInflictor;
		// binary: DoD gib-style map @000d9ca0 (link 0xc9ca0): ALWAYSGIB->2; if NEVERGIB clear then
		// PARALYZE->3, POISON->5, NERVEGAS->4, GIB6->6 else 1; NEVERGIB set forces style 1.
		if ( bitsDamageType & DMG_ALWAYSGIB )
			Killed( pevAttacker, 2 );
		else if ( bitsDamageType & DMG_NEVERGIB )
			Killed( pevAttacker, 1 );
		else if ( bitsDamageType & DMG_PARALYZE )
			Killed( pevAttacker, 3 );
		else if ( bitsDamageType & DMG_POISON )
			Killed( pevAttacker, 5 );
		else if ( bitsDamageType & DMG_NERVEGAS )
			Killed( pevAttacker, 4 );
		else if ( bitsDamageType & DMG_DOD_GIB6 )
			Killed( pevAttacker, 6 );
		else
			Killed( pevAttacker, 1 );
		g_pevLastInflictor = NULL;
		return 0;
	}

	if ( ( pev->flags & FL_MONSTER ) && !FNullEnt( pevAttacker ) )
	{
		if ( pevAttacker->flags & ( FL_MONSTER | FL_CLIENT ) )
		{
			if ( pevInflictor )
			{
				if ( m_hEnemy == NULL || pevInflictor == m_hEnemy->pev || !HasConditions( bits_COND_SEE_ENEMY ) )
					m_vecEnemyLKP = pevInflictor->origin;
			}
			else
				m_vecEnemyLKP = pev->origin + ( g_vecAttackDir * 64 );
			MakeIdealYaw( m_vecEnemyLKP );
			if ( flDamage > 0 )
				SetConditions( bits_COND_LIGHT_DAMAGE );
			if ( flDamage >= 20 )
				SetConditions( bits_COND_HEAVY_DAMAGE );
		}
	}
	return 1;
}

//=========================================================
// DeadTakeDamage - takedamage function called when a monster's
// corpse is damaged.
//=========================================================
int CBaseMonster::DeadTakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Vector vecDir = Vector( 0, 0, 0 );
	if ( !FNullEnt( pevInflictor ) )
	{
		CBaseEntity *pInflictor = CBaseEntity::Instance( pevInflictor );
		if ( pInflictor )
		{
			vecDir = ( pInflictor->Center() - Vector( 0, 0, 10 ) - Center() ).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

	// binary: TEST bits & 0xE1 @000d9a50 (link 0xc9a50). DoD drops SONIC vs SDK DMG_GIB_CORPSE 0x2E1.
	if ( bitsDamageType & DMG_DOD_GIB_CORPSE )
	{
		if ( pev->health <= flDamage )
		{
			pev->health = -50;
			Killed( pevAttacker, 2 );  // GIB_ALWAYS
			return 0;
		}
		pev->health -= flDamage * 0.1;
	}
	return 1;
}

float CBaseMonster ::DamageForce( float damage )
{
	float force = damage * ( ( 32 * 32 * 72.0 ) / ( pev->size.x * pev->size.y * pev->size.z ) ) * 5;

	if ( force > 1000.0 )
	{
		force = 1000.0;
	}

	return force;
}

//
// RadiusDamage - this entity is exploding, or otherwise needs to inflict damage upon entities within a certain range.
//
// only damage ents that can clearly be seen by the explosion!
void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius,
				   int iClassIgnore, int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	TraceResult tr;
	float flAdjustedDamage, falloff;
	Vector vecSpot;
	float falloff_ = flRadius ? flDamage / flRadius : 1.0;
	falloff = falloff_;
	UTIL_PointContents( vecSrc );
	vecSrc.z += 1;
	if ( !pevAttacker )
		pevAttacker = pevInflictor;
	CBaseEntity *pAttackerRef = CBaseEntity::Instance(
		pevAttacker );	// binary: edict+0x7C = pvPrivateData (gdb ptype /o edict_t: pvPrivateData @124=0x7C; CBaseEntity+0x7C would be m_rrHealth float, not ptr). Disasm MOV EAX,[EDX+0x208](pContainingEntity) + MOV EAX,[EAX+0x7C](GET_PRIVATE) @000da368/000da376 (link 0xca368/0xca376, addr2line enginecallback.h:103 GET_PRIVATE). So attackerRef = Instance(pevAttacker).

	if ( pAttackerRef )
		pAttackerRef->IsPlayer();

	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, flRadius ) ) != NULL )
	{
		if ( pEntity->pev->takedamage == DAMAGE_NO )
			continue;
		if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			continue;
		vecSpot = pEntity->BodyTarget( vecSrc );
		UTIL_TraceLine( vecSrc, vecSpot, dont_ignore_monsters, ENT( pevInflictor ), &tr );
		if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() ||
			 ( vecSrc - pEntity->pev->origin ).Length() < 10.0 )	// binary: extra DoD check _DAT_0018fde8=10.0 (link 0x17fde8) @000ca49e
		{
			if ( tr.fStartSolid )
			{
				tr.vecEndPos = vecSrc;
				tr.flFraction = 0.0;
			}
			flAdjustedDamage = ( vecSrc - tr.vecEndPos ).Length() * falloff;
			flAdjustedDamage = flDamage - flAdjustedDamage;
			if ( flAdjustedDamage < 0 )
				flAdjustedDamage = 0;
			if ( tr.flFraction != 1.0 )
			{
				ClearMultiDamage();
				int bits = bitsDamageType;
				if ( pEntity == pAttackerRef )
					bits |=
						DMG_DOD_SELFSPLASH;	 // binary: OR 0x10000000 when victim==attackerRef @000da300 (decompile (uint)bits|0x10000000)
				pEntity->TraceAttack( pevInflictor, flAdjustedDamage, ( tr.vecEndPos - vecSrc ).Normalize(), &tr,
									  bits );
				ApplyMultiDamage( pevInflictor, pevAttacker );
			}
			else
				pEntity->TakeDamage( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
		}
	}
}

void CBaseMonster ::RadiusDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore,
								  int bitsDamageType )
{
	float mod = ( bitsDamageType & DMG_SONIC ) ? 1.5f : 2.5f;  // binary: FCMOVE picks 1.5 (link 0x17fde4) when DMG_SONIC(0x200) set else 2.5 @000ca7b0
	::RadiusDamage( pev->origin, pevInflictor, pevAttacker, flDamage, flDamage * mod, iClassIgnore, bitsDamageType );
}

void CBaseMonster ::RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage,
								  int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage( vecSrc, pevInflictor, pevAttacker, flDamage, flDamage * 2.5, iClassIgnore, bitsDamageType );
}

//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
//
// Used for many contact-range melee attacks. Bites, claws, etc.
//=========================================================
CBaseEntity *CBaseMonster ::CheckTraceHullAttack( float flDist, int iDamage, int iDmgType )
{
	TraceResult tr;

	if ( IsPlayer() )
		UTIL_MakeVectors( pev->angles );
	else
		UTIL_MakeAimVectors( pev->angles );

	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + ( gpGlobals->v_forward * flDist );

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT( pev ), &tr );

	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if ( iDamage > 0 )
		{
			pEntity->TakeDamage( pev, pev, iDamage, iDmgType );
		}

		return pEntity;
	}

	return NULL;
}

//=========================================================
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall.
//=========================================================
BOOL CBaseMonster ::FInViewCone( CBaseEntity *pEntity )
{
	Vector2D vec2LOS;
	float flDot;

	UTIL_MakeVectors( pev->angles );

	vec2LOS = ( pEntity->pev->origin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct( vec2LOS, gpGlobals->v_forward.Make2D() );

	if ( flDot > m_flFieldOfView )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// FInViewCone - returns true is the passed vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall.
//=========================================================
BOOL CBaseMonster ::FInViewCone( Vector *pOrigin )
{
	Vector2D vec2LOS;
	float flDot;

	UTIL_MakeVectors( pev->angles );

	vec2LOS = ( *pOrigin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct( vec2LOS, gpGlobals->v_forward.Make2D() );

	if ( flDot > m_flFieldOfView )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target
//=========================================================
BOOL CBaseEntity ::FVisible( CBaseEntity *pEntity )
{
	TraceResult tr;
	Vector vecLookerOrigin;
	Vector vecTargetOrigin;

	if ( FBitSet( pEntity->pev->flags, FL_NOTARGET ) )
		return FALSE;

	// don't look through water
	if ( ( pev->waterlevel != 3 && pEntity->pev->waterlevel == 3 ) ||
		 ( pev->waterlevel == 3 && pEntity->pev->waterlevel == 0 ) )
		return FALSE;

	vecLookerOrigin = pev->origin + pev->view_ofs;	//look through the caller's 'eyes'
	vecTargetOrigin = pEntity->EyePosition();

	UTIL_TraceLine( vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, ENT( pev ) /*pentIgnore*/, &tr );

	if ( tr.flFraction != 1.0 )
	{
		return FALSE;  // Line of sight is not established
	}
	else
	{
		return TRUE;  // line of sight is valid.
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target vector
//=========================================================
BOOL CBaseEntity ::FVisible( const Vector &vecOrigin )
{
	TraceResult tr;
	Vector vecLookerOrigin;

	vecLookerOrigin = EyePosition();  //look through the caller's 'eyes'

	UTIL_TraceLine( vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, ENT( pev ) /*pentIgnore*/, &tr );

	if ( tr.flFraction != 1.0 )
	{
		return FALSE;  // Line of sight is not established
	}
	else
	{
		return TRUE;  // line of sight is valid.
	}
}

/*
================
TraceAttack
================
*/
void CBaseEntity::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr,
							   int bitsDamageType )
{
	if ( pev->takedamage )
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
		int blood = BloodColor();
		if (
			blood != DONT_BLEED &&
			!( bitsDamageType &
			   DMG_DOD_NOBLOOD ) )	// binary: TEST bits & 0x8000000 @000d7460 (link 0xc7460, decompile (bits&0x8000000)==0)
		{
			Vector vecOrigin = ptr->vecEndPos - vecDir * 4;	 // binary: _DAT_001951b8 float 4.0 (link 0x1851b8)
			SpawnBlood( vecOrigin, blood, flDamage );	// a little surface blood.
			TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		}
	}
}

/*
//=========================================================
// TraceAttack
//=========================================================
void CBaseMonster::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4;

	ALERT ( at_console, "%d\n", ptr->iHitgroup );


	if ( pev->takedamage )
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );

		int blood = BloodColor();
		
		if ( blood != DONT_BLEED )
		{
			SpawnBlood(vecOrigin, blood, flDamage);// a little surface blood.
		}
	}
}
*/

//=========================================================
// TraceAttack
//=========================================================
void CBaseMonster ::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr,
								 int bitsDamageType )
{
	if ( pev->takedamage )
	{
		m_LastHitGroup = ptr->iHitgroup;

		switch ( ptr->iHitgroup )
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			flDamage *= gSkillData.monHead;
			break;
		case HITGROUP_CHEST:
			flDamage *= gSkillData.monChest;
			break;
		case HITGROUP_STOMACH:
			flDamage *= gSkillData.monStomach;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			flDamage *= gSkillData.monArm;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= gSkillData.monLeg;
			break;
		default:
			break;
		}

		SpawnBlood( ptr->vecEndPos, BloodColor(), flDamage );  // a little surface blood.
		TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
	}
}

float MapWallMaterialModFromTexture( char texture )
{
	// binary: FireTWalls wall table CSWTCH.266 @ghidra 0x194fa0 (link 0x184fa0, gdb x/26fw 0x184fa0: 12,5.5,7.3,5.5,2,2,12,12,5.5,5.5,12,5.5,12,2,5.5,5.5,5.5,7.3,5.5,2,5.5,5.5,2,5.5,2,2), default 5.5 (FLD [0x1951d8] @000daec1). No 0.9 factor.
	static float wallmods[26] = {
		12.0, 5.5, 7.3, 5.5, 2.0, 2.0, 12.0, 12.0, 5.5, 5.5, 12.0, 5.5, 12.0,
		2.0,  5.5, 5.5, 5.5, 7.3, 5.5, 2.0,	 5.5,  5.5, 2.0, 5.5,  2.0, 2.0,
	};
	float mod = 5.5;  // binary: _DAT_001951d8
	if ( (unsigned char) ( texture - 'A' ) < 26 )
		mod = wallmods[(unsigned char) ( texture - 'A' )];
	return mod;
}

void CBaseEntity::FireTWalls( float flDamage, int iBulletType, entvars_t *pevAttacker, Vector vecDir, Vector vecSrc,
							  Vector vecRight, Vector vecUp, Vector vecDest )
{
	if ( g_pGameRules->RoundState() !=
		 1 )  // binary: vtbl+0x3d RoundState gate @000dacd0 (if !=1 return, decompile if (RoundState==1) {...})
		return;
	TraceResult tr, beam_tr;
	edict_t *pentIgnore = NULL;
	if ( FNullEnt( pevAttacker ) )
		pevAttacker = pev;

	UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, ENT( pev ), &tr );
	pentIgnore = tr.pHit;
	// trace back from beyond the exit point to find wall thickness
	Vector vecBack = tr.vecEndPos + ( tr.vecEndPos - vecSrc );
	UTIL_TraceLine( vecBack, vecDest, dont_ignore_monsters, tr.pHit, &beam_tr );

	CBaseEntity *pEntity = NULL;
	if ( tr.pHit && !FNullEnt( tr.pHit ) )
		pEntity = CBaseEntity::Instance( tr.pHit );

	BOOL bPlayerHit = FALSE;
	char chTextureType = 'C';
	if ( pEntity && !pEntity->IsPlayer() )
	{
		const char *pTextureName = TRACE_TEXTURE( tr.pHit ? tr.pHit : ENT( 0 ), vecSrc, vecDest );
		if ( !pTextureName )
			ALERT( at_aiconsole, "Texture: nothing!\n" );
		else
		{
			ALERT( at_aiconsole, "Texture: %s\n", pTextureName );
			if ( *pTextureName == '+' || *pTextureName == '-' )
				pTextureName += 2;	// binary: skip +/- prefix (uses __src[2]) @000dacd0
			if ( *pTextureName == '!' || *pTextureName == '{' || *pTextureName == '~' || *pTextureName == ' ' )
				pTextureName++;	 // binary: skip !{~ space prefix
			char szbuffer[64];
			strcpy( szbuffer, pTextureName );
			szbuffer[CBTEXTURENAMEMAX - 1] = 0;
			chTextureType = TEXTURETYPE_Find( szbuffer );
		}
	}
	else
	{
		bPlayerHit = TRUE;
		chTextureType = 'F';
	}

	if ( beam_tr.fAllSolid )
		return;

	edict_t *pWorld = bPlayerHit ? pentIgnore : NULL;
	UTIL_TraceLine( beam_tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, pWorld, &beam_tr );
	float thickness = ( beam_tr.vecEndPos - tr.vecEndPos ).Length();
	float flMaterialMod = MapWallMaterialModFromTexture(
		chTextureType );  // binary: _DAT_001951d8 float 5.5 default (link 0x1851d8, FLD [0x1951d8] @000daec1) + table CSWTCH.266 @0x194fa0 (link 0x184fa0, FLD [EAX*4+0x194fa0] @000db045), no 0.9 factor unlike FireBulletsNC
	ALERT( at_aiconsole, "Entering surface of materialmod %f, thickness %f, with damage %f\n", flMaterialMod, thickness,
		   flDamage );
	flDamage -= flMaterialMod * thickness;
	if ( flDamage <= 0 )
	{
		ALERT( at_aiconsole, "Bullet was stopped by wall\n" );
		return;
	}
	ALERT( at_aiconsole, "Exiting surface  - damage %f\n", flDamage );
	DecalGunshot( &beam_tr, iBulletType );
	UTIL_TraceLine( beam_tr.vecEndPos, vecDest, dont_ignore_monsters, pentIgnore, &beam_tr );
	DecalGunshot( &beam_tr, iBulletType );
	if ( beam_tr.pHit && !FNullEnt( beam_tr.pHit ) )
	{
		CBaseEntity *pEnt = CBaseEntity::Instance( beam_tr.pHit );
		if ( pEnt && pEnt->pev->takedamage != DAMAGE_NO )
		{
			ClearMultiDamage();
			pEnt->TraceAttack( pev, flDamage, vecDir, &beam_tr, DMG_BULLET );
			ApplyMultiDamage( pev, pev );
		}
	}
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.

This version is used by Monsters.
================
*/
void CBaseEntity::FireBullets( ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance,
							   int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker )
{
	static int tracerCount;
	TraceResult tr;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;

	if (
		g_pGameRules->RoundState() !=
		1 )	 // binary: vtbl+0x3d RoundState()==ROUNDSTATE_RUNNING (CALL [EDX+0xF4] + DEC/JZ @000db22a, decompile CDoDTeamPlay::RoundState returns m_RoundState @000e36e0)
		return;
	if ( pevAttacker == NULL )
		pevAttacker = pev;

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;	// 0x1002

	for ( ULONG iShot = 1; iShot <= cShots; iShot++ )
	{
		float x, y;
		// gaussian spread (shared-rand variant for players: see FireBulletsNC)
		do
		{
			x = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
			y = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
		} while ( x * x + y * y > 1 );
		Vector vecDir = vecDirShooting + x * vecSpread.x * vecRight + y * vecSpread.y * vecUp;
		Vector vecEnd = vecSrc + vecDir * flDistance;

		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

		if ( iTracerFreq != 0 && ( tracerCount++ % iTracerFreq ) == 0 )
		{
			Vector vecTracerSrc;
			if ( IsPlayer() )
				vecTracerSrc = vecSrc + Vector( 0, 0, -4 ) + gpGlobals->v_right * 2 + gpGlobals->v_forward * 16;  // binary: +Vector(0,0,-4)+v_right*2+v_forward*16 @FireBullets (fadds/FMUL 0x185204=-4,0x17fdf4=16)
			else
				vecTracerSrc = vecSrc;
			PLAYBACK_EVENT_FULL( 0, NULL, g_pGameRules->BulletTracers(), 0, (float *)&vecTracerSrc, (float *)&tr.vecEndPos, 0, 0, 0, 0, 0, 0 );
		}

		if ( tr.flFraction == 1.0 )
			continue;

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if ( iDamage != 0 )
		{
			pEntity->TraceAttack( pevAttacker, iDamage, vecDir, &tr, DMG_BULLET );
			TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
			DecalGunshot( &tr, iBulletType );
			continue;
		}

		if ( iBulletType == 0 )
		{
			// glass: a blow, the texture sound and the glass decal if the entity is rendered
			pEntity->TraceAttack( pevAttacker, 50.0f, vecDir, &tr, DMG_CLUB );
			TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
			if ( tr.pHit && !FNullEnt( tr.pHit ) && VARS( tr.pHit )->rendermode != 0 )
				UTIL_DecalTrace( &tr, DECAL_GLASSBREAK1 + RANDOM_LONG( 0, 2 ) );  // 0x19 + rand
			continue;
		}

		float flDmg;
		BOOL bPenetrate = FALSE;
		switch ( iBulletType )
		{
		default:
			flDmg = gSkillData.plrDmg9MM;
			break;
		case 2:
			flDmg = gSkillData.plrDmgMP5;
			break;
		case 6:
			flDmg = gSkillData.plrDmgCOLT;
			break;
		case 7:
			flDmg = gSkillData.plrDmgLUGER;
			break;
		case 8:
			flDmg = gSkillData.plrDmgGARAND;
			bPenetrate = TRUE;
			break;
		case 9:
			flDmg = gSkillData.plrDmgSCOPEDKAR;
			bPenetrate = TRUE;
			break;
		case 10:
			flDmg = gSkillData.plrDmgTHOMPSON;
			bPenetrate = TRUE;
			break;
		case 11:
			flDmg = gSkillData.plrDmgMP44;
			bPenetrate = TRUE;
			break;
		case 12:
			flDmg = gSkillData.plrDmgSPRING;
			bPenetrate = TRUE;
			break;
		case 13:
			flDmg = gSkillData.plrDmgKAR;
			bPenetrate = TRUE;
			break;
		case 14:
			flDmg = gSkillData.plrDmgBAR;
			bPenetrate = TRUE;
			break;
		case 15:
			flDmg = gSkillData.plrDmgMP40;
			bPenetrate = TRUE;
			break;
		case 16:
			flDmg = gSkillData.plrDmgMG42;
			bPenetrate = TRUE;
			break;
		case 17:
			flDmg = gSkillData.plrDmgMG34;
			bPenetrate = TRUE;
			break;
		case 18:
			flDmg = gSkillData.plrDmg30CAL;
			bPenetrate = TRUE;
			break;
		case 19:
			flDmg = gSkillData.plrDmgCARBINE;
			bPenetrate = TRUE;
			break;
		case 20:
			flDmg = gSkillData.plrDmgGREASEGUN;
			bPenetrate = TRUE;
			break;
		case 21:
			flDmg = gSkillData.plrDmgFG42;
			bPenetrate = TRUE;
			break;
		case 22:
			flDmg = gSkillData.plrDmgK43;
			bPenetrate = TRUE;
			break;
		case 23:
			flDmg = gSkillData.plrDmgEnfield;
			bPenetrate = TRUE;
			break;
		case 24:
			flDmg = gSkillData.plrDmgSten;
			bPenetrate = TRUE;
			break;
		case 25:
			flDmg = gSkillData.plrDmgBren;
			bPenetrate = TRUE;
			break;
		case 26:
			flDmg = gSkillData.plrDmgWebley;
			bPenetrate = TRUE;
			break;
		}
		pEntity->TraceAttack( pevAttacker, flDmg, vecDir, &tr, bPenetrate ? ( DMG_BULLET | DMG_DOD_PENETRATE ) : DMG_BULLET );
		if ( bPenetrate && g_pGameRules->RoundState() == ROUNDSTATE_RUNNING )
			FireTWalls( flDmg, iBulletType, pevAttacker, vecDir, vecSrc, vecRight, vecUp, vecEnd );
	}
	ApplyMultiDamage( pev, pevAttacker );
}

void CBaseEntity::RifleMeleeAttack( CBasePlayer *pPlayer, int weapon_id )
{
	if ( g_pGameRules->RoundState() != ROUNDSTATE_RUNNING )
		return;

	UTIL_MakeVectors( pPlayer->pev->v_angle );

	Vector vecSrc = pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 64;

	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pPlayer->pev ), &tr );

	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( pPlayer->pev ), &tr );

		if ( tr.flFraction < 1.0 )
		{
			// The search for the best intersection of the hull is skipped: the original starts it only for
			// a trace longer than the whole way, which never happens.

			// This is the point on the actual surface (the hull could have hit space)
			vecEnd = tr.vecEndPos;
		}

		if ( tr.flFraction >= 1.0 )
			return;
	}

	CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

	ClearMultiDamage();

	switch ( weapon_id )
	{
	case WEAPON_KAR:
		pEntity->TraceAttack( pPlayer->pev, gSkillData.plrDmgBayonet, gpGlobals->v_forward, &tr, DMG_DOD_MELEE_BAYONET );
		break;
	case WEAPON_GARAND:
		pEntity->TraceAttack( pPlayer->pev, gSkillData.plrDmgGarandButt, gpGlobals->v_forward, &tr, DMG_DOD_MELEE_GARAND );
		break;
	case WEAPON_K43:
		pEntity->TraceAttack( pPlayer->pev, gSkillData.plrDmgK43Butt, gpGlobals->v_forward, &tr, DMG_DOD_MELEE_K43 );
		break;
	case WEAPON_ENFIELD:
		pEntity->TraceAttack( pPlayer->pev, gSkillData.plrDmgEnfieldButt, gpGlobals->v_forward, &tr, DMG_DOD_MELEE_ENFIELD );
		break;
	}

	ApplyMultiDamage( pPlayer->pev, pPlayer->pev );

	if ( !pEntity || pEntity->Classify() == CLASS_NONE || pEntity->Classify() == CLASS_MACHINE )
	{
		// hit the world, play the sound of the texture
		float fvolbar = TEXTURETYPE_PlaySound( &tr, vecSrc, vecSrc + ( vecEnd - vecSrc ) * 2, BULLET_PLAYER_CROWBAR );

		if ( g_pGameRules->IsMultiplayer() )
			fvolbar = 1.0;

		if ( weapon_id == WEAPON_ENFIELD || weapon_id == WEAPON_KAR )
		{
			// delay the decal a bit
			SetThink( &CBaseEntity::DecalThink );
			m_trDecalTrace = tr;
			pev->nextthink = gpGlobals->time + 0.1;

			if ( RANDOM_LONG( 0, 1 ) == 0 )
				EMIT_SOUND_DYN( ENT( pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
			else
				EMIT_SOUND_DYN( ENT( pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
		}

		pPlayer->m_iWeaponVolume = 512;
		return;
	}

	if ( weapon_id == WEAPON_KAR || weapon_id == WEAPON_ENFIELD )
	{
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:
			EMIT_SOUND_DYN( ENT( pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 1:
			EMIT_SOUND_DYN( ENT( pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit2.wav", 1, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 2:
			EMIT_SOUND_DYN( ENT( pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit3.wav", 1, ATTN_NORM, 0, PITCH_NORM );
			break;
		}
	}
	else if ( weapon_id == WEAPON_GARAND || weapon_id == WEAPON_K43 )
	{
		if ( pEntity->IsPlayer() )
			EMIT_SOUND_DYN( ENT( pPlayer->pev ), CHAN_VOICE, "weapons/butt_hit.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	}

	pPlayer->m_iWeaponVolume = 128;

	if ( pEntity->IsAlive() )
		pPlayer->m_iWeaponVolume = 51;
}

void CBaseEntity::DecalThink( void )
{
	DecalGunshot( &m_trDecalTrace, 5 );
}

float MapBulletToDmg( int bullet )
{
	switch ( bullet )
	{
	case 0:
		return 50.0;  // binary: _DAT_00190f40 float 50.0 (link 0x180f40, ghidra 0x190f40, gdb x/fw 0x180f40=50)
	case 6:
		return gSkillData.plrDmgCOLT;
	case 7:
		return gSkillData.plrDmgLUGER;
	case 8:
		return gSkillData.plrDmgGARAND;
	case 9:
		return gSkillData.plrDmgSCOPEDKAR;
	case 10:
		return gSkillData.plrDmgTHOMPSON;
	case 11:
		return gSkillData.plrDmgMP44;
	case 12:
		return gSkillData.plrDmgSPRING;
	case 13:
		return gSkillData.plrDmgKAR;
	case 14:
		return gSkillData.plrDmgBAR;
	case 15:
		return gSkillData.plrDmgMP40;
	case 16:
		return gSkillData.plrDmgMG42;
	case 17:
		return gSkillData.plrDmgMG34;
	case 18:
		return gSkillData.plrDmg30CAL;
	case 19:
		return gSkillData.plrDmgCARBINE;
	case 20:
		return gSkillData.plrDmgGREASEGUN;
	case 21:
		return gSkillData.plrDmgFG42;
	case 22:
		return gSkillData.plrDmgK43;
	case 23:
		return gSkillData.plrDmgEnfield;
	case 24:
		return gSkillData.plrDmgSten;
	case 25:
		return gSkillData.plrDmgBren;
	case 26:
		return gSkillData.plrDmgWebley;
	default:
		return 0.0;
	}
}

float MapMaterialModFromTexture( char texture )
{
	// index: A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
	static float mods[26] = {
		7.3, 5.5, 7.3, 5.5, 1.3, 1.5, 15.0, 15.0, 5.5, 5.5, 7.3, 5.5, 15.0,
		2.0, 5.5, 5.5, 5.5, 7.3, 5.5, 2.0,	5.5,  5.5, 2.0, 5.5, 2.0, 2.0,
	};
	float mod =
		4.95;  // binary: _DAT_00195208 float 4.95 (link 0x185208, ghidra 0x195208, gdb x/fw=4.95; FLD [0x195208] @000de769 for out-of-range default)
	if ( (unsigned char) ( texture - 'A' ) < 26 )
		mod =
			0.9 *
			mods[(
				unsigned char) ( texture -
								 'A' )];  // binary: _DAT_00195230 double 0.9 (link 0x185230, ghidra 0x195230, FLD [0x195230] @000de5f4 + FMUL table[26] @0x195020, gdb x/gf=0.9; table CSWTCH.333 x/26fw 0x185020 matches mods[])
	return mod;
}

Vector CBaseEntity::FireBulletsNC( Vector vecSrc, Vector vecDirShooting, float flSpread, float flDistance,
								   int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker,
								   int shared_rand )
{
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;
	float x, y;

	if ( !pevAttacker )
		pevAttacker = pev;  // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	if ( IsPlayer() )
	{
		// use player's random seed.
		// get circular gaussian spread
		x = UTIL_SharedRandomFloat( shared_rand, -0.5, 0.5 ) + UTIL_SharedRandomFloat( shared_rand + 1, -0.5, 0.5 );
		y = UTIL_SharedRandomFloat( shared_rand + 2, -0.5, 0.5 ) + UTIL_SharedRandomFloat( shared_rand + 3, -0.5, 0.5 );
	}
	else
	{
		do
		{
			x = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
			y = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
		}
		while ( x * x + y * y > 1 );
	}

	Vector vecDir = vecDirShooting + x * flSpread * vecRight + y * flSpread * vecUp;

	if ( g_pGameRules->RoundState() == ROUNDSTATE_RUNNING )
	{
		CBaseEntity *pLastPlayerHit = NULL;
		int iLastHitgroup = -1;
		int iPenetrations = 0;
		float flMaterialMod = 0;
		float flDmg = MapBulletToDmg( iBulletType );
		Vector origSrc = vecSrc;
		edict_t *pentIgnore = ENT( pev );

		while ( 1 )
		{
			Vector vecEnd = vecSrc + vecDir * flDistance;
			TraceResult tr;

			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pentIgnore, &tr );

			if ( tr.flFraction >= 1.0 )
				break;

			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

			if ( iPenetrations )
			{
				// the bullet came out on the other side, see how much damage the material took away
				TraceResult reverseTr;

				UTIL_TraceLine( tr.vecEndPos, origSrc, dont_ignore_monsters, NULL, &reverseTr );

				if ( reverseTr.fAllSolid )
					break;

				flDmg = flDmg - ( reverseTr.vecEndPos - vecSrc ).Length() * flMaterialMod;

				if ( flDmg < 0 )
				{
					ALERT( at_aiconsole, "Bullet was stopped\n" );
					break;
				}

				ALERT( at_aiconsole, "Exiting surface  - damage %f\n", flDmg );

				if ( pLastPlayerHit == pEntity && tr.iHitgroup == iLastHitgroup )
				{
					vecSrc = vecSrc + vecDir + vecDir;
					ALERT( at_aiconsole, "Hit same hitgroup, skipping\n" );
					continue;
				}

				UTIL_GunshotDecalTrace( &tr, 0 );

				if ( !CBaseEntity::Instance( reverseTr.pHit )->IsPlayer() )
					UTIL_GunshotDecalTrace( &reverseTr, 0 );
			}

			ALERT( at_aiconsole, "penetration: %d, starting dmg: %f\n", iPenetrations, flDmg );

			BOOL bHitPlayer = pEntity->IsPlayer();

			pEntity->TraceAttack( pevAttacker, flDmg, vecDir, &tr, DMG_BULLET );

			if ( !bHitPlayer )
			{
				// what the bullet went through decides how much damage it loses on the way
				const char *pTextureName = TRACE_TEXTURE( tr.pHit ? tr.pHit : ENT( 0 ), vecSrc, vecEnd );
				char chTextureType = 'C';

				if ( pTextureName )
				{
					char szbuffer[64];

					if ( *pTextureName == '+' || *pTextureName == '-' )
						pTextureName += 2;

					if ( *pTextureName == '!' || *pTextureName == '{' || *pTextureName == '~' || *pTextureName == ' ' )
						pTextureName++;

					strcpy( szbuffer, pTextureName );
					szbuffer[CBTEXTURENAMEMAX - 1] = 0;

					chTextureType = TEXTURETYPE_Find( szbuffer );
				}

				flMaterialMod = MapMaterialModFromTexture( chTextureType );
			}
			else
			{
				pLastPlayerHit = pEntity;
				iLastHitgroup = tr.iHitgroup;

				flMaterialMod = MapMaterialModFromTexture( 'F' );
			}

			// the machine guns go through more
			if ( (unsigned int) ( iBulletType - 16 ) <= 2 )
				flMaterialMod = flMaterialMod * 0.65;

			vecSrc = tr.vecEndPos + vecDir + vecDir;

			// arms and hands are not in the way of the bullet
			if ( !bHitPlayer || (unsigned int) ( tr.iHitgroup - 4 ) > 1 )
				pentIgnore = tr.pHit;
			else
				pentIgnore = NULL;

			if ( ++iPenetrations > 5 )
				break;
		}

		ApplyMultiDamage( pev, pevAttacker );
	}

	return Vector( x * flSpread, y * flSpread, 0 );
}

void CBaseEntity ::TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if ( BloodColor() == DONT_BLEED )
		return;

	if ( flDamage == 0 )
		return;

	if ( !( bitsDamageType & ( DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB | DMG_MORTAR ) ) )
		return;

	// make blood decal on the wall!
	TraceResult Bloodtr;
	Vector vecTraceDir;
	float flNoise;
	int cCount;
	int i;

	/*
	if ( !IsAlive() )
	{
		// dealing with a dead monster. 
		if ( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return; 
		}
		else
		{
			pev->max_health--;
		}
	}
*/

	if ( flDamage < 10 )
	{
		flNoise = 0.1;
		cCount = 1;
	}
	else if ( flDamage < 25 )
	{
		flNoise = 0.2;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}

	for ( i = 0; i < cCount; i++ )
	{
		vecTraceDir =
			vecDir * -1;  // trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172, ignore_monsters, ENT( pev ), &Bloodtr );

		if ( Bloodtr.flFraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}

//=========================================================
//=========================================================
void CBaseMonster ::MakeDamageBloodDecal( int cCount, float flNoise, TraceResult *ptr, const Vector &vecDir )
{
	// make blood decal on the wall!
	TraceResult Bloodtr;
	Vector vecTraceDir;
	int i;

	if ( !IsAlive() )
	{
		// dealing with a dead monster.
		if ( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return;
		}
		else
		{
			pev->max_health--;
		}
	}

	for ( i = 0; i < cCount; i++ )
	{
		vecTraceDir = vecDir;

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * 172, ignore_monsters, ENT( pev ), &Bloodtr );

		/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SHOWLINE);
			WRITE_COORD( ptr->vecEndPos.x );
			WRITE_COORD( ptr->vecEndPos.y );
			WRITE_COORD( ptr->vecEndPos.z );
			
			WRITE_COORD( Bloodtr.vecEndPos.x );
			WRITE_COORD( Bloodtr.vecEndPos.y );
			WRITE_COORD( Bloodtr.vecEndPos.z );
		MESSAGE_END();
*/

		if ( Bloodtr.flFraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}

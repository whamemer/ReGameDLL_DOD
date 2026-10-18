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

#ifndef ACTIVITY_H
#define ACTIVITY_H

typedef enum
{
	ACT_RESET = 0,	// Set m_Activity to this invalid value to force a reset to m_IdealActivity
	ACT_IDLE = 1,
	ACT_GUARD,
	ACT_WALK,
	ACT_RUN,
	ACT_FLY,  // Fly (and flap if appropriate)
	ACT_SWIM,
	ACT_HOP,   // vertical jump
	ACT_LEAP,  // long forward jump
	ACT_FALL,
	ACT_LAND,
	ACT_STRAFE_LEFT,
	ACT_STRAFE_RIGHT,
	ACT_ROLL_LEFT,	 // tuck and roll, left
	ACT_ROLL_RIGHT,	 // tuck and roll, right
	ACT_TURN_LEFT,	 // turn quickly left (stationary)
	ACT_TURN_RIGHT,	 // turn quickly right (stationary)
	ACT_CROUCH,		 // the act of crouching down from a standing position
	ACT_CROUCHIDLE,	 // holding body in crouched position (loops)
	ACT_STAND,		 // the act of standing from a crouched position
	ACT_USE,
	ACT_SIGNAL1,
	ACT_SIGNAL2,
	ACT_SIGNAL3,
	ACT_TWITCH,
	ACT_COWER,
	ACT_SMALL_FLINCH,
	ACT_BIG_FLINCH,
	ACT_RANGE_ATTACK1,
	ACT_RANGE_ATTACK2,
	ACT_MELEE_ATTACK1,
	ACT_MELEE_ATTACK2,
	ACT_RELOAD,
	ACT_ARM,	 // pull out gun, for instance
	ACT_DISARM,	 // reholster gun
	ACT_EAT,	 // monster chowing on a large food item (loop)
	ACT_DIESIMPLE,
	ACT_DIEBACKWARD,
	ACT_DIEFORWARD,
	ACT_DIEVIOLENT,
	ACT_BARNACLE_HIT,	 // barnacle tongue hits a monster
	ACT_BARNACLE_PULL,	 // barnacle is lifting the monster ( loop )
	ACT_BARNACLE_CHOMP,	 // barnacle latches on to the monster
	ACT_BARNACLE_CHEW,	 // barnacle is holding the monster in its mouth ( loop )
	ACT_SLEEP,
	ACT_INSPECT_FLOOR,	// for active idles, look at something on or near the floor
	ACT_INSPECT_WALL,  // for active idles, look at something directly ahead of you ( doesn't HAVE to be a wall or on a wall )
	ACT_IDLE_ANGRY,		  // alternate idle animation in which the monster is clearly agitated. (loop)
	ACT_WALK_HURT,		  // limp  (loop)
	ACT_RUN_HURT,		  // limp  (loop)
	ACT_HOVER,			  // Idle while in flight
	ACT_GLIDE,			  // Fly (don't flap)
	ACT_FLY_LEFT,		  // Turn left in flight
	ACT_FLY_RIGHT,		  // Turn right in flight
	ACT_DETECT_SCENT,	  // this means the monster smells a scent carried by the air
	ACT_SNIFF,			  // this is the act of actually sniffing an item in front of the monster
	ACT_BITE,			  // some large monsters can eat small things in one bite. This plays one time, EAT loops.
	ACT_THREAT_DISPLAY,	  // without attacking, monster demonstrates that it is angry. (Yell, stick out chest, etc )
	ACT_FEAR_DISPLAY,	  // monster just saw something that it is afraid of
	ACT_EXCITED,		  // for some reason, monster is excited. Sees something he really likes to eat, or whatever.
	ACT_SPECIAL_ATTACK1,  // very monster specific special attacks.
	ACT_SPECIAL_ATTACK2,
	ACT_COMBAT_IDLE,  // agitated idle.
	ACT_WALK_SCARED,
	ACT_RUN_SCARED,
	ACT_VICTORY_DANCE,	// killed a player, do a victory dance.
	ACT_DIE_HEADSHOT,	// die, hit in head.
	ACT_DIE_CHESTSHOT,	// die, hit in chest
	ACT_DIE_GUTSHOT,	// die, hit in gut
	ACT_DIE_BACKSHOT,	// die, hit in back
	ACT_FLINCH_HEAD,
	ACT_FLINCH_CHEST,
	ACT_FLINCH_STOMACH,
	ACT_FLINCH_LEFTARM,
	ACT_FLINCH_RIGHTARM,
	ACT_FLINCH_LEFTLEG,
	ACT_FLINCH_RIGHTLEG,
} Activity;

// DOD activities: exact DWARF enum values from dod.so (gdb ptype Activity).
// binary: ACT_FLINCH_RIGHTLEG=76, then DOD_PRONE_GETUP=77, PRONE_GETDOWN=78, PRONE_IDLE=79,
//   PRONE_CRAWL=80, CROUCH_HEADSHOT=81, CROUCH_BACKWARD=82, PRONE1=83,
//   EXPLOSION_BACK=84, EXPLOSION_FORWARD=85, SIMPLE=86, BUTTSTOCK=87.
// Verified via GetDeathActivity disasm @000d8a70: MOV EAX,0x57(87 BUTTSTOCK default),
//   0x52(82 CROUCH_BACKWARD), 0x51(81 CROUCH_HEADSHOT), 0x56(86 SIMPLE), 0x53(83 PRONE1).
// DOD_GREEN is NOT an Activity in binary (gdb ptype DOD_GREEN = team enum BLUE,RED,GREEN,YELLOW,DOD_GREEN); removed.
static const Activity DOD_PRONE_GETUP = (Activity) 77;		  // binary: DWARF Activity[77]
static const Activity DOD_PRONE_GETDOWN = (Activity) 78;	  // binary: DWARF Activity[78]
static const Activity DOD_PRONE_IDLE = (Activity) 79;		  // binary: DWARF Activity[79]
static const Activity DOD_PRONE_CRAWL = (Activity) 80;		  // binary: DWARF Activity[80]
static const Activity DOD_CROUCH_HEADSHOT = (Activity) 81;	  // binary: disasm MOV 0x51 @000d8b43
static const Activity DOD_CROUCH_BACKWARD = (Activity) 82;	  // binary: disasm MOV 0x52 @000d8b2c
static const Activity DOD_PRONE1 = (Activity) 83;			  // binary: disasm MOV 0x53 @000d8bbaf, IsProne return
static const Activity DOD_EXPLOSION_BACK = (Activity) 84;	  // binary: DWARF Activity[84]
static const Activity DOD_EXPLOSION_FORWARD = (Activity) 85;  // binary: DWARF Activity[85]
static const Activity DOD_SIMPLE = (Activity) 86;			  // binary: disasm MOV 0x56 @000d8b5b
static const Activity DOD_BUTTSTOCK = (Activity) 87;  // binary: disasm MOV 0x57 @000d8af4, default deathActivity

typedef struct
{
	int type;
	const char *name;
} activity_map_t;

extern activity_map_t activity_map[];

#endif	//ACTIVITY_H

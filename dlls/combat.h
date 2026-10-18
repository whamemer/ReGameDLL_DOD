#pragma once
// DoD damage bits verified vs binary ~/rev/dod.so (link addr = ghidra - 0x10000).
// SDK occupied bits (halflife/dlls/cbase.h, dod/dlls/cbase.h): 0..10,12..14,15..23.
//   0x2000=DMG_ALWAYSGIB 0x8000=DMG_PARALYZE 0x10000=DMG_NERVEGAS 0x20000=DMG_POISON
//   0x800=bit11 free but unused by DoD. Bits 24..28 partially free, used by DoD.
// Free DoD bits get DMG_DOD_* names (no Valve strings in binary: strings dod.so has no DMG_).
// Addrs: TraceAttack 000d7460 (0x8000000 gate), FireBullets 000db1d0 (0x4000002),
//   TakeDamage 000d9ca0 (gib map), RadiusDamage 000da300 (0x10000000), RifleMelee 000dcf70.
#include "cbase.h"
#define DMG_DOD_NOBLOOD                                                                                                \
	( 1                                                                                                                \
	  << 27 )  // 0x8000000, no-blood gate in CBaseEntity::TraceAttack, binary: TEST bits & 0x8000000 @000d7460 (link 0xc7460)
#define DMG_DOD_PENETRATE                                                                                              \
	( 1 << 26 )	 // 0x4000000, rifle penetration mark in FireBullets, binary: OR 0x4000000 when iBulletType>=8 @000db1d0
#define DMG_DOD_GIB6                                                                                                   \
	( 1                                                                                                                \
	  << 25 )  // 0x2000000, free bit reused as gib style 6 in TakeDamage, binary: TEST 0x2000000 -> Killed(,6) @000d9ca0
#define DMG_DOD_SELFSPLASH                                                                                             \
	( 1                                                                                                                \
	  << 28 )  // 0x10000000, self-splash marker in RadiusDamage, binary: OR 0x10000000 when victim==attackerRef @000da300 (link 0xc9300)
// Gib-style map reuses SDK bits + one free bit (TakeDamage @000d9ca0 -> Killed styles 2..6, default 1):
//   style2: DMG_ALWAYSGIB (0x2000, SDK), style3: DMG_PARALYZE (0x8000), style4: DMG_NERVEGAS (0x10000),
//   style5: DMG_POISON (0x20000), style6: DMG_DOD_GIB6 (0x2000000, free).
#define DMG_DOD_GIBMASK_GETDEATH                                                                                       \
	( DMG_NERVEGAS | DMG_DOD_GIB6 )	 // 0x2010000 mask in GetDeathActivity @000d8a70 (TEST [ESI+0x364],0x2010000)
// Melee combos are CLUB + gib-style bit (RifleMeleeAttack @000dcf70):
#define DMG_DOD_MELEE_BAYONET ( DMG_CLUB | DMG_PARALYZE )  // 0x8080, weapon_id 10, plrDmgBayonet
#define DMG_DOD_MELEE_GARAND ( DMG_CLUB | DMG_NERVEGAS )   // 0x10080, weapon_id 5, plrDmgGarandButt
#define DMG_DOD_MELEE_K43 ( DMG_CLUB | DMG_DOD_GIB6 )	   // 0x2000080, weapon_id 0x18, plrDmgK43Butt
#define DMG_DOD_MELEE_ENFIELD ( DMG_CLUB | DMG_POISON )	   // 0x20080, weapon_id 0x19, plrDmgEnfieldButt
// Corpse-gib mask in DeadTakeDamage @000d9a50: 0xE1 = CRUSH|FALL|BLAST|CLUB (SDK DMG_GIB_CORPSE 0x2E1 drops SONIC 0x200).
#define DMG_DOD_GIB_CORPSE ( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_CLUB )	// 0xE1, binary: TEST bits & 0xE1
// TakeHealth mask @000d9a00: 0x3fbf = 0x3fff & ~DMG_BLAST (SDK 0x3fff drops BLAST 0x40).
#define DMG_DOD_TAKEHEALTH_MASK ( 0x3fff & ~DMG_BLAST )	 // 0x3fbf

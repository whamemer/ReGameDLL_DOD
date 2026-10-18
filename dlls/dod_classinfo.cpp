/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	DoD shared data tables, recovered from the original dod.so (.data / DWARF).
*	Source of record: original dod/dlls/dod_shared.cpp
*	Every class entry below was byte-verified against the binary:
*	  g_ClassInfo     @link 0x1d82e0 size 0x27d0 (28 * 364)  dod_shared.cpp:315
*	  g_ParaClassInfo @link 0x1daac0 size 0x1ddc (21 * 364)  dod_shared.cpp:290
*	  weaponnames     @link 0x1dd1c0 size 0x1000 (64 * 64)   dod_shared.cpp:19
*	NOTE: unlike the client copy, the SERVER weaponnames[] is 1-indexed by weapon id
*	([0] is the empty string); consumers index it as weaponnames[id] (dod_gamerules.cpp).
*
*	This file is owned by the weapons-core agent (see VERIFY_BRIEF).
*
****/

#include "dod_shared.h"

// .data:0x1dd1c0 - 1-indexed by weapon id (0 = none)
char weaponnames[64][64] =
{
	"",
	"weapon_amerknife",
	"weapon_gerknife",
	"weapon_colt",
	"weapon_luger",
	"weapon_garand",
	"weapon_scopedkar",
	"weapon_thompson",
	"weapon_mp44",
	"weapon_spring",
	"weapon_kar",
	"weapon_bar",
	"weapon_mp40",
	"weapon_handgrenade",
	"weapon_stickgrenade",
	"weapon_stickgrenade_ex",
	"weapon_handgrenade_ex",
	"weapon_mg42",
	"weapon_30cal",
	"weapon_spade",
	"weapon_m1carbine",
	"weapon_mg34",
	"weapon_greasegun",
	"weapon_fg42",
	"weapon_k43",
	"weapon_enfield",
	"weapon_sten",
	"weapon_bren",
	"weapon_webley",
	"weapon_bazooka",
	"weapon_pschreck",
	"weapon_piat",
	"weapon_mortar",
	"weapon_binoculars",
	"weapon_satchel"
};

// .data:0x1daac0 - 21 entries, indexed by PLAYERCLASS_* when allies/axis are para
DodClassInfo_t g_ParaClassInfo[21] =
{
	{ "", "", "", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
	{ "cls_garand", "#class_alliedpara_garand", "us-para", 1, 600, 42, 0, 2, 0, 0, 0, 2, 0, 2, "" },
	{ "cls_carbine", "#class_alliedpara_carbine", "us-para", 1, 600, 1048586, 0, 2, 0, 1, 1, 5, 0, 2, "" },
	{ "cls_tommy", "#class_alliedpara_thompson", "us-para", 1, 600, 138, 0, 1, 0, 2, 1, 0, 0, 2, "" },
	{ "cls_grease", "#class_alliedpara_grease", "us-para", 1, 600, 4194314, 0, 1, 0, 4, 1, 6, 0, 2, "" },
	{ "cls_spring", "#class_alliedpara_spring", "us-para", 1, 600, 522, 0, 0, 0, 3, 0, 1, 0, 2, "" },
	{ "cls_bar", "#class_alliedpara_bar", "us-para", 1, 600, 2058, 0, 1, 0, 5, 0, 4, 0, 2, "" },
	{ "cls_30cal", "#class_alliedpara_30cal", "us-para", 1, 600, 262154, 0, 0, 0, 6, 0, 3, 0, 2, "" },
	{ "cls_bazooka", "#class_alliedpara_bazooka", "us-para", 1, 600, 536870922, 0, 0, 0, 6, 0, 3, 0, 2, "" },
	{ "cls_mortar", "#class_alliedpara_mortar", "us-para", 1, 600, 10, 1, 0, 0, 6, 0, 3, 0, 2, "" },
	{ "cls_k98", "#class_axispara_kar98", "axis-para", 2, 600, 1044, 0, 2, 0, 0, 0, 2, 0, 1, "" },
	{ "cls_k43", "#class_axispara_k43", "axis-para", 2, 600, 16777236, 0, 2, 0, 1, 0, 5, 0, 2, "" },
	{ "cls_mp40", "#class_axispara_mp40", "axis-para", 2, 600, 4116, 0, 1, 0, 3, 0, 0, 0, 3, "" },
	{ "cls_mp44", "#class_axispara_mp44", "axis-para", 2, 600, 276, 0, 1, 0, 4, 0, 4, 0, 4, "" },
	{ "cls_k98s", "#class_axispara_scopedkar", "axis-para", 2, 600, 84, 0, 0, 0, 2, 0, 1, 0, 3, "" },
	{ "cls_fg42", "#class_axispara_fg42bipod", "axis-para", 2, 600, 8388628, 0, 1, 0, 5, 0, 6, 0, 6, "" },
	{ "cls_fg42_s", "#class_axispara_fg42scope", "axis-para", 2, 600, 8388628, 0, 1, 0, 5, 0, 6, 0, 6, "" },
	{ "cls_mg34", "#class_axispara_mg34", "axis-para", 2, 600, 2097172, 0, 0, 0, 6, 0, 3, 0, 7, "" },
	{ "cls_mg42", "#class_axispara_mg42", "axis-para", 2, 600, 131092, 0, 0, 0, 6, 0, 3, 0, 7, "" },
	{ "cls_pschreck", "#class_axispara_pschreck", "axis-para", 2, 600, 1073741844, 0, 0, 0, 6, 0, 3, 0, 7, "" },
	{ "cls_germortar", "#class_axispara_mortar", "axis-para", 2, 600, 20, 1, 0, 0, 6, 0, 3, 0, 7, "" }
};

// .data:0x1d82e0 - 28 entries, indexed by PLAYERCLASS_* (default, non-para)
DodClassInfo_t g_ClassInfo[28] =
{
	{ "", "", "", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
	{ "cls_garand", "#class_allied_garand", "us-inf", 1, 600, 42, 0, 2, 0, 0, 2, 0, 0, 3, "" },
	{ "cls_carbine", "#class_allied_carbine", "us-inf", 1, 600, 1048586, 0, 2, 0, 0, 0, 0, 0, 3, "" },
	{ "cls_tommy", "#class_allied_thompson", "us-inf", 1, 600, 138, 0, 1, 0, 1, 0, 1, 1, 4, "" },
	{ "cls_grease", "#class_allied_grease", "us-inf", 1, 600, 4194314, 0, 1, 0, 1, 2, 1, 1, 4, "" },
	{ "cls_spring", "#class_allied_sniper", "us-inf", 1, 600, 522, 0, 0, 0, 4, 1, 4, 4, 7, "" },
	{ "cls_bar", "#class_allied_heavy", "us-inf", 1, 600, 2058, 0, 1, 0, 2, 2, 2, 2, 5, "" },
	{ "cls_30cal", "#class_allied_mg", "us-inf", 1, 600, 262154, 0, 0, 0, 3, 1, 3, 3, 6, "" },
	{ "cls_bazooka", "#class_allied_bazooka", "us-inf", 1, 600, 536870922, 0, 0, 0, 3, 1, 3, 3, 6, "" },
	{ "cls_mortar", "#class_allied_mortar", "us-inf", 1, 600, 10, 1, 0, 0, 3, 1, 3, 3, 6, "" },
	{ "cls_k98", "#class_axis_kar98", "axis-inf", 2, 600, 525328, 0, 2, 0, 0, 0, 0, 0, 2, "" },
	{ "cls_k43", "#class_axis_k43", "axis-inf", 2, 600, 17301520, 0, 2, 0, 1, 0, 1, 1, 3, "" },
	{ "cls_mp40", "#class_axis_mp40", "axis-inf", 2, 600, 528400, 0, 1, 0, 2, 1, 2, 2, 4, "" },
	{ "cls_mp44", "#class_axis_mp44", "axis-inf", 2, 600, 524560, 0, 1, 0, 3, 0, 3, 3, 5, "" },
	{ "cls_k98s", "#class_axis_sniper", "axis-inf", 2, 600, 524368, 0, 0, 0, 5, 1, 5, 5, 7, "" },
	{ "cls_fg42", "#class_axispara_fg42bipod", "axis-para", 2, 600, 8388628, 0, 1, 0, 5, 0, 6, 0, 7, "" },
	{ "cls_fg42_s", "#class_axispara_fg42scope", "axis-para", 2, 600, 8388628, 0, 1, 0, 5, 0, 6, 0, 7, "" },
	{ "cls_mg34", "#class_axis_mg34", "axis-inf", 2, 600, 2621456, 0, 0, 0, 4, 0, 4, 4, 6, "" },
	{ "cls_mg42", "#class_axis_mg42", "axis-inf", 2, 600, 655376, 0, 0, 0, 4, 0, 4, 4, 6, "" },
	{ "cls_pschreck", "#class_axis_pschreck", "axis-inf", 2, 600, 1074266128, 0, 0, 0, 4, 0, 4, 4, 6, "" },
	{ "cls_germortar", "#class_axis_mortar", "axis-inf", 2, 600, 524304, 1, 0, 0, 4, 0, 4, 4, 6, "" },
	{ "cls_enfield", "#class_brit_light", "brit-inf", 1, 600, 301989890, 0, 2, 0, 0, 0, 0, 0, 2, "" },
	{ "cls_sten", "#class_brit_medium", "brit-inf", 1, 600, 335544322, 0, 1, 0, 1, 0, 1, 0, 3, "" },
	{ "cls_enfields", "#class_brit_sniper", "brit-inf", 1, 600, 301989890, 0, 0, 0, 3, 1, 3, 0, 5, "" },
	{ "cls_bren", "#class_brit_heavy", "brit-inf", 1, 600, 402653186, 0, 1, 0, 2, 0, 2, 0, 4, "" },
	{ "cls_piat", "#class_brit_piat", "brit-inf", 1, 600, -1879048190, 0, 0, 0, 2, 0, 2, 0, 4, "" },
	{ "cls_britmortar", "#class_brit_mortar", "brit-inf", 1, 600, 268435458, 1, 1, 0, 2, 0, 2, 0, 4, "" },
	{ "cls_random", "", "", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" }
};

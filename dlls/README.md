<a id="english"></a>
**English** | [Русский](#russian)

# Day of Defeat server DLL

This document describes the structure of the Day of Defeat 1.3 server library (`dod.dll` / `dod.so`), its console variables,
the mechanics that exist only in Day of Defeat, the entities that the maps of the game use, and what differs from
[hlsdk-portable](https://github.com/FWGS/hlsdk-portable), the SDK this code base is built on. It is written for people who run a
server, make maps, or want to read, build or modify the library.

The library targets **Xash3D FWGS**. It replaces the server library of the original game and works with the original client
library as well as with [dod13-client](https://github.com/whamemer/dod13-client).

* [Layout](#layout)
* [Build options](#build-options)
* [Fixes of the bugs of the original game](#fixes-of-the-bugs-of-the-original-game)
* [Server cvars](#server-cvars)
* [Mechanics that exist only in Day of Defeat](#mechanics-that-exist-only-in-day-of-defeat)
* [Entities of the maps](#entities-of-the-maps)
* [Differences from hlsdk-portable](#differences-from-hlsdk-portable)

## Layout

| Path | What is there |
|------|---------------|
| `dlls/h_export.cpp`, `client.cpp`, `cbase.cpp` | The functions the engine looks up by name (`GiveFnptrsToDll`) and the tables it gets (`GetEntityAPI`): connect, spawn, think, touch, commands of the client, the keyvalues of the entities. |
| `dlls/gamerules.cpp`, `dod_gamerules.cpp`, `.h` | `CGameRules` and `CDoDTeamPlay`, the rules of the game: rounds, teams, classes, waves, scores, clan matches, the map change; `info_doddetect` and `dod_preround` are here too. `CSPDoDRules` (the rules of one player) is in `gamerules.h`. |
| `dlls/player.cpp`, `observer.cpp`, `spectator.cpp` | `CBasePlayer`: spawn, stamina, stances, objects, hints, damage, death; the observer and spectator modes. |
| `dlls/triggers.cpp` | `CControlPoint`, `CControlPointMaster`, `CAreaCapture` (`dod_capture_area`), `CObjectCapture` (`dod_object_goal`), `dod_point_relay`, `dod_round_timer`, `dod_location`, `dod_trigger_sandbag`, `dod_camera` and the triggers of Half-Life. |
| `dlls/objectives.cpp` | `CObject` (`dod_object`), `CScoreEvent` (`dod_score_ent`) and `CParticleShooter`. |
| `dlls/weapons.cpp`, `weapons.h`, `dlls/wpn_shared/` | The weapon base classes and one class per weapon. The files of `wpn_shared/` are the code that the client runs for the prediction too. |
| `dlls/dod_shared.cpp`, `cl_dll/dod_shared.h` | The tables shared with the client: weapon ids and names, class limits, voice commands, player models, the constants of the stances. |
| `dlls/maprules.cpp` | The `game_*` entities (`game_score`, `game_text`, `game_team_master`, ...). |
| `dlls/mapmarker.cpp`, `hintmessage.cpp`, `dod_stats.cpp` | Map markers, the hint queue, the statistics of the players. |
| `dlls/game.cpp`, `game.h` | The cvars: every cvar is declared and registered here. |
| `dlls/world.cpp`, `util.cpp`, `subs.cpp`, `doors.cpp`, `plats.cpp`, ... | The world, the helpers and the entities of Half-Life that the maps use. |
| `dlls/monsters.cpp`, `bot_*.cpp`, `h_ai.cpp`, ... | The monsters of Half-Life and two bot classes. They are in the original library, no map of the game uses them. |
| `pm_shared/` | Player movement, shared with the client prediction. |
| `game_shared/` | Voice and a few helpers shared with the client. |
| `public/`, `common/`, `engine/`, `external/` | The headers of the engine interface and third-party code of the SDK. |
| `mod_options.txt` | The options of the build, see below. |

`pm_shared/`, `game_shared/`, `common/`, `engine/`, `public/` and `external/` are the same files as in dod13-client: they are
shared with the client prediction and must stay identical.

## Build options

`mod_options.txt` in the root of the repository holds the options of the build, one per line: `NAME=VALUE # description`.
Both waf (`wscript`) and CMake read it. `ON` defines the macro `NAME` for the whole library, `OFF` defines nothing, any other
value becomes a variable of the build.

| Option | Value | Meaning |
|--------|-------|---------|
| `GAMEDIR` | `dod` | Directory of the game; the install target puts the library into `<GAMEDIR>/<SERVER_INSTALL_DIR>`. |
| `SERVER_INSTALL_DIR` | `dlls` | Where the library goes (must match `liblist.gam` / `gameinfo.txt`). |
| `SERVER_LIBRARY_NAME` | `dod` | Name of the library; the platform adds the suffix of the library naming scheme (`_amd64`, `_arm64`). |
| `FIX_ENTITY_HASH_OVERFLOW` | `ON` | A fix of a bug of the original game, see [below](#fixes-of-the-bugs-of-the-original-game). |
| `FIX_CAPTURE_AREA_DIVIDE_BY_ZERO` | `ON` | A fix of a bug of the original game, see [below](#fixes-of-the-bugs-of-the-original-game). |

## Fixes of the bugs of the original game

The original library has bugs that this code base reproduces on purpose, because the restoration follows the binary. A fix of
such a bug is **optional**: it sits behind a macro that is switched in `mod_options.txt` (see [Build options](#build-options)).
With the option `OFF` the library behaves exactly like the original. Both fixes below are `ON` by default.

| Option | The bug of the original | The fix |
|--------|-------------------------|---------|
| `FIX_ENTITY_HASH_OVERFLOW` | The hash of a classname (`CaseInsensitiveHash`) takes `% (size + 1)`, but the table of entities has only `size` elements, so an entity can be added one element behind the table. | The table gets one more element (`cbase.cpp`, `AddEntityHashValue`). |
| `FIX_CAPTURE_AREA_DIVIDE_BY_ZERO` | The progress bar of a capture area divides by the number of players of the capturing team. When that team has left the area while the other one still stands in it, the number is 0 and the server dies with a division by zero. | The update of the progress bar is skipped when nobody of the capturing team is in the area (`CAreaCapture::Think`, `ReSendSwipeInfo`). |

### Adding a fix

1. Add a line to `mod_options.txt`: `NAME=ON # what the fix does`.
2. Wrap the changed code in `#if defined( NAME )` ... `#else` (the original code) ... `#endif`. A switched off option defines
   nothing.
3. Run `waf configure` again (CMake reads the file at configure time too). The build log lists the option and its state.

## Server cvars

All values are the defaults at registration. `S` = `FCVAR_SERVER`: the value is sent to the clients and shown in the server
info. A cvar without a flag is an ordinary server variable. Set them in `server.cfg` or `listenserver.cfg`.

### Game and rounds

| Cvar | Default | Flags | Effect |
|------|---------|-------|--------|
| `mp_timelimit` | 0 | S | Time limit of a map in minutes; 0 is no limit. When it runs out the intermission starts and the next map is loaded. |
| `mp_chattime` | 10 | | Seconds of the intermission (the scoreboard) before the next map; the game forces the value into 10...120. A player can end it earlier by pressing a button once the time has passed. |
| `mp_friendlyfire` | 0 | S | 1 allows damage to the players of your own team; the damage to yourself always counts. |
| `mp_tkpenalty` | 6 | | The limit of the counter of team kills, after which the player is kicked automatically (`#game_kick_tk`); 0 and less turns the kick off. As in the original every team kill adds 2 to the counter, so the default 6 kicks on the third team kill in a row. A kill of an enemy resets the counter. The host of a listen server is never kicked. |
| `mp_teamlimit` | 2 | | How many more players a team may have than the other one. A player who wants to join a team that is already ahead by this many is refused (`#game_uneven_teams`). 0 turns the limit off. |
| `mp_allowspectators` | 1 | | 0 forbids joining the spectators. |
| `mp_weaponstay` | 0 | | 1 makes a weapon that lies in the world respawn at once, except the weapons that are limited in the world. Otherwise the weapons come back after 20 seconds. |
| `mp_spawnbazookas` | 0 | | 1 lets the rocket launchers (bazooka, Pschreck, PIAT) that lie on the map spawn; without it they are removed. |
| `mp_log_scores` | 0 | | More than 0 writes the team scores into the log of the server (`Team Scores: Allies: .., Axis: ..`) every `mp_log_scores` seconds (1...600; a value between 0 and 1 means every second). |
| `mp_log_scores_delay` | 60 | | Seconds from the start of the map to the first entry of `mp_log_scores` (1...600; the later entries follow `mp_log_scores`). |
| `mp_deathmsg` | 1 | | 0 hides the death notices. |
| `mp_chatmsg` | 1 | | 0 turns the chat off for everyone (`say` and `say_team` are ignored). |
| `mp_defaultteam` | 0 | | Set by the map: the worldspawn flag 4 makes it 1. Not meant to be changed by hand. |
| `mp_falldamage` | 0 | S | Registered as in the original; not read by the game code. |
| `mp_flashlight` | 0 | S | Registered as in the original; not read by the game code (the game has no flashlight). |
| `mp_teamplay` | 0 | | Copied into the engine's `teamplay` global every frame; the rules of the game do not read it. |
| `mp_teamoverride` | 1 | | Registered as in the original; not read by the game code. |
| `mp_teamlist` | `hgrunt;scientist` | | Names of the teams of Half-Life. Registered as in the original; only a comment of `game_team_master` mentions it. |
| `decalfrequency` | 30 | | Seconds between two sprays of the player. |
| `displaysoundlist` | 0 | | Registered as in the original; not read by the game code. |
| `dev_serverhitbox` | 0 | | Registered as in the original; not read by the game code. |

### Spectators

| Cvar | Default | Flags | Effect |
|------|---------|-------|--------|
| `mp_forcecamera` | 1 | S | Not 0: a dead player of a team can watch only his team mates, and the free roaming is replaced by the locked chase view. The spectators are not restricted. |
| `mp_forcechasecam` | 1 | S | The game handles it together with `mp_forcecamera`: either of the two gives the same restriction. |
| `mp_fadetoblack` | 0 | S | Not 0: the strictest level, a dead player sees only through the eyes of a team mate. In an active clan match the screen of the dead player also fades to black. |
| `mp_clan_showcutscenes` | 1 | | 0 turns the cinematic cameras of the map (`dod_camera`) off. In a clan match they are always off. |
| `mp_nummapmarkers` | 1 | | How many map markers a player may have at the same moment; when the limit is reached the oldest one of his markers goes away. |
| `mp_markerstaytime` | 30 | | Seconds a map marker stays on the map; at least 2. |

### Classes and weapon limits

The limits count the players who have the class or have chosen it for the next spawn. `-1` is unlimited, `0` closes the
class. A class that has reached its limit is shown as unavailable in the menu, and the join is refused.

| Cvar | Default | Flags | Class (id) |
|------|---------|-------|------------|
| `mp_limitalliesgarand` | -1 | S | Garand rifleman (1) |
| `mp_limitalliescarbine` | -1 | S | M1 Carbine (2) |
| `mp_limitalliesthompson` | -1 | S | Thompson (3) |
| `mp_limitalliesgreasegun` | -1 | S | Grease gun (4) |
| `mp_limitalliesspring` | -1 | S | Springfield sniper (5) |
| `mp_limitalliesbar` | -1 | S | BAR (6) |
| `mp_limitallies30cal` | -1 | S | .30 cal (7) |
| `mp_limitalliesbazooka` | -1 | S | Bazooka (8) |
| `mp_limitalliesmortar` | -1 | S | Mortar (9), see below |
| `mp_limitaxiskar` | -1 | S | Kar98k (10) |
| `mp_limitaxisk43` | -1 | S | K43 (11) |
| `mp_limitaxismp40` | -1 | S | MP40 (12) |
| `mp_limitaxismp44` | -1 | S | MP44 (13) |
| `mp_limitaxisscopedkar` | -1 | S | Scoped Kar98k (14) |
| `mp_limitaxisfg42` | -1 | S | FG42 (15) |
| `mp_limitaxisfg42s` | -1 | S | Scoped FG42 (16) |
| `mp_limitaxismg34` | -1 | S | MG34 (17) |
| `mp_limitaxismg42` | -1 | S | MG42 (18) |
| `mp_limitaxispschreck` | -1 | S | Pschreck (19) |
| `mp_limitaxismortar` | -1 | S | Mortar (20), see below |
| `mp_limitbritlight` | -1 | S | British light infantry (21) |
| `mp_limitbritassault` | -1 | S | British assault infantry (22) |
| `mp_limitbritsniper` | -1 | S | British sniper (23) |
| `mp_limitbritmg` | -1 | S | British machine gunner (24) |
| `mp_limitbritpiat` | -1 | S | PIAT (25) |
| `mp_limitbritmortar` | -1 | S | British mortar (26), see below |
| `mp_combinemglimits` | 0 | | 1 makes MG34 and MG42 (17 and 18) share one limit: the sum of the two limits against the sum of the two classes. |
| `mp_alliesclasses` | -1 | | A bit mask of the classes the Allies may choose. Bit 0 is the first class of the team (id 1 for the Americans, 21 for the British). -1 allows all. |
| `mp_axisclasses` | -1 | | The same for the Axis, bit 0 is the class 10. |

The mortar classes (9, 20 and 26) cannot be chosen, as in the original: the tables and the limits exist, the game refuses the
join. Class 27 is the random class; a clan match refuses it.

### Clan matches

A clan match is switched on by `mp_clan_match 1`. The warm-up, the countdown and the ready signal are described in
[Clan matches](#clan-matches-1).

| Cvar | Default | Flags | Effect |
|------|---------|-------|--------|
| `mp_clan_match` | 0 | S | 1 starts the clan match mode (warm-up); 0 leaves it. |
| `mp_clan_restartround` | 0 | S | Set to 1 to restart the round after the countdown of `mp_clan_timer`; the game resets it to 0. |
| `mp_clan_readyrestart` | 0 | S | Set to 1 to wait for both teams to say the ready signal; the game resets it to 0. |
| `mp_clan_ready_signal` | `ready` | S | The word a team says in the chat to be ready. |
| `mp_clan_match_warmup` | 0 | S | Set to 1 to go back to the warm-up; the game resets it to 0. |
| `mp_clan_timer` | 20 | S | Seconds of the countdown before a restart. |
| `mp_clan_respawntime` | 0 | S | When more than 0 it is the time of a reinforcement wave in a clan match, instead of the time that depends on the number of players. |
| `mp_clan_scoring` | 0 | S | 1 turns the scoring of the clan match on: the points for the control points come from the strings below, once per `mp_clan_scoring_delay`. |
| `mp_clan_scoring_values_allies` | `111111` | S | One digit per control point, in the order of the points in the map (the order of the entities): the points the Allies get for owning it. |
| `mp_clan_scoring_values_axis` | `111111` | S | The same for the Axis. A point that has no digit gives 0 and the server writes a warning. |
| `mp_clan_scoring_delay` | 60 | S | Seconds between two gives of the points in the clan scoring (30 if the cvar is missing). |
| `mp_clan_scoring_bonus_allies` | -1 | S | 0 or more (at most 1000): the points the Allies get for a round won through a `dod_score_ent`, instead of the points of the entity. -1 keeps the points of the entity. |
| `mp_clan_scoring_bonus_axis` | -1 | S | The same for the Axis. |

### Half-Life skill cvars

The `sk_*` cvars (`sk_agrunt_health1`, `sk_plr_9mm_bullet1`, `sk_player_head1`, ...; the last digit is the skill level 1...3)
are the skill system of Half-Life. They are registered as in the original and read into `gSkillData` when the game starts,
but the damage of the weapons of Day of Defeat comes from the code of the weapons, and the damage of the three rocket launchers
is fixed to 100. They matter for the Half-Life monsters, which no map uses.

### Cvars of the engine that the game reads

`sv_gravity` (the gravity of the world), `sv_aim` and `mp_footsteps` (the sounds of footsteps).

## Mechanics that exist only in Day of Defeat

### Rounds

The state of the round is kept in `CDoDTeamPlay::m_RoundState`:

| State | Value | Meaning |
|-------|-------|---------|
| `ROUNDSTATE_PREROUND` | 0 | The players are frozen and wait for the start. The `dod_preround` triggers of the map fire here, the camera may play. |
| `ROUNDSTATE_RUNNING` | 1 | The round is on: objectives work, the points and the waves run. |
| `ROUNDSTATE_ALLIES_WON` | 3 | The round is over, the Allies won. |
| `ROUNDSTATE_AXIS_WON` | 4 | The round is over, the Axis won. |
| `ROUNDSTATE_DRAW` | 5 | The round is over, a draw. |

A round ends when a `dod_score_ent` fires (a team captured everything, a bomb exploded: the map decides what fires it) or when
the built-in timer of the map runs out (`detect_use_timer`, below). The time limit of the map (`mp_timelimit`) ends the map, not
the round. After a short pause (`TeamWon`) the entities of the
map are reset (`ResetEnts`: doors, buttons, breakables, objects, control points, ... come back as they were at the start of the
map) and the next round starts. `info_doddetect` sets the rules of the map (see below).

A round in which one of the teams has no players is "bogus": the points do not count.

### Teams, classes and weapons

Teams: Allies (1), Axis (2), spectators (3). The Allies are Americans or British, `detect_allies_country` of the map
says which. Paratroopers (`detect_allies_paras`, `detect_axis_paras`) have their own classes; while the teams are not both infinite
they may join only in the first 30 seconds of a round. The weapons are taken from the class: `Classify()` of a weapon says whether it is a pistol, a rifle, a
scoped rifle, a submachine gun, a machine gun, a rocket launcher or a grenade (`CLASS_*` in `dod_shared.h`), and the game
rules allow one weapon of every group. `mp_weaponstay`, `mp_spawnbazookas` and the class limits are described above.

### Reinforcement waves and infinite lives

If a team has infinite lives (`detect_allies_infinite` / `detect_axis_infinite` of `info_doddetect`, on by default) its dead
players do not respawn at once: all of them come back together in a wave. The time of a wave is the number of players of the
team: 6 seconds for up to 2 players, then 8 (3...5), 10 (6...7), 11 (8...9), 12 (10...11), 13 (12...13) and 14 seconds for 14
and more, multiplied by `detect_allies_respawnfactor` / `detect_axis_respawnfactor` of the map. In a clan match
`mp_clan_respawntime` replaces the time. A team without infinite lives does not respawn: a player who dies waits for the end of
the round.

### Control points

A control point (`dod_control_point`) has an owner: none, Allies or Axis. A team takes it in one of two ways:
* **By touch.** A player of that team walks into the point and takes it at once (`point_can_allies_touch`,
  `point_can_axis_touch` can forbid it for a team; after a change the point waits one second before it can change again).
* **By a capture area.** `dod_capture_area` targets the point; the players of a team that stand inside it capture it in
  `area_time_to_cap` seconds, if at least `area_*_numcap` of them are there.

When a point changes its owner it changes its model and bodygroup, plays the sound of the team, fires the target of the team
(`point_allies_target`, `point_axis_target`, `point_reset_target`), gives `point_points_for_cap` to the capturer and
`point_team_points` to his team, and tells the clients (the icons of the HUD, the line of the chat `point_win_string`).

The points are collected by a **control point master** (`dod_control_point_master`): it finds the points with the same group
name, numbers them (`point_index`; if the indices are missing or repeat, by the distance from the first Allied spawn point),
tells the clients about them (`InitObjDisplay`) and every `point_give_delay_time` seconds gives each team the sum of
`point_pointvalue` of its points as team score (in a clan match with `mp_clan_scoring` it uses the strings of the cvars). The
team that owns all the points of the master wins the round: the master fires `allies_capture_target` or `axis_capture_target`.

### Capture areas

A capture area counts the living players of both teams inside. A team can start a capture when the other team has nobody in
the area, the team may capture (`area_allies_cancap`, `area_axis_cancap`), it does not own the point already, and it has
enough players (`area_allies_numcap`, `area_axis_numcap`). If the area needs an object (`area_object_group`) only the players
who carry an object of that group count. The capture runs for `area_time_to_cap` seconds, then it ends; if a player of the
other team steps in, or the capturing team has too few players left, it breaks. The area fires `area_*_startcap`,
`area_*_breakcap` and `area_*_endcap` and sends the clients the progress bar and the number of the players inside.

With the flag 1 of the area and not both teams infinite, a team that has fewer players than needed but has **all its living
players** inside still captures, more slowly: the speed is the number of players divided by the number needed.

### Objects and object goals

An object (`dod_object`, the dynamite of `dod_escape`) lies on the ground until a player of the **enemy** team touches it:
`object_owner` is the team that owns the object, the other team picks it up. A player carries one object, his speed
is multiplied by `object_carryspeed`, the HUD and a sprite over his head show it (`object_hud_sprite`,
`object_player_sprite`). The object is dropped when the player dies, disconnects or types `dropobject`; it lies where it
fell and goes back to its start after `object_resettime` seconds (at most 600), or at once when a player of the owner team touches it
(`object_resetontouch`).

An object goal (`dod_object_goal`, a brush) takes the object: a carrier whose object is of the group of the goal
(`capobj_group`) steps into it and gets `object_cappoints` of the object. While other objects of the group are still
under way (and the flag 1 is not set) the game says how many are left, the goal fires its own `target` and the team mates
of the carrier get `object_otherpoints`. When the last object is delivered, or at once with the flag 1 ("Dont Multicap": one
object is enough), the goal fires `capobj_donetarget` and the `object_donetarget` of the object. With the flag 2 the goal works
once per round.

### Score entities

A `dod_score_ent` ends the round for its team when it is fired: it gives `score_points` (or the clan bonus), shows its
message, resets the items and the players after `score_resetitemtime` / `score_resetplayerstime` seconds, may change the map
(`score_nextmap` after `score_nextmapdelay`) and may end the game (`noise2` = 1).

### Players

* **Stamina.** A bar of 0...100 (`pev->fuser4`). Running (the run key with a direction key, speed over 100) takes 30 per second.
  It comes back by 6 per tenth of a second when standing, 1 when moving, and 5 when crouching on the ground. Firing takes
  1 or 2 points (more for the machine guns) unless the player crouches or lies. A low bar slows the player down and shows the
  hint `#Hint_low_stamina`.
* **Prone.** The `sprone` command (the key of the game) lies down and gets up. It is refused with a deployed sandbag or mortar,
  while firing, on a ladder, in water deeper than the knees, while ducking, and for a second after the last change.
* **Bipod and sandbags.** The weapons with a bipod can be deployed. A player who lies deploys the bipod; inside a
  `dod_trigger_sandbag` (`sandbag_range` degrees in either direction) the weapon can be deployed on a sandbag. The
  state is in `pev->iuser3` (`STANCE_*`) and `pev->vuser1` (`SANDBAG_*`).
* **Machine gun heat.** A deployed machine gun gets hot with every shot (3 points) and cools down; at 99 it stops with the hint
  `#Hint_mg_overheat`.
* **Hints.** `CBasePlayer::HintMessage` shows a hint once per kind (the history is a mask, `HINT_*`), unless the player turned
  hints off.
* **Team kills.** See `mp_tkpenalty`.
* **Map markers.** A player can put a marker on the map (`dod_mapmarker`), the team sees it.
* **Voice commands.** The menu of voice commands (attack, hold, fall back, ...) plays the sound of the class and shows an icon.
* **Statistics.** `dod_stats` counts kills, deaths, hits and shots of every player.

### Clan matches

With `mp_clan_match 1` the game goes to the clan match mode and shows the rules of the warm-up. `mp_clan_readyrestart 1` asks the teams to say
`mp_clan_ready_signal` in the chat; when both did, the countdown of `mp_clan_timer` seconds starts and the match begins with
a restart. `mp_clan_restartround 1` restarts at once after the countdown, `mp_clan_match_warmup 1` goes back to the warm-up.
Cutscenes and the random class are off in a clan match. The scoring is described in the table above.

## Entities of the maps

The keys below are the keyvalues of the entity in the map (the ones the game reads). The editor definitions are in `dod.fgd`.

### `info_doddetect`: rules of the map

| Key | Default | Meaning |
|-----|---------|---------|
| `detect_allies_infinite`, `detect_axis_infinite` | 1 | 1 gives the team infinite respawns (waves), 2 turns them off. |
| `detect_allies_paras`, `detect_axis_paras` | 0 | 1 makes the team paratroopers. |
| `detect_allies_country` | 0 | 0 the Americans, 1 the British. |
| `detect_allies_respawnfactor`, `detect_axis_respawnfactor` | 1.0 | Multiplies the time of a wave. |
| `detect_points_axiseliminated`, `detect_points_allieseliminated` | 5 | Points for wiping out the team. Read and stored, nothing uses them, as in the original. |
| `detect_target_axiseliminated`, `detect_target_allieseliminated` | | Fired when the team is eliminated. Read and stored, nothing uses them, as in the original. |
| `detect_use_timer`, `detect_target_timerexpired`, `detect_timer_team`, `detect_points_timerexpired` | 0 | The built-in timer of the map (`para_roundtimer`): when it runs out the round ends in favour of `detect_timer_team` (a draw for 0), and that team gets `detect_points_timerexpired`. |
| `detect_wind_velocity_x`, `detect_wind_velocity_y`, `detect_weather_type` | 0 | Wind and weather (0 none, 1 rain, 2 snow); read by the client. |

### `dod_control_point`

| Key | Default | Meaning |
|-----|---------|---------|
| `point_name` | `a control point` | Name in the messages. |
| `point_default_owner` | 0 | 0 nobody, 1 Allies, 2 Axis. |
| `point_can_allies_touch`, `point_can_axis_touch` | 0 | 1 forbids the team to take the point by touch. |
| `point_pointvalue` | 1 | Team score per give while the team owns the point. |
| `point_points_for_cap` | 1 | Score of the player who takes it. |
| `point_team_points` | 0 | Team score for taking it. |
| `point_allies_model`, `point_axis_model`, `point_reset_model` (+ `_bodygroup`) | | Models for every owner. |
| `point_allies_capsound`, `point_axis_capsound`, `point_resetsound` | | Sounds. |
| `point_allies_target`, `point_axis_target`, `point_reset_target` | | Fired on a change. |
| `point_hud_icon_allies`, `point_hud_icon_axis`, `point_hud_icon_neutral` | | Icon numbers of the HUD (flag, artillery, bridge, radio, documents, truck, custom ones). |
| `point_win_string`, `point_allies_win_string`, `point_axis_win_string` | | The text sent to the clients on a capture; empty sends nothing. |
| `point_group`, `point_index` | | The master that collects the point, and the number of the point. |
| `master` | | The point works only while this master is triggered. |
| flag 1 | | Do not show the point on the HUD. |

### `dod_control_point_master`

`point_give_delay_time` (20), `allies_capture_target`, `axis_capture_target`, `cpm_group`, `master`.

### `dod_capture_area` (a brush)

`area_allies_cancap`, `area_axis_cancap` (1), `area_allies_numcap`, `area_axis_numcap` (1), `area_time_to_cap` (5),
`area_allies_startcap`, `area_allies_breakcap`, `area_allies_endcap`, `area_axis_startcap`, `area_axis_breakcap`,
`area_axis_endcap`, `area_hud_sprite`, `area_object_group`, `master`; `target` is the control point it captures.

### `dod_object`

`object_name`, `object_owner` (0 both, 1 Allies, 2 Axis: the owner team; the other one picks the object up), `object_group`,
`object_takesound`, `object_capsound`, `object_dropsound`, `object_returnsound`, `object_carryspeed` (0.5),
`object_resetontouch` (1), `object_resettime` (60), `object_cappoints` (10), `object_otherpoints` (5), `object_donetarget`,
`object_player_sprite`, `object_hud_sprite`, `master`; the flag 1 reports the pickups and drops. A dod_object also fires its
`target` when it is picked up.

### `dod_object_goal` (a brush)

`capobj_group`, `capobj_donetarget`, `capobj_hud_sprite`, `master`, `target`; the flags 1 (no multicap: one object is enough) and 2 (once per round).

### `dod_score_ent`

`team` (0 both), `score_points` (10), `score_resetitemtime` (3), `score_resetplayerstime` (3), `noise` (the group to reset,
empty is all), `noise2` (1 ends the game), `message`, `score_nextmap`, `score_nextmapdelay` (3).

### Other

| Entity | Keys and meaning |
|--------|------------------|
| `dod_point_relay` | `dod_relay_team`: 1 gives the point to the Allies, 2 to the Axis, 0 resets it. Targets a control point. |
| `dod_round_timer` | `round_timer_length` (60) seconds, `master`; flags 1 (shown on the HUD) and 2 (start off). |
| `dod_preround` | `allieswin`, `alliesloose`, `axiswin`, `axisloose`, `draw`: targets fired at the start of a round, by the result of the last one. |
| `dod_location` | `location_name`: the name of the place shown for the players inside. |
| `dod_trigger_sandbag` (a brush) | `sandbag_range` (15): degrees in either direction. |
| `dod_camera` | The cinematic camera. |
| `dod_mapmarker`, `dod_stats` | Created by the game. |

## Differences from hlsdk-portable

* The Half-Life game rules (`CHalfLifeRules`, deathmatch, teamplay, busters) are gone, the library has only the rules of Day
  of Defeat (`CDoDTeamPlay`, `CSPDoDRules`).
* The Half-Life weapons are gone, the weapons are those of Day of Defeat (`wpn_shared`).
* The code follows the original library: `dod.so` of the Steam version with its debug information gave the names of the files,
  the order of the functions, the layout of the classes, the tables of the virtual functions and the names of the locals.
  A function that differs from the original is a bug, unless it is listed in
  [Fixes of the bugs of the original game](#fixes-of-the-bugs-of-the-original-game) or written down as a deliberate
  difference (`UTIL_StripToken` keeps a length for the buffer, some strings are `const char *` where the original has `char *`,
  `string_t` is `int`).
* `GiveFnptrsToDll` and the tables are exported as in the original (`dod.def` on Windows).
* The 64-bit build follows hlsdk-portable: `MAKE_STRING` copies a string into the pool of the engine when it cannot be reached
  by a 32-bit offset, the save tables keep 32 bits in a file.
* The dead code (`#if 0` blocks) of Half-Life is removed.

---

<a id="russian"></a>
[English](#english) | **Русский**

# Серверная DLL Day of Defeat

Этот документ описывает структуру серверной библиотеки Day of Defeat 1.3 (`dod.dll` / `dod.so`), её консольные переменные,
механики, которые есть только в Day of Defeat, сущности, которые используют карты игры, и отличия от
[hlsdk-portable](https://github.com/FWGS/hlsdk-portable), SDK, на котором построен этот код. Он написан для тех, кто запускает
сервер, делает карты или хочет читать, собирать и менять библиотеку.

Библиотека предназначена для **Xash3D FWGS**. Она заменяет серверную библиотеку оригинальной игры и работает как с
оригинальной клиентской библиотекой, так и с [dod13-client](https://github.com/whamemer/dod13-client).

* [Структура](#структура)
* [Параметры сборки](#параметры-сборки)
* [Исправления багов оригинальной игры](#исправления-багов-оригинальной-игры)
* [Серверные переменные](#серверные-переменные)
* [Механики, которые есть только в Day of Defeat](#механики-которые-есть-только-в-day-of-defeat)
* [Сущности карт](#сущности-карт)
* [Отличия от hlsdk-portable](#отличия-от-hlsdk-portable)

## Структура

| Путь | Что там |
|------|---------|
| `dlls/h_export.cpp`, `client.cpp`, `cbase.cpp` | Функции, которые движок ищет по имени (`GiveFnptrsToDll`), и таблицы, которые он получает (`GetEntityAPI`): подключение, создание, think, touch, команды клиента, keyvalue сущностей. |
| `dlls/gamerules.cpp`, `dod_gamerules.cpp`, `.h` | `CGameRules` и `CDoDTeamPlay`, правила игры: раунды, команды, классы, волны, очки, клановые матчи, смена карты; здесь же `info_doddetect` и `dod_preround`. `CSPDoDRules` (правила для одного игрока) лежит в `gamerules.h`. |
| `dlls/player.cpp`, `observer.cpp`, `spectator.cpp` | `CBasePlayer`: появление, выносливость, стойки, объекты, подсказки, урон, смерть; режимы наблюдателя и зрителя. |
| `dlls/triggers.cpp` | `CControlPoint`, `CControlPointMaster`, `CAreaCapture` (`dod_capture_area`), `CObjectCapture` (`dod_object_goal`), `dod_point_relay`, `dod_round_timer`, `dod_location`, `dod_trigger_sandbag`, `dod_camera` и триггеры Half-Life. |
| `dlls/objectives.cpp` | `CObject` (`dod_object`), `CScoreEvent` (`dod_score_ent`) и `CParticleShooter`. |
| `dlls/weapons.cpp`, `weapons.h`, `dlls/wpn_shared/` | Базовые классы оружия и по одному классу на оружие. Файлы `wpn_shared/` это код, который клиент запускает и для предсказания. |
| `dlls/dod_shared.cpp`, `cl_dll/dod_shared.h` | Таблицы, общие с клиентом: идентификаторы и названия оружия, ограничения классов, голосовые команды, модели игроков, константы стоек. |
| `dlls/maprules.cpp` | Сущности `game_*` (`game_score`, `game_text`, `game_team_master`, ...). |
| `dlls/mapmarker.cpp`, `hintmessage.cpp`, `dod_stats.cpp` | Метки на карте, очередь подсказок, статистика игроков. |
| `dlls/game.cpp`, `game.h` | Квары: каждый квар объявлен и зарегистрирован здесь. |
| `dlls/world.cpp`, `util.cpp`, `subs.cpp`, `doors.cpp`, `plats.cpp`, ... | Мир, вспомогательный код и сущности Half-Life, которые используют карты. |
| `dlls/monsters.cpp`, `bot_*.cpp`, `h_ai.cpp`, ... | Монстры Half-Life и два класса ботов. Они есть в оригинальной библиотеке, ни одна карта игры их не использует. |
| `pm_shared/` | Движение игрока, общее с клиентским предсказанием. |
| `game_shared/` | Голос и несколько помощников, общих с клиентом. |
| `public/`, `common/`, `engine/`, `external/` | Заголовки интерфейса движка и сторонний код SDK. |
| `mod_options.txt` | Параметры сборки, см. ниже. |

`pm_shared/`, `game_shared/`, `common/`, `engine/`, `public/` и `external/` это те же файлы, что и в dod13-client: они общие с
клиентским предсказанием и должны оставаться одинаковыми.

## Параметры сборки

`mod_options.txt` в корне репозитория содержит параметры сборки, по одному в строке: `ИМЯ=ЗНАЧЕНИЕ # описание`. Его читают и
waf (`wscript`), и CMake. `ON` определяет макрос `ИМЯ` для всей библиотеки, `OFF` ничего не определяет, любое другое значение
становится переменной сборки.

| Параметр | Значение | Смысл |
|----------|----------|-------|
| `GAMEDIR` | `dod` | Каталог игры; цель install кладёт библиотеку в `<GAMEDIR>/<SERVER_INSTALL_DIR>`. |
| `SERVER_INSTALL_DIR` | `dlls` | Куда кладётся библиотека (должно совпадать с `liblist.gam` / `gameinfo.txt`). |
| `SERVER_LIBRARY_NAME` | `dod` | Имя библиотеки; платформа добавляет суффикс схемы именования (`_amd64`, `_arm64`). |
| `FIX_ENTITY_HASH_OVERFLOW` | `ON` | Исправление бага оригинальной игры, см. [ниже](#исправления-багов-оригинальной-игры). |
| `FIX_CAPTURE_AREA_DIVIDE_BY_ZERO` | `ON` | Исправление бага оригинальной игры, см. [ниже](#исправления-багов-оригинальной-игры). |

## Исправления багов оригинальной игры

В оригинальной библиотеке есть баги, которые этот код воспроизводит намеренно, потому что восстановление следует бинарному
файлу. Исправление такого бага **необязательно**: оно стоит за макросом, который включается в `mod_options.txt` (см.
[Параметры сборки](#параметры-сборки)). С параметром `OFF` библиотека ведёт себя в точности как оригинал. Оба исправления
ниже по умолчанию `ON`.

| Параметр | Баг оригинала | Исправление |
|----------|---------------|-------------|
| `FIX_ENTITY_HASH_OVERFLOW` | Хеш имени класса (`CaseInsensitiveHash`) берёт `% (size + 1)`, но в таблице сущностей только `size` элементов, поэтому сущность может оказаться записанной на один элемент за таблицей. | В таблице на один элемент больше (`cbase.cpp`, `AddEntityHashValue`). |
| `FIX_CAPTURE_AREA_DIVIDE_BY_ZERO` | Полоса прогресса зоны захвата делит на число игроков захватывающей команды. Если эта команда ушла из зоны, а другая ещё стоит в ней, число равно 0, и сервер падает с делением на ноль. | Обновление полосы прогресса пропускается, если в зоне никого из захватывающей команды нет (`CAreaCapture::Think`, `ReSendSwipeInfo`). |

### Как добавить исправление

1. Добавьте строку в `mod_options.txt`: `ИМЯ=ON # что делает исправление`.
2. Оберните изменённый код в `#if defined( ИМЯ )` ... `#else` (код оригинала) ... `#endif`. Выключенный параметр ничего не
   определяет.
3. Выполните `waf configure` заново (CMake тоже читает файл при конфигурации). В журнале сборки видны параметр и его состояние.

## Серверные переменные

Все значения это умолчания при регистрации. `S` = `FCVAR_SERVER`: значение отправляется клиентам и показывается в
информации о сервере. Квар без флага это обычная серверная переменная. Задавайте их в `server.cfg` или `listenserver.cfg`.

### Игра и раунды

| Квар | Умолчание | Флаги | Действие |
|------|-----------|-------|----------|
| `mp_timelimit` | 0 | S | Лимит времени карты в минутах; 0 без лимита. Когда время выйдет, начинается пауза между картами и загружается следующая. |
| `mp_chattime` | 10 | | Секунды паузы между картами (таблица счёта) до следующей карты; игра загоняет значение в 10...120. Игрок может закончить паузу раньше, нажав кнопку, когда время уже прошло. |
| `mp_friendlyfire` | 0 | S | 1 разрешает урон игрокам своей команды; урон самому себе засчитывается всегда. |
| `mp_tkpenalty` | 6 | | Предел счётчика убийств своих, после которого игрока автоматически кикают (`#game_kick_tk`); 0 и меньше отключает кик. Как в оригинале, каждое убийство своего добавляет к счётчику 2, поэтому умолчание 6 кикает на третьем убийстве своих подряд. Убийство врага обнуляет счётчик. Хоста игры с listen-сервером не кикают никогда. |
| `mp_teamlimit` | 2 | | На сколько игроков одна команда может быть больше другой. Игроку, который хочет войти в команду, уже опережающую на столько, отказывают (`#game_uneven_teams`). 0 отключает предел. |
| `mp_allowspectators` | 1 | | 0 запрещает переход в зрители. |
| `mp_weaponstay` | 0 | | 1 делает так, что оружие, лежащее в мире, возрождается сразу, кроме оружия, ограниченного в мире. Иначе оружие возвращается через 20 секунд. |
| `mp_spawnbazookas` | 0 | | 1 позволяет появляться ракетницам (базука, Pschreck, PIAT), лежащим на карте; без него они удаляются. |
| `mp_log_scores` | 0 | | Больше 0 пишет счёт команд в журнал сервера (`Team Scores: Allies: .., Axis: ..`) каждые `mp_log_scores` секунд (1...600; значение между 0 и 1 означает каждую секунду). |
| `mp_log_scores_delay` | 60 | | Секунды от начала карты до первой записи `mp_log_scores` (1...600; следующие записи идут по `mp_log_scores`). |
| `mp_deathmsg` | 1 | | 0 скрывает сообщения об убийствах. |
| `mp_chatmsg` | 1 | | 0 отключает чат для всех (`say` и `say_team` игнорируются). |
| `mp_defaultteam` | 0 | | Задаётся картой: флаг 4 worldspawn делает его равным 1. Вручную менять не нужно. |
| `mp_falldamage` | 0 | S | Зарегистрирован как в оригинале; код игры его не читает. |
| `mp_flashlight` | 0 | S | Зарегистрирован как в оригинале; код игры его не читает (в игре нет фонаря). |
| `mp_teamplay` | 0 | | Копируется в глобальную переменную `teamplay` движка каждый кадр; правила игры его не читают. |
| `mp_teamoverride` | 1 | | Зарегистрирован как в оригинале; код игры его не читает. |
| `mp_teamlist` | `hgrunt;scientist` | | Имена команд Half-Life. Зарегистрирован как в оригинале; его упоминает только комментарий у `game_team_master`. |
| `decalfrequency` | 30 | | Секунды между двумя нанесениями спрея игрока. |
| `displaysoundlist` | 0 | | Зарегистрирован как в оригинале; код игры его не читает. |
| `dev_serverhitbox` | 0 | | Зарегистрирован как в оригинале; код игры его не читает. |

### Наблюдатели

| Квар | Умолчание | Флаги | Действие |
|------|-----------|-------|----------|
| `mp_forcecamera` | 1 | S | Не 0: мёртвый игрок команды может смотреть только за своими, а свободный полёт заменяется закреплённым видом сзади. Зрителей это не ограничивает. |
| `mp_forcechasecam` | 1 | S | Игра обрабатывает его вместе с `mp_forcecamera`: любой из двух даёт то же ограничение. |
| `mp_fadetoblack` | 0 | S | Не 0: самый строгий уровень, мёртвый игрок видит только глазами своего. В активном клановом матче экран мёртвого игрока ещё и темнеет. |
| `mp_clan_showcutscenes` | 1 | | 0 отключает кинематографические камеры карты (`dod_camera`). В клановом матче они выключены всегда. |
| `mp_nummapmarkers` | 1 | | Сколько меток на карте может быть у игрока одновременно; при достижении предела самая старая из его меток исчезает. |
| `mp_markerstaytime` | 30 | | Секунды, сколько метка остаётся на карте; не меньше 2. |

### Классы и ограничения оружия

Ограничения считают игроков, у которых класс уже есть или выбран на следующее появление. `-1` без ограничения, `0` закрывает
класс. Класс, достигший ограничения, показывается в меню как недоступный, вход в него отклоняется.

| Квар | Умолчание | Флаги | Класс (номер) |
|------|-----------|-------|---------------|
| `mp_limitalliesgarand` | -1 | S | Стрелок с Garand (1) |
| `mp_limitalliescarbine` | -1 | S | M1 Carbine (2) |
| `mp_limitalliesthompson` | -1 | S | Thompson (3) |
| `mp_limitalliesgreasegun` | -1 | S | Grease gun (4) |
| `mp_limitalliesspring` | -1 | S | Снайпер Springfield (5) |
| `mp_limitalliesbar` | -1 | S | BAR (6) |
| `mp_limitallies30cal` | -1 | S | .30 cal (7) |
| `mp_limitalliesbazooka` | -1 | S | Bazooka (8) |
| `mp_limitalliesmortar` | -1 | S | Миномёт (9), см. ниже |
| `mp_limitaxiskar` | -1 | S | Kar98k (10) |
| `mp_limitaxisk43` | -1 | S | K43 (11) |
| `mp_limitaxismp40` | -1 | S | MP40 (12) |
| `mp_limitaxismp44` | -1 | S | MP44 (13) |
| `mp_limitaxisscopedkar` | -1 | S | Kar98k с прицелом (14) |
| `mp_limitaxisfg42` | -1 | S | FG42 (15) |
| `mp_limitaxisfg42s` | -1 | S | FG42 с прицелом (16) |
| `mp_limitaxismg34` | -1 | S | MG34 (17) |
| `mp_limitaxismg42` | -1 | S | MG42 (18) |
| `mp_limitaxispschreck` | -1 | S | Pschreck (19) |
| `mp_limitaxismortar` | -1 | S | Миномёт (20), см. ниже |
| `mp_limitbritlight` | -1 | S | Британская лёгкая пехота (21) |
| `mp_limitbritassault` | -1 | S | Британская штурмовая пехота (22) |
| `mp_limitbritsniper` | -1 | S | Британский снайпер (23) |
| `mp_limitbritmg` | -1 | S | Британский пулемётчик (24) |
| `mp_limitbritpiat` | -1 | S | PIAT (25) |
| `mp_limitbritmortar` | -1 | S | Британский миномёт (26), см. ниже |
| `mp_combinemglimits` | 0 | | 1 делает так, что MG34 и MG42 (17 и 18) делят одно ограничение: сумма двух ограничений против суммы двух классов. |
| `mp_alliesclasses` | -1 | | Битовая маска классов, которые могут выбрать союзники. Бит 0 это первый класс команды (номер 1 у американцев, 21 у британцев). -1 разрешает все. |
| `mp_axisclasses` | -1 | | То же для Оси, бит 0 это класс 10. |

Классы миномёта (9, 20 и 26) выбрать нельзя, как и в оригинале: таблицы и ограничения есть, игра отклоняет вход. Класс 27 это
случайный класс; клановый матч его отклоняет.

### Клановые матчи

Клановый матч включается `mp_clan_match 1`. Разминка, обратный отсчёт и сигнал готовности описаны в разделе
[Клановые матчи](#клановые-матчи-1).

| Квар | Умолчание | Флаги | Действие |
|------|-----------|-------|----------|
| `mp_clan_match` | 0 | S | 1 запускает режим кланового матча (разминка); 0 выходит из него. |
| `mp_clan_restartround` | 0 | S | Поставьте 1, чтобы перезапустить раунд после отсчёта `mp_clan_timer`; игра сама сбрасывает в 0. |
| `mp_clan_readyrestart` | 0 | S | Поставьте 1, чтобы дождаться сигнала готовности обеих команд; игра сама сбрасывает в 0. |
| `mp_clan_ready_signal` | `ready` | S | Слово, которое команда пишет в чат, чтобы показать готовность. |
| `mp_clan_match_warmup` | 0 | S | Поставьте 1, чтобы вернуться в разминку; игра сама сбрасывает в 0. |
| `mp_clan_timer` | 20 | S | Секунды обратного отсчёта перед перезапуском. |
| `mp_clan_respawntime` | 0 | S | Если больше 0, это время волны подкреплений в клановом матче вместо времени, которое зависит от числа игроков. |
| `mp_clan_scoring` | 0 | S | 1 включает очки кланового матча: очки за контрольные точки берутся из строк ниже раз в `mp_clan_scoring_delay`. |
| `mp_clan_scoring_values_allies` | `111111` | S | По одной цифре на контрольную точку, в порядке точек на карте (порядке сущностей): очки, которые союзники получают за владение. |
| `mp_clan_scoring_values_axis` | `111111` | S | То же для Оси. Точка без цифры даёт 0, сервер пишет предупреждение. |
| `mp_clan_scoring_delay` | 60 | S | Секунды между начислениями очков в клановом подсчёте (30, если квара нет). |
| `mp_clan_scoring_bonus_allies` | -1 | S | 0 и больше (не более 1000): очки, которые союзники получают за раунд, выигранный через `dod_score_ent`, вместо очков сущности. -1 оставляет очки сущности. |
| `mp_clan_scoring_bonus_axis` | -1 | S | То же для Оси. |

### Квары навыка Half-Life

Квары `sk_*` (`sk_agrunt_health1`, `sk_plr_9mm_bullet1`, `sk_player_head1`, ...; последняя цифра это уровень навыка 1...3) это
система навыка Half-Life. Они зарегистрированы как в оригинале и читаются в `gSkillData` при старте игры, но урон оружия Day of
Defeat берётся из кода оружия, а урон трёх ракетниц зафиксирован равным 100. Они важны для монстров Half-Life, которых ни одна
карта не использует.

### Квары движка, которые читает игра

`sv_gravity` (гравитация мира), `sv_aim` и `mp_footsteps` (звуки шагов).

## Механики, которые есть только в Day of Defeat

### Раунды

Состояние раунда хранится в `CDoDTeamPlay::m_RoundState`:

| Состояние | Значение | Смысл |
|-----------|----------|-------|
| `ROUNDSTATE_PREROUND` | 0 | Игроки заморожены и ждут начала. Здесь срабатывают триггеры `dod_preround` карты, может играть камера. |
| `ROUNDSTATE_RUNNING` | 1 | Раунд идёт: работают цели, очки и волны. |
| `ROUNDSTATE_ALLIES_WON` | 3 | Раунд закончен, победили союзники. |
| `ROUNDSTATE_AXIS_WON` | 4 | Раунд закончен, победила Ось. |
| `ROUNDSTATE_DRAW` | 5 | Раунд закончен, ничья. |

Раунд заканчивается, когда срабатывает `dod_score_ent` (команда захватила всё, взорвалась бомба: что именно его запускает,
решает карта) или когда выходит встроенный таймер карты (`detect_use_timer`, ниже). Лимит времени карты (`mp_timelimit`)
заканчивает карту, а не раунд. После короткой паузы (`TeamWon`) сущности карты
сбрасываются (`ResetEnts`: двери, кнопки, разрушаемые объекты, объекты, контрольные точки, ... возвращаются в состояние начала
карты) и начинается следующий раунд. Правила карты задаёт `info_doddetect` (см. ниже).

Раунд, в котором у одной из команд нет игроков, «пустой»: очки не засчитываются.

### Команды, классы и оружие

Команды: союзники (1), Ось (2), зрители (3). Союзники это американцы или британцы, это определяет `detect_allies_country`
карты. У парашютистов (`detect_allies_paras`, `detect_axis_paras`) свои классы; пока у команд не обе бесконечные жизни, они могут
входить только в первые 30 секунд раунда. Оружие берётся из класса: `Classify()` оружия говорит, пистолет это, винтовка, винтовка с прицелом, пистолет-пулемёт,
пулемёт, ракетница или граната (`CLASS_*` в `dod_shared.h`), и правила игры разрешают одно оружие из каждой группы.
`mp_weaponstay`, `mp_spawnbazookas` и ограничения классов описаны выше.

### Волны подкреплений и бесконечные жизни

Если у команды бесконечные жизни (`detect_allies_infinite` / `detect_axis_infinite` в `info_doddetect`, по умолчанию включено),
её погибшие игроки возрождаются не сразу: все вместе в одной волне. Время волны зависит от числа игроков команды: 6 секунд до
2 игроков, затем 8 (3...5), 10 (6...7), 11 (8...9), 12 (10...11), 13 (12...13) и 14 секунд при 14 и более, умноженное на
`detect_allies_respawnfactor` / `detect_axis_respawnfactor` карты. В клановом матче время заменяет `mp_clan_respawntime`.
Команда без бесконечных жизней не возрождается: погибший ждёт конца раунда.

### Контрольные точки

У контрольной точки (`dod_control_point`) есть владелец: никто, союзники или Ось. Команда берёт её одним из двух способов:
* **Касанием.** Игрок этой команды заходит в точку и берёт её сразу (`point_can_allies_touch`, `point_can_axis_touch` могут
  запретить это команде; после смены точка секунду не может смениться снова).
* **Зоной захвата.** `dod_capture_area` указывает на точку; игроки команды, стоящие внутри, захватывают её за
  `area_time_to_cap` секунд, если их там не меньше `area_*_numcap`.

Когда у точки меняется владелец, меняются её модель и бодигруппа, играет звук команды, срабатывает цель команды
(`point_allies_target`, `point_axis_target`, `point_reset_target`), захватившему идут `point_points_for_cap`, а его команде
`point_team_points`, и клиентам сообщается об этом (значки HUD, строка чата `point_win_string`).

Точки собирает **мастер контрольных точек** (`dod_control_point_master`): он находит точки с тем же именем группы, нумерует их
(`point_index`; если индексов нет или они повторяются, то по расстоянию от первой точки появления союзников), сообщает о них
клиентам (`InitObjDisplay`) и каждые `point_give_delay_time` секунд начисляет каждой команде сумму `point_pointvalue` её точек
как командный счёт (в клановом матче с `mp_clan_scoring` берутся строки квара). Команда, владеющая всеми точками мастера,
выигрывает раунд: мастер запускает `allies_capture_target` или `axis_capture_target`.

### Зоны захвата

Зона захвата считает живых игроков обеих команд внутри. Команда может начать захват, когда в зоне нет игроков другой команды,
команде разрешено захватывать (`area_allies_cancap`, `area_axis_cancap`), точка ещё не принадлежит ей и игроков достаточно
(`area_allies_numcap`, `area_axis_numcap`). Если зоне нужен объект (`area_object_group`), считаются только игроки, несущие
объект этой группы. Захват идёт `area_time_to_cap` секунд, затем заканчивается; если входит игрок другой команды или у
захватывающей команды остаётся слишком мало игроков, он срывается. Зона запускает `area_*_startcap`, `area_*_breakcap` и
`area_*_endcap` и отправляет клиентам полосу прогресса и число игроков внутри.

С флагом 1 зоны и не при бесконечных жизнях у обеих команд команда, у которой игроков меньше нужного, но **все живые игроки**
находятся внутри, всё равно захватывает, но медленнее: скорость равна числу игроков, делённому на нужное.

### Объекты и места доставки

Объект (`dod_object`, динамит на `dod_escape`) лежит на земле, пока его не коснётся игрок **вражеской** команды: `object_owner`
это команда, которой принадлежит объект, берёт его другая. Игрок несёт один объект, его скорость умножается на
`object_carryspeed`, объект показывают HUD и спрайт над головой (`object_hud_sprite`, `object_player_sprite`). Объект
бросается, когда игрок погибает, отключается или вводит `dropobject`; он лежит там, где упал, и возвращается на старт
через `object_resettime` секунд (не более 600) или сразу, когда его коснётся игрок команды-владельца (`object_resetontouch`).

Место доставки (`dod_object_goal`, браш) принимает объект: несущий, чей объект принадлежит группе места (`capobj_group`),
заходит в него и получает `object_cappoints` объекта. Пока другие объекты группы ещё в пути (и флаг 1 не стоит), игра сообщает,
сколько их осталось, место запускает свою цель `target`, а товарищи по команде несущего получают `object_otherpoints`. Когда
доставлен последний объект, или сразу с флагом 1 («Dont Multicap»: достаточно одного объекта), место запускает
`capobj_donetarget` и `object_donetarget` объекта. С флагом 2 место срабатывает раз за раунд.

### Сущности очков

`dod_score_ent` при запуске заканчивает раунд для своей команды: даёт `score_points` (или клановый бонус), показывает своё
сообщение, сбрасывает предметы и игроков через `score_resetitemtime` / `score_resetplayerstime` секунд, может сменить карту
(`score_nextmap` через `score_nextmapdelay`) и может закончить игру (`noise2` = 1).

### Игроки

* **Выносливость.** Шкала 0...100 (`pev->fuser4`). Бег (клавиша бега с клавишей направления, скорость больше 100) отнимает 30 в
  секунду. Восстанавливается по 6 за десятую секунды стоя, по 1 в движении и по 5 в приседе на земле. Выстрел отнимает 1 или 2
  (больше у пулемётов), если игрок не сидит и не лежит. Низкая шкала замедляет игрока и показывает подсказку
  `#Hint_low_stamina`.
* **Лёжа.** Команда `sprone` (клавиша игры) ложится и встаёт. Она отклоняется с развёрнутым мешком песка или миномётом, во
  время стрельбы, на лестнице, в воде глубже колена, в приседе и в течение секунды после предыдущей смены.
* **Сошки и мешки с песком.** Оружие с сошками можно развернуть. Лежащий игрок разворачивает сошки; внутри
  `dod_trigger_sandbag` (`sandbag_range` градусов в обе стороны) оружие можно развернуть на мешке с песком.
  Состояние лежит в `pev->iuser3` (`STANCE_*`) и `pev->vuser1` (`SANDBAG_*`).
* **Нагрев пулемёта.** Развёрнутый пулемёт нагревается с каждым выстрелом (на 3) и остывает; на 99 он останавливается с
  подсказкой `#Hint_mg_overheat`.
* **Подсказки.** `CBasePlayer::HintMessage` показывает подсказку один раз для каждого вида (история это маска, `HINT_*`), если
  игрок не отключил подсказки.
* **Убийства своих.** См. `mp_tkpenalty`.
* **Метки на карте.** Игрок может поставить метку на карте (`dod_mapmarker`), её видит команда.
* **Голосовые команды.** Меню голосовых команд (в атаку, держать, отступать, ...) играет звук класса и показывает значок.
* **Статистика.** `dod_stats` считает убийства, смерти, попадания и выстрелы каждого игрока.

### Клановые матчи

С `mp_clan_match 1` игра переходит в режим кланового матча и показывает правила разминки. `mp_clan_readyrestart 1` просит команды написать в чат
`mp_clan_ready_signal`; когда это сделали обе, начинается отсчёт `mp_clan_timer` секунд, и матч начинается с перезапуска.
`mp_clan_restartround 1` перезапускает сразу после отсчёта, `mp_clan_match_warmup 1` возвращает в разминку. Кат-сцены и
случайный класс в клановом матче выключены. Очки описаны в таблице выше.

## Сущности карт

Ниже перечислены keyvalue сущности на карте (те, что читает игра). Определения для редактора лежат в `dod.fgd`.

### `info_doddetect`: правила карты

| Ключ | Умолчание | Смысл |
|------|-----------|-------|
| `detect_allies_infinite`, `detect_axis_infinite` | 1 | 1 даёт команде бесконечные возрождения (волны), 2 их отключает. |
| `detect_allies_paras`, `detect_axis_paras` | 0 | 1 делает команду парашютистами. |
| `detect_allies_country` | 0 | 0 американцы, 1 британцы. |
| `detect_allies_respawnfactor`, `detect_axis_respawnfactor` | 1.0 | Умножает время волны. |
| `detect_points_axiseliminated`, `detect_points_allieseliminated` | 5 | Очки за уничтожение команды. Читаются и запоминаются, ничем не используются, как в оригинале. |
| `detect_target_axiseliminated`, `detect_target_allieseliminated` | | Запускается, когда команда уничтожена. Читаются и запоминаются, ничем не используются, как в оригинале. |
| `detect_use_timer`, `detect_target_timerexpired`, `detect_timer_team`, `detect_points_timerexpired` | 0 | Встроенный таймер карты (`para_roundtimer`): когда он выходит, раунд заканчивается в пользу `detect_timer_team` (ничья при 0), и эта команда получает `detect_points_timerexpired`. |
| `detect_wind_velocity_x`, `detect_wind_velocity_y`, `detect_weather_type` | 0 | Ветер и погода (0 нет, 1 дождь, 2 снег); читает клиент. |

### `dod_control_point`

| Ключ | Умолчание | Смысл |
|------|-----------|-------|
| `point_name` | `a control point` | Название в сообщениях. |
| `point_default_owner` | 0 | 0 никто, 1 союзники, 2 Ось. |
| `point_can_allies_touch`, `point_can_axis_touch` | 0 | 1 запрещает команде брать точку касанием. |
| `point_pointvalue` | 1 | Командный счёт за начисление, пока команда владеет точкой. |
| `point_points_for_cap` | 1 | Очки игрока, взявшего точку. |
| `point_team_points` | 0 | Командный счёт за её взятие. |
| `point_allies_model`, `point_axis_model`, `point_reset_model` (+ `_bodygroup`) | | Модели для каждого владельца. |
| `point_allies_capsound`, `point_axis_capsound`, `point_resetsound` | | Звуки. |
| `point_allies_target`, `point_axis_target`, `point_reset_target` | | Запускаются при смене. |
| `point_hud_icon_allies`, `point_hud_icon_axis`, `point_hud_icon_neutral` | | Номера значков HUD (флаг, артиллерия, мост, рация, документы, грузовик, свои). |
| `point_win_string`, `point_allies_win_string`, `point_axis_win_string` | | Текст, отправляемый клиентам при захвате; пустой ничего не отправляет. |
| `point_group`, `point_index` | | Мастер, который собирает точку, и номер точки. |
| `master` | | Точка работает, только пока этот мастер сработал. |
| флаг 1 | | Не показывать точку на HUD. |

### `dod_control_point_master`

`point_give_delay_time` (20), `allies_capture_target`, `axis_capture_target`, `cpm_group`, `master`.

### `dod_capture_area` (браш)

`area_allies_cancap`, `area_axis_cancap` (1), `area_allies_numcap`, `area_axis_numcap` (1), `area_time_to_cap` (5),
`area_allies_startcap`, `area_allies_breakcap`, `area_allies_endcap`, `area_axis_startcap`, `area_axis_breakcap`,
`area_axis_endcap`, `area_hud_sprite`, `area_object_group`, `master`; `target` это контрольная точка, которую зона захватывает.

### `dod_object`

`object_name`, `object_owner` (0 обе, 1 союзники, 2 Ось: команда-владелец; объект берёт другая), `object_group`,
`object_takesound`, `object_capsound`, `object_dropsound`, `object_returnsound`, `object_carryspeed` (0.5),
`object_resetontouch` (1), `object_resettime` (60), `object_cappoints` (10), `object_otherpoints` (5), `object_donetarget`,
`object_player_sprite`, `object_hud_sprite`, `master`; флаг 1 сообщает о подборе и сбросе. При подборе `dod_object` также
запускает свою цель `target`.

### `dod_object_goal` (браш)

`capobj_group`, `capobj_donetarget`, `capobj_hud_sprite`, `master`, `target`; флаги 1 (без мультизахвата: достаточно одного объекта) и 2 (раз за раунд).

### `dod_score_ent`

`team` (0 обе), `score_points` (10), `score_resetitemtime` (3), `score_resetplayerstime` (3), `noise` (группа для сброса,
пустая это все), `noise2` (1 заканчивает игру), `message`, `score_nextmap`, `score_nextmapdelay` (3).

### Прочее

| Сущность | Ключи и смысл |
|----------|---------------|
| `dod_point_relay` | `dod_relay_team`: 1 отдаёт точку союзникам, 2 Оси, 0 сбрасывает. Указывает на контрольную точку. |
| `dod_round_timer` | `round_timer_length` (60) секунд, `master`; флаги 1 (показывать на HUD) и 2 (выключен в начале). |
| `dod_preround` | `allieswin`, `alliesloose`, `axiswin`, `axisloose`, `draw`: цели, запускаемые в начале раунда по итогу прошлого. |
| `dod_location` | `location_name`: название места, которое видят находящиеся внутри игроки. |
| `dod_trigger_sandbag` (браш) | `sandbag_range` (15): градусов в обе стороны. |
| `dod_camera` | Кинематографическая камера. |
| `dod_mapmarker`, `dod_stats` | Создаются игрой. |

## Отличия от hlsdk-portable

* Правила игры Half-Life (`CHalfLifeRules`, deathmatch, teamplay, busters) удалены, в библиотеке остались только правила Day of
  Defeat (`CDoDTeamPlay`, `CSPDoDRules`).
* Оружия Half-Life нет, оружие это оружие Day of Defeat (`wpn_shared`).
* Код следует оригинальной библиотеке: `dod.so` Steam-версии с отладочной информацией дал имена файлов, порядок функций,
  раскладку классов, таблицы виртуальных функций и имена локальных переменных. Функция, отличающаяся от оригинала, это ошибка,
  если только она не перечислена в разделе
  [Исправления багов оригинальной игры](#исправления-багов-оригинальной-игры) или не записана как намеренное отличие
  (`UTIL_StripToken` хранит длину для защиты буфера, некоторые строки имеют тип `const char *`, где у оригинала `char *`,
  `string_t` это `int`).
* `GiveFnptrsToDll` и таблицы экспортируются как в оригинале (`dod.def` в Windows).
* 64-битная сборка следует hlsdk-portable: `MAKE_STRING` копирует строку в пул движка, когда до неё нельзя дотянуться 32-битным
  смещением, таблицы сохранения хранят в файле 32 бита.
* Мёртвый код (блоки `#if 0`) Half-Life удалён.

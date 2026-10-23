#
# Copyright (c) 2015 Pavlo Lavrenenko
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.10)
project (SVDLL)

set (SVDLL_LIBRARY server)

if(NOT MSVC)
	add_compile_options(-fno-exceptions) # GCC/Clang flag
	add_compile_options(-Wno-invalid-offsetof) # GCC/Clang flag
	add_compile_options(-fvisibility=hidden) # GCC/Clang flag
	add_definitions(-D_LINUX) # It seems enough for all non-Win32 systems
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf )
	if(CMAKE_BUILD_TYPE MATCHES "Release"
		OR (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo"
		AND CMAKE_SYSTEM_NAME STREQUAL "Android"))
		add_compile_options(-fno-unwind-tables -fno-asynchronous-unwind-tables) # GCC/Clang flag
		add_compile_options(-fomit-frame-pointer) # GCC/Clang flag
		add_compile_options(-ftree-vectorize) # GCC/Clang flag
		add_compile_options(-funsafe-math-optimizations) # GCC/Clang flag
	endif()
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

# The sources are listed explicitly, a new file has to be added here as well as it is picked up by waf (dlls/wscript) automatically.
# wxdebug.h is only a header, the weapons in wpn_shared are shared with the client.
set (SVDLL_SOURCES
	../public/MemPool.cpp
	alias.cpp
	animating.cpp
	animation.cpp
	bmodels.cpp
	bot_allied_barney.cpp
	bot_allied_grunt.cpp
	bot_axis_grunt.cpp
	buttons.cpp
	cbase.cpp
	client.cpp
	combat.cpp
	controller.cpp
	defaultai.cpp
	dod_gamerules.cpp
	dod_shared.cpp
	dod_stats.cpp
	doors.cpp
	effects.cpp
	explode.cpp
	func_break.cpp
	func_tank.cpp
	game.cpp
	gamerules.cpp
	genericmonster.cpp
	ggrenade.cpp
	globals.cpp
	h_ai.cpp
	h_cine.cpp
	h_cycler.cpp
	h_export.cpp
	hintmessage.cpp
	items.cpp
	lights.cpp
	mapmarker.cpp
	maprules.cpp
	monsters.cpp
	monsterstate.cpp
	mortar.cpp
	nodes.cpp
	objectives.cpp
	observer.cpp
	pathcorner.cpp
	plane.cpp
	plats.cpp
	player.cpp
	rat.cpp
	roach.cpp
	sarge.cpp
	schedule.cpp
	scripted.cpp
	singleplay_gamerules.cpp
	skill.cpp
	sound.cpp
	soundent.cpp
	spectator.cpp
	squadmonster.cpp
	stats.cpp
	subs.cpp
	talkmonster.cpp
	triggers.cpp
	turret.cpp
	util.cpp
	weapons.cpp
	world.cpp
	wpn_shared/30cal.cpp
	wpn_shared/amer_knife.cpp
	wpn_shared/bar.cpp
	wpn_shared/bazooka.cpp
	wpn_shared/bipod.cpp
	wpn_shared/bren.cpp
	wpn_shared/colt.cpp
	wpn_shared/enfield.cpp
	wpn_shared/fg42.cpp
	wpn_shared/garand.cpp
	wpn_shared/ger_knife.cpp
	wpn_shared/greasegun.cpp
	wpn_shared/handgrenade.cpp
	wpn_shared/handgrenade_ex.cpp
	wpn_shared/k43.cpp
	wpn_shared/kar.cpp
	wpn_shared/knife.cpp
	wpn_shared/luger.cpp
	wpn_shared/m1carbine.cpp
	wpn_shared/mg34.cpp
	wpn_shared/mg42.cpp
	wpn_shared/mp40.cpp
	wpn_shared/mp44.cpp
	wpn_shared/piat.cpp
	wpn_shared/pistol.cpp
	wpn_shared/pschreck.cpp
	wpn_shared/scoped98k.cpp
	wpn_shared/spade.cpp
	wpn_shared/spring.cpp
	wpn_shared/sten.cpp
	wpn_shared/stickgrenade.cpp
	wpn_shared/stickgrenade_ex.cpp
	wpn_shared/thompson.cpp
	wpn_shared/webley.cpp
	../game_shared/voice_gamemgr.cpp
	../pm_shared/pm_debug.cpp
	../pm_shared/pm_math.cpp
	../pm_shared/pm_shared.cpp
	../public/safe_snprintf.c
)

if (HAVE_STRLCPY)
	add_definitions(-DHAVE_STRLCPY)
else()
	list(APPEND SVDLL_SOURCES
		../external/openbsd/strlcpy.c)
endif()

if (HAVE_STRLCAT)
	add_definitions(-DHAVE_STRLCAT)
else()
	list(APPEND SVDLL_SOURCES
		../external/openbsd/strlcat.c)
endif()

# ../cl_dll is here for dod_shared.h, it is shared with the client
include_directories (. wpn_shared ../common ../engine ../pm_shared ../game_shared ../public ../cl_dll)

if(MSVC)
	set(SVDLL_SOURCES
		${SVDLL_SOURCES}
		dod.def)
endif()
add_library (${SVDLL_LIBRARY} SHARED ${SVDLL_SOURCES})
target_link_libraries(${SVDLL_LIBRARY} vcs_info)

set_target_properties (${SVDLL_LIBRARY} PROPERTIES POSITION_INDEPENDENT_CODE 1)

if(APPLE AND NOT "${SERVER_LIBRARY_NAME_OSX}" STREQUAL "")
	# on Apple platforms we always use gamedll_osx liblist/gameinfo key, if it's set
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME_OSX}")
elseif(NOT WIN32 AND NOT "${SERVER_LIBRARY_NAME_LINUX}" STREQUAL "")
	# on non-Windows platforms we use gamedll_linux liblist/gameinfo key, if it's set
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME_LINUX}")
else()
	# on Windows or if no gamedll_osx or gamedll_linux is set, use default gamedll key
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME}")
endif()

set_target_postfix_with_name(${SVDLL_LIBRARY} ${SVDLL_NAME})

if(HAVE_LTO)
	set_property(TARGET ${SVDLL_LIBRARY} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS ${SVDLL_LIBRARY}
	RUNTIME DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
	LIBRARY DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE)

if(CMAKE_BUILD_TYPE MATCHES "Release" AND NOT WIN32)
	if(APPLE)
		# Apple strip treats -s as a symbol list file, -x keeps exported symbols
		set(STRIP_FLAGS -x)
	else()
		set(STRIP_FLAGS -s)
	endif()
	add_custom_command(TARGET ${SVDLL_LIBRARY}
			POST_BUILD DEPENDS ${SVDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} ${STRIP_FLAGS} $<TARGET_FILE:${SVDLL_LIBRARY}>)
endif()

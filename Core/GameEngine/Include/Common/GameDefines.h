/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "WWDefines.h"

//MODDD - NOTE - the goal of this fork disagrees with below - enable all fixes regardless.
// Constants below up to a significant 'MODDD' separator are from TheSuperHackers repo.
// Changing constants such as 'PRESERVE_RETAIL_BEHAVIOR' back to 1 will no longer guarantee compatibility
// because of several other changes as of this fork.
// ---
// Note: Retail compatibility must not be broken before this project officially does.
// Use RETAIL_COMPATIBLE_CRC and RETAIL_COMPATIBLE_XFER_SAVE to guard breaking changes.

#ifndef PRESERVE_RETAIL_BEHAVIOR
//MODDD - was 1, changed to 0 for more fixes
#define PRESERVE_RETAIL_BEHAVIOR (0) // Retain behavior present in retail Generals 1.08 and Zero Hour 1.04
#endif

#ifndef RETAIL_COMPATIBLE_CRC
//MODDD - was 1, changed to 0 for more fixes
#define RETAIL_COMPATIBLE_CRC (0) // Game is expected to be CRC compatible with retail Generals 1.08, Zero Hour 1.04
#endif

#ifndef RETAIL_COMPATIBLE_XFER_SAVE
//MODDD - was 1, changed to 0 for more fixes
#define RETAIL_COMPATIBLE_XFER_SAVE (0) // Game is expected to be Xfer Save compatible with retail Generals 1.08, Zero Hour 1.04
#endif

// This is here to easily toggle between the retail compatible with fixed pathfinding fallback and pure fixed pathfinding mode
#ifndef RETAIL_COMPATIBLE_PATHFINDING
//MODDD - was 1, changed to 0 for more fixes
#define RETAIL_COMPATIBLE_PATHFINDING (0)
#endif

// This is here to easily toggle between the retail compatible pathfinding memory allocation and the new static allocated data mode
#ifndef RETAIL_COMPATIBLE_PATHFINDING_ALLOCATION
//MODDD - was 1, changed to 0 for more fixes
#define RETAIL_COMPATIBLE_PATHFINDING_ALLOCATION (0)
#endif

// Disable non retail fixes in the networking, such as putting more data per UDP packet
#ifndef RETAIL_COMPATIBLE_NETWORKING
//MODDD - was 1, changed to 0 for more fixes
#define RETAIL_COMPATIBLE_NETWORKING (0)
#endif

// This is essentially synonymous for RETAIL_COMPATIBLE_CRC. There is a lot wrong with AIGroup, such as use-after-free, double-free, leaks,
// but we cannot touch it much without breaking retail compatibility. Do not shy away from using massive hacks when fixing issues with AIGroup,
// but put them behind this macro.

#ifndef RETAIL_COMPATIBLE_AIGROUP
//MODDD - was 1, changed to 0 for more fixes
#define RETAIL_COMPATIBLE_AIGROUP (0) // AIGroup logic is expected to be CRC compatible with retail Generals 1.08, Zero Hour 1.04
#endif

#ifndef ENABLE_GAMETEXT_SUBSTITUTES
#define ENABLE_GAMETEXT_SUBSTITUTES (1) // The code can provide substitute texts when labels and strings are missing in the STR or CSF translation file
#endif

// Previously the configurable shroud sat behind #if defined(RTS_DEBUG)
// Enable the configurable shroud to properly draw the terrain in World Builder without RTS_DEBUG compiled in.
// Disable the configurable shroud to make shroud hacking a bit less accessible in Release game builds.
// MODDD - NOTE - leaving this on, but the setting itself hasn't been tested with my REMOVE_FOG_OF_WAR/ALT feature.
#ifndef ENABLE_CONFIGURABLE_SHROUD
#define ENABLE_CONFIGURABLE_SHROUD (1) // When enabled, the GlobalData contains a field to turn on/off the shroud, otherwise shroud is always enabled
#endif

// Enable buffered IO in File System. Was disabled in retail game.
// Buffered IO generally is much faster than unbuffered for small reads and writes.
#ifndef USE_BUFFERED_IO
#define USE_BUFFERED_IO (1)
#endif

// Enable cache for local file existence. Reduces amount of disk accesses for better performance,
// but decreases file existence correctness and runtime stability, if a cached file is deleted on runtime.
#ifndef ENABLE_FILESYSTEM_EXISTENCE_CACHE
#define ENABLE_FILESYSTEM_EXISTENCE_CACHE (1)
#endif

// Enable prioritization of textures by size. This will improve the texture quality of 481 textures in Zero Hour
// by using the larger resolution textures from Generals. Content wise these textures are identical.
#ifndef PRIORITIZE_TEXTURES_BY_SIZE
#define PRIORITIZE_TEXTURES_BY_SIZE (1)
#endif

// Enable obsolete code. This mainly refers to code that existed in Generals but was removed in GeneralsMD.
// Disable and remove this when Generals and GeneralsMD are merged.
#if RTS_GENERALS
#ifndef USE_OBSOLETE_GENERALS_CODE
#define USE_OBSOLETE_GENERALS_CODE (1)
#endif
#endif

// Overwrite window settings until wnd data files are adapted or fixed.
#ifndef ENABLE_GUI_HACKS
#define ENABLE_GUI_HACKS (1)
#endif

// Tell our computer identity in the LAN lobby. Disable for privacy.
// Was enabled in the retail game and exposed the computer login and host names.
#ifdef RTS_DEBUG
#ifndef TELL_COMPUTER_IDENTITY_IN_LAN_LOBBY
#define TELL_COMPUTER_IDENTITY_IN_LAN_LOBBY (1)
#endif
#endif

#define MIN_DISPLAY_BIT_DEPTH       16
#define DEFAULT_DISPLAY_BIT_DEPTH   32
#define DEFAULT_DISPLAY_WIDTH      800 // The standard resolution this game was designed for
#define DEFAULT_DISPLAY_HEIGHT     600 // The standard resolution this game was designed for


//MODDD - new config settings per-build for this fork for the remainder of the file.
// Several are listed at the very bottom - look there for things commented out beneath the descriptions for definitions.
// 
// -------------------------------------------------------------
// Use this to be able to play generals challenge maps (possibly any single player campaign map?) as a skirmish map
// for two reasons:
// * Able to choose any particular generals challenge map + your general without loading a game or
//   playing a bunch of challenges to get to the one you want
// * Co-op: anything that works in skirmish works for multiplayer all the same (tested in LAN). Create a game set
//   to the generals challenge map, have someone join, and take on the opposing general together.
//   (set each team to 1).
// For either of above (yourself or co-op), you have to open the map in the World Builder and add a waypoint
// named "Player_2_Start" or else the in-game skirmish menu won't even offer player slots -> unable to play
// the map. The main point of this feature is co-op support and you can just ignore the 2nd slot after making that
// change for single-player usage, so I don't really plan on addressing this, but hey, maybe.
// Keep in mind that if you pick a faction that wasn't originally intended, like vanilla factions
// for a generals challenge map (ex: base USA instead of the laser general), there may be unexpected
// effects, such as Dr. Thrax's map never triggering victory because vanilla factions are never
// explicitly checked for that in map scripts.
// Lastly, don't use this constant for playing the generals challenge normally (starting a new GC campaign from
// the main menu UI: choosing a difficulty + general -> vs. screen, OR loading a saved game from winning one).
// Adjustments made to force a skirmish map to work for that may break playing them the original way.
//#define GENERALS_CHALLENGE_FORCE FALSE

// Simpler one to let non-GC campaign missions work as usual, that is, ones with a fixed side / starting units
// the player controls instead of hacking in skirmish.
// This isn't well tested yet - differences from the base game (playing the campaign normally instead) have been seen.
// Ex: 1st USA zero hour mission, the plane dropping the MOAB toward the end doesn't appear, mission ends in victory anyway.
//#define CAMPAIGN_FORCE FALSE

// Difficulty for scripts & AI where unspecified by skirmish setting, such as generals challenge played as a skirmish map
// (AI player doesn't come from player slots assigned in the skirmish menu)
//#define DEFAULT_GLOBAL_SKIRMISH_DIFFICULTY DIFFICULTY_HARD

// Starting human (not AI / map-baked-in) player money.
// This const overrides any other way of setting start money, like the skirmish money setting or the game's hardcoded
// 10k seen on campaign / GC maps that don't change the starting money with scripts.
// If set to 0, has no effect (retail behavior: up to skirmish setting / map scripts / hardcoded default)
// Also, note that this doesn't apply to AI slot players (i.e. if in a co-op GC map, you set player 2 to an AI player instead).
// Not supporting a check for this because their AI would be dead anyway - just leave the slot empty if you don't have any friends.
//#define FORCE_HUMAN_PLAYER_START_MONEY 35000

// Blocks scripts that set money, not ones that add money.
// Good if starting player money is forced so that a map script doesn't immediately override it at the start of the game.
// If a map mistakenly uses a script to add instead of set money at the start of a game, it will work anyway.
// This doesn't affect start moeny set by the skirmish menu.
//#define BLOCK_SET_MONEY_SCRIPT_FOR_HUMAN_PLAYERS TRUE

// Also applies regardless of GENERALS_CHALLENGE_FORCE.
// Multiple of all income for computer players to compensate for the AI's lack of ambition for renewable economic structures
// when supply docks/piles run out late in the game. Pretty crude check: assumes any money change below X is from most
// normal in-game means (supply truck, oil derrick, etc.) and not start money or a map script doing it.
//#define COMPUTER_PLAYER_MONEY_SCALAR 10

// Disable the fog-of-war mechanic entirely. Debugger's dream. Or to watch the AI duke it out without
// having to be a replay.
// Hey, you know AI players were already playing without it anyway.
//#define REMOVE_FOG_OF_WAR FALSE

// Different way to disable the fog-of-war mechanic, by changing how the game interprets cells vs. acting like
// every single map started with a map-reveal script action. This way might play better in some cases.
// (mutually exclusive with above - don't enable both)
//#define REMOVE_FOG_OF_WAR_ALT TRUE

// If you pick a particular faction (not AI players), gives some bonuses, basically cheats.
// Hard-coded check for faction name for this. This is the opposite of professional. 
//#define NOOB_MODE FALSE

// Forces extra modifiers based on the current game difficulty, such as reduced unit health/damage for hard
// difficulty, to be disabled regardless of the setting in the GameData.ini file.
#define DISABLE_UNIT_HEALTH_WEAPON_DIFFICULTY_BONUSES TRUE

// Changes a few places so that units running over neutral units (civilians, units owned by the neutral/civilian player)
// is no longer possible. Changes most checks that forbid this from being strictly allies to more broadly being non-enemies.
#define DONT_RUN_OVER_NEUTRAL_UNITS TRUE

// Setting added in the hopes of preventing a major source of hardware lag: adding to a continually building
// '<documents>/Command and... Data/Replays/00000000.REP' file for a replay the user may never intend to save/look at anyway.
// Assuming the issue is what I think it is (hard drive writes becoming obnoxious in long-running games,
// possibly worse in mods for this reason), a better fix would probably be to try and hold the entire replay
// in memory & do write operations to there instead since modern machines can handle this, hopefully without
// this having to be a 64-bit program (needing over 4 gig of memory).
// Also, consider a yes/no option for whether a game should record for a replay before starting the game.
#define BLOCK_REPLAY_RECORDING_ALWAYS TRUE

// Double the max player count from 16 to 32. Needed for multiplayer for the retail Laser General challenge map to work,
// since as-is, the map uses enough players to only allow for adding 1 more.
// Toggling this causes a tiny compile error that shows save compatibility being broken.
#define DOUBLE_MAX_PLAYER_COUNT TRUE

// Map display names have the player count extension (ex: "my map name (4)" for a 4-player map)
// even if the player count is 1 or 0(?).
#define MAP_NAME_PLAYER_COUNT_EXTENSION_ALWAYS TRUE

// Replace logic in MapUtil.cpp's 'MapCache::updateCache' to load maps from the filesystem the first time
// they are requested and avoid the MapCache.ini files. Similar to 'SHOW_SP_OFFICIAL_MAPS_IN_RELEASE=TRUE'
// below, but this also fixes a small display bug: map names for maps without a display name (raw file name)
// in the System tab always being lowercase.
#define NEW_MAP_LIST_LOAD_LOGIC TRUE

// Show single player official maps even in release builds. These are normally hidden because they include
// campaign maps and a few debug/test ones.
// Note that this has no effect if 'NEW_MAP_LIST_LOAD_LOGIC' is on - can imply this setting to be TRUE in that case.
#define SHOW_SP_OFFICIAL_MAPS_IN_RELEASE TRUE

// Overrides default behavior to block generating the map cache in release mode by default.
// Otherwise, maps added to your install's 'Maps' folder won't have any effect on the maps shown in map selection.
// Note that deleting your install folder's 'Maps/MapCache.ini' folder won't be enough to regenerate it -
// it appears the default one from the '.big' files will be used instead.
// Although it's intended to use the 'Documents/<game>/Maps maps' folder for adding custom maps, it can be
// difficult to tell which mod a map is intended for if all maps across several mods are in the same
// Documents' 'Maps' folder.
// It's easier to use an install's 'Maps' folder for maps only intended for that mod since running the game
// from there is the only way to see those maps. Having a separate install folder per mod is a good rule of thumb.
#define FORCE_UPDATE_MAP_CACHE_IN_RELEASE TRUE

// Is there a confirmation prompt on exiting the entire program (not just a particular 'game')?
// Retail's way was to depend on the game being windowed or not, this setting applies regardless of that.
#define CONFIRMATION_PROMPT_TO_EXIT_PROGRAM FALSE

// Value to force 'MaxCameraHeight' in GameData.ini to during cinematic cutscenes, often in campaign maps.
// They were made with a specific max camera height in mind, and changing it can affect what's shown significantly.
// This is the number to change the setting to, not a bool.
// A value of 0 will mean this feature is unused.
// For mods, see the value in their GameData.ini in case a different choice is used from retail and use the
// default seen there for this setting before making your own changes.
// (retail choice is 310)
#define FORCE_CINEMATIC_MAX_CAMERA_HEIGHT 310

// Whether the 'AudioFootprintInBytes' setting from AudioSettings.ini is forced to a higher value if it's using
// below a certain value (128 Megabytes) - applies to retail's 5 MB. Setting to 'FALSE' just uses what's in the
// file like retail.
// There is a poorly understood crash related to long-running games with a large variety of sounds, often
// during modded games, often an inner crash in mss32.dll (miles sound system library) with not much helpful
// in the stack trace. Sometimes some faint source from this codebase is involved, often in
// MilesAudioManager.cpp - 'releaseMilesHandles' (case PAT_3DSample -> AIL_stop_3D_sample), though data in
// vars during the exception are unavailable/inaccurate even in debug mode, and it's even difficult to tell
// if the trace is accurate.
// Crashes can be observed from this in Rise of the Reds and Firestorm.
// The crash appears to occur when the audio cache is full and the game is releasing sounds to make room for
// new ones to load from requests to play sounds not in the cache - this can coincide with an rare unit
// firing or using a support power (infrequent enough). The crash can even be observed by leaving the
// Firestorm mod's shell map running for hours in base TheSuperHackers builds, as well as retail, though this
// could be caused by other bugs since fixed by TheSuperHackers build.
// Maybe something to do with how threading works in MilesAudioManager is responsible, like a race condition:
// two threads tring to release/use the same memory at the same time causing the crash?
// As a quick fix, changing 'AudioFootprintInBytes' from retail's 5 MB to 128 MB appears to stop the crash -
// the shell map scenario above did not crash for a solid 8 hours with this change applied.
// This constant enforces the minimum of '128 MB' used over the config value if needed so the crash is prevented.
#define FORCE_HIGHER_AUDIO_CACHE_SIZE TRUE

// Context: shroud generation is a mechanic in Generals that the retail game never used, but has
// been used by some mods like ProGen (stealth general radar van upgrade, ActiveShroudUpgrade in the INI).
// It's just like the gap generator from Red Alert: turns territory back into shroud from the enemy perspective.
// ---
// This constant changes retail behavior so that shroud generated by active shrouders (AKA jammers) isn't persistent.
// That is, territory behind a moving jammer doesn't remain shrouded like the jammer is painting the map black
// with shroud and leaving it behind as it moves.
// The shroud-cloud appears to follow the jammer around and disappears if the jammer is destroyed.
// Also note that permanent map reveals will always go through jam-induced shroud just like retail if this is FALSE.
#define PARTITIONMANAGER_SHROUD_NONPERSISTENT TRUE

// This constant adds separation between jammable and unjammable looks - cells looked at that lose precedence
// to shroud generators (jammable) or not (unjammable). Every object has its current look radius as jammable, and
// another smaller radius as unjammable. This means that an object has to move further into shrouded territory
// for its 'unjammable' radius to move in and start revealing it.
// This mechanic also allows temporary map reveals like CIA intelligence or satellite hacks to not reveal jammed territory.
// Kindof defeats the purpose if they did - jamming should force you to get up close & personal.
// Toggling this will break save compatibility with saves made with a different setting.
#define PARTITIONMANAGER_ADVANCED_SHROUD_MECHANICS FALSE

// Experimental idea mainly in PartitionManager.h/.cpp to store unlook-queue-items per cell instead of per
// whole circular-area look. Idea is for an area within a radar jammer to revert back to shroud faster than a
// typical unlook, but this seems to add a lot of overhead (significant lag) as of testing under debug mode,
// still the case for release mode to a great extent.
// Toggling this will break save compatibility with saves made with a different setting.
// Only tested with 'PARTITIONMANAGER_ADVANCED_SHROUD_MECHANICS' set to 'TRUE'.
#define PARTITIONMANAGER_QUEUE_PER_CELL FALSE

// ----------------------------------------------------------------------------------------------------------
// Real-time time-of-day change. The time of day changes over the course of a game. Still uses the map's baked-in time to start.
// This isn't tested well, so bugs are possible (purple textures on civilian tower buildings, neutral
// artillery bases stuck in the default-fire-muzzle-flash state), but I haven't experienced any crashes
// because of this.
// When this constant is on, all models are precached for both day/night variants - under several mods, the game can
// run out of memory and crash so the game executable now has the 'LARGEADDRESSAWARE' flag - see more info in
// the repo's README.md file.
// See 'determineTimeOfDayGlobals' in GameLogic.cpp. Some decimals toward the top affect how far before/after
// counts as 'night' so that lights aren't only on immediately after midnight and on until the start of morning.
// They're tweaked to be more of an average case, but a one-size-fits-all is difficult, especially considering that most maps probably
// never gave much thought to times of day other than the one they're saved with - keep that in mind of shadows look wonky on some maps/TOD's.
// NOTE - beware of bugs observed while this is on, though they don't usually make the game unplayable - see the readme.
#define REAL_TIME_TOD_CHANGE FALSE

// TODO - edit the save/load feature to include the time-of-day at the time the game is saved?
// Leaving that out for now since the saved game would only work with builds where 'REAL_TIME_TOD_CHANGE' is in agreement.

// TODO - cloud shadows are still a strict on/off thing, so suddenly appearing/disappearing is jarring.
// I think the intent is that they're only on for non-night times since no daylight means strong cloud
// shadows would look weird. Editing these to support fading out on evening->night and slowly
// re-appearing on night->morning would probably be fine.

// Total length of the time-of-day cycle in frames. Be a number divisible by 4 for each of the 4 time-of-day
// choices for now.
// I don't know whether the 'LOGICFRAMES_PER_SECOND' or 'BaseFps' constants makes more sense to use here -
// the point is to involve the game's hard-wired '30 frames -> 1 second' for timings.
#define TOD_CYCLE_LENGTH_FRAMES (LOGICFRAMES_PER_SECOND * 60 * 15)

// Every 'x' frames, the current time of day is applied (applied to the renderer -> affects in-game).
// At 1, runs every frame (likely not necessary).
#define TOD_UPDATE_INTERVAL 10

// helper '#define' to avoid a lot of '#if... #else' replacements.
// (edits elsewhere mainly replaced 'TheGlobalData->m_timeOfDay', unlabled changes since that's straightforward)
#if !REAL_TIME_TOD_CHANGE
	#define TIME_OF_DAY_SOURCE TheGlobalData->m_timeOfDay
#else
	#define TIME_OF_DAY_SOURCE tod_model
#endif

// ----------------------------------------------------------------------------------------------------------
// Bundles of settings here for convenient access
#define GENERALS_CHALLENGE_FORCE FALSE
#define CAMPAIGN_FORCE TRUE
#define DEFAULT_GLOBAL_SKIRMISH_DIFFICULTY DIFFICULTY_HARD
#define FORCE_HUMAN_PLAYER_START_MONEY 0
#define BLOCK_SET_MONEY_SCRIPT_FOR_HUMAN_PLAYERS TRUE

#define COMPUTER_PLAYER_MONEY_SCALAR 4.5
#define REMOVE_FOG_OF_WAR FALSE
#define REMOVE_FOG_OF_WAR_ALT FALSE
#define NOOB_MODE TRUE

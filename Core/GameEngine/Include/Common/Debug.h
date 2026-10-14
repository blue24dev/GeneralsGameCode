/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
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

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Lib/Debug.h"

class AsciiString;

//MODDD NOTE - looks like this should be "RELEASE_DEBUG_LOGGING" to disable assert calls for testing
// a debug build without annoying popups, like to be able to breakpoint & show uncaught exceptions without crashing to desktop.
// Change back to retail's "NO_RELEASE_DEBUG_LOGGING" for debug mode to get assert popups again.
// ---
// TLDR: two choices here, change this const to:
//   * NO_RELEASE_DEBUG_LOGGING: assert popups in debug mode
//   * RELEASE_DEBUG_LOGGING: no assert popups in debug mode (moreso like playing in retail)
// (release mode unaffected by this entirely)
#define RELEASE_DEBUG_LOGGING

//MODDD - forcing this off for now.
#define NO_DEBUG_CRC
#define DISABLE_ALLOW_DEBUG_UTILS

#ifdef RELEASE_DEBUG_LOGGING  ///< Creates a DebugLogFile.txt (No I or D) with all the debug log goodness.  Good for startup problems.
//MODDD - no to these for now
//	#define ALLOW_DEBUG_UTILS 1
//	#define DEBUG_LOGGING 1
	#define DISABLE_DEBUG_CRASHING 1
	#define DISABLE_DEBUG_STACKTRACE 1
	#define DISABLE_DEBUG_PROFILE 1
#endif

//MODDD
// If on, some features normally only for debug builds are enabled for release builds.
// Not every idea is covered by this - see 'GameMemory.h'.
// Also, heed include order - see 'PreRTS.h', the 'GameCommon.h' include covers 'GameDefines.h' where most macro settings
// are, but 'AsciiString.h' is included before 'GameCommon.h' -> things in 'GameDefines.h' do not reach 'AsciiString.h'.
#define DEBUG_HELP_FOR_RELEASE 1

#ifdef ALLOW_DEBUG_UTILS

	// Initializes the debug utilities for an application that uses the game engine.
	DEBUG_EXTERN_C void GameDebugInit(int flags);

	#define GAME_DEBUG_INIT(f)			do { GameDebugInit(f); } while (0)

#else

	#define GAME_DEBUG_INIT(f)			((void)0)

#endif

DEBUG_EXTERN_C void ReleaseCrash(const char* reason);
DEBUG_EXTERN_C void ReleaseCrashLocalized(const AsciiString& p, const AsciiString& m);

#define RELEASE_CRASH(m)				do { ReleaseCrash(m); } while (0)
#define RELEASE_CRASHLOCALIZED(p, m)		do { ReleaseCrashLocalized(p, m); } while (0)

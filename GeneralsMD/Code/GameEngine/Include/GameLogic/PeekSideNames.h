//MODDD - new file
// Only relevant if the 'CAMPAIGN_FORCE' constant is on.
// Offers custom 'SidesDataChunk' & needed parsers to find the names of all sides (players to generate on
// running a game) within a map's file data.
// Has a few utility methods and exposed result variables near the beginning.
// Setting 'peekedSideNamesSoftCount' back to 0 before expected parsing is run is recommended.

#pragma once

#if CAMPAIGN_FORCE
#include "Common/AsciiString.h"
#include "Common/UnicodeString.h"
#include "Common/STLTypedefs.h"

class AsciiString;
class DataChunkInput;
struct DataChunkInfo;

namespace PeekSideNames
{
	// refer to result variables as needed, mainly after parsing is run
	extern AsciiString peekedSideNames[MAX_PLAYER_COUNT];
	extern Int peekedSideNamesSoftCount;

	// external interface
	Bool ParseSidesDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);
	Int getSlotPlayerCountSuggestedByPeekedSideNames();

	// internals (likely not useful to call from other places - think of as private)
	Int indexOfNameInPeekedSideNames(const AsciiString& targetName);
	Bool ParseScriptsDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);
	Bool ParseScriptListDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);
	Bool ParseGroupDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);
	Bool ParseScript(DataChunkInput &file, unsigned short version);
	Bool ParseScriptFromListDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);
	Bool ParseScriptFromGroupDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);
	Bool ParseOrConditionDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);
	Bool ParseConditionDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);
	void ReadParameter(DataChunkInput &file);
	void ParseAction(DataChunkInput &file, DataChunkInfo *info, void *userData);
	Bool ParseActionDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);
	Bool ParseActionFalseDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);

}
#endif

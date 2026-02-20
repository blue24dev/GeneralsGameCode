//MODDD - new file

#pragma once

#if CAMPAIGN_FORCE
#include "Common/AsciiString.h"
#include "Common/UnicodeString.h"
#include "Common/STLTypedefs.h"

namespace PeekSideNames
{	
	extern AsciiString peekedSideNames[MAX_PLAYER_COUNT];
	extern Int peekedSideNamesSoftCount;

	Int indexOfNameInPeekedSideNames(const AsciiString& targetName);
	Int getSlotPlayerCountSuggestedByPeekedSideNames();

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

	Bool ParseSidesDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);
}
#endif

//MODDD - new file

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#if CAMPAIGN_FORCE
#include "Common/DataChunk.h"
#include "Common/WellKnownKeys.h"
#include "GameLogic/PeekSideNames.h"
#include "GameLogic/Scripts.h"

namespace PeekSideNames
{	
	// Copied from SidesList.cpp
	static const Int K_SIDES_DATA_VERSION_1 = 1;
	static const Int K_SIDES_DATA_VERSION_2 = 2;	// includes Team list.
	static const Int K_SIDES_DATA_VERSION_3 = 3;	// includes Team list.
	// Copied from ScriptEngine/Scripts.cpp
	enum { K_SCRIPT_LIST_DATA_VERSION_1 = 1,
				K_SCRIPT_GROUP_DATA_VERSION_1 = 1,
				K_SCRIPT_GROUP_DATA_VERSION_2 = 2,
				K_SCRIPT_DATA_VERSION_1 = 1,
				K_SCRIPT_DATA_VERSION_2 = 2,
				K_SCRIPT_OR_CONDITION_DATA_VERSION_1=1,
				K_SCRIPT_ACTION_VERSION_1 = 1,
				K_SCRIPT_ACTION_VERSION_2 = 2,
				K_SCRIPT_CONDITION_VERSION_1 = 1,
				K_SCRIPT_CONDITION_VERSION_2 = 2,
				K_SCRIPT_CONDITION_VERSION_3 = 3,
				K_SCRIPT_CONDITION_VERSION_4 = 4,
				K_SCRIPTS_DATA_VERSION_1,
				end_of_the_enumeration
	};


	AsciiString peekedSideNames[MAX_PLAYER_COUNT];
	Int peekedSideNamesSoftCount;


	Int indexOfNameInPeekedSideNames(const AsciiString& targetName)
	{
		for (Int i_peekedSideName = 0; i_peekedSideName < peekedSideNamesSoftCount; i_peekedSideName++)
		{
			const AsciiString& peekedSideName = peekedSideNames[i_peekedSideName];
			if (peekedSideName == targetName)
			{
				// Found it
				return i_peekedSideName;
			}
		}
		// not found
		return -1;
	}

	Int getSlotPlayerCountSuggestedByPeekedSideNames()
	{
		Int slotPlayerCount = 1;
		Bool foundAnyNamedPlayers = false;
		for (int i = 1; i < MAX_SLOTS; i++)
		{
			AsciiString targetPlayerName;
			targetPlayerName.format("player%d", i);

			// Is this particular name found in the list of peeked side names?
			Int index = indexOfNameInPeekedSideNames(targetPlayerName);
			if (index != -1)
			{
				// Found it somewhere, all that matters
				++slotPlayerCount;
				foundAnyNamedPlayers = true;
			}
			else
			{
				// this name wasn't found - no need to look for further "player#" names
				break;
			}
		}

		if (!foundAnyNamedPlayers)
		{
			// Force 0 to mean this didn't lead anywhere.
			// The 'slotPlayerCount' default of 1 is so that finding the first-checked "player1" bumps 'slotPlayerCount'
			// to 2.
			return 0;
		}
		return slotPlayerCount;
	}


	Bool ParseScriptsDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData)
	{
		file.registerParser( "ScriptList", info->label, ParseScriptListDataChunk );
		if (file.parse(nullptr)) {
			return true;
		}
		return false;
	}

	Bool ParseScriptListDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData)
	{
		file.registerParser( "Script", info->label, ParseScriptFromListDataChunk );
		file.registerParser( "ScriptGroup", info->label, ParseGroupDataChunk );
		return file.parse(nullptr);
	}
	
	Bool ParseGroupDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData)
	{
		file.readAsciiString();
		file.readByte();
		if (info->version == K_SCRIPT_GROUP_DATA_VERSION_2) {
			file.readByte();
		}
		file.registerParser( "Script", info->label, ParseScriptFromGroupDataChunk );
		return file.parse(nullptr);
	}
	
	Bool ParseScript(DataChunkInput &file, unsigned short version)
	{
		file.readAsciiString();
		file.readAsciiString();
		file.readAsciiString();
		file.readAsciiString();

		file.readByte();
		file.readByte();
		file.readByte();
		file.readByte();
		file.readByte();
		file.readByte();
		if (version>=K_SCRIPT_DATA_VERSION_2) {
			file.readInt();
		}
		file.registerParser( "OrCondition", "Script", ParseOrConditionDataChunk );
		file.registerParser( "ScriptAction",  "Script", ParseActionDataChunk );
		file.registerParser( "ScriptActionFalse",  "Script", ParseActionFalseDataChunk );
		if (! file.parse(nullptr) )
		{
			return false;
		}
		DEBUG_ASSERTCRASH(file.atEndOfChunk(), ("Unexpected data left over."));
		return true;
	}
	
	Bool ParseScriptFromListDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData)
	{
		if(!ParseScript(file, info->version))return false;
		DEBUG_ASSERTCRASH(file.atEndOfChunk(), ("Unexpected data left over."));
		return true;
	}
	
	Bool ParseScriptFromGroupDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData)
	{
		if(!ParseScript(file, info->version))return false;
		DEBUG_ASSERTCRASH(file.atEndOfChunk(), ("Unexpected data left over."));
		return true;
	}

	Bool ParseOrConditionDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData)
	{
		file.registerParser( "Condition", info->label, ParseConditionDataChunk );
		return file.parse(nullptr);
	}

	Bool ParseConditionDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData)
	{
		file.readInt();
		if (info->version >= K_SCRIPT_CONDITION_VERSION_4) {
			file.readNameKey();
		}
		Int m_numParms =file.readInt();
		Int i;
		for (i=0; i<m_numParms; i++)
		{
			ReadParameter(file);
		}

		/*
		if (file.getChunkVersion() < K_SCRIPT_CONDITION_VERSION_2) {
			// ...
		}
		*/

		DEBUG_ASSERTCRASH(file.atEndOfChunk(), ("Unexpected data left over."));
		return true;
	}

	void ReadParameter(DataChunkInput &file)
	{
		Int parameterType = file.readInt();
		if (parameterType == Parameter::COORD3D) {
			file.readReal();
			file.readReal();
			file.readReal();
		}
		else
		{
			file.readInt();
			file.readReal();
			file.readAsciiString();
		}
	}
	
	void ParseAction(DataChunkInput &file, DataChunkInfo *info, void *userData)
	{
		/*enum ScriptActionType actionType = (enum ScriptActionType)*/file.readInt();

		if (info->version >= K_SCRIPT_ACTION_VERSION_2) {
			file.readNameKey();
		}
		Int numParms =file.readInt();
		Int i;
		for (i=0; i<numParms; i++)
		{
			ReadParameter(file);
		}

		DEBUG_ASSERTCRASH(file.atEndOfChunk(), ("Unexpected data left over."));
	}

	Bool ParseActionDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData)
	{
		ParseAction(file, info, userData);
		DEBUG_ASSERTCRASH(file.atEndOfChunk(), ("Unexpected data left over."));
		return true;
	}

	Bool ParseActionFalseDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData)
	{
		ParseAction(file, info, userData);
		DEBUG_ASSERTCRASH(file.atEndOfChunk(), ("Unexpected data left over."));
		return true;
	}

	Bool ParseSidesDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData)
	{
		Int count = file.readInt();
		peekedSideNamesSoftCount = count;

		Int i, j;
		for (i=0; i<count; i++)
		{
			if (i >= MAX_PLAYER_COUNT) break;
			Dict d =  file.readDict();

			peekedSideNames[i] = d.getAsciiString(TheKey_playerName);

			// read through the rest anyway to progress to the next side
			// (HOLY-)
			Int count = file.readInt();
			for (j=0; j<count; j++)
			{
				file.readAsciiString();
				file.readAsciiString();
				file.readReal();
				file.readReal();
				file.readReal();
				file.readReal();
				file.readByte();
				file.readInt();
				if (info->version >= K_SIDES_DATA_VERSION_3)
				{
					file.readAsciiString();
					file.readInt();
					file.readByte();
					file.readByte();
					file.readByte();
				}
			}
		}
		if (info->version >= K_SIDES_DATA_VERSION_2)
		{
			count = file.readInt();
			for (i=0; i<count; i++)
			{
				file.readDict();
			}
		}

		file.registerParser( "PlayerScriptsList", info->label, ParseScriptsDataChunk );
		if (!file.parse(nullptr)) {
			throw(ERROR_CORRUPT_FILE_FORMAT);
		}

		DEBUG_ASSERTCRASH(file.atEndOfChunk(), ("Incorrect data file length."));
		return true;
	}
}
#endif

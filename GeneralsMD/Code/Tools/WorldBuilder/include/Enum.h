//MODDD - new file for enum's common to the worldbuilder, include as needed
// (todo: header stuff)

#pragma once

//MODDD - moved from BorderTool.h, renamed from 'ModificationType'.
enum BorderModificationType CPP_11(: Int)
{
	BORDERMOD_NONE = -1,
	BORDERMOD_INVALID = 0,
	BORDERMOD_UP,
	BORDERMOD_FREE,
	BORDERMOD_RIGHT
};

//MODDD - new file for enum's common to the worldbuilder, include as needed
// (todo: header stuff)
// ---
#pragma once

//MODDD - moved from BorderTool.h, renamed from 'ModificationType'.
enum BorderModificationType CPP_11(: Int) {
	BORDERMOD_TYPE_NONE = -1,
	BORDERMOD_TYPE_INVALID,
	BORDERMOD_TYPE_UP,
	BORDERMOD_TYPE_FREE,
	BORDERMOD_TYPE_RIGHT
};

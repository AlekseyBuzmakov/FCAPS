// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef COMPARERESULTS_H_INCLUDED
#define COMPARERESULTS_H_INCLUDED
////////////////////////////////////////////////////////////////

// Possible results of comparison of two elements in a partial order
enum TCompareResult {
	// The first elem is less general than the second
	CR_LessGeneral = 1 << 0,
	// Exactly the same elems
	CR_Equal = 1 << 1,
	// The first elem is more general than the second
	CR_MoreGeneral = 1 << 2,
	// Elems cannot be compared
	CR_Incomparable = 1 << 3,

	CR_EnumCount // Fictive constant to count size of enum
};

const DWORD CR_AllResults = CR_Equal | CR_LessGeneral | CR_MoreGeneral;
const DWORD CR_LessOrEqual = CR_Equal | CR_LessGeneral;
const DWORD CR_MoreOrEqual = CR_Equal | CR_MoreGeneral;

////////////////////////////////////////////////////////////////////
#endif // COMPARERESULTS_H_INCLUDED

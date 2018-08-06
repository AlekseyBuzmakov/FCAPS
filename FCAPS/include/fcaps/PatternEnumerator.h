// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: A simple interface for enumerating patterns. Every pattern is described by its image.

#ifndef PATTERNENUMERATOR_H_INCLUDED
#define PATTERNENUMERATOR_H_INCLUDED

#include <common.h>
#include <fcaps/BasicTypes.h>
#include <fcaps/Extent.h>

////////////////////////////////////////////////////////////////////////

const char PatternEnumeratorModuleType[] = "PatternEnumeratorModules";

// Enumeration of possbile usage of the current pattern
enum TCurrentPatternUsage {
    // The super-pattern could be useful
    CPU_Expand = 0,
    // Any super-pattern is useless
    CPU_Reject,

    CPU_EnumCount
};

// Shows the relation of the next pattern to the previous one.
enum TNextPatternStatut {
	// No next pattern is found. The end of the procedure
	NPS_None = 0,
	// New pattern has no relation to the previous one
	NPS_New,
	// New pattern is a more concrete pattern than the previous one
	NPS_Expanded,

	NPS_EnumCount
};

interface IPatternEnumerator : public virtual IObject {
    // Returns the next pattern.
    virtual TNextPatternStatut GetNextPattern( TCurrentPatternUsage usage, CPatternImage& pattern ) = 0;
    // Clear memory in the pattern
    virtual void ClearMemory( CPatternImage& pattern ) = 0;
};

#endif // PATTERNENUMERATOR_H_INCLUDED

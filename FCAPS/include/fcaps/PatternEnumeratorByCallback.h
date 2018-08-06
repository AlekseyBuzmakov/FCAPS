// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2016, v0.7

// Author: Aleksey Buzmakov
// Description: A simple interface for enumerating patterns through a callback function. Every pattern is described by only its image.

#ifndef PATTERNENUMERATORBYCALLBACK_H_INCLUDED
#define PATTERNENUMERATORBYCALLBACK_H_INCLUDED

#include <fcaps/PatternEnumerator.h>

#include <common.h>
#include <fcaps/BasicTypes.h>

////////////////////////////////////////////////////////////////////////

const char PatternEnumeratorByCallbackModuleType[] = "PatternEnumeratorByCallbackModules";

////////////////////////////////////////////////////////////////////////

// Any data that should be passed to callback.
typedef void* PECDataRef;

/**
 * A type of function that is used for reporting newly found patterns
 * @param pattern is the reported pattern
 * @return is whether the pattern can be further expanded
 */
typedef bool (*PECReportPatternCallback)( PECDataRef data, const CPatternImage& ptrn );

interface IPatternEnumeratorByCallback : public virtual IObject {
	/**
	 * Run the algorithm that will envoke callback every time a new pattern is found
	 * @param callback is the callback function that is run every time a new pattern is found
	 * @param data is an arbitrary data that is directly passed to the callback function
	 */
	virtual void Run( PECReportPatternCallback callback, PECDataRef data = 0 ) = 0;
};

#endif // PATTERNENUMERATORBYCALLBACK_H_INCLUDED

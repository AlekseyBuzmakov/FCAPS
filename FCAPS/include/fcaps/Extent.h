// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2018, v0.7

// Author: Aleksey Buzmakov
// Description: An interface for patterns that are extents

#ifndef EXTENT_H_INCLUDED
#define EXTENT_H_INCLUDED

#include <common.h>
#include <fcaps/BasicTypes.h>

////////////////////////////////////////////////////////////////////////

// A structure for encoding the image of a pattern
struct CPatternImage {
    typedef unsigned int TPatternId;

	// Unique id of the pattern
	TPatternId PatternId;
	// The cardinality of the image, known as support.
    int ImageSize;
	// The image (the set of objects) of the pattern.
    int* Objects;

	CPatternImage() :
		PatternId( -1 ),
		ImageSize( 0 ),
		Objects( 0 ) {}
};

interface IExtent : public virtual IObject {
    // Get number of objects in the extent
    virtual DWORD Size() const = 0;
    // Get the list of objects
    // The memory for objects can be allocated in the method. After usage it should be freed.
    virtual void GetExtent( CPatternImage& extent ) const = 0;
    // Free the memeory allocated with the previous method 
    virtual void ClearMemory( CPatternImage& extent ) const = 0;
};

#endif // EXTENT_H_INCLUDED

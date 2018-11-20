// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2018, v0.7

// Author: Aleksey Buzmakov
// Description: A simple interface for accesing attributes as their extents by the number 

#ifndef PATTERNENUMERATOR_H_INCLUDED
#define PATTERNENUMERATOR_H_INCLUDED

#include <common.h>
#include <fcaps/BasicTypes.h>
#include <fcaps/Extent.h>

////////////////////////////////////////////////////////////////////////

const char ContextAttributesModuleType[] = "ContextAttributesModules";

interface IContextAttributes : public virtual IObject {
    // Returns the total number of objects in the dataset
    virtual int GetObjectNumber() const = 0;
    // Check if an attribute exits
    virtual bool HasAttribute(int a) = 0;
    // Returns the attribute
    virtual void GetAttribute( int a, CPatternImage& pattern ) = 0;
    // Clear memory in the pattern
    virtual void ClearMemory( CPatternImage& pattern ) = 0;

    // Attributes iteration
    virtual int GetNextAttribute(int a) = 0;
    // Returns the number of the next (larger) attribute that is not a specification of the given attribute
    virtual int GetNextNonChildAttribute(int a) = 0;
};

#endif // PATTERNENUMERATOR_H_INCLUDED

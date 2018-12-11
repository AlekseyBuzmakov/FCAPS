// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2018, v0.7

// Author: Aleksey Buzmakov
// Description: An interface for objects that can swap to disk

#ifndef SWAPPABLE_H_INCLUDED
#define SWAPPABLE_H_INCLUDED

#include <common.h>
#include <fcaps/BasicTypes.h>

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

interface ISwappable : public virtual IObject {
    // Check if the object is swapped to disk
    virtual bool IsSwapped() const = 0;
    // Swap object to disk
    virtual void Swap() = 0;
};

////////////////////////////////////////////////////////////////////////
#endif // SWAPPABLE_H_INCLUDED

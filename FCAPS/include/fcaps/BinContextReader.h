// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2020, v0.7

// Author: Aleksey Buzmakov
// Description: A simple interface for accesing objects encoded sets of attributes

#ifndef PATTERNENUMERATOR_H_INCLUDED
#define PATTERNENUMERATOR_H_INCLUDED

#include <common.h>
#include <fcaps/BasicTypes.h>
#include <fcaps/Extent.h>

////////////////////////////////////////////////////////////////////////

const char BinContextReaderModuleType[] = "BinContextReaderModules";

interface IBinContextReader : public virtual IObject {
	typedef int TAttributeID;
	struct CObjectIntent {
		// The number of attributes in the intent
		int Size;
		// The attributes
		TAttributeID* Attributes;

	};
    // Returns the total number of objects in the dataset
    virtual int GetObjectNumber() const = 0;

    // Check if an attribute exits
    virtual bool HasAttribute(TAttributeID a) const = 0;
    // Get the name of the attribute.
    //   Returns the JSON string, i.e., in the form "NAME" including the parethesis
    virtual JSON GetAttributeName(TAttributeID a) const = 0;
    // Get the number of objects including the attribute a
    virtual int GetSupport(TAttributeID a) const = 0;

    // Starts the object iteration procedure
    virtual void Start() = 0;
    // Returns the number of attributes for the next object, or negative value if no object is available
    virtual int GetNextObjectIntentSize() = 0;
    // Returns the next object intent
    //   Requires that 
    //   * res.Size >= size, where size is the previous call to GetNextObjectIntentSize() 
    //   * res.Attributes is allocated and contains at least res.Size elements
    virtual void GetNextObject(CObjectIntent& ) = 0;
};

#endif // PATTERNENUMERATOR_H_INCLUDED

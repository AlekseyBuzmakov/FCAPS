// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Interface of an object that describes a partial order. To be used by CPartialOrderPatternManager.

#ifndef PARTIALORDERELEMENTSCOMPARATOR_H_INCLUDED
#define PARTIALORDERELEMENTSCOMPARATOR_H_INCLUDED

#include <fcaps/BasicTypes.h>
#include <fcaps/CompareResults.h>

///////////////////////////////////////////////////////////////////

const char PartialOrderElementsComparatorModuleType[] = "PartialOrderElementsComparatorModules";

///////////////////////////////////////////////////////////////////

// A unique identifier of this element type. GUID is suggested.
typedef const char* TPartialOrderType;

// Object-data for pattern.
interface IPartialOrderElement : public virtual IObject {
	// Type of that pattern.
	virtual TPartialOrderType GetType() const = 0;
	// Is the descriptor most general, i.e. it is subsumed by other elements.
	virtual bool IsMostGeneral() const = 0;
};

// Interface to the object to compare patterns.
interface IPartialOrderElementsComparator : public virtual IObject {
    // A set of elements. The memory is not controlled and should be explicitly removed by FreeElement function.
	struct CElementSet {
        int Count;
        const IPartialOrderElement** Elements;
	};

	// Get types of partial order object works with.
	virtual TPartialOrderType GetElementType() const = 0;

    // Load/Save elements from/in JSON
	virtual JSON SaveElement( const IPartialOrderElement* ) const = 0;
	//  It is possible that during loading several smaller elements one would like to load,
	//   e.g., when applying a projection.
	virtual void LoadElements( const JSON&, CElementSet& ) = 0;

	// Returns a set of IPartialOrderElement, which are less general
	//  then both arguments but the most specific among all posibilities.
	virtual void FindAllParents( const IPartialOrderElement* first, const IPartialOrderElement* second,
		CElementSet& parents ) const = 0;

	// Compare two elements.
	//  All results that do not from interestingResults are returning as CR_Incomparable.
	virtual TCompareResult Compare( DWORD interestingResults,
		const IPartialOrderElement* first, const IPartialOrderElement* second ) const = 0;

	// Free memory of the element @param el
	virtual void FreeElement( const IPartialOrderElement* el) = 0;
	// Free memory of the array with elemnts
	virtual void FreeElementSet( const CElementSet& set ) = 0;
	// Free elemnts and the element set
	void FreeElements(const CElementSet& set ) {
        for( int i = 0; i < set.Count; ++i) FreeElement( set.Elements[i] );
        FreeElementSet( set );
	}
};

////////////////////////////////////////////////////////////////////

#endif // PARTIALORDERELEMENTSCOMPARATOR_H_INCLUDED

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef PARTIALORDERPATTERNDESCRIPTOR_H
#define PARTIALORDERPATTERNDESCRIPTOR_H

#include <common.h>

#include <fcaps/PatternManager.h>
#include <fcaps/Module.h>
#include <fcaps/ModuleTools.h>

#include <ListWrapper.h>

#include <vector>

// A unique identifier of this element type. GUID is suggested.
typedef char* TPartialOrderType;

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
	virtual TPartialOrderType GetType() const = 0;

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

class CPartialOrderPatternDescriptor: public IPatternDescriptor {
public:
	typedef CList<const IPartialOrderElement*> CElementSet;
public:
	CPartialOrderPatternDescriptor( const CSharedPtr<IPartialOrderElementsComparator>& _elemsCmp ) :
		elemsCmp( _elemsCmp ) { assert( elemsCmp != 0 ); }
	virtual ~CPartialOrderPatternDescriptor()
		{}

	// Methods of IPatternDescriptor
	virtual TPatternType GetType() const
		{ return PT_PartialOrder; }
	virtual bool IsMostGeneral() const
		{ return elements.IsEmpty(); }
	virtual size_t Hash() const
		{ return 0; /*TODO*/ }

	// Owns methods
	CElementSet& GetElements()
		{ return elements; }
	const CElementSet& GetElements() const
		{ return elements; }
	// Add a new ellement to the set.
	void AddElement( const IPartialOrderElement* el );

private:
    // Cmp for removing elements
	CSharedPtr<IPartialOrderElementsComparator> elemsCmp;
	// A set of associated elements
	CElementSet elements;
};

///////////////////////////////////////////////////////////////

class CPartialOrderPatternManager : public IPatternManager, public IModule {
public:
	typedef CPartialOrderPatternDescriptor::CElementSet CElementSet;
public:
	CPartialOrderPatternManager()
		{}

	// Methods of IPatternManager
	virtual TPatternType GetPatternsType() const
		{ return PT_PartialOrder; }

	virtual const CPartialOrderPatternDescriptor* LoadObject( const JSON& );
	virtual JSON SavePattern( const IPatternDescriptor* ) const;
	virtual const CPartialOrderPatternDescriptor* LoadPattern( const JSON& );

	virtual const CPartialOrderPatternDescriptor* CalculateSimilarity(
		const IPatternDescriptor* first, const IPatternDescriptor* second );
	virtual TCompareResult Compare(
		const IPatternDescriptor* first, const IPatternDescriptor* second,
		DWORD interestingResults = CR_AllResults, DWORD possibleResults = CR_AllResults | CR_Incomparable );

	virtual void FreePattern( const IPatternDescriptor * );

	virtual void Write( const IPatternDescriptor* pattern, std::ostream& dst ) const;

	// Initialization
	//  _strElemenstCmp -- the comparator for string elements.
	void Initialize( const CSharedPtr<IPartialOrderElementsComparator>& _elemsCmp );

protected:
	// Methods of IPatternManager
	virtual CPartialOrderPatternDescriptor* NewPattern() const;

private:
	CSharedPtr<IPartialOrderElementsComparator> elemsCmp;

	bool addElementToPattern( const IPartialOrderElement* el, CPartialOrderPatternDescriptor& ptrn ) const;
};

#endif // PARTIALORDERPATTERNDESCRIPTOR_H

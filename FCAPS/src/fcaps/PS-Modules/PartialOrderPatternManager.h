// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef PARTIALORDERPATTERNDESCRIPTOR_H
#define PARTIALORDERPATTERNDESCRIPTOR_H

#include <common.h>

#include <fcaps/PatternManager.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <fcaps/PartialOrderElementsComparator.h>

#include <ListWrapper.h>

////////////////////////////////////////////////////////////////////

const char PartialOrderPatternManager[] = "PartialOrderPatternManagerModule";

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

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return PatternManagerModuleType;}
	static const char* const Name()
		{ return PartialOrderPatternManager; }
	static const char* const Desc()
		{ return "{}"; }

	// Initialization
	//  _strElemenstCmp -- the comparator for string elements.
	void Initialize( const CSharedPtr<IPartialOrderElementsComparator>& _elemsCmp );

protected:
	// Methods of IPatternManager
	virtual CPartialOrderPatternDescriptor* NewPattern() const;

private:
	static const CModuleRegistrar<CPartialOrderPatternManager> registrar;

	CSharedPtr<IPartialOrderElementsComparator> elemsCmp;

	bool addElementToPattern( const IPartialOrderElement* el, CPartialOrderPatternDescriptor& ptrn ) const;
};

#endif // PARTIALORDERPATTERNDESCRIPTOR_H

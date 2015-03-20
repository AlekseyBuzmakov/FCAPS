#ifndef PARTIALORDERPATTERNDESCRIPTOR_H
#define PARTIALORDERPATTERNDESCRIPTOR_H

#include <PatternManager.h>
#include <ListWrapper.h>

#include <vector>

enum TPartialOrderType {
	// String with general semilattice elements
	POT_GeneralStrings,
	// String with simble types elements
	POT_SimpleStrings,

	POT_EnumCount
};

// Object-data for pattern.
interface IPartialOrderElement : public virtual IObject {
	// Type of that pattern.
	virtual TPartialOrderType GetType() const = 0;
	// Is the descriptor most general, i.e. describes all the objects.
	virtual bool IsMostGeneral() const = 0;
};

// Interface to the object to compare patterns.
interface IPartialOrderElementsComparator : public virtual IObject {
	typedef CList< CSharedPtr<IPartialOrderElement> > CElementSet;

	// Get types of partial order object works with.
	virtual TPartialOrderType GetType() const = 0;

	// Preprocess partial order object, which is an object description.
	//  Used, for example, in projections.
	//  After a projection the el can be broken on several part, if so parts is not empty.
	virtual void PreprocessElement( const CSharedPtr<IPartialOrderElement>& el, CElementSet& parts ) const
		{};

	// Returns a set of IPartialOrderElement, which are less general
	//  then both arguments but the most specific among all posibilities.
	virtual void FindAllParents(
		const CSharedPtr<IPartialOrderElement>& first, const CSharedPtr<IPartialOrderElement>& second,
		CElementSet& parents ) const = 0;
	// Compare two elements.
	//  All results that do not from interestingResults are returning as CR_Incomparable.
	virtual TCompareResult Compare( DWORD interestingResults,
		const CSharedPtr<IPartialOrderElement>& first, const CSharedPtr<IPartialOrderElement>& second ) const = 0;

	// Write the pattern to the stream.
	virtual void Write( const CSharedPtr<IPartialOrderElement>& element, std::ostream& dst ) const = 0;
};

////////////////////////////////////////////////////////////////////

class CPartialOrderPatternDescriptor: public IPatternDescriptor {
public:
	typedef IPartialOrderElementsComparator::CElementSet CElementSet;
public:
	CPartialOrderPatternDescriptor()
		{}
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
	void AddElement( const CSharedPtr<IPartialOrderElement>& el );

private:
	CElementSet elements;
};

class CPartialOrderPatternManager : public IPatternManager {
public:
	typedef CPartialOrderPatternDescriptor::CElementSet CElementSet;
public:
	CPartialOrderPatternManager()
		{}

	// Methods of IPatternManager
	virtual TPatternType GetPatternsType() const
		{ return PT_PartialOrder; }
	virtual void PreprocessObjectDescription( const CSharedPtr<IPatternDescriptor>& desc ) const;
	virtual CSharedPtr<IPatternDescriptor> CalculateSimilarity(
		const CSharedPtr<IPatternDescriptor>& first, const CSharedPtr<IPatternDescriptor>& second ) const;
	virtual TCompareResult Compare(
		const CSharedPtr<IPatternDescriptor>& first, const CSharedPtr<IPatternDescriptor>& second,
		DWORD interestingResults = CR_AllResults, DWORD possibleResults = CR_AllResults | CR_Incomparable ) const;
	virtual void Write( const CSharedPtr<IPatternDescriptor>& pattern, std::ostream& dst ) const;

	// Initialization
	//  _strElemenstCmp -- the comparator for string elements.
	void Initialize( const CSharedPtr<IPartialOrderElementsComparator>& _elemsCmp );

protected:
	// Methods of IPatternManager
	virtual CPartialOrderPatternDescriptor* NewPattern() const;

private:
	CSharedPtr<IPartialOrderElementsComparator> elemsCmp;

	void addElementToPattern( const CSharedPtr<IPartialOrderElement>& el, CPartialOrderPatternDescriptor& ptrn ) const;
};

#endif // PARTIALORDERPATTERNDESCRIPTOR_H

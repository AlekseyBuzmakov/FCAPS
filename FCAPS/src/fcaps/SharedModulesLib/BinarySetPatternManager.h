// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Realisation of binary set pattern

#ifndef BINARYSETPATTERNDESCRIPTOR_H
#define BINARYSETPATTERNDESCRIPTOR_H

#include <common.h>

#include <fcaps/Module.h>
#include <ModuleTools.h>
#include <fcaps/PatternManager.h>

#include <ListWrapper.h>

#include <vector>

////////////////////////////////////////////////////////////////////

const char BinarySetDescriptorsComparator[] = "BinarySetJoinPatternManagerModule";
const char UnionBinarySetDescriptorsComparator[] = "BinarySetUnionPatternManagerModule";

////////////////////////////////////////////////////////////////////

// Flags on modes of writing intents
// Use indices
const DWORD BSDC_UseInds = 1;
// Use string names
const DWORD BSDC_UseNames = 2;

////////////////////////////////////////////////////////////////////

class CBinarySetDescriptorsComparator;

class CBinarySetPatternDescriptor : public IPatternDescriptor {
public:
	typedef CList<DWORD> CAttrsList;

public:
	CBinarySetPatternDescriptor();
	~CBinarySetPatternDescriptor();

	//Methods of IPatternDescriptor
	virtual TPatternType GetType() const
		{ return PT_BinarySet; }
	virtual bool IsMostGeneral() const
		{ return attribsSet.IsEmpty(); }
	virtual size_t Hash() const;

	// Methods of class
	// Add attribute to the set.
	void AddNextAttribNumber( DWORD attribNum );
	//  only in the case of sorted array.
	void AddSortedNextAttribNumber( DWORD attribNum );
	// Add a set of attributes to the set.
	void AddList( const CAttrsList& listToAdd );

	CAttrsList& GetAttribs()
		{ return attribsSet; }
	const CAttrsList& GetAttribs() const
		{ return attribsSet; }

private:
	// List of numbers of attributes
	CAttrsList attribsSet;
	mutable size_t hash;
};

inline void CBinarySetPatternDescriptor::AddSortedNextAttribNumber( DWORD attribNum )
{
	if( attribsSet.IsEmpty() ) {
		attribsSet.PushBack( attribNum );
		hash = 0;
		return;
	}
	assert( attribNum >= attribsSet.Back() );

	if( attribNum == attribsSet.Back() ) {
		return;
	}
	attribsSet.PushBack( attribNum );
	hash = 0;
}

////////////////////////////////////////////////////////////////////

class CBinarySetDescriptorsComparatorBase : public IPatternManager, public IModule {
public:
	CBinarySetDescriptorsComparatorBase();

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return PatternManagerModuleType; };

	// Methods of IPatternManager.
	virtual TPatternType GetPatternsType() const
		{ return PT_BinarySet; }

	virtual const CBinarySetPatternDescriptor* LoadObject( const JSON& json );
	virtual JSON SavePattern( const IPatternDescriptor* ptrn ) const;
	virtual const CBinarySetPatternDescriptor* LoadPattern( const JSON& json );

	virtual TCompareResult Compare(
		const IPatternDescriptor* first, const IPatternDescriptor* second,
		DWORD interestingResults, DWORD possibleResults );
	virtual void FreePattern( const IPatternDescriptor * );

	virtual void Write( const IPatternDescriptor* pattern, std::ostream& dst ) const;

	// Methods of Class
	// Creating a new pattern
	virtual CBinarySetPatternDescriptor* NewPattern() const
		{ return new CBinarySetPatternDescriptor; }

	// Non virtual method for comparison
	TCompareResult Compare(
		const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second,
		DWORD interestingResults, DWORD possibleResults ) const;

	// Set ids to names map, for writing proposes
	const std::vector<std::string>& GetNames() const
		{ return names; }
	void SetNames( const std::vector<std::string>& newNames )
		{ names = newNames; }

    // Set or gets flags
    DWORD GetFlags() const
        { return flags;}
    void SetFlags(DWORD f)
        {flags=f;}

protected:
	// Build Set as union of sets
	CBinarySetPatternDescriptor* UnionSets(
		 const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const;
	// Build Set as intersection of sets
	CBinarySetPatternDescriptor* IntersectSets(
		 const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const;
	// Check equality of sets
	bool IsEqualSets(
		 const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second  ) const;
	// Check Subset relation
	bool IsSubsetOf(
		 const CBinarySetPatternDescriptor& subset, const CBinarySetPatternDescriptor& set ) const;

	// Loads pattern for read/write
	CBinarySetPatternDescriptor* LoadRWPattern( const JSON& json );

private:
	static const char jsonAttrNames[];
	static const char jsonUseInds[];
	static const char jsonUseNames[];
	static const char jsonInds[];
	static const char jsonNames[];
	static const char jsonCount[];

	// Names of attributes
	std::vector<std::string> names;
	// Flags of processing BSDC_*
	DWORD flags;

	// Get Object Name
	virtual const char* getModuleName() const = 0;
	// Compare Patterns
	virtual bool isEqual( const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const = 0;
	virtual bool isLessGeneral( const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const = 0;
	// Compare patterns by size wrt possible results
	virtual TCompareResult fastCompare( DWORD firstSize, DWORD secondSize ) const = 0;

	JSON savePattern( const IPatternDescriptor* ptrn ) const;
};

////////////////////////////////////////////////////////////////////

class CBinarySetDescriptorsComparator : public CBinarySetDescriptorsComparatorBase {
public:
	// Methods of IModule
	virtual const char* const GetName() const
		{ return BinarySetDescriptorsComparator; }
	// Methods of IPatternManager.
	virtual const CBinarySetPatternDescriptor* CalculateSimilarity(
		const IPatternDescriptor* first, const IPatternDescriptor* second );

private:
	virtual const char* getModuleName() const
		{ return BinarySetDescriptorsComparator;}
	virtual bool isEqual( const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const;
	virtual bool isLessGeneral( const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const;
	virtual TCompareResult fastCompare( DWORD firstSize, DWORD secondSize ) const;
};

////////////////////////////////////////////////////////////////////

class CUnionBinarySetDescriptorsComparator : public CBinarySetDescriptorsComparatorBase {
public:
	// Methods of IModule
	virtual const char* const GetName() const
		{ return UnionBinarySetDescriptorsComparator; }
	// Methods of IPatternManager.
	virtual const CBinarySetPatternDescriptor* CalculateSimilarity(
		const IPatternDescriptor* first, const IPatternDescriptor* second );

private:
	virtual const char* getModuleName() const
		{ return UnionBinarySetDescriptorsComparator;}
	virtual bool isEqual( const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const;
	virtual bool isLessGeneral( const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const;
	virtual TCompareResult fastCompare( DWORD firstSize, DWORD secondSize ) const;
};

////////////////////////////////////////////////////////////////////
// Inline Methods
inline TCompareResult CBinarySetDescriptorsComparatorBase::Compare(
	const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second,
	DWORD interestingResults, DWORD possibleResults ) const
{
	const DWORD firstSize = first.GetAttribs().Size();
	const DWORD secondSize = second.GetAttribs().Size();
	interestingResults &= possibleResults;
	const bool isIncomparablePossible = HasAllFlags( possibleResults, CR_Incomparable );

	switch( fastCompare( firstSize, secondSize ) ) {
		case CR_Equal:
			if( !HasAllFlags( interestingResults, CR_Equal ) ) {
				return CR_Incomparable;
			}
			if( !isIncomparablePossible || isEqual( first, second ) ) {
				assert( HasAllFlags( possibleResults, CR_Equal ) );
				return CR_Equal;
			} else {
				return CR_Incomparable;
			}
			break;
		case CR_LessGeneral:
			if( !HasAllFlags( interestingResults, CR_LessGeneral ) ) {
				return CR_Incomparable;
			}
			if( !isIncomparablePossible || isLessGeneral( first, second ) ) {
				assert( HasAllFlags( possibleResults, CR_LessGeneral ) );
				return CR_LessGeneral;
			} else {
				return CR_Incomparable;
			}
			break;
		case CR_MoreGeneral:
			if( !HasAllFlags( interestingResults, CR_MoreGeneral ) ) {
				return CR_Incomparable;
			}
			if( !isIncomparablePossible || isLessGeneral( second, first ) ) {
				assert( HasAllFlags( possibleResults, CR_MoreGeneral ) );
				return CR_MoreGeneral;
			} else {
				return CR_Incomparable;
			}
			break;

		default:
			assert( false );
			return CR_Incomparable;
	}
}

#endif // BINARYSETPATTERNDESCRIPTOR_H

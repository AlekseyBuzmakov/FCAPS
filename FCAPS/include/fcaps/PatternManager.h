// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Interface of abstruct pattern

#ifndef PATTERNMANAGER_H
#define PATTERNMANAGER_H
#include <common.h>
#include <fcaps/BasicTypes.h>
#include <fcaps/CompareResults.h>
#include <fcaps/PatternDescriptor.h>

////////////////////////////////////////////////////////////////////

const char PatternManagerModuleType[] = "PatternManagerModules";

////////////////////////////////////////////////////////////////////

// Interface to the object to compare patterns.
interface IPatternManager : public virtual IObject {
	// Load object description from JSON
	virtual const IPatternDescriptor* LoadObject( const JSON& ) = 0;

	// Load/Save patterns from/in JSON
	virtual JSON SavePattern( const IPatternDescriptor* ) const = 0;
	virtual const IPatternDescriptor* LoadPattern( const JSON& ) = 0;

	// Returns new pattern, defined as similarity between patterns.
	//  Works only with GetType() patterns.
	virtual const IPatternDescriptor* CalculateSimilarity(
		const IPatternDescriptor* first, const IPatternDescriptor* second ) = 0;
	// Compare two patterns.
	//  All results that not from interestingResults are returning as CR_Incomparable.
	//  Works only with GetType() patterns.
	//  first, secnod -- patterns to compare
	//  interestingResults, possibleResults -- flags CR_*
	//  interestingResults -- the result of comparison can be only those in interestingResults
	//	all other results are returned as CR_Incomparable
	//  possibleResults -- an apriori knowledge of possible results of the function
	virtual TCompareResult Compare(
		const IPatternDescriptor* first, const IPatternDescriptor* second,
		DWORD interestingResults = CR_AllResults, DWORD possibleResults = CR_AllResults | CR_Incomparable ) = 0;

	// Deletes an allocated pattern
	virtual void FreePattern( const IPatternDescriptor * ) = 0;

	// The zero pattern is supposed to be the most specific one. ie 0 < pattern.
	// The following funnctions are the same as before but allows zero patterns.
	const IPatternDescriptor* CalculateSimilarityZero(
		const IPatternDescriptor* first, const IPatternDescriptor* second );
	TCompareResult CompareZero(
		const IPatternDescriptor* first, const IPatternDescriptor* second,
		DWORD interestingResults = CR_AllResults, DWORD possibleResults = CR_AllResults | CR_Incomparable );

	// TOKILL
	// Write the pattern to the stream.
	//  Works only with GetType() patterns.
	virtual void Write( const IPatternDescriptor* pattern, std::ostream& dst ) const = 0;
	void WriteZero( const IPatternDescriptor* pattern, std::ostream& dst ) const;
};

inline const IPatternDescriptor* IPatternManager::CalculateSimilarityZero(
const IPatternDescriptor* first, const IPatternDescriptor* second )
{
	if( first == 0 ) {
		return second;
	}
	if( second == 0 ) {
		return first;
	}

	return CalculateSimilarity( first, second );
}

inline TCompareResult IPatternManager::CompareZero(
	const IPatternDescriptor* first, const IPatternDescriptor* second,
	DWORD interestingResults, DWORD possibleResults )
{
	TCompareResult result = CR_Incomparable;
	const bool isFirstBottom = first == 0;
	const bool isSecondBottom = second == 0;
	if( isFirstBottom || isSecondBottom ) {
		if( isFirstBottom && isSecondBottom ) {
			result = CR_Equal;
		} else if( isFirstBottom ) {
			result = CR_LessGeneral;
		} else {
			result = CR_MoreGeneral;
		}
		if( HasAllFlags( interestingResults, result ) ) {
			return result;
		} else {
			return CR_Incomparable;
		}
	}

	return Compare( first, second, interestingResults, possibleResults );
}

inline void IPatternManager::WriteZero( const IPatternDescriptor* pattern, std::ostream& dst ) const
{
	if( pattern == 0 ) {
		dst << "*BOTTOM*";
		return;
	}
	Write( pattern, dst );
}

enum TPrunnerScope {
	PS_Pattern = (1 << 0),
	PS_Parents = (1 << 1),
	PS_Children = ( 1 << 2),

	PS_EnumCount
};

////////////////////////////////////////////////////////////////////
// Deleter of patterns with help of Pattern Manager
class CPatternDeleter {
public:
	CPatternDeleter( const CSharedPtr<IPatternManager>& _cmp ) :
		cmpHolder( _cmp ), cmp( *cmpHolder ) {}
	CPatternDeleter( IPatternManager& _cmp ) :
		cmp( _cmp ) {}

	void operator()( const IPatternDescriptor* ptrn )
		{ cmp.FreePattern( ptrn ); }

private:
	CSharedPtr<IPatternManager> cmpHolder;
	IPatternManager& cmp;
};

////////////////////////////////////////////////////////////////
#endif // PATTERNMANAGER_H


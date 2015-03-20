#ifndef IntentStorage_h
#define IntentStorage_h

#include <common.h>
#include <fcaps/BasicTypes.h>

#include <fcaps/CompareResults.h>

interface IPatternManager;
interface IPatternDescriptor;

typedef intptr_t TIntentId;

////////////////////////////////////////////////////////////////////////
// Transform pattern to their IDs, ID == -1 (NotFound) -- there is some error while loading this pattern.
// Responcable for deleting patterns

interface IIntentStorage : public virtual IObject {
	// Set the comparater this intent storage working with
	virtual void Initialize( const CSharedPtr<IPatternManager>& cmp ) = 0;

	// Load object description from JSON
	virtual TIntentId LoadObject( const JSON& ) = 0;

	// Load/Save patterns from/in JSON
	virtual JSON SavePattern( TIntentId id ) const = 0;
	virtual TIntentId LoadPattern( const JSON& ) = 0;

	// Delete pattern from storage by id
	virtual void DeletePattern( TIntentId id ) = 0;
	// Get pattern by ID
	virtual const IPatternDescriptor* GetPattern( TIntentId id ) const = 0;
	// Returns ID of a new pattern, defined as similarity between patterns.
	virtual TIntentId CalculateSimilarity( TIntentId first, TIntentId second ) = 0;
	// Compare two patterns by their IDs.
	//  All results that not from interestingResults are returning as CR_Incomparable.
	//  first, second -- IDs of patterns to compare
	//  interestingResults, possibleResults -- flags CR_*
	//  interestingResults -- the result of comparison can be only those in interestingResults
	//  all other results are returned as CR_Incomparable
	//  possibleResults -- an apriori knowledge of possible results of the function
	virtual TCompareResult Compare( TIntentId first, TIntentId second,
		TIntentId interestingResults = CR_AllResults, TIntentId possibleResults = CR_AllResults | CR_Incomparable ) const = 0;
	// Write the pattern to the stream.
	virtual void Write( TIntentId id, std::ostream& dst ) const = 0;
};

#endif // IntentStorage_h


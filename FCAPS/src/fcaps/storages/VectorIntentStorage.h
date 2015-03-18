#ifndef VECTORINTENTSTORAGE_H
#define VECTORINTENTSTORAGE_H

#include <fcaps/storages/IntentStorage.h>
#include <deque>

class CVectorIntentStorage : public IIntentStorage {
public:
	CVectorIntentStorage();
	~CVectorIntentStorage();
	// Methods of IIntentStorage
	virtual void Initialize( const CSharedPtr<IPatternDescriptorComparator>& _cmp )
		{ cmp = _cmp; assert( cmp != 0 );}

	virtual TIntentId LoadObject( const JSON& );
	virtual JSON SavePattern( TIntentId id ) const;
	virtual TIntentId LoadPattern( const JSON& );

	virtual void DeletePattern( TIntentId id );
	virtual const IPatternDescriptor* GetPattern( TIntentId id ) const;
	virtual TIntentId CalculateSimilarity( TIntentId first, TIntentId second );
	virtual TCompareResult Compare( TIntentId first, TIntentId second,
		TIntentId interestingResults = CR_AllResults, TIntentId possibleResults = CR_AllResults | CR_Incomparable ) const;
	virtual void Write( TIntentId id, std::ostream& dst ) const;

private:
	CSharedPtr<IPatternDescriptorComparator> cmp;
	std::deque<const IPatternDescriptor*> patterns;

	const IPatternDescriptor* getPattern(TIntentId id) const;
	void deletePattern( const IPatternDescriptor* ) const;
};

#endif // VECTORINTENTSTORAGE_H

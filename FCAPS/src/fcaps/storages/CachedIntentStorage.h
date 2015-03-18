#ifndef CACHEDINTENTSTORAGE_H
#define CACHEDINTENTSTORAGE_H

#include <fcaps/storages/IntentStorage.h>
#include <fcaps/storages/CachedPatternStorage.h>

class CCachedIntentStorage : public IIntentStorage {
public:
	CCachedIntentStorage();

	// Methods of IIntentStorage
	virtual void Initialize( const CSharedPtr<IPatternDescriptorComparator>& cmp );

	virtual TIntentId LoadObject( const JSON& );
	virtual JSON SavePattern( TIntentId id ) const;
	virtual TIntentId LoadPattern( const JSON& );

	virtual void DeletePattern( TIntentId id );
	virtual const IPatternDescriptor* GetPattern( TIntentId id ) const;
	virtual TIntentId CalculateSimilarity( TIntentId first, TIntentId second );
	virtual TCompareResult Compare( TIntentId first, TIntentId second,
		TIntentId interestingResults = CR_AllResults, TIntentId possibleResults = CR_AllResults | CR_Incomparable ) const;
	virtual void Write( TIntentId id, std::ostream& dst ) const;

	// Methods of this class
	void Reserve( size_t size );

private:
	CSharedPtr<IPatternDescriptorComparator> cmp;
	CPatternComparatorHasher hasher;
	CCachedPatternStorage<CPatternComparatorHasher> storage;

	const IPatternDescriptor* getPattern( TIntentId id ) const;
	TIntentId addPattern(const IPatternDescriptor*);
};

#endif // CACHEDINTENTSTORAGE_H

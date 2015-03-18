#include <fcaps/storages/CachedIntentStorage.h>

#include <fcaps/PatternDescriptor.h>

using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////

CCachedIntentStorage::CCachedIntentStorage()
{
	//ctor
}

void CCachedIntentStorage::Initialize( const CSharedPtr<IPatternDescriptorComparator>& _cmp )
{
	cmp = _cmp;
	storage.Clear();
	storage.Initialize(CPatternComparatorHasher(cmp));
}

TIntentId CCachedIntentStorage::LoadObject( const JSON& json )
{
	assert(cmp != 0 );
	const IPatternDescriptor* ptrn = cmp->LoadObject(json);
	return addPattern(ptrn);
}
JSON CCachedIntentStorage::SavePattern( TIntentId id ) const
{
	assert(cmp != 0 );
	return cmp->SavePattern( getPattern(id) );
}
TIntentId CCachedIntentStorage::LoadPattern( const JSON& json )
{
	assert(cmp != 0 );
	const IPatternDescriptor* ptrn = cmp->LoadObject(json);
	return addPattern(ptrn);
}

void CCachedIntentStorage::DeletePattern( TIntentId id )
{
	const IPatternDescriptor* p = getPattern( id );
	storage.RemovePattern(p);
}
const IPatternDescriptor* CCachedIntentStorage::GetPattern( TIntentId id ) const
{
	const IPatternDescriptor* p = reinterpret_cast<const IPatternDescriptor*>(id);
	if( storage.HasPattern(p) ) {
		return p;
	} else {
		return 0;
	}
}
TIntentId CCachedIntentStorage::CalculateSimilarity( TIntentId first, TIntentId second )
{
	assert(cmp!=0);
	const IPatternDescriptor* res = cmp->CalculateSimilarity(getPattern(first),getPattern(second));
	return addPattern(res);
}
TCompareResult CCachedIntentStorage::Compare( TIntentId first, TIntentId second,
	TIntentId interestingResults, TIntentId possibleResults ) const
{
	return cmp->Compare(getPattern(first),getPattern(second),interestingResults, possibleResults );
}
void CCachedIntentStorage::Write( TIntentId id, std::ostream& dst ) const
{
	dst << SavePattern(id);
}

void CCachedIntentStorage::Reserve( size_t size )
{
	storage.Reserve(size);
}

// Returns a pattern that is included into the
const IPatternDescriptor* CCachedIntentStorage::getPattern( TIntentId id ) const
{
	const IPatternDescriptor* p = reinterpret_cast<const IPatternDescriptor*>(id);
	assert( storage.HasPattern(p) );
	return p;
}

// Adds pattern to hashmap. Returns the pattern in hashtable
//  removes the pattern p if a dublicate.
TIntentId CCachedIntentStorage::addPattern(const IPatternDescriptor*p)
{
	const IPatternDescriptor* newP = storage.AddPattern( p );
	const TIntentId rslt = reinterpret_cast<TIntentId>(newP);
	return rslt;
}

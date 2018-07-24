// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/storages/VectorIntentStorage.h>
#include <fcaps/PatternManager.h>

using namespace std;

////////////////////////////////////////////////////////////////////

CVectorIntentStorage::CVectorIntentStorage()
{
//	patterns.reserve( 5000000 ); // More patterns than that is unlikely processable.
}

CVectorIntentStorage::~CVectorIntentStorage()
{
	for( size_t i = 0; i < patterns.size(); ++i ) {
		if( getPattern(i) != 0 ) {
			deletePattern( getPattern(i) );
		}
	}
}

TIntentId CVectorIntentStorage::LoadObject( const JSON& json )
{
	assert( cmp != 0 );
	unique_ptr<const IPatternDescriptor> ptrn( cmp->LoadObject( json ) );
	if( ptrn.get() == 0 ) {
		return -1;
	}
	patterns.push_back( ptrn.release() );
	return patterns.size() - 1;
}
JSON CVectorIntentStorage::SavePattern( TIntentId id ) const
{
	assert( cmp != 0 );
	if( id != -1 ) {
		return cmp->SavePattern( getPattern(id) );
	} else {
		return JSON( "\"BOTTOM\"" );
	}
}
TIntentId CVectorIntentStorage::LoadPattern( const JSON& json )
{
	assert( cmp != 0 );
	unique_ptr<const IPatternDescriptor> ptrn( cmp->LoadPattern( json ) );
	if( ptrn.get() == 0 ) {
		return -1;
	}
	patterns.push_back( ptrn.release() );
	return patterns.size() - 1;
}

void CVectorIntentStorage::DeletePattern( TIntentId id )
{
	deletePattern( getPattern(id) );
	if( id == patterns.size() - 1 ) {
		patterns.pop_back();
	} else {
		patterns[id] = 0;
	}
}

const IPatternDescriptor* CVectorIntentStorage::GetPattern( TIntentId id ) const
{
	return getPattern(id);
}

TIntentId CVectorIntentStorage::CalculateSimilarity( TIntentId first, TIntentId second )
{
	patterns.push_back( cmp->CalculateSimilarity( getPattern(first), getPattern(second) ) );
	return patterns.size() - 1;
}

TCompareResult CVectorIntentStorage::Compare( TIntentId first, TIntentId second,
	TIntentId interestingResults, TIntentId possibleResults ) const
{
	return cmp->CompareZero( getPattern(first), getPattern(second), interestingResults, possibleResults );
}

void CVectorIntentStorage::Write( TIntentId id, std::ostream& dst ) const
{
	cmp->WriteZero( getPattern(id), dst );
}

inline const IPatternDescriptor* CVectorIntentStorage::getPattern(TIntentId id) const
{
	assert( id == -1 || patterns[id] != 0 );
	assert( id == -1 || id < patterns.size() );
	return id == -1 ? 0 : patterns[id];
}

void CVectorIntentStorage::deletePattern( const IPatternDescriptor* p ) const
{
	delete p;
}

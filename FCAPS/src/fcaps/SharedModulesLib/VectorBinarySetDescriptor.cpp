// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include "VectorBinarySetDescriptor.h"

#include <JSONTools.h>

#include <rapidjson/document.h>

#include <cstdlib>

#define IS64 (sizeof(uintptr_t) == 8)

CVectorBinarySetDescriptor::CVectorBinarySetDescriptor()
{
	//ctor
}

////////////////////////////////////////////////////////////////////

CVectorBinarySetJoinComparator::CVectorBinarySetJoinComparator() :
	blockSize( sizeof( CVectorBinarySetDescriptor) + 128 ),
	nextFreeBlock( 0 ),
	allocatedPatterns( 0 ),
	shouldWriteNames(false)
#ifdef _DEBUG
	, fingerprint( rand() )
#endif // _DEBUG
{
}

const CVectorBinarySetDescriptor* CVectorBinarySetJoinComparator::LoadObject( const JSON& json )
{
	// TODO
	assert( false );
	return 0;
}
JSON CVectorBinarySetJoinComparator::SavePattern( const IPatternDescriptor* ptrn ) const
{
	assert( ptrn != 0 && dynamic_cast<const CVectorBinarySetDescriptor*>(ptrn) != 0  );

	const CVectorBinarySetDescriptor& pattern = debug_cast<const CVectorBinarySetDescriptor&>( *ptrn );
	CList<DWORD> attrs;
	EnumValues( pattern, attrs );

	rapidjson::Document patternJson;

	patternJson.SetObject();
	patternJson
		.AddMember( "Count", rapidjson::Value().SetUint( attrs.Size() ), patternJson.GetAllocator() );


	if( names.empty() || !shouldWriteNames ) {
		static const char jsonInds[] = "Inds";
		patternJson
			.AddMember( jsonInds, rapidjson::Value().SetArray(), patternJson.GetAllocator() );
		rapidjson::Value& indsJson = patternJson[jsonInds];
		indsJson.Reserve( attrs.Size(), patternJson.GetAllocator() );

		CStdIterator<CList<DWORD>::CConstIterator, false> itr( attrs );
		for( ; !itr.IsEnd(); ++itr ) {
			indsJson.PushBack( rapidjson::Value().SetUint( *itr ), patternJson.GetAllocator() );
		}
	}
	if( shouldWriteNames && !names.empty() ) {
		static const char jsonNames[] = "Names";
		patternJson
			.AddMember( jsonNames, rapidjson::Value().SetArray(), patternJson.GetAllocator() );
		rapidjson::Value& namesJson = patternJson[jsonNames];
		namesJson.Reserve( attrs.Size(), patternJson.GetAllocator() );

		CStdIterator<CList<DWORD>::CConstIterator, false> itr( attrs );
		for( ; !itr.IsEnd(); ++itr ) {
			if( *itr < names.size() ) {
				namesJson.PushBack( rapidjson::Value().SetString(
					rapidjson::StringRef( names[*itr].c_str() ) ), patternJson.GetAllocator() );
			} else {
				namesJson.PushBack( rapidjson::Value().SetUint( *itr ), patternJson.GetAllocator() );
			}
		}
	}
	JSON result;
	CreateStringFromJSON( patternJson, result );
	return result;
}
const CVectorBinarySetDescriptor* CVectorBinarySetJoinComparator::LoadPattern( const JSON& json )
{
	// TODO
	assert( false );
	return 0;
}

TCompareResult CVectorBinarySetJoinComparator::Compare(
	const IPatternDescriptor* first, const IPatternDescriptor* second,
	DWORD interestingResults, DWORD possibleResults )
{
	return Compare( getVectorBinarySet( first ), getVectorBinarySet( second ), interestingResults, possibleResults );
}

const CVectorBinarySetDescriptor* CVectorBinarySetJoinComparator::CalculateSimilarity(
	const IPatternDescriptor* first, const IPatternDescriptor* second )
{
	return CalculateSimilarity( getVectorBinarySet( first ), getVectorBinarySet( second ) );
}

void CVectorBinarySetJoinComparator::FreePattern( const IPatternDescriptor * ptrn )
{
	freePattern( getVectorBinarySet( ptrn ) );
}

void CVectorBinarySetJoinComparator::Write( const IPatternDescriptor* ptrn, std::ostream& dst ) const
{
	const DWORD attrBlockNum = getAttrBlockCount();
	const CVectorBinarySetDescriptor& pattern = getVectorBinarySet( ptrn );

	const uintptr_t* attrBlocks = getAttrBlocks( pattern );
	for( DWORD attrBlock = 0; attrBlock < attrBlockNum; ++attrBlock ) {
		const uintptr_t block = getAttrBlock( attrBlocks, attrBlockNum );
		char nextBit = -1;
		while( (nextBit = getNextBit( block, nextBit )) != NotFound ) {
			const uintptr_t attr = attrBlock * sizeof( uintptr_t ) + nextBit;
			dst << attr << " ";
		}
	}
}

DWORD CVectorBinarySetJoinComparator::GetMaxAttrNumber() const
{
	return (blockSize - getDescriptorSize()) * sizeof(uintptr_t) * 8;
}
void CVectorBinarySetJoinComparator::SetMaxAttrNumber( DWORD num )
{
	assert( memory.Size() == 0 );
	blockSize = getDescriptorSize()
		+ (num / (sizeof(uintptr_t) * 8)+ ((num % (sizeof(uintptr_t) * 8)) == 0 ? 0 : 1));
}

void CVectorBinarySetJoinComparator::Reserve( size_t blockCount )
{
	if( GetAvailableBlockCount() >= blockCount ) {
		return;
	}
	allocate( blockCount );
}

size_t CVectorBinarySetJoinComparator::GetAvailableBlockCount() const
{
	size_t totalBlockCount = 0;
	CStdIterator<CMemory::CConstIterator, false> itr( memory );
	for( ; !itr.IsEnd(); ++itr ) {
		totalBlockCount += (*itr).size();
		assert( ((*itr).size() % blockSize ) == 0 );
	}
	return totalBlockCount / blockSize;
}


TCompareResult CVectorBinarySetJoinComparator::Compare(
	const CVectorBinarySetDescriptor& first, const CVectorBinarySetDescriptor& second,
	DWORD interestingResults, DWORD possibleResults ) const
{
	if( first.Size() == second.Size() ) {
		possibleResults &= CR_Equal | CR_Incomparable;
	} else if ( first.Size() < second.Size() ) {
		possibleResults &= CR_MoreGeneral | CR_Incomparable;
	} else {
		assert( first.Size() > second.Size() );
		possibleResults &= CR_LessGeneral | CR_Incomparable;
	}

	const DWORD attrBlockNum = getAttrBlockCount();

	const uintptr_t* firstAttrBlock = getAttrBlocks( first );
	const uintptr_t* secondAttrBlock = getAttrBlocks( second );

	for( DWORD attrBlock = 0; attrBlock < attrBlockNum; ++attrBlock ) {
		const uintptr_t& firstBlock = getAttrBlock( firstAttrBlock, attrBlock );
		const uintptr_t& secondBlock = getAttrBlock( secondAttrBlock, attrBlock );
		if( firstBlock == secondBlock ) {
			continue;
		}
		const uintptr_t meet = firstBlock & secondBlock;
		if( firstBlock == meet ) {
			possibleResults &= CR_MoreGeneral | CR_Incomparable;
		} else if ( secondBlock == meet ) {
			possibleResults &= CR_LessGeneral | CR_Incomparable;
		} else {
			return CR_Incomparable;
		}
		switch( possibleResults ) {
		case CR_LessGeneral:
			return checkCompareResult( CR_LessGeneral, interestingResults );
		case CR_MoreGeneral:
			return checkCompareResult( CR_MoreGeneral, interestingResults );
		case CR_Incomparable:
			return CR_Incomparable;
		}
	}
	if( HasAllFlags( possibleResults, CR_Equal ) ) {
		return checkCompareResult( CR_Equal, interestingResults );
	} else if ( HasAllFlags( possibleResults, CR_MoreGeneral ) ) {
		assert( !HasAllFlags( possibleResults, CR_LessGeneral ) );
		return checkCompareResult( CR_MoreGeneral, interestingResults );
	} else if ( HasAllFlags( possibleResults, CR_LessGeneral ) ) {
		return checkCompareResult( CR_LessGeneral, interestingResults );
	} else {
		return CR_Incomparable;
	}

}

// counts the number of '1' bits in v.
//  works only for unsigned types
template<typename T>
inline T getBitsCount( T v )
{
	// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
	v = v - ((v >> 1) & (T)~(T)0/3);                           // temp
	v = (v & (T)~(T)0/15*3) + ((v >> 2) & (T)~(T)0/15*3);      // temp
	v = (v + (v >> 4)) & (T)~(T)0/255*15;                      // temp
	return (T)(v * ((T)~(T)0/255)) >> (sizeof(T) - 1) * CHAR_BIT; // count
}

#if defined(_M_X64) || defined(__amd64__)
template<>
inline uintptr_t getBitsCount( uintptr_t v )
{
	v = v - ((v >> 1) & 0x5555555555555555 );                           // temp
	v = (v & 0x3333333333333333) + ((v >> 2) & 0x3333333333333333);      // temp
	v = (v + (v >> 4)) & 0xf0f0f0f0f0f0f0f;                      // temp
	return (uintptr_t)((uintptr_t)(v) * 0x101010101010101) >> 56; // count
}
#else
template<>
inline uintptr_t getBitsCount( uintptr_t v )
{
	v = v - ((v >> 1) & 0x55555555 );                           // temp
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);      // temp
	v = (v + (v >> 4)) & 0xF0F0F0F;                      // temp
	return (uintptr_t)((uintptr_t)(v) * 0x1010101) >> 24; // count
}
#endif

const CVectorBinarySetDescriptor* CVectorBinarySetJoinComparator::CalculateSimilarity(
	const CVectorBinarySetDescriptor& first, const CVectorBinarySetDescriptor& second )
{
	const DWORD attrBlockNum = getAttrBlockCount();
	CVectorBinarySetDescriptor* result = newPattern( false );
	assert( result != 0 );

	const uintptr_t* firstAttrBlock = getAttrBlocks( first );
	const uintptr_t* secondAttrBlock = getAttrBlocks( second );
	uintptr_t* resultAttrBlock = getAttrBlocks( *result );

	result->hash = 0;
	result->size = 0;
	for( DWORD attrBlock = 0; attrBlock < attrBlockNum; ++attrBlock ) {
		const uintptr_t& firstBlock = getAttrBlock( firstAttrBlock, attrBlock );
		const uintptr_t& secondBlock = getAttrBlock( secondAttrBlock, attrBlock );
		uintptr_t& resultBlock = getAttrBlock( resultAttrBlock, attrBlock );
		resultBlock = firstBlock & secondBlock;
		result->hash ^= resultBlock;
		result->size += getBitsCount( resultBlock );
	}

	return result;
}

void CVectorBinarySetJoinComparator::AddList( const CList<DWORD>& values, CVectorBinarySetDescriptor& descr )
{
	CStdIterator<CList<DWORD>::CConstIterator, false> itr( values );
	for( ; !itr.IsEnd(); ++itr ) {
		AddValue( *itr, descr );
	}
}
void CVectorBinarySetJoinComparator::AddValue( DWORD value, CVectorBinarySetDescriptor& descr )
{
	assert( value < GetMaxAttrNumber() );
	uintptr_t* ptr = getAttrBlocks( descr );

#if defined(_M_X64) || defined(__amd64__)
		BOOST_ASSERT( sizeof(uintptr_t) == 8 );
		const size_t index = value >> 6;
		const size_t subindex = value & 63;
#else
		BOOST_ASSERT( sizeof(uintptr_t) == 4 );
		const size_t index = value >> 5;
		const size_t subindex = value & 31;
#endif

	uintptr_t& result = getAttrBlock( ptr, index );
	const uintptr_t bit = (uintptr_t)(1)<<subindex;
	if( ( result & bit ) != 0 ) {
		return;
	}

	result |= bit;

	++descr.size;
	descr.hash ^= bit;
}

void CVectorBinarySetJoinComparator::EnumValues( const CVectorBinarySetDescriptor& descr, CList<DWORD>& result ) const
{
	const DWORD attrBlockNum = getAttrBlockCount();

	const uintptr_t* attrBlocks = getAttrBlocks( descr );
	for( DWORD attrBlock = 0; attrBlock < attrBlockNum; ++attrBlock ) {
		const uintptr_t block = getAttrBlock( attrBlocks, attrBlock );
		char nextBit = -1;
		while( (nextBit = getNextBit( block, nextBit )) != NotFound ) {
			assert( nextBit < sizeof( uintptr_t ) * 8 );
			const uintptr_t attr = attrBlock * sizeof( uintptr_t ) * 8 + nextBit;
			result.PushBack( static_cast<DWORD>( attr ) );
		}
	}
}
// TODO make this function the same as the other EnumValues
void CVectorBinarySetJoinComparator::EnumValues( const CVectorBinarySetDescriptor& descr, int* buffer, int bufferSize ) const
{
	assert(bufferSize >= descr.Size());
	if(bufferSize < descr.Size()) {
		return;
	}
	const DWORD attrBlockNum = getAttrBlockCount();

	const uintptr_t* attrBlocks = getAttrBlocks( descr );
	int currIndex = 0;
	for( DWORD attrBlock = 0; attrBlock < attrBlockNum; ++attrBlock ) {
		const uintptr_t block = getAttrBlock( attrBlocks, attrBlock );
		char nextBit = -1;
		while( (nextBit = getNextBit( block, nextBit )) != NotFound ) {
			assert( nextBit < sizeof( uintptr_t ) * 8 );
			const uintptr_t attr = attrBlock * sizeof( uintptr_t ) * 8 + nextBit;

			assert(currIndex < bufferSize);
			buffer[currIndex] = static_cast<int>(attr);
			++currIndex;
		}
	}
}

// Casts pointer to pattern interface to the reference to the pattern object
inline const CVectorBinarySetDescriptor& CVectorBinarySetJoinComparator::getVectorBinarySet( const IPatternDescriptor* ptrn )
{
	assert( ptrn != 0 && dynamic_cast<const CVectorBinarySetDescriptor*>(ptrn) != 0  );
	return debug_cast<const CVectorBinarySetDescriptor&>( *ptrn );
}

// Checks if the result is within the interesting result and changes it correspondingly.
inline TCompareResult CVectorBinarySetJoinComparator::checkCompareResult( TCompareResult result, DWORD interestingResults )
{
	if( HasAllFlags( interestingResults, result ) ) {
		return result;
	} else {
		return CR_Incomparable;
	}
}

// Returns size of descriptor itself in dwords
inline size_t CVectorBinarySetJoinComparator::getDescriptorSize()
{
	static const size_t size = sizeof(CVectorBinarySetDescriptor);
	static const size_t result = ( size + ((size % sizeof(uintptr_t)) == 0 ? 0 : sizeof(uintptr_t) - (size%sizeof(uintptr_t))) ) / sizeof(uintptr_t);
	return result;
}

// Returns the number of attr blocks. Every block is one uintptr_t.
inline DWORD CVectorBinarySetJoinComparator::getAttrBlockCount() const
{
	assert( blockSize > getDescriptorSize() );
	return blockSize - getDescriptorSize();
}

// Returns the pointer to the begin of array of attributes.
uintptr_t* CVectorBinarySetJoinComparator::getAttrBlocks( CVectorBinarySetDescriptor& descr )
{
	return reinterpret_cast<uintptr_t*>( reinterpret_cast<char*>( (&descr) ) + getDescriptorSize()*sizeof(uintptr_t) );
}

// Returns the value in the required block of attributes (const and not-const)
inline uintptr_t& CVectorBinarySetJoinComparator::getAttrBlock( uintptr_t* ptr, size_t blockNum )
{
	assert( checkMemory( ptr + blockNum ) );
	assert( checkSameBlock( ptr, ptr + blockNum ) );
	return ptr[blockNum];
}
inline const uintptr_t& CVectorBinarySetJoinComparator::getAttrBlock( const uintptr_t* ptr, size_t blockNum ) const
{
	assert( checkMemory( ptr + blockNum ) );
	assert( checkSameBlock( ptr, ptr + blockNum ) );
	 return ptr[blockNum];
}

// Checks if memory is within the allocator
bool CVectorBinarySetJoinComparator::checkMemory( const uintptr_t* ptr, bool startOfBlock ) const
{
	CStdIterator<CMemory::CConstIterator, false> itr( memory );
	for( ; !itr.IsEnd(); ++itr ) {
		if( &(*itr).front() <= ptr && ptr < (&(*itr).back()+ 1) ) {
			if( !startOfBlock ) {
				return true;
			} else {
				const bool rslt = ptr +blockSize <= (&(*itr).back() + 1)
					&& ( (reinterpret_cast<uintptr_t>(ptr)
						-reinterpret_cast<uintptr_t>(&(*itr).front())) % (blockSize*sizeof(uintptr_t)) ) == 0;
				// For breakpoitns on false.
				if( rslt ) {
					return true;
				} else {
					return false;
				}

			}
		}
	}
	return false;
}

// Checks if poth pointers belong to memory of the same descriptior
bool CVectorBinarySetJoinComparator::checkSameBlock( const uintptr_t* p1, const uintptr_t* p2 ) const
{
	CStdIterator<CMemory::CConstIterator, false> itr( memory );
	for( ; !itr.IsEnd(); ++itr ) {
		if( !(&(*itr).front() <= p1 && p1 < (&(*itr).back() + 1)) ) {
			continue;
		}
		const bool result = ( &(*itr).front() <= p2 && p2 < (&(*itr).back() + 1) )
			&& (reinterpret_cast<uintptr_t>(p1) - reinterpret_cast<uintptr_t>(&(*itr).front())) / (blockSize*sizeof(uintptr_t))
				== (reinterpret_cast<uintptr_t>(p2) - reinterpret_cast<uintptr_t>(&(*itr).front())) / (blockSize*sizeof(uintptr_t));
		if( result ) {
			return true;
		} else {
			return false;
		}
	}
	return false;

}

// Allocates new memory
inline void CVectorBinarySetJoinComparator::allocate()
{
	DWORD s = 0;
	if(memory.Size() == 0) {
		s=1000;
	}else{
		const DWORD lastS = memory.Back().size() / blockSize;
		// TOCHANGE : Exp is so power that in certain moment we can be out of memory because of such a multiplication
		s = lastS < 100000 ? lastS * 2 : lastS;
	}
	allocate( s );
}

// Allocates the requeested amount of blocks
void CVectorBinarySetJoinComparator::allocate( DWORD count )
{
	memory.PushBack( CMemoryBlock() );
	memory.Back().resize( count * blockSize );

	uintptr_t* ptr = &memory.Back().front();
#ifndef NDEBUG
	// memeseting memory only in debug
	memset( ptr, 0xFA, count * blockSize * sizeof(uintptr_t) );
#endif // NDEBUG

	for( DWORD i = 0; i < count - 1; ++i ) {
		*ptr = reinterpret_cast<uintptr_t>( ptr + blockSize );
		assert( checkMemory( ptr + blockSize, true ) );
		ptr += blockSize;
	}
	*ptr = reinterpret_cast<uintptr_t>( nextFreeBlock );

	nextFreeBlock = &memory.Back().front();
	assert( checkMemory( nextFreeBlock, true ) );
}

// Allocates new pattern and solves problem with memory.
CVectorBinarySetDescriptor* CVectorBinarySetJoinComparator::newPattern( bool clear )
{
	++allocatedPatterns;

	if( nextFreeBlock == 0 ) {
		allocate();
	}
	assert( nextFreeBlock != 0 );
	assert( checkMemory( nextFreeBlock, true ) );

	uintptr_t* const nextBlock = reinterpret_cast<uintptr_t*>( *nextFreeBlock );
	assert( nextBlock == 0 || checkMemory(nextBlock, true ) );
#ifndef NDEBUG
	memset( nextFreeBlock, 0, blockSize * sizeof(uintptr_t) );
#else
	if( clear ) {
		memset( nextFreeBlock, 0, blockSize * sizeof(uintptr_t) );
	}
#endif // NDEBUG

	CVectorBinarySetDescriptor* result = new(nextFreeBlock) CVectorBinarySetDescriptor;
	nextFreeBlock = nextBlock;
#ifdef _DEBUG
	result->fingerprint = fingerprint;
#endif // _DEBUG
	return result;
}

// Frees the pattern
void CVectorBinarySetJoinComparator::freePattern( const CVectorBinarySetDescriptor& descr )
{
#ifdef _DEBUG
	assert( descr.fingerprint == fingerprint );
#endif // _DEBUG

	--allocatedPatterns;

	uintptr_t* ptr = reinterpret_cast<uintptr_t*>( const_cast<CVectorBinarySetDescriptor*>( &descr ) );
	descr.~CVectorBinarySetDescriptor();
	assert( checkMemory( ptr, true ) );
	assert( nextFreeBlock == 0 || checkMemory( nextFreeBlock, true ) );

	*ptr = reinterpret_cast<uintptr_t>( nextFreeBlock );
	nextFreeBlock = ptr;
}

// Return the position of the next non-zero bit begining from bit+1.
inline uintptr_t CVectorBinarySetJoinComparator::getNextBit( uintptr_t block, char bit )
{
	block >>= (++bit);
	while( block != 0
		&& bit < sizeof(uintptr_t) * 8 ) // A bug in the compiler (?) after update in my ubuntu (1 >> 64) == 1
	{
		if( (block & 1) != 0 ) {
			return bit;
		}
		block = (block >> 1);
		++bit;
	}
	return NotFound;
}

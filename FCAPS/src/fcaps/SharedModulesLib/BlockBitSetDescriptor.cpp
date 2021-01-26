// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include "BlockBitSetDescriptor.h"

#include <JSONTools.h>

#include <rapidjson/document.h>

// #include <boost/uuid/uuid.hpp>
// #include <boost/uuid/uuid_generators.hpp>
// #include <boost/uuid/uuid_io.hpp>

#include <cstdlib>
#include <ios>

#define IS64 (sizeof(uintptr_t) == 8)

#if defined(_M_X64) || defined(__amd64__)
		BOOST_ASSERT( sizeof(CBlockAllocator::TElementType) == 8 );
	#define LOG_PTR_SIZE 6
    #define PTR_MASK = 63
#else
		BOOST_ASSERT( sizeof(CBlockAllocator::TElementType) == 4 );
	#define LOG_PTR_SIZE 5
    #define PTR_MASK = 31
#endif

using namespace std;

// Returns size of descriptor itself in dwords
inline size_t getDescriptorSize()
{
	static const size_t size = sizeof(CBlockBitSetDescriptor);
	static const size_t result = ( size + ((size % sizeof(uintptr_t)) == 0 ? 0 : sizeof(uintptr_t) - (size%sizeof(uintptr_t))) ) / sizeof(uintptr_t);
	return result;
}
const int CBlockBitSetDescriptor::descriptorSize = getDescriptorSize();

CBlockBitSetDescriptor::CBlockBitSetDescriptor():
		blockSize(4),
		blockNum(0)
{
}

CBlockBitSetDescriptor::~CBlockBitSetDescriptor()
{
	//dtor
}

////////////////////////////////////////////////////////////////////

const CBlockBitSetDescriptor* CBlockBitSetJoinComparator::LoadObject( const JSON& json );
{
	// TODO
	assert( false );
	return 0;
}
JSON CBlockBitSetJoinComparator::SavePattern( const IPatternDescriptor* ptrn ) const
{
	assert( ptrn != 0 && dynamic_cast<const CBlockBitSetDescriptor*>(ptrn) != 0  );

	const CBlockBitSetDescriptor& pattern = debug_cast<const CBlockBitSetDescriptor&>( *ptrn );
	rapidjson::Document patternJson;

	patternJson.SetObject();
	patternJson
		.AddMember( "Count", rapidjson::Value().SetUint( pattern.Size() ), patternJson.GetAllocator() );


	static const char jsonInds[] = "Inds";
	patternJson
		.AddMember( jsonInds, rapidjson::Value().SetArray(), patternJson.GetAllocator() );
	rapidjson::Value& indsJson = patternJson[jsonInds];
	indsJson.Reserve( pattern.Size(), patternJson.GetAllocator() );

	for(int a = Begin(pattern)  ; a >= 0; a = Next(pattern, a) ) {
		indsJson.PushBack( rapidjson::Value().SetUint( a ), patternJson.GetAllocator() );
	}

	JSON result;
	CreateStringFromJSON( patternJson, result );
	return result;
}
const CBlockBitSetDescriptor* CBlockBitSetJoinComparator::LoadPattern( const JSON& json )
{
	CJsonError error;
	rapidjson::Document ptrn;
	if( !ReadJsonString( json, ptrn, error ) ) {
		throw new CJsonException( "CBlockBitSetJoinComparator::LoadPattern", error );
	}
	if(!ptrn.IsObject() || !ptrn.HasMember("Inds") || !ptrn["Inds"].IsArray()) {
		throw new CTextException(  "CBlockBitSetJoinComparator::LoadPattern", "Extent does not have an inds member");
	}

	rapidjson::Value& inds = ptrn["Inds"];
	unique_ptr<CBlockBitSetDescriptor> res(NewPattern());
	for( int i = 0; i < inds.Size(); ++i ) {
		if(!inds[i].IsUint()) {
			throw new CTextException(  "CBlockBitSetJoinComparator::LoadPattern", "Extent contains not a Uint");
		}
		const DWORD attr = inds[i].GetUint();
		AddValue(attr,*res);
	}

	return res.release();
}

TCompareResult CBlockBitSetJoinComparator::Compare(
	const IPatternDescriptor* first, const IPatternDescriptor* second,
	DWORD interestingResults, DWORD possibleResults )
{
	return Compare( getBlockBitSet( first ), getBlockBitSet( second ), interestingResults, possibleResults );
}

const CBlockBitSetDescriptor* CBlockBitSetJoinComparator::CalculateSimilarity(
	const IPatternDescriptor* first, const IPatternDescriptor* second )
{
	return CalculateSimilarity( getBlockBitSet( first ), getBlockBitSet( second ) );
}

void CBlockBitSetJoinComparator::FreePattern( const IPatternDescriptor * ptrn )
{
	freePattern( getBlockBitSet( ptrn ) );
}

DWORD CBlockBitSetJoinComparator::GetMaxAttrNumber() const
{
	return GetBlockSize() * GetBlockNumber() * blockAllocator.GetElementSize();
		
}
void CBlockBitSetJoinComparator::SetMaxAttrNumber( DWORD num )
{
	SetBlockNumber(
		ceil( static_cast<double>(num) / (GetBlockSize() * blockAllocator.GetElementSize()) ));
	assert(GetMaxAttrNumber() >= num);
}

void CBlockBitSetJoinComparator::SetBlockSize( DWORD s )
{
	if( blockAllocator.GetAvailableBlockCount() > 0 ) {
		throw new CTextException(  "CBlockBitSetJoinComparator::SetBlockSize", "Changing block size after allocation");
	}
	blockSize = s;
	blockAllocator.SetBlockSize(blockSize);
}

void CBlockBitSetJoinComparator::SetBlockNumber(DWORD n)
{
	if( patternAllocator.GetAvailableBlockCount() > 0 ) {
		throw new CTextException(  "CBlockBitSetJoinComparator::SetBlockSize", "Changing block number after allocation");
	}
	blockNum = n;
	patternAllocator.SetBlockSize(descriptorSize + blockNum);
}

void CBlockBitSetJoinComparator::Reserve( size_t patternCount )
{
	if( patternAllocator.GetAvailableBlockCount() >= patternCount ) {
		return;
	}

	patternAllocator.Reserve(patternCount);
	blockAllocator.Reserve(patternCount * blockNum );
}

TCompareResult CBlockBitSetJoinComparator::Compare(
	const CBlockBitSetDescriptor& first, const CBlockBitSetDescriptor& second,
	DWORD interestingResults, DWORD possibleResults ) const
{
		return compare(first,second, interestingResults, possibleResults);
}

const CBlockBitSetDescriptor* CBlockBitSetJoinComparator::CalculateSimilarity(
			const CBlockBitSetDescriptor& first, const CBlockBitSetDescriptor& second )
{
	if( first.nonEmptyBlocksNum <= second.nonEmptyBlocksNum) {
		return calculateSimilarity(first,second);
	} else {
		return calculateSimilarity(second,first);
	}
}

void CBlockBitSetJoinComparator::AddValue( DWORD value, CBlockBitSetDescriptor& descr )
{
	assert( value < GetMaxAttrNumber() );
	CBlockAllocator::TElementType* ptr = getAttrBlocks( descr );

	const size_t index = value >> LOG_PTR_SIZE >> blockSize;
	const size_t blockIndex = (value >> LOG_PTR_SIZE) & ((1 << blockSize) - 1);
	const size_t subindex = value & PTR_MASK;

	const uintptr_t bit = (uintptr_t)(1)<<subindex;

	CBlockAllocator::TElementType& blockResult = getAttrBlock( ptr, index );
	CBlockAllocator::TElementType* blockPtr = 0;
	if( blockResult > blockNum ) {
		// This is the valid pointer
		blockPtr = reinterpret_cast<CBlockAllocator::TElementType*>(blockResult);
	} else {
		// This is the reference to the next not empty block
		// Thus this block is empty and we should create it.
		blockPtr = blockAllocator.New(true);
		++descr.nonEmptyBlocksNum;
		blockResult = reinterpret_cast<CBlockAllocator::TElementType>(blockPtr);
		assert(blockResult > blockNum);
		for( int i = index - 1; i >=0; --i ) {
			CBlockAllocator::TElementType& blockResult = getAttrBlock( ptr, i);
			if( blockResult > blockNum ) {
				break;
			}
			blockResult = index;
		}
	}
	assert( blockSize < blockAllocator.GetBlockSize() );
	CBlockAllocator::TElementType& result = getAttrBlock( blockPtr, blockSize );
	if( ( result & bit ) != 0 ) {
		return;
	}

	result |= bit;

	++descr.size;
	descr.hash ^= bit;
}

int CBlockBitSetJoinComparator::Next(const CBlockBitSetDescriptor& d, int prev) const
{
	++prev;

	CBlockAllocator::TElementType* ptr = getAttrBlocks( descr );
	size_t index = prev >> LOG_PTR_SIZE >> blockSize;
	size_t blockIndex = (prev >> LOG_PTR_SIZE) & ((1 << blockSize) - 1);
	size_t subindex = prev & PTR_MASK;
	
	while( index < blockNum ) {
		CBlockAllocator::TElementType& blockResult = getAttrBlock( ptr, index );
		if( blockResult <= blockNum ) {
			// Moving to the next block
			assert( index < blockResult);
			index = static_cast<size_t>(blockResult);
			blockIndex = 0;
			subindex = 0;
			continue;
		}
		CBlockAllocator::TElementType* blockPtr = reinterpret_cast<CBlockAllocator::TElementType*>(blockResult);
		assert(blockIndex < (1<<blockSize));
		CBlockAllocator::TElementType& result = getAttrBlock(blockPtr, blockIndex);
		uintptr_t bit = (uintptr_t)(1) << subindex;
		if( ( result & bit ) != 0 ) {
			return (((index << blockSize) + blockIndex) << LOG_PTR_SIZE) | subindex;
		}
		if( result == 0 ) {
			// Moving to next element in the block
			++blockIndex;
			subindex = 0;
		} else {
			++subindex;
			if( subindex > PTR_MASK) {
				subindex = 0;
				++blockIndex;
			}
		}
		if( blockIndex >= (1<<blockSize)) {
			++index;
			blockIndex = 0;
			subindex = 0;
		}
	}
	return -1;
}

// Casts pointer to pattern interface to the reference to the pattern object
inline const CBlockBitSetDescriptor& CBlockBitSetJoinComparator::getBlockBitSet( const IPatternDescriptor* ptrn )
{
	assert( ptrn != 0 && dynamic_cast<const CBlockBitSetDescriptor*>(ptrn) != 0  );
	return debug_cast<const CBlockBitSetDescriptor&>( *ptrn );
}

// Returns the pointer to the begin of array of attributes
inline CBlockAllocator::TElementType* CBlockBitSetJoinComparator::getAttrBlocks( CBlockBitSetDescriptor& descr )
{
	return reinterpret_cast<CBlockAllocator::TElementType*>(&descr) + descriptorSize;
}

// Returns the value in the required block of attributes (const and not-const)
inline CBlockAllocator::TElementType& CBlockBitSetJoinComparator::getAttrBlock( CBlockAllocator::TElementType* ptr, size_t blockNum )
{
	assert( patternAllocator.CheckMemory( ptr + blockNum ) || blockAllocator.CheckMemory(ptr + blockNum) );
	assert( patternAllocator.CheckSameBlock( ptr, ptr + blockNum ) || blockAllocator.CheckSameBlock( ptr, ptr + blockNum ) );
	return ptr[blockNum];
}
inline const CBlockAllocator::TElementType& CBlockBitSetJoinComparator::getAttrBlock( const uintptr_t* ptr, size_t blockNum ) const
{
	assert( patternAllocator.CheckMemory( ptr + blockNum ) || blockAllocator.CheckMemory(ptr + blockNum) );
	assert( patternAllocator.CheckSameBlock( ptr, ptr + blockNum ) || blockAllocator.CheckSameBlock( ptr, ptr + blockNum ) );
	return ptr[blockNum];
}

// Allocates new pattern and solves problem with memory.
CBlockBitSetDescriptor* CBlockBitSetJoinComparator::newPattern( bool clear )
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

	CBlockBitSetDescriptor* result = new(nextFreeBlock) CBlockBitSetDescriptor;
	nextFreeBlock = nextBlock;
#ifdef _DEBUG
	result->fingerprint = fingerprint;
#endif // _DEBUG
	return result;
}

// Frees the pattern
void CBlockBitSetJoinComparator::freePattern( const CBlockBitSetDescriptor& descr )
{
#ifdef _DEBUG
	assert( descr.fingerprint == fingerprint );
#endif // _DEBUG

	--allocatedPatterns;

	uintptr_t* ptr = reinterpret_cast<uintptr_t*>( const_cast<CBlockBitSetDescriptor*>( &descr ) );
	descr.~CBlockBitSetDescriptor();
	assert( checkMemory( ptr, true ) );
	assert( nextFreeBlock == 0 || checkMemory( nextFreeBlock, true ) );

	*ptr = reinterpret_cast<uintptr_t>( nextFreeBlock );
	nextFreeBlock = ptr;
}

// Return the position of the next non-zero bit begining from bit+1.
inline uintptr_t CBlockBitSetJoinComparator::getNextBit( uintptr_t block, char bit )
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

// Compares the patterns first is used as list and second as an array.
TCompareResult CBlockBitSetJoinComparator::compare(
	const CBlockBitSetDescriptor& first, const CBlockBitSetDescriptor& second,
	DWORD interestingResults, DWORD possibleResults )
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

// Checks if the result is within the interesting result and changes it correspondingly.
inline TCompareResult CBlockBitSetJoinComparator::checkCompareResult( TCompareResult result, DWORD interestingResults )
{
	if( HasAllFlags( interestingResults, result ) ) {
		return result;
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

// Calculate the intersection between the patterns. First one is used as a list
const CBlockBitSetDescriptor* CBlockBitSetJoinComparator::calculateSimilarity(
			const CBlockBitSetDescriptor& first, const CBlockBitSetDescriptor& second )
	const DWORD attrBlockNum = getAttrBlockCount();
	CBlockBitSetDescriptor* result = newPattern( false );
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


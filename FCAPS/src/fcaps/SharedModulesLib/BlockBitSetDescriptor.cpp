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

using namespace std;

	CBlockAllocator() :
		blockSize(4), nextFreeBlock(0), allocatedBlocks(0) {}

CBlockBitSetDescriptor::CBlockBitSetDescriptor():
		blockSize(4),
		nextFreeBlock(0),
		allocatedBlocks(0) 
{
	patternAllocator.SetBlockSize(size_of(CBlockBitSetDescriptor));
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
	if( !isAllocated() ) {
		assert( false );
		throw new CTextException(  "CBlockBitSetJoinComparator::SetBlockSize", "Changing block size after allocation");
	}
	blockSize = s;
	blockAllocator.SetBlockSize(blockSize);
}

void CBlockBitSetJoinComparator::SetBlockNumber(DWORD n)
{
	if( !isAllocated() ) {
		assert( false );
		throw new CTextException(  "CBlockBitSetJoinComparator::SetBlockSize", "Changing block number after allocation");
	}
	blockNum = n;
	patternRefsAllocator.SetBlockSize(size_of(blockAllocator::TElementType*) * blockNum);
}

void CBlockBitSetJoinComparator::Reserve( size_t patternCount )
{
	if( patternAllocator.GetAvailableBlockCount() >= patternCount ) {
		return;
	}

	patternAllocator.Reserve(patternCount);
	patternRefsAllocator.Reserve(patternCount);
	blockAllocator.Reserve(patternCount * blockNum );
}

size_t CBlockBitSetJoinComparator::GetAvailableBlockCount() const
{
	size_t totalBlockCount = 0;
	CStdIterator<CMemory::CConstIterator, false> itr( memory );
	for( ; !itr.IsEnd(); ++itr ) {
		totalBlockCount += (*itr).size();
		assert( ((*itr).size() % blockSize ) == 0 );
	}
	return totalBlockCount / blockSize;
}


TCompareResult CBlockBitSetJoinComparator::Compare(
	const CBlockBitSetDescriptor& first, const CBlockBitSetDescriptor& second,
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

const CBlockBitSetDescriptor* CBlockBitSetJoinComparator::CalculateSimilarity(
	const CBlockBitSetDescriptor& first, const CBlockBitSetDescriptor& second )
{
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

void CBlockBitSetJoinComparator::AddList( const CList<DWORD>& values, CBlockBitSetDescriptor& descr )
{
	CStdIterator<CList<DWORD>::CConstIterator, false> itr( values );
	for( ; !itr.IsEnd(); ++itr ) {
		AddValue( *itr, descr );
	}
}
void CBlockBitSetJoinComparator::AddValue( DWORD value, CBlockBitSetDescriptor& descr )
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

void CBlockBitSetJoinComparator::EnumValues( const CBlockBitSetDescriptor& descr, CList<DWORD>& result ) const
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
void CBlockBitSetJoinComparator::EnumValues( const CBlockBitSetDescriptor& descr, int* buffer, int bufferSize ) const
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

CBlockBitSetJoinComparator::TSwappedPattern CBlockBitSetJoinComparator::SwapPattern( const CBlockBitSetDescriptor * p )
{
	if( !swapStream.is_open()) {
		swapStream.exceptions(fstream::failbit | fstream::badbit);
		swapStream.open(swapFile, fstream::in | fstream::out | fstream::binary | fstream::trunc);
		assert(!swapStream.fail());
	}
	TSwappedPattern newSwapPositionIndx = -1;
	if( freeIndxSwapPosition == -1) {
		newSwapPositionIndx = swappedPositions.size();
		swappedPositions.resize(swappedPositions.size() + 1);
		swapStream.seekp(0,ios_base::end);
		CSwapPosition& pos = swappedPositions[newSwapPositionIndx];
		pos.Position = swapStream.tellp();
	} else {
		newSwapPositionIndx = freeIndxSwapPosition;
		freeIndxSwapPosition = swappedPositions[freeIndxSwapPosition].Last;
		CSwapPosition& pos = swappedPositions[newSwapPositionIndx];
		swapStream.seekp(pos.Position,ios_base::beg);
	}

#ifdef _DEBUG
	swapStream.write(reinterpret_cast<const char*>(&p->fingerprint), sizeof(p->fingerprint));
#endif
	swapStream.write(reinterpret_cast<const char*>(&p->hash), sizeof(p->hash));
	swapStream.write(reinterpret_cast<const char*>(&p->size), sizeof(p->size));
	swapStream.write(reinterpret_cast<const char*>(getAttrBlocks(*p)),getAttrBlockCount() * sizeof(uintptr_t));

	freePattern(*p);
	return newSwapPositionIndx;
}
const CBlockBitSetDescriptor* CBlockBitSetJoinComparator::SwapRestore(TSwappedPattern p)
{
	assert( swapStream.is_open() );
	CSwapPosition& pos = swappedPositions[p];
	swapStream.seekg(pos.Position);
	CBlockBitSetDescriptor* newP = newPattern(false);
#ifdef _DEBUG
	swapStream.read(reinterpret_cast<char*>(&newP->fingerprint), sizeof(newP->fingerprint));
#endif
	swapStream.read(reinterpret_cast<char*>(&newP->hash), sizeof(newP->hash));
	swapStream.read(reinterpret_cast<char*>(&newP->size), sizeof(newP->size));
	swapStream.read(reinterpret_cast<char*>(getAttrBlocks(*newP)),getAttrBlockCount() * sizeof(uintptr_t));

	assert(!swapStream.fail());
	assert(newP->fingerprint == fingerprint);

	SwapRemove(p);
	return newP;
}
void CBlockBitSetJoinComparator::SwapRemove(TSwappedPattern p)
{
	CSwapPosition& pos = swappedPositions[p];
	pos.Last = freeIndxSwapPosition;
	freeIndxSwapPosition = p;
}

// Casts pointer to pattern interface to the reference to the pattern object
inline const CBlockBitSetDescriptor& CBlockBitSetJoinComparator::getBlockBitSet( const IPatternDescriptor* ptrn )
{
	assert( ptrn != 0 && dynamic_cast<const CBlockBitSetDescriptor*>(ptrn) != 0  );
	return debug_cast<const CBlockBitSetDescriptor&>( *ptrn );
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

// Returns size of descriptor itself in dwords
inline size_t CBlockBitSetJoinComparator::getDescriptorSize()
{
	static const size_t size = sizeof(CBlockBitSetDescriptor);
	static const size_t result = ( size + ((size % sizeof(uintptr_t)) == 0 ? 0 : sizeof(uintptr_t) - (size%sizeof(uintptr_t))) ) / sizeof(uintptr_t);
	return result;
}

// Returns the number of attr blocks. Every block is one uintptr_t.
inline DWORD CBlockBitSetJoinComparator::getAttrBlockCount() const
{
	assert( blockSize > getDescriptorSize() );
	return blockSize - getDescriptorSize();
}

// Returns the pointer to the begin of array of attributes.
uintptr_t* CBlockBitSetJoinComparator::getAttrBlocks( CBlockBitSetDescriptor& descr )
{
	return reinterpret_cast<uintptr_t*>( reinterpret_cast<char*>( (&descr) ) + getDescriptorSize()*sizeof(uintptr_t) );
}

// Returns the value in the required block of attributes (const and not-const)
inline uintptr_t& CBlockBitSetJoinComparator::getAttrBlock( uintptr_t* ptr, size_t blockNum )
{
	assert( checkMemory( ptr + blockNum ) );
	assert( checkSameBlock( ptr, ptr + blockNum ) );
	return ptr[blockNum];
}
inline const uintptr_t& CBlockBitSetJoinComparator::getAttrBlock( const uintptr_t* ptr, size_t blockNum ) const
{
	assert( checkMemory( ptr + blockNum ) );
	assert( checkSameBlock( ptr, ptr + blockNum ) );
	 return ptr[blockNum];
}

// Checks if memory is within the allocator
bool CBlockBitSetJoinComparator::checkMemory( const uintptr_t* ptr, bool startOfBlock ) const
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
bool CBlockBitSetJoinComparator::checkSameBlock( const uintptr_t* p1, const uintptr_t* p2 ) const
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
inline void CBlockBitSetJoinComparator::allocate()
{
	DWORD s = 0;
	if(memory.Size() == 0) {
		s=1000;
	}else{
		const DWORD lastS = memory.Back().size() / blockSize;
		const DWORD criticalSize = 1024*1024*100;
		// TOCHANGE : Exp is so power that in certain moment we can be out of memory because of such a multiplication
		s = memory.Back().size() * sizeof(uintptr_t) < criticalSize ? lastS * 2 : criticalSize / sizeof(uintptr_t) / blockSize;
	}
	allocate( s );
}

// Allocates the requeested amount of blocks
void CBlockBitSetJoinComparator::allocate( DWORD count )
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

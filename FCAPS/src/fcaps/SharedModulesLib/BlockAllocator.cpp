// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2020, v0.8

#include "BlockAllocator.h"

using namespace std;

void CBlockAllocator::Reserve( size_t blockCount )
{
	if( GetAvailableBlockCount() >= blockCount ) {
		return;
	}
	allocate( blockCount );
}
size_t CBlockAllocator::GetAvailableBlockCount() const
{
	size_t totalBlockCount = 0;
	auto itr = memory.begin();
	for( ; itr != memory.end(); ++itr ) {
		totalBlockCount += (*itr).size();
		assert( ((*itr).size() % blockSize ) == 0 );
	}
	return totalBlockCount / blockSize;
}

bool CBlockAllocator::CheckMemory( const TElementType* ptr, bool startOfBlock ) const
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

bool CBlockAllocator::CheckSameBlock( const TElementType* p1, const TElementType* p2 ) const
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
inline void CBlockAllocator::allocate()
{
	DWORD s = 0;
	if(memory.empty() == 0) {
		s=1000;
	}else{
		const DWORD lastS = memory.back().size() / blockSize;
		const DWORD criticalSize = 1024*1024*100;
		// TOCHANGE : Exp is so power that in certain moment we can be out of memory because of such a multiplication
		s = min<DWORD>( lastS*2, criticalSize / sizeof(TElementType) / blockSize);
	}
	allocate( s );
}

// Allocates the requeested amount of blocks
void CBlockAllocator::allocate( DWORD count )
{
	memory.push_back( CMemoryBlock() );
	memory.back().resize( count * blockSize );

	TElementType* ptr = &memory.back().front();
#ifndef NDEBUG
	// memeseting memory only in debug
	memset( ptr, 0xFA, count * blockSize * sizeof(TElementType) );
#endif // NDEBUG

	for( DWORD i = 0; i < count - 1; ++i ) {
		*ptr = reinterpret_cast<TElementType>( ptr + blockSize );
		assert( checkMemory( ptr + blockSize, true ) );
		ptr += blockSize;
	}
	*ptr = reinterpret_cast<TElementType>( nextFreeBlock );

	nextFreeBlock = &memory.back().front();
	assert( checkMemory( nextFreeBlock, true ) );
}

// Checks if memory is within the allocator
bool CBlockAllocator::checkMemory( const TElementType* ptr, bool startOfBlock ) const
{
	auto itr = memory.begin();
	for( ; itr != memory.end(); ++itr ) {
		if( &(*itr).front() <= ptr && ptr < (&(*itr).back()+ 1) ) {
			if( !startOfBlock ) {
				return true;
			} else {
				const bool rslt = ptr +blockSize <= (&(*itr).back() + 1)
					&& ( (reinterpret_cast<TElementType>(ptr)
						-reinterpret_cast<TElementType>(&(*itr).front())) % (blockSize*sizeof(TElementType)) ) == 0;
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

// Allocates new block and solves problem with memory.
CBlockAllocator::TElementType* CBlockAllocator::newBlock( bool clear )
{
	++allocatedBlocks;

	if( nextFreeBlock == 0 ) {
		allocate();
	}
	assert( nextFreeBlock != 0 );
	assert( checkMemory( nextFreeBlock, true ) );

	TElementType* const nextBlock = reinterpret_cast<TElementType*>( *nextFreeBlock );
	assert( nextBlock == 0 || checkMemory(nextBlock, true ) );
#ifndef NDEBUG
	memset( nextFreeBlock, 0, blockSize * sizeof(TElementType) );
#else
	if( clear ) {
		memset( nextFreeBlock, 0, blockSize * sizeof(TElementType) );
	}
#endif // NDEBUG

	return nextBlock;
}

// Frees the block
void CBlockAllocator::freeBlock( TElementType* ptr )
{
	--allocatedBlocks;

	assert( checkMemory( ptr, true ) );
	assert( nextFreeBlock == 0 || checkMemory( nextFreeBlock, true ) );

	*ptr = reinterpret_cast<TElementType>( nextFreeBlock );
	nextFreeBlock = ptr;
}

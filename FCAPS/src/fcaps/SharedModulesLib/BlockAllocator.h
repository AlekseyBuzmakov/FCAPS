// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2020, v0.8

#ifndef CBLOCKALLOCATOR_H
#define CBLOCKALLOCATOR

#include <common.h>

#include <vector>
#include <list>
#include <stdint.h>

class CBlockAllocator {
	typedef uintptr_t TElementType;
public:
	// Get the size of the basic block
	static size_t GetElementSize()
		{return sizeof(TElementType);}

	// Get/Set the size of blocks is ints
	size_t GetBlockSize() const
		{return blockSize;}
	// Can be called only when no blocks are allocated
	void SetBlockSize(size_t size)
		{assert(allocatedBlocks == 0); blockSize = size;}

	// Reserve memory for at least n elements
	void Reserve( size_t blockCount );
	size_t GetAvailableBlockCount() const;
	size_t GetMemoryConsumption() const
		{ return allocatedBlocks * blockSize * sizeof(TElementType); }
	size_t GetTotalMemoryConsumption() const
		{ return GetAvailableBlockCount() * blockSize * sizeof(TElementType); }

	// New and free operations on top of the block alocator
	TElementType* New( bool clear)
		{ return newBlock(clear); }
	void Free( TElementType* ptr )
		{freeBlock(ptr);}

private:
	// Memory allocator related types.
	typedef std::vector<TElementType> CMemoryBlock;
	typedef std::list<CMemoryBlock> CMemory;

private:
	// Size of the blocks in elements
	size_t blockSize;
	// Memory for storing data
	CMemory memory;
	// Pointer to the next free memory
	uintptr_t* nextFreeBlock;
	// Patterns allocated
	size_t allocatedBlocks;

	void allocate();
	void allocate( DWORD count );
	bool checkMemory( const uintptr_t* ptr, bool startOfBlock ) const;
	TElementType* newBlock( bool clear );
	void freeBlock( TElementType* ptr );
};

#endif // CBLOCKALLOCATOR_H

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CBLOCKBITSETDESCRIPTOR_H
#define CBLOCKBITSETDESCRIPTOR_H

#include <fcaps/PatternManager.h>
#include "BlockAllocator.h"

////////////////////////////////////////////////////////////////////

class CBlockBitSetDescriptor : public IPatternDescriptor {
	friend class CBlockBitSetJoinComparator;
public:
	// Methods of IPatternDescriptor
	virtual bool IsMostGeneral() const
		{ return size == 0; };
	virtual size_t Hash() const
		{ return hash; }

	// Get size of the set
	size_t Size() const
		{ return size; }
private:
	size_t hash;
	size_t size;

	CBlockAllocator::TElementType* description;

	CBlockBitSetDescriptor()
		{}
	~CBlockBitSetDescriptor()
		{ assert(false); }
};

////////////////////////////////////////////////////////////////////

class CBlockBitSetJoinComparator : public  IPatternManager {
public:
	CBlockBitSetJoinComparator() :
		blockNum(0), blockSize(3), isAllocated(false) {}
	~CBlockBitSetJoinComparator();

	// Methods of IPatternManager.
	virtual const CBlockBitSetDescriptor* LoadObject( const JSON& json );
	virtual JSON SavePattern( const IPatternDescriptor* ptrn ) const;
	virtual const CBlockBitSetDescriptor* LoadPattern( const JSON& json )
		{ return LoadObject(json); }

	virtual TCompareResult Compare(
		const IPatternDescriptor* first, const IPatternDescriptor* second,
		DWORD interestingResults, DWORD possibleResults );
	virtual const CBlockBitSetDescriptor* CalculateSimilarity(
		const IPatternDescriptor* first, const IPatternDescriptor* second );

	virtual void FreePattern( const IPatternDescriptor * );
	virtual void Write( const IPatternDescriptor* pattern, std::ostream& dst ) const
		{}

	// Methods of Class
	// Get/Set maximal number of attributes.
	DWORD GetMaxAttrNumber() const;
	//  Sets the blockNum in such a way to cover at least num attributes
	void SetMaxAttrNumber( DWORD num );

	// Get/Set the number of CBlockAllocator::TElementType elements in one block
	DWORD GetBlockSize() const
		{return 1 << blockSize;}
	//   The block size is adjust in such a way to be a larger power of 2
	void SetBlockSize( DWORD s );

	// Get/Set the number of blocks
	DWORD GetBlockNumber() const
		{ return blockNum; }
	void SetBlockNumber(DWORD n);

	// Reserve memory.
	void Reserve( size_t patternNumber );

	// Allocate new pattern
	CBlockBitSetDescriptor* NewPattern()
		{ return newPattern( true ); }

	// Add new values to the descriptors.
	void AddValue( DWORD value, CBlockBitSetDescriptor& descr );
	// Enumerate values
	//   Returns the first atttribute
	int Begin( const CBlockBitSetDescriptor& d ) const
		{ return Next(d ,-1); }
	//   Returns the next attribute after prev or -1 if nothing is found
	int Next(const CBlockBitSetDescriptor& d, int prev) const;
	

	// Memory consumption
	size_t GetMemoryConsumption() const
		{ return patternAllocator.GetMemoryConsumption() + patternRefsAllocator.GetMemoryConsumption() + blockAllocator.GetMemoryConsumption(); }
	size_t GetTotalMemoryConsumption() const
		{ return patternAllocator.GetTotalMemoryConsumption() + patternRefsAllocator.GetTotalMemoryConsumption() + blockAllocator.GetTotalMemoryConsumption(); }

private:
	// Here the patterns itself are stored
	CBlockAllocator patternAllocator;
	// Here the array of reference is stored. It is used to store the references to the blocks
	CBlockAllocator patternRefsAllocator;
	// It generates the blocks
	CBlockAllocator blockAllocator;

	// The used block size
	DWORD blockSize;
	// The number of blocks in the pattern
	DWORD blockNum;

	// The function verifies that memory is allocated.
	//  After allocation it is not possible to change settings of allocation, e.g., maxAttrNum or blockSize
	bool isAllocated();
	CBlockBitSetDescriptor* newPattern( bool clear );
	void freePattern( const CBlockBitSetDescriptor& descr );

};

#endif // CBLOCKBITSETDESCRIPTOR_H

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
		maxAttrNum(0) {}
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
	DWORD GetMaxAttrNumber() const
		{ return maxAttrNum; }
	//  Can be called only once before any other commands processing.
	void SetMaxAttrNumber( DWORD num );

	// Get/Set the number of CBlockAllocator::TElementType elements in one block
	DWORD GetBlockSize() const
		{return 2 << blockSize;}
	//   The block size is adjust in such a way to be a larger power of 2
	void SetBlockSize( DWORD s );

	// Reserve memory.
	void Reserve( size_t patternNumber );

	// Allocate new pattern
	CBlockBitSetDescriptor* NewPattern()
		{ return newPattern( true ); }

	// Add new values to the descriptors.
	void AddValue( DWORD value, CBlockBitSetDescriptor& descr );

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

	// The maximal number of the attributes that can be stored in the class
	DWORD maxAttrNum;
	// The used block size
	DWORD blockSize;
	
	CBlockBitSetDescriptor* newPattern( bool clear );
	void freePattern( const CBlockBitSetDescriptor& descr );
};

#endif // CBLOCKBITSETDESCRIPTOR_H

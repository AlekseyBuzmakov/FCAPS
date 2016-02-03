// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CVECTORBINARYSETDESCRIPTOR_H
#define CVECTORBINARYSETDESCRIPTOR_H

#include <fcaps/PatternManager.h>
#include <ListWrapper.h>
#include <vector>
#include <stdint.h>

////////////////////////////////////////////////////////////////////

class CVectorBinarySetDescriptor : public IPatternDescriptor {
	friend class CVectorBinarySetJoinComparator;
public:
	// Methods of IPatternDescriptor
	virtual TPatternType GetType() const
		{ return PT_VectorBinarySet; };
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

#ifdef _DEBUG
	// The fingerprint of the created comparator
	int fingerprint;
#endif

	CVectorBinarySetDescriptor();
};

////////////////////////////////////////////////////////////////////

class CVectorBinarySetJoinComparator : public  IPatternManager {
public:
	CVectorBinarySetJoinComparator();

	// Methods of IPatternManager.
	virtual TPatternType GetPatternsType() const
		{ return PT_VectorBinarySet; }

	virtual const CVectorBinarySetDescriptor* LoadObject( const JSON& json );
	virtual JSON SavePattern( const IPatternDescriptor* ptrn ) const;
	virtual const CVectorBinarySetDescriptor* LoadPattern( const JSON& json );

	virtual TCompareResult Compare(
		const IPatternDescriptor* first, const IPatternDescriptor* second,
		DWORD interestingResults, DWORD possibleResults );
	virtual const CVectorBinarySetDescriptor* CalculateSimilarity(
		const IPatternDescriptor* first, const IPatternDescriptor* second );

	virtual void FreePattern( const IPatternDescriptor * );

	virtual void Write( const IPatternDescriptor* pattern, std::ostream& dst ) const;

	// Methods of Class
	// Get/Set maximal number of attributes.
	DWORD GetMaxAttrNumber() const;
	//  Can be called only once before any other commands processing.
	void SetMaxAttrNumber( DWORD num );

	// Reserve memory.
	void Reserve( size_t blockCount );
	size_t GetAvailableBlockCount() const;

	// Non virtual method for comparison and intersection
	TCompareResult Compare(
		const CVectorBinarySetDescriptor& first, const CVectorBinarySetDescriptor& second,
		DWORD interestingResults = CR_AllResults, DWORD possibleResults = CR_AllResults | CR_Incomparable ) const;
	const CVectorBinarySetDescriptor* CalculateSimilarity(
		const CVectorBinarySetDescriptor& first, const CVectorBinarySetDescriptor& second );

	// Set ids to names map, for writing proposes
	const std::vector<std::string>& GetNames() const
		{ return names; }
	void SetNames( const std::vector<std::string>& newNames )
		{ names = newNames; }

	// Allocate new pattern
	CVectorBinarySetDescriptor* NewPattern()
		{ return newPattern( true ); }

	// Add new values to the descriptors.
	void AddList( const CList<DWORD>& values, CVectorBinarySetDescriptor& descr );
	void AddValue( DWORD value, CVectorBinarySetDescriptor& descr );

	// Enumerate values in the descriptor.
	void EnumValues( const CVectorBinarySetDescriptor& descr, CList<DWORD>& result ) const;

	// Get/Set should write names
	bool GetWriteNames() const
		{ return shouldWriteNames; }
	void SetWriteNames(bool b)
		{ shouldWriteNames = b; }

private:
	// Memory allocator related types.
	typedef std::vector<uintptr_t> CMemoryBlock;
	typedef CList<CMemoryBlock> CMemory;
private:
	// Size of one block equal to size of CVectorBinarySetDescriptor + buffer
	size_t blockSize;
	// Memory for storing data
	CMemory memory;
	// Pointer to the next free memory
	uintptr_t* nextFreeBlock;
	// Patterns allocated
	size_t allocatedPatterns;

	// Names of attributes
	std::vector<std::string> names;
	// Should the names be written
	bool shouldWriteNames;

#ifdef _DEBUG
	// The fingerprint of itself for its patterns
	int fingerprint;
#endif


	static const CVectorBinarySetDescriptor& getVectorBinarySet( const IPatternDescriptor* );
	static TCompareResult checkCompareResult( TCompareResult result, DWORD interestingResults );

	static size_t getDescriptorSize();
	DWORD getAttrBlockCount() const;
	uintptr_t* getAttrBlocks( CVectorBinarySetDescriptor& descr );
	const uintptr_t* getAttrBlocks( const CVectorBinarySetDescriptor& descr ) const
		{ return const_cast<CVectorBinarySetJoinComparator*>(this)->getAttrBlocks( const_cast<CVectorBinarySetDescriptor&>(descr) ); }
	uintptr_t& getAttrBlock( uintptr_t* ptr, size_t blockNum );
	const uintptr_t& getAttrBlock( const uintptr_t* ptr, size_t blockNum ) const;
	bool checkMemory( const uintptr_t*, bool startOfBlock = false ) const;
	bool checkSameBlock( const uintptr_t* p1, const uintptr_t* p2 ) const;

	void allocate();
	void allocate( DWORD count );

	CVectorBinarySetDescriptor* newPattern( bool clear );
	void freePattern( const CVectorBinarySetDescriptor& descr );

	static uintptr_t getNextBit( uintptr_t block, char bit );
};

#endif // CVECTORBINARYSETDESCRIPTOR_H

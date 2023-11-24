// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8
// A module that pass over a pattern space in breath first order expanding in the direction of the most promissing patterns

#ifndef STABILITYCbOLOCALPROJECTIONCHAIN_H
#define STABILITYCbOLOCALPROJECTIONCHAIN_H

#include <fcaps/LocalProjectionChain.h>

#include <fcaps/PatternManager.h>

#include <ModuleTools.h>

#include <deque>

////////////////////////////////////////////////////////////////////

interface IPatternDescriptor;
interface IContextAttributes;

////////////////////////////////////////////////////////////////////
class CPattern;
class CBinarySetDescriptorsComparator;
class CVectorBinarySetJoinComparator;
class CVectorBinarySetDescriptor;
class CBinarySetPatternDescriptor;
class CIgnoredAttrs;
////////////////////////////////////////////////////////////////////

const char StabilityCbOLocalProjectionChain[] = "StabilityCbOLocalProjectionChainModule";

////////////////////////////////////////////////////////////////////
// A class for storing intents in a tree
class CIntentsTree {
public:
	typedef uint_fast32_t TAttribute;
	typedef int_fast32_t TIntent;
	typedef TIntent TIntentItr;

	static const TAttribute InvalidAttribute;

public:
	CIntentsTree() :
		freeNode(-1) {}

	// Returns the iterator to the END of the list
	TIntentItr GetIterator(TIntent intent) const
		{return intent;};
	
	// Returns the value of the next attribute,
	//  and changes the iterator to the next position
	TAttribute GetNextAttribute(TIntentItr& itr) const;

	// Add attributes to the intent
	//  the old intent is INVALIDATED
	TIntent AddAttribute(TIntent intent, TAttribute newAttr);

	// Creating of a new intent
	TIntent Create()
		{return -1;}
	// Copying an existing intent
	TIntent Copy(TIntent intent)
		{return intent;}
	// Deletion of an intent
	void Delete(TIntent intent);

	//TOKILL
	int Size() const
		{return memory.size();}
	int MemorySize() const { return memory.size() * sizeof(TTreeNode) + sizeof(CIntentsTree);}

private:
	struct TTreeNode{
		TAttribute Attribute;
		TIntentItr Next;
		int_fast32_t BranchesCount;

		TTreeNode() :
			Attribute(InvalidAttribute),Next(-1),BranchesCount(0) {}
	};

private:
	// The memory the whole tree is stored in
	std::deque<TTreeNode> memory;
	// The pointer to the next free node
	TIntentItr freeNode;

	TIntentItr newNode();
};

//////////////////////////////////////////////////////////////////////////////////////////////

class CStabilityCbOLocalProjectionChain : public ILocalProjectionChain, public IModule {
public:
	CStabilityCbOLocalProjectionChain();
	~CStabilityCbOLocalProjectionChain();
	// Methods of ILocalProjectionChain
	virtual int GetObjectNumber() const;
	virtual double GetInterestThreshold() const;
	virtual void UpdateInterestThreshold( const double& thld );
	virtual double GetPatternInterest( const IPatternDescriptor* p );
	// TODO: probably will be used later
	// virtual bool AreEqual(const IPatternDescriptor* p, const IPatternDescriptor* q) const;
	// virtual bool IsSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const;
	virtual bool IsTopoSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const;
	virtual void FreePattern(const IPatternDescriptor* p ) const;
	virtual void ComputeZeroProjection( CPatternList& ptrns );
	virtual TPreimageResult Preimages( const IPatternDescriptor* d, CPatternList& preimages );

	virtual bool IsExpandable( const IPatternDescriptor* d ) const;
	virtual bool IsFinalInterestKnown( const IPatternDescriptor* d ) const;

	virtual int GetExtentSize( const IPatternDescriptor* d ) const;
	virtual JSON SaveExtent( const IPatternDescriptor* d ) const;
	virtual JSON SaveIntent( const IPatternDescriptor* d ) const;
	virtual size_t GetTotalAllocatedPatterns() const;
	virtual size_t GetTotalConsumedMemory() const;

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };

	// For CModuleRegistrar
	static const char* const Type()
		{ return LocalProjectionChainModuleType;}
	static const char* const Name()
		{ return StabilityCbOLocalProjectionChain; }
	static const char* const Desc();

private:
	enum TChildAnalysisMode {
		CAM_None = 0, // No analysis of attributes larger than projection
		CAM_Cls, // Computing only closure of the intent 
		CAM_Unstable, // Computing closure and adding unstable attributes to the intent
		CAM_MinDeltaFirst, // Finding also attribute that generate the closed child

		CAM_EnumCount
	};

private:
	static const CModuleRegistrar<CStabilityCbOLocalProjectionChain> registrar;
	// Object that enumerates attribute extents
	CSharedPtr<IContextAttributes> attrs;
	// Cached attributes
	std::deque<const CVectorBinarySetDescriptor*> attrsHolder;
	// The threshold for delta measure
	double thld;
	// Comparator for extents
	CSharedPtr<CVectorBinarySetJoinComparator> extCmp;
	CPatternDeleter extDeleter;
	// Holder for the intents
	CIntentsTree intentsTree;
	// A temporary storage for intents. Here for not allocating memory too often
	std::vector<int> intentStorage;
	// A flag indicating if all attributes for a concept should be processed in once
	bool areAllInOnce;
	TChildAnalysisMode childAnalysisMode;


	// Memory consumption
	mutable size_t totalAllocatedPatterns;
	mutable size_t totalAllocatedPatternSize;

	const CPattern& to_pattern(const IPatternDescriptor* d) const;
	const CPattern* newPattern(
		const CVectorBinarySetDescriptor* ext,
		CIntentsTree::TIntent intent,
		CIgnoredAttrs& ignored,
		int nextAttr, DWORD delta, int clossestAttr = 0,
		int nextMostClosedAttr = -1);
	const CVectorBinarySetDescriptor* getAttributeImg(int a);
	const CPattern* initializeNewPattern(
		const CPattern& parent,
		int genAttr, int kernelAttr,
		std::unique_ptr<const CVectorBinarySetDescriptor,CPatternDeleter>& ext);
	int getNextAttribute( const CPattern& p) const;
	int switchToNextProjection( const CPattern& p) const;
	int getNextKernelAttribute( const CPattern& p) const;
	DWORD getAttributeDelta(int a, const CVectorBinarySetDescriptor& ext);
};

#endif // STABILITYCbOLOCALPROJECTIONCHAIN_H

// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8
// A module that pass over a pattern space in breath first order expanding in the direction of the most promissing patterns

#ifndef STABILITYCbOLOCALPROJECTIONCHAIN_H
#define STABILITYCbOLOCALPROJECTIONCHAIN_H

#include <fcaps/LocalProjectionChain.h>

#include <fcaps/PatternManager.h>

// #include <ListWrapper.h>
#include <ModuleTools.h>

// #include <rapidjson/document.h>

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
	// Methods of IComputationProcedure
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
	virtual bool Preimages( const IPatternDescriptor* d, CPatternList& preimages );
	virtual int GetExtentSize( const IPatternDescriptor* d ) const;
	virtual JSON SaveExtent( const IPatternDescriptor* d ) const;
	virtual JSON SaveIntent( const IPatternDescriptor* d ) const;

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
	static const CModuleRegistrar<CStabilityCbOLocalProjectionChain> registrar;
	// Object that enumerates attribute extents
	CSharedPtr<IContextAttributes> attrs;
	// Cached attributes
	std::deque< CSharedPtr<const CVectorBinarySetDescriptor> > attrsHolder;
	// The threshold for delta measure
	double thld;
	// Comparator for extents
	CSharedPtr<CVectorBinarySetJoinComparator> extCmp;
	CPatternDeleter extDeleter;
	// Holder for the intents
	CIntentsTree intentsTree;
	// A temporary storage for intents. Here for not allocating memory too often
	std::vector<int> intentStorage;

	const CPattern& to_pattern(const IPatternDescriptor* d) const;
	const CPattern* newPattern(
		const CSharedPtr<const CVectorBinarySetDescriptor>& ext,
		CIntentsTree::TIntent intent,
		int nextAttr, DWORD delta, int clossestAttr = 0);
	void getAttributeImg(int a, CSharedPtr<const CVectorBinarySetDescriptor>& rslt);
	const CPattern* initializeNewPattern(
		const CPattern& parent,
		int genAttr,
		const CSharedPtr<const CVectorBinarySetDescriptor>& ext);
	DWORD getAttributeDelta(int a, const CSharedPtr<const CVectorBinarySetDescriptor>& ext);
};

#endif // STABILITYCbOLOCALPROJECTIONCHAIN_H

// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8
// A module that pass over a pattern space in breath first order expanding in the direction of the most promissing patterns

#ifndef STABILITYLPCBYPATRICIATREE_H
#define STABILITYLPCBYPATRICIATREE_H

#include <fcaps/LocalProjectionChain.h>

#include <fcaps/PatternManager.h>

#include <ModuleTools.h>

#include <deque>

#include "details/PatritiaTree.h"
#include "details/CountingAllocator.h"

////////////////////////////////////////////////////////////////////

interface IPatternDescriptor;
interface IBinContextReader;

////////////////////////////////////////////////////////////////////
class CPTPattern;
class CBinarySetDescriptorsComparator;
class CVectorBinarySetJoinComparator;
class CVectorBinarySetDescriptor;
class CBinarySetPatternDescriptor;
class CIgnoredAttrs;
////////////////////////////////////////////////////////////////////

const char StabilityLPCbyPatriciaTree[] = "StabilityLPCbyPatriciaTreeModule";

//////////////////////////////////////////////////////////////////////////////////////////////

class CStabilityLPCbyPatriciaTree : public ILocalProjectionChain, public IModule {
public:
	CStabilityLPCbyPatriciaTree();
	~CStabilityLPCbyPatriciaTree();

	// Methods of ILocalProjectionChain
	virtual int GetObjectNumber() const;
	virtual double GetInterestThreshold() const;
	virtual void UpdateInterestThreshold( const double& thld );
	virtual double GetPatternInterest( const IPatternDescriptor* p );
	virtual bool IsTopoSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const;
	virtual void FreePattern(const IPatternDescriptor* p ) const;
	virtual void ComputeZeroProjection( CPatternList& ptrns );
	virtual TPreimageResult Preimages( const IPatternDescriptor* d, CPatternList& preimages );

	virtual bool IsExpandable( const IPatternDescriptor* d ) const;
	virtual bool IsFinalInterestKnown( const IPatternDescriptor* d ) const
	  { return IsExpandable(d); }
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
		{ return StabilityLPCbyPatriciaTree; }
	static const char* const Desc();

private:
	static const CModuleRegistrar<CStabilityLPCbyPatriciaTree> registrar;
	// An object that counts the consumed memory
	CMemoryCounter memoryCounter;

	// Object that enumerates attribute extents
	CSharedPtr<IBinContextReader> attrs;
	// The patricia tree of the context
	CPatritiaTree pTree;
	// The maximal attribute number found in the tree
	int maxAttribute;

	// The correspondance between the attributes and the nodes of pTree
	// std::deque< std::list<const CPatritiaTreeNode*> > attributes;
	// The threshold for delta measure
	double thld;

	// Should all extension be found at once
	bool areAllInOnce;

	// Memory consumption
	mutable size_t totalAllocatedPatterns;

	void buildPatritiaTree();
	// void extractAttrsFromPatritiaTree();
	// void addAttributeNode(CPatritiaTree::TAttribute attr, const CPatritiaTreeNode& node);

	void buildPatritiaTree2();
	void insertObjectToPTNode(CPatritiaTree::TNodeIndex nodeId, std::set<int>& intent,
	                          std::multimap<const CPatritiaTreeNode*, CPatritiaTree::TObject>& nodeToObjectMap, CPatritiaTree::TObject objectId);
	bool checkNodeValidity(const CPatritiaTreeNode& node);
	
	void addObjectsToPTNodes(std::multimap<const CPatritiaTreeNode*, CPatritiaTree::TObject>& nodeToObjectMap);
	void computeCommonAttributesinPT();
	void computeNextAttributeIntersectionsinPT();
	bool checkPTValidity();

	const CPTPattern& to_pattern(const IPatternDescriptor* d) const;
	CPTPattern* computePreimage(const CPTPattern& p, CPatritiaTree::TAttribute a);

	bool initializePreimage(const CPTPattern& parent, int genAttr, CPTPattern& res);
	DWORD getAttributeDelta(const CPTPattern& p, CPatritiaTree::TAttribute a, DWORD maxDelta);
	bool checkAttributeDeltaProblem(const CPTPattern& p, const CPTPattern& ch, CPatritiaTree::TAttribute a);
};

#endif // STABILITYLPCBYPATRICIATREE_H

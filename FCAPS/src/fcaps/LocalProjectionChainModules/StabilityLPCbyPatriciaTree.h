// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8
// A module that pass over a pattern space in breath first order expanding in the direction of the most promissing patterns

#ifndef STABILITYLPCBYPATRICIATREE_H
#define STABILITYLPCBYPATRICIATREE_H

#include <fcaps/LocalProjectionChain.h>

#include <fcaps/PatternManager.h>

#include <ModuleTools.h>

#include <deque>

////////////////////////////////////////////////////////////////////

interface IPatternDescriptor;
interface IBinContextReader;

////////////////////////////////////////////////////////////////////
class CPattern;
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
	// Object that enumerates attribute extents
	CSharedPtr<IBinContextReader> attrs;
	// The patricia tree of the context
	CPatritiaTree pTree;
	// Cached attributes
	std::deque<const CVectorBinarySetDescriptor*> attrsHolder;
	// The threshold for delta measure
	double thld;
	// Comparator for extents
	CSharedPtr<CVectorBinarySetJoinComparator> extCmp;
	CPatternDeleter extDeleter;
	// A temporary storage for intents. Here for not allocating memory too often
	std::vector<int> intentStorage;

	// Memory consumption
	mutable size_t totalAllocatedPatterns;
	mutable size_t totalAllocatedPatternSize;

	void buildPatritiaTree();
	const CPattern& to_pattern(const IPatternDescriptor* d) const;
	const CPattern* newPattern(
		const CVectorBinarySetDescriptor* ext,
		CIntentsTree::TIntent intent,
		CIgnoredAttrs& ignored,
		int nextAttr, DWORD delta, int clossestAttr = 0);
	const CVectorBinarySetDescriptor* getAttributeImg(int a);
	const CPattern* initializeNewPattern(
		const CPattern& parent,
		int genAttr,
		std::unique_ptr<const CVectorBinarySetDescriptor,CPatternDeleter>& ext);
	DWORD getAttributeDelta(int a, const CVectorBinarySetDescriptor& ext);
};

#endif // STABILITYLPCBYPATRICIATREE_H

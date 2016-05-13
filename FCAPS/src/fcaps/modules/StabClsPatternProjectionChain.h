// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Implementation of a projection chain that takes patterns (its images) from an algorithm iterating patterns, e.g., frequent graphs,
//	 and based on these patterns find stable sets of these patterns.

#ifndef STABCLSPATTERNPROJECTIONCHAIN_H
#define STABCLSPATTERNPROJECTIONCHAIN_H

#include <fcaps/ProjectionChain.h>
#include <fcaps/PatternEnumerator.h>

#include <fcaps/Module.h>
#include <ModuleTools.h>
#include <fcaps/modules/VectorBinarySetDescriptor.h>

#include <vector>

////////////////////////////////////////////////////////////////////////

const char StabClsPatternProjectionChainModule [] = "StabClsPatternProjectionChainModule";

////////////////////////////////////////////////////////////////////////

class CStabClsPatternProjectionChain : public IProjectionChain, public IModule {
public:
	CStabClsPatternProjectionChain();

	// Methods of IProjectionChain
	virtual const std::vector<std::string>& GetObjNames() const;
	virtual void SetObjNames( const std::vector<std::string>& names );

	virtual void AddObject( DWORD objectNum, const JSON& intent );
	virtual void UpdateInterestThreshold( const double& thld );
	virtual double GetPatternInterest( const IPatternDescriptor* p );
	virtual bool AreEqual(const IPatternDescriptor* p, const IPatternDescriptor* q) const;
	virtual bool IsSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const;
	virtual bool IsTopoSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const;
	virtual void FreePattern(const IPatternDescriptor* p ) const;
	virtual void ComputeZeroProjection( CPatternList& ptrns );
	virtual bool NextProjection();
	virtual double GetProgress() const;
	virtual void Preimages( const IPatternDescriptor* d, CPatternList& preimages );
	virtual JSON SaveExtent( const IPatternDescriptor* d ) const;
	virtual JSON SaveIntent( const IPatternDescriptor* d ) const;

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
	{ return ProjectionChainModuleType; };
	virtual const char* const GetName() const
	{ return StabClsPatternProjectionChainModule; };

protected:
	class CStabClsPatternDescription;

private:
	static CModuleRegistrar<CStabClsPatternProjectionChain> registar;
	// Number of added objects
	DWORD objectCount;
	// Number of enumerated patterns
	DWORD patternCount;
	CSharedPtr<IPatternEnumerator> enumerator;
	// Comparator for extents
	CSharedPtr<CVectorBinarySetJoinComparator> extCmp;
	CPatternDeleter extDeleter;
	// Threshold for DMeasure
	DWORD thld;
	// The image of the next foundto process
	CSharedPtr<CVectorBinarySetDescriptor> nextImage;
	// Weather stable patterns were created for current projection
	bool isStablePtrnFound;
	// Requested reserve for patterns
	DWORD requestedReserve;

	const CStabClsPatternDescription& Ptrn( const IPatternDescriptor* p ) const;
	bool initializeNewPattern( const CStabClsPatternDescription& parent, CStabClsPatternDescription& newPtrn);
	void addChild( const CSharedPtr<const CVectorBinarySetDescriptor>& child, const CStabClsPatternDescription& ptrn );
};

#endif // STABCLSPATTERNPROJECTIONCHAIN_H

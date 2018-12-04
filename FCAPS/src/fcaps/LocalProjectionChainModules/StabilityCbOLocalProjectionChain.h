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
	// Comparator for intents;
	CSharedPtr<CBinarySetDescriptorsComparator> intCmp;
	CPatternDeleter intDeleter;

	const CPattern& to_pattern(const IPatternDescriptor* d) const;
	const CPattern* newPattern(
		const CSharedPtr<const CVectorBinarySetDescriptor>& ext,
		const CSharedPtr<CBinarySetPatternDescriptor>& intent,
	int nextAttr, DWORD delta, int clossestAttr = 0);
	void getAttributeImg(int a, CSharedPtr<const CVectorBinarySetDescriptor>& rslt);
	const CPattern* initializeNewPattern(
		const CPattern& parent,
		int genAttr,
		const CSharedPtr<const CVectorBinarySetDescriptor>& ext);
	DWORD getAttributeDelta(int a, const CSharedPtr<const CVectorBinarySetDescriptor>& ext);
};

#endif // STABILITYCbOLOCALPROJECTIONCHAIN_H

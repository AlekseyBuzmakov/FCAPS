#ifndef CSTABBINCLSPATTERNSPROJECTIONCHAIN_H
#define CSTABBINCLSPATTERNSPROJECTIONCHAIN_H

#include <fcaps/modules/details/BinClsPatternsProjectionChain.h>
#include <fcaps/Module.h>
#include <fcaps/ModuleTools.h>

#include <fcaps/tools/StabilityChildrenApproximation.h>

////////////////////////////////////////////////////////////////////

char StabBinClsPatternsProjectionChain[] = "StabBinClsPatternsProjectionChainModule";

class CStabBinClsPatternsProjectionChain : public CBinClsPatternsProjectionChain, public IModule {
	public:
		CStabBinClsPatternsProjectionChain();
	// Methods of IProjectionChain
	virtual void UpdateInterestThreshold( const double& thld );
	virtual double GetPatternInterest( const IPatternDescriptor* p );
	virtual void ComputeZeroProjection( CPatternList& ptrns );
	virtual bool NextProjection();
	virtual void Preimages( const IPatternDescriptor* d, CPatternList& preimages );
	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
protected:
	class CStabPatternDescription : public CPatternDescription {
	public:
		CStabPatternDescription(const CSharedPtr<const CVectorBinarySetDescriptor>& ext) :
			CPatternDescription( ext ), stability(0),stabAttrNum(-1),minAttr(-1) {}

		double& Stability() const { return stability; }
		DWORD& StabAttrNum() const { return stabAttrNum;}
		DWORD& MinAttr() const { return minAttr;}
	private:
		// The stability of a pattern
		mutable double stability;
		// The number of attributes the stability is computed for
		mutable DWORD stabAttrNum;
		// An attribute index the most closed to this pattern
		mutable DWORD minAttr;
	};
	// Methods of CBinClsPatternsProjectionChain
	virtual CStabPatternDescription* NewPattern(const CSharedPtr<const CVectorBinarySetDescriptor>& ext)
		{ return new CStabPatternDescription(ext); }
	// Methods of the class
	const CStabPatternDescription& StabPattern( const CPatternDescription& d);

private:
	static CModuleRegistrar<CStabBinClsPatternsProjectionChain> registar;
	// Approximation of stability by direct descendents.
	CStabilityChildrenApproximation stabApprox;

	bool canBeStable( const CVectorBinarySetDescriptor& d, DWORD minAttr );
};

#endif // CSTABBINCLSPATTERNSPROJECTIONCHAIN_H

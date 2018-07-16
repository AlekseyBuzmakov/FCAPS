// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CSTABBINCLSPATTERNSPROJECTIONCHAIN_H
#define CSTABBINCLSPATTERNSPROJECTIONCHAIN_H

#include <fcaps/SofiaModules/details/BinClsPatternsProjectionChain.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <fcaps/SharedModulesLib/StabilityChildrenApproximation.h>

////////////////////////////////////////////////////////////////////

const char StabBinClsPatternsProjectionChain[] = "StabBinClsPatternsProjectionChainModule";

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
	virtual const char* const GetType() const
		{ return ProjectionChainModuleType; };
	virtual const char* const GetName() const
		{ return StabBinClsPatternsProjectionChain; };

protected:
	class CStabPatternDescription : public CPatternDescription {
	public:
		CStabPatternDescription(const CVectorBinarySetJoinComparator& cmp, const CSharedPtr<const CVectorBinarySetDescriptor>& ext) :
			CPatternDescription(cmp, ext), stability(0),stabAttrNum(-1),minAttr(-1), globMinAttr( -1 ), globMinValue( -1 ) {}

		double& Stability() const { return stability; }
		DWORD& StabAttrNum() const { return stabAttrNum;}
		DWORD& MinAttr() const { return minAttr;}

		void SetGlobMinAttr( DWORD minAttr, DWORD minValue ) const
			{ globMinAttr = minAttr; globMinValue = minValue; NextOptimAttr()=minAttr; }
		const DWORD& GlobMinAttr() const {return globMinAttr; }
		const DWORD& GlobMinValue() const {return globMinValue; }
	private:
		// The stability of a pattern
		mutable double stability;
		// The number of attributes the stability is computed for
		mutable DWORD stabAttrNum;
		// An attribute index the most closed to this pattern
		mutable DWORD minAttr;

		// Globally minimal attribute number and the corresponding stability value.
		mutable DWORD globMinAttr;
		mutable DWORD globMinValue;
	};
	// Methods of CBinClsPatternsProjectionChain
	virtual CStabPatternDescription* NewPattern(const CSharedPtr<const CVectorBinarySetDescriptor>& ext)
		{ return new CStabPatternDescription(ExtCmp(), ext); }
	virtual void ReportAttrSimilarity( const CPatternDescription& p, DWORD i, const CVectorBinarySetDescriptor& res );
	// Methods of the class
	const CStabPatternDescription& StabPattern( const CPatternDescription& d);

private:
	static CModuleRegistrar<CStabBinClsPatternsProjectionChain> registar;
	// Approximation of stability by direct descendents.
	CStabilityChildrenApproximation stabApprox;

	bool canBeStable( const CVectorBinarySetDescriptor& d, DWORD minAttr );
};

#endif // CSTABBINCLSPATTERNSPROJECTIONCHAIN_H

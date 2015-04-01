#ifndef CSTABINTERVALCLSPATTERNSPROJECTIONCHAIN_H
#define CSTABINTERVALCLSPATTERNSPROJECTIONCHAIN_H

#include <fcaps/modules/details/IntervalClsPatternsProjectionChain.h>
#include <fcaps/Module.h>
#include <fcaps/ModuleTools.h>

////////////////////////////////////////////////////////////////////

const char StabIntervalClsPatternsProjectionChain[] = "StabIntervalClsPatternsProjectionChainModule";

class CStabIntervalClsPatternsProjectionChain : public CIntervalClsPatternsProjectionChain, public IModule {
public:
	CStabIntervalClsPatternsProjectionChain();

	// Methods of IProjectionChain
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
		{ return StabIntervalClsPatternsProjectionChain; };

protected:
	class CStabPatternDescription : public CPatternDescription {
	public:
		CStabPatternDescription(const CSharedPtr<const CVectorBinarySetDescriptor>& ext) :
			CPatternDescription( ext ), stability(0),stabState(-1),minAttr(-1) {}

		double& Stability() const { return stability; }
		DWORD& StabState() const { return stabState;}
		int& MinAttr() const { return minAttr;}
	private:
		// The stability of a pattern
		mutable double stability;
		// The number of attributes the stability is computed for
		mutable DWORD stabState;
		// An attribute 1-based index the most closed to this pattern
		//  a negative value for the left interval and positive for the right part of the interval.
		//  0 value means not known.
		mutable int minAttr;
	};

protected:
	// Methods of CBinClsPatternsProjectionChain
	virtual CStabPatternDescription* NewPattern(const CSharedPtr<const CVectorBinarySetDescriptor>& ext)
		{ return new CStabPatternDescription(ext); }
	// Methods of the class
	const CStabPatternDescription& Pattern( const IPatternDescriptor* p )
		{ return debug_cast<const CStabPatternDescription&>(*p); }
	const CStabPatternDescription& Pattern( const CPatternDescription& p )
		{ return debug_cast<const CStabPatternDescription&>(p); }

private:
	static CModuleRegistrar<CStabIntervalClsPatternsProjectionChain> registar;

	DWORD currStateNum;

	const double& computeStability( const CStabPatternDescription& p ) const;
	bool isStable(const CStabPatternDescription& p, int attr ) const;
	bool isLeftStable(const CList<DWORD>& ext, const CStabPatternDescription& p, int attr ) const;
	bool isRightStable(const CList<DWORD>& ext, const CStabPatternDescription& p, int attr ) const;
	bool isLeftStableForAttr(const CList<DWORD>& ext, const CStabPatternDescription& p, int attr ) const;
	bool isRightStableForAttr(const CList<DWORD>& ext, const CStabPatternDescription& p, int attr ) const;
	bool canChangeLeft(const CStabPatternDescription& p, int attr ) const;
	bool canChangeRight(const CStabPatternDescription& p, int attr ) const;
	DWORD computeLeftIntersectionSize(const CList<DWORD>& ext, int attr, DWORD valueNum) const;
	DWORD computeRightIntersectionSize(const CList<DWORD>& ext, int attr, DWORD valueNum) const;
};

#endif // CSTABINTERVALCLSPATTERNSPROJECTIONCHAIN_H

// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, GPL v2 license, 2018, v0.7

#ifndef WESTFALLYOUNGOPTIMISTICESTIMATOR_H
#define WESTFALLYOUNGOPTIMISTICESTIMATOR_H

#include <common.h>

#include <fcaps/WestfallYoungOptimisticEstimator.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <unordered_set>

////////////////////////////////////////////////////////////////////

const char WYOptimisticEstimator[] = "WYOptimisticEstimatorModule";

////////////////////////////////////////////////////////////////////

class CWYOEst : public IWestfallYoungOptimisticEstimator, public IModule {
public:
	CWYOEst();

	// Methods of IWestfallYoungOptimisticEstimator
	virtual bool CheckObjectNumber(DWORD n) const
	    { assert(oest != 0); return oest->CheckObjectNumber(n); }
	virtual int GetPermutationCount() const
	    { return permutationCount; }
	virtual void GetValue(const IExtent* ext, CWYOEstValue& val ) const;
	virtual bool IsBetter(const double& a, const double& b) const
		{ assert(oest != 0); return oest->IsBetter(a,b); }
	virtual JSON GetJsonQuality(const IExtent* ext) const
		{ assert(oest != 0); return oest->GetJsonQuality(ext); }

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return WestfallYoungOptimisticEstimatorModuleType;}
	static const char* const Name()
		{ return WYOptimisticEstimator; }
	static const char* const Desc();

private:
	static const CModuleRegistrar<CWYOEst> registrar;
	// This this the external object that evaluates Q and OEst of Q on original and permutated objects
	CSharedPtr<IOptimisticEstimator> oest;
	// Number of permutations that need to be computed
	DWORD permutationCount;
};

#endif // WESTFALLYOUNGOPTIMISTICESTIMATOR_H

// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, GPL v2 license, 2018, v0.7

#ifndef LOCALTREATMENTEFFECTOEST_H
#define LOCALTREATMENTEFFECTOEST_H

#include <common.h>

#include <fcaps/OptimisticEstimator.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <unordered_set>

////////////////////////////////////////////////////////////////////

const char LocalTreatmentEffectOptimisticEstimator[] = "LocalTreatmentEffectOptimisticEstimatorModule";

////////////////////////////////////////////////////////////////////

class CLocalTreatmentEffectOEst : public IOptimisticEstimator, public IModule {
public:
	CLocalTreatmentEffectOEst();

	virtual bool CheckObjectNumber(DWORD n) const;
	// Methods of IOptimisticEstimator
	virtual double GetValue(const IExtent* ext) const;
	virtual double GetBestSubsetEstimate(const IExtent* ext) const;
	virtual JSON GetJsonQuality(const IExtent* ext) const; 

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return OptimisticEstimatorModuleType; };
	virtual const char* const GetName() const
		{ return LocalTreatmentEffectOptimisticEstimator; };

private:
	static const CModuleRegistrar<CLocalTreatmentEffectOEst> registrar;
	// The filename with text classes
	std::string objInfoFilePath;
	// The set of values that should be considered as control
	std::unordered_set<std::string> cntrlLevels;
	// The response and the treatment of every object
	std::vector<double> objY;
	std::vector<bool> objTrt;
};

#endif // LOCALTREATMENTEFFECTOEST_H

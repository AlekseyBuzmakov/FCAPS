// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2018, v0.7

// Author: Aleksey Buzmakov
// Description: An interface for optimistic estimater as it defined in Subgroup Discovery. 

#ifndef PATTERNENUMERATOR_H_INCLUDED
#define PATTERNENUMERATOR_H_INCLUDED

#include <common.h>
#include <fcaps/BasicTypes.h>
#include <fcaps/Extent.h>

////////////////////////////////////////////////////////////////////////

const char OptimisticEstimatorModuleType[] = "OptimisticEstimatorModules";

interface IOptimisticEstimator : public virtual IObject {
	struct COEstValue {
		double Value;
		double BestSubsetEstimate;

		COEstValue() : Value(-1), BestSubsetEstimate(-1) {}
	};

	// Check if the availbale number of objects is suitable for the Estimator
	virtual bool CheckObjectNumber(DWORD) const = 0;
	// Returns the exact value and the best subset estimate for the extent ext
	virtual void GetValue(const IExtent* ext, COEstValue& val ) const = 0;
	// A predicate verifying if one value is better than another value
	virtual bool IsBetter(const double& a, const double& b) const 
		{return a > b;}
	// Get JSON description of pattern quality
	virtual JSON GetJsonQuality(const IExtent* ext) const = 0;
};


#endif // PATTERNENUMERATOR_H_INCLUDED

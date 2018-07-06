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
	// Returns the exact value of the extent ext
	virtual double GetValue(const IExtent* ext) const = 0;
	// Returns the best estimate of the value on extent subsets
	virtual double GetBestSubsetEstimate(const IExtent* ext) const = 0;
	// A predicate verifying if one value is better than another value
	virtual bool IsBetter(const double& a, const double& b) const 
		{return a > b;}

};

#endif // PATTERNENUMERATOR_H_INCLUDED

// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2018, v0.7

// Author: Aleksey Buzmakov
// Description: An interface for optimistic estimator with WestFall Young Permotation procedure as a statistical test for pattern significance

#ifndef WESTFALLYOUNGOPTIMISTICESTIMATOR_H_INCLUDED
#define WESTFALLYOUNGOPTIMISTICESTIMATOR_H_INCLUDED

#include <common.h>
#include <fcaps/BasicTypes.h>
#include <fcaps/Extent.h>
#include <fcaps/OptimisticEstimator.h>

////////////////////////////////////////////////////////////////////////

const char WestfallYoungOptimisticEstimatorModuleType[] = "WestfallYoungOptimisticEstimatorModules";

interface IWestfallYoungOptimisticEstimator : public virtual IObject {
	typedef IOptimisticEstimator::COEstValue COEstValue;
	struct CWYOEstValue {
		// Quality of the pattern
		COEstValue Value;
		// Number of permutations
		int PermutationCount;
		// An array with PermutationCount number of values
		COEstValue* PermuttedValues;

		CWYOEstValue():
			PermutationCount(0),PermuttedValues(0)  {}
		void SetPermutationUsage(int i, bool shouldBeUsed)
			{ assert( 0 <= i && i < PermutationCount); PermuttedValues[i].Value = shouldBeUsed ? -1 : -1e6;}
		bool IsPermutationUsed(int i) 
			{ assert( 0 <= i && i < PermutationCount); return PermuttedValues[i].Value > -1e6;}
	};

	// Check if the availbale number of objects is suitable for the Estimator
	virtual bool CheckObjectNumber(DWORD) const = 0;
	// Get the number of permutations in the test
	virtual int GetPermutationCount() const = 0;
	// Returns the exact value and the best subset estimate for the extent ext
	//   Any permutation i such that val.PermuttedValues[i] less than -1e6 is not computed:
	//   In some cases it is clear that certain permutations cannot provide better quality
	virtual void GetValue(const IExtent* ext, CWYOEstValue& val ) const = 0;
	// A predicate verifying if one value is better than another value, a linear order is assumed
	virtual bool IsBetter(const double& a, const double& b) const = 0;
	// Get JSON description of pattern quality
	virtual JSON GetJsonQuality(const IExtent* ext) const = 0;
};


#endif // WESTFALLYOUNGOPTIMISTICESTIMATOR_H_INCLUDED

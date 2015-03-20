#ifndef CSTABILITYMONTECARLOAPPROXIMATION_H
#define CSTABILITYMONTECARLOAPPROXIMATION_H

#include <common.h>

#include <fcaps/tools/StabilityCalculation.h>
#include <fcaps/PatternManager.h>

class CVectorBinarySetJoinComparator;
class CVectorBinarySetDescriptor;

class CStabilityMonteCarloApproximation {
public:
CStabilityMonteCarloApproximation(
	CVectorBinarySetJoinComparator& _cmp,
	const CVectorBinarySetDescriptor& _extent,
	const CVectorBinarySetDescriptor& _intent,
	const CBinarySetCollection& _objToDescrMap,
	bool _isLog );

	// Static members:
	// Calculate number of trials
	static DWORD CalculateTrialsCount( const double& epsilon, const double& delta );

	// Get/Set threshold for the stable concepts
	const double& GetStableThreshold() const
		{ return threshold; }
	void SetStableThreshold( double value );

	// Get/Set precision of calculation
	//  +-epsilon -- the goal interval
	//  (1 - delta) -- probability to be within this interval
	const double& GetEpsilon() const
		{ return e; }
	const double& GetDelta() const
		{ return d; }
	void SetPrecision( const double& epsilon, const double& delta );

	// Get number of trials corresponding to given epsilon and delta
	DWORD GetTrialsCount() const
		{ return trialsNum; }

	// Compute approximation of stability
	void Compute();

	// Get left and right limits of stability for the concept
	const double& GetStabilityLeftLimit() const
		{ return leftLimit; }
	const double& GetStabilityRightLimit() const
		{ return rightLimit; }

private:
	CVectorBinarySetJoinComparator& cmp;
	CPatternDeleter cmpDeleter;
	const CVectorBinarySetDescriptor& extent;
	const CVectorBinarySetDescriptor& intent;
	const CBinarySetCollection& objToDescrMap;

	// threshold for stable concepts
	double threshold;
	// epsilon from Monte Carlo -- the goal interval
	double e;
	// delta from Monte Carlo -- the probability to be outside the goal interval
	double d;

	// The number of trials
	DWORD trialsNum;
	// Il Logariphmic scale?
	bool isLog;

	// Result.
	double leftLimit;
	double rightLimit;
};

#endif // CSTABILITYMONTECARLOAPPROXIMATION_H

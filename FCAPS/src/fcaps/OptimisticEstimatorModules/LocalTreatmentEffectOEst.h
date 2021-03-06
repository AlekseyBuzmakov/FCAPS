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

	virtual bool CheckObjectNumber(DWORD n) const
	    { assert(objY.size() == objTrt.size()); return objY.size() == n;}
	// Methods of IOptimisticEstimator
	virtual void GetValue(const IExtent* ext, COEstValue& val ) const;
	virtual JSON GetJsonQuality(const IExtent* ext) const; 

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return OptimisticEstimatorModuleType;}
	static const char* const Name()
		{ return LocalTreatmentEffectOptimisticEstimator; }
	static const char* const Desc();

	// Methods of the class
	bool operator()(int a, int b) const;

private:
	struct CObjValues {
		// Test set of objects
		std::vector<double> Test;
		// Control set of objects
		std::vector<double> Cntrl;
		// Low bound of the confidence interval for the test set
		std::vector<double> TestConfLowBound;
		// Upper bound of the confidence interval for the control set
		std::vector<double> CntrlConfUpperBound;

		// Used only in OEst
		// The closesest position in the test set corresponding to the cntrl set
		std::vector<int> CntrlToTestPosition;
		// The best value change in the test test if only elements starting from [i] are allowed
		std::vector<double> BestTestValueChange;
	};
	enum TConfIntervalMode {
		CIM_Median = 0, // Nonparametric confidence interval for the median
		CIM_2Sigma, // Just two standard deviations as the confidence interval
		CIM_TTest, // t-test is used to compute confidence interval

		CIM_EnumCount
	};
	enum TQualityMode {
		QM_Linear = 0, // A linear sum of LATE and the regularizer term N
		QM_MinImpact, // A product of minimal of N1 and N0 and LATE

		QM_EnumCount
	};

private:
	static const CModuleRegistrar<CLocalTreatmentEffectOEst> registrar;
	// The filename with text classes
	std::string objInfoFilePath;
	// The set of values that should be considered as control
	std::unordered_set<std::string> cntrlLevels;
	// The response and the treatment of every object
	std::vector<double> objY;
	std::vector<bool> objTrt;
	// The significance level for confidence interval computation
	double signifLevel;
	// The multiplier before SIGMA for the corresponding significance level
	double zp;
	// The minimal number of objects in each group
	int minObjNum;
	// The parameters to compute the linear combianiton of delta and size
	double alpha;
	double beta;
	// The mode for computing the quality function
	TQualityMode qMode;
	// The way to compute the confidence interval
	TConfIntervalMode confIntMode;

	// The precomputed number of objects that corresponds to the significance level (1-based)
	std::vector<int> signifObjectNum;
	// The order of objects: first  cntrl, then test; inside every group by increasing the value.
	std::vector<int> order;
	// Inverse of the order. Shows at which position every object should be.
	std::vector<int> posToOrder;
	// Size of the control group
	int controlSize;
	// The basic delta computed for minObjNum objects in every  group
	double delta0Max;
	// The basic delta in the whole dataset
	double delta0;
	// The variable are mutable since they have only local meaning, but more efficient to be here.
	// The flag vectors indicating which objects are in the current pattern
	mutable std::vector<bool> currentObjects;
	// The object that contains Test and control objects for current pattern
	mutable CObjValues objValues;

	bool cmp(int a, int b) const;
	void setZP();
	void computeSignificantObjectNumbers();
	static double incompleteBeta(int i, int j);
	void buildOrder();
	void computeDelta0Max();
	void computeDelta0();
	bool extractObjValues(const IExtent* ext) const;
	bool extractObjValues(const CPatternImage& img) const;
	void computeMedianConfidenceIntervalBounds() const;
	void compute2SigmasConfidenceIntervalBounds() const;

	double getValue() const;
	double getBestSubsetEstimate() const;

	double getBestSubsetEstimateLinear() const;
	double getValueLinear() const;
	double getValueLinear(const double& delta, int size) const;
	double getDeltaValue(const double& ddelta, int dsize) const;
	double getBestValueForSubsets(int cntrlLastObject, int testFirstObject) const;
	bool checkObjValues() const;

	double getValueMinImpact() const;
	double getBestSubsetEstimateMinImpact() const;
};

#endif // LOCALTREATMENTEFFECTOEST_H

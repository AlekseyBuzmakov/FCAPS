// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, GPL v2 license, 2018, v0.7

#ifndef BINARYCLASSIFICATIONOEST_H
#define BINARYCLASSIFICATIONOEST_H

#include <common.h>

#include <fcaps/OptimisticEstimator.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <unordered_set>

////////////////////////////////////////////////////////////////////

const char BinaryClassificationOptimisticEstimator[] = "BinaryClassificationOptimisticEstimatorModule";

////////////////////////////////////////////////////////////////////

class CBinaryClassificationOEst : public IOptimisticEstimator, public IModule {
public:
	CBinaryClassificationOEst();

	virtual bool CheckObjectNumber(DWORD n)
		{ assert( strClasses.size() == classes.size()); return n==classes.size(); }
	// Methods of IOptimisticEstimator
	virtual double GetValue(const IExtent* ext) const;
	// Returns the best estimate of the value on extent subsets
	virtual double GetBestSubsetEstimate(const IExtent* ext) const;

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return OptimisticEstimatorModuleType; };
	virtual const char* const GetName() const
		{ return BinaryClassificationOptimisticEstimator; };

private:
	static const CModuleRegistrar<CBinaryClassificationOEst> registrar;
	// The filename with text classes
	std::string classesFilePath;
	// The vector of text classes
	std::vector<std::string> strClasses;
	// The vector of classes that are considered as positive classes
	std::unordered_set<std::string> targetClasses;
	// The vector specifying which class label is a target label
	std::vector<bool> classes;
	// The number of positive objects
	DWORD nPlus;

	DWORD getPositiveObjectsCount(const IExtent* ext) const;
};

#endif // BINARYCLASSIFICATIONOEST_H

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

	virtual bool CheckObjectNumber(DWORD n) const
		{ assert( strClasses.size() == classes.size()); return n==classes.size(); }
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
		{ return BinaryClassificationOptimisticEstimator; }
	static const char* const Desc();

private:
	static const CModuleRegistrar<CBinaryClassificationOEst> registrar;
	// The filename with text classes
	std::string classesFilePath;
	// The vector of text classes
	std::vector<std::string> strClasses;
	// The vector of classes that are considered as positive classes
	std::unordered_set<std::string> targetClasses;
	// The weight of the frequency in the resulting formula
	double freqWeight;
	// The vector specifying which class label is a target label
	std::vector<bool> classes;
	// The vector specifying the weights of the objects
	std::vector<double> weights;

	// The weight of positive objects
	double wPlus;
	// The total weiht of all objets
	double wAll;

	void getObjectsWeight(const IExtent* ext, double& wPlus, double& wAll) const;
	double getValue( const double& curWPlus, const double& curWAll ) const;
};

#endif // BINARYCLASSIFICATIONOEST_H

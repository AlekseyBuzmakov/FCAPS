// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Class implements a projection chain for cosine similarity in accordance with
//  Cao, J., Wu, Z., & Wu, J.
//  Scaling up cosine interesting pattern discovery: A depth-first method.
//  Information Sciences, 266(0), 31–46.
//  doi:http://dx.doi.org/10.1016/j.ins.2013.12.062

#ifndef COSINEBINCLSPATTERNSPROJECTIONCHAIN_H
#define COSINEBINCLSPATTERNSPROJECTIONCHAIN_H

#include <fcaps/SofiaModules/details/BinClsPatternsProjectionChain.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

////////////////////////////////////////////////////////////////////

char CosineBinClsPatternsProjectionChain[] = "CosineBinClsPatternsProjectionChainModule";

class CCosineBinClsPatternsProjectionChain : public CBinClsPatternsProjectionChain, public IModule {
public:
	CCosineBinClsPatternsProjectionChain();

	// Methods of IProjectionChain
	virtual void ComputeZeroProjection( CPatternList& ptrns );
	virtual double GetPatternInterest( const IPatternDescriptor* p );
	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return ProjectionChainModuleType; };
	virtual const char* const GetName() const
		{ return CosineBinClsPatternsProjectionChain; };

private:
	static CModuleRegistrar<CCosineBinClsPatternsProjectionChain> registar;

	std::vector<double> itemValues;

	void computeItemValues();
};

#endif // COSINEBINCLSPATTERNSPROJECTIONCHAIN_H

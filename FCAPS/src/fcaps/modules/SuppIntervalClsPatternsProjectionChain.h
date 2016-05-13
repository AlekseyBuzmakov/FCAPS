// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CSUPPINTERVALCLSPATTERNSPROJECTIONCHAIN_H
#define CSUPPINTERVALCLSPATTERNSPROJECTIONCHAIN_H

#include <fcaps/modules/details/IntervalClsPatternsProjectionChain.h>

#include <fcaps/Module.h>
#include <ModuleTools.h>

const char SuppIntervalClsPatternsProjectionChain[] = "SuppIntervalClsPatternsProjectionChainModule";

class CSuppIntervalClsPatternsProjectionChain : public CIntervalClsPatternsProjectionChain, public IModule {
public:
	CSuppIntervalClsPatternsProjectionChain();
	// Methods of IProjectionChain
	virtual double GetPatternInterest( const IPatternDescriptor* p );
	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return ProjectionChainModuleType; };
	virtual const char* const GetName() const
		{ return SuppIntervalClsPatternsProjectionChain; };

private:
	static CModuleRegistrar<CSuppIntervalClsPatternsProjectionChain> registar;
};

#endif // CSUPPINTERVALCLSPATTERNSPROJECTIONCHAIN_H

#ifndef CSUPPINTERVALCLSPATTERNSPROJECTIONCHAIN_H
#define CSUPPINTERVALCLSPATTERNSPROJECTIONCHAIN_H

#include <fcaps/modules/details/IntervalClsPatternsProjectionChain.h>

#include <fcaps/Module.h>
#include <fcaps/ModuleTools.h>

const char SuppIntervalClsPatternsProjectionChain[] = "SuppIntervalClsPatternsProjectionChainModule";

class CSuppIntervalClsPatternsProjectionChain : public CIntervalClsPatternsProjectionChain, public IModule {
public:
	CSuppIntervalClsPatternsProjectionChain();
	// Methods of IProjectionChain
	virtual double GetPatternInterest( const IPatternDescriptor* p );
	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
private:
	static CModuleRegistrar<CSuppIntervalClsPatternsProjectionChain> registar;
};

#endif // CSUPPINTERVALCLSPATTERNSPROJECTIONCHAIN_H

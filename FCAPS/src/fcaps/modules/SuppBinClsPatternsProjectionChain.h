#ifndef CSUPPBINCLSPATTERNSPROJECTIONCHAIN_H
#define CSUPPBINCLSPATTERNSPROJECTIONCHAIN_H

#include <fcaps/modules/details/BinClsPatternsProjectionChain.h>
#include <fcaps/Module.h>
#include <fcaps/ModuleTools.h>

////////////////////////////////////////////////////////////////////

char SuppBinClsPatternsProjectionChain[] = "SuppBinClsPatternsProjectionChainModule";

class CSuppBinClsPatternsProjectionChain : public CBinClsPatternsProjectionChain, public IModule {
public:
	CSuppBinClsPatternsProjectionChain();
	// Methods of IProjectionChain
	virtual double GetPatternInterest( const IPatternDescriptor* p );
	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
private:
	static CModuleRegistrar<CSuppBinClsPatternsProjectionChain> registar;
};

#endif // CSUPPBINCLSPATTERNSPROJECTIONCHAIN_H

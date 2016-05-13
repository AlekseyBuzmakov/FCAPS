// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CSUPPBINCLSPATTERNSPROJECTIONCHAIN_H
#define CSUPPBINCLSPATTERNSPROJECTIONCHAIN_H

#include <fcaps/modules/details/BinClsPatternsProjectionChain.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

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
	virtual const char* const GetType() const
		{ return ProjectionChainModuleType; };
	virtual const char* const GetName() const
		{ return SuppBinClsPatternsProjectionChain; };

private:
	static CModuleRegistrar<CSuppBinClsPatternsProjectionChain> registar;
};

#endif // CSUPPBINCLSPATTERNSPROJECTIONCHAIN_H

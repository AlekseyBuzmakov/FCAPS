// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CSUPPINTERVALCLSPATTERNSPROJECTIONCHAIN_H
#define CSUPPINTERVALCLSPATTERNSPROJECTIONCHAIN_H

#include <fcaps/SofiaModules/details/IntervalClsPatternsProjectionChain.h>

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
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return ProjectionChainModuleType;}
	static const char* const Name()
		{ return SuppIntervalClsPatternsProjectionChain; }
	static const char* const Desc()
		{ return "{}"; }

private:
	static CModuleRegistrar<CSuppIntervalClsPatternsProjectionChain> registar;
};

#endif // CSUPPINTERVALCLSPATTERNSPROJECTIONCHAIN_H

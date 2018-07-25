// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CADDINTENTCONTEXTPROCESSOR_H
#define CADDINTENTCONTEXTPROCESSOR_H

#include <fcaps/ContextProcessor.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include "details/Lattice.h"
#include "details/FCAUtils.h"

interface IComputationCallback;
interface IPatternManager;
interface IIntentStorage;
interface IExtentStorage;
interface ILatticeBuilder;

////////////////////////////////////////////////////////////////////

const char AddIntentContextProcessorModule[] = "AddIntentContextProcessorModule";

////////////////////////////////////////////////////////////////////

class CAddIntentContextProcessor : public IContextProcessor, public IModule {
public:
	CAddIntentContextProcessor();

	// Methods of IContextProcessor
	virtual void SetCallback( const IComputationCallback * cb )
		{callback = cb;}
	virtual const std::vector<std::string>& GetObjNames() const;
	virtual void SetObjNames( const std::vector<std::string>& names );
	virtual void PassDescriptionParams( const JSON& json );
	virtual void Prepare()
		{}
	virtual void AddObject( DWORD objectNum, const JSON& intent );
	virtual void ProcessAllObjectsAddition();
	virtual void SaveResult( const std::string& path );

	// Methods of IModule
	virtual void LoadParams( const JSON& json );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return ContextProcessorModuleType;}
	static const char* const Name()
		{ return AddIntentContextProcessorModule; }
	static const char* const Desc();

private:
	static const CModuleRegistrar<CAddIntentContextProcessor> registrar;
	const IComputationCallback * callback;
	CSharedPtr<IPatternManager> cmp;
	CSharedPtr<IIntentStorage> intStorage;
	CSharedPtr<IExtentStorage> extStorage;
	CSharedPtr<ILatticeBuilder> builder;
	CLattice lattice;
	DWORD objectCount;
	CLatticeFilterParams outputParams;
};

////////////////////////////////////////////////////////////////////

#endif // CADDINTENTCONTEXTPROCESSOR_H

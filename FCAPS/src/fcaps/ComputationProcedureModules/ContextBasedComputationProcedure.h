// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8

#ifndef CCONTEXTBASEDCOMPUTATIONPROCEDURE_H
#define CCONTEXTBASEDCOMPUTATIONPROCEDURE_H

#include <fcaps/ComputationProcedure.h>

#include <ModuleTools.h>

////////////////////////////////////////////////////////////////////

interface IComputationProcedure;
interface IContextProcessor;

////////////////////////////////////////////////////////////////////

const char ContextBasedComputationProcedure[] = "ContextBasedComputationProcedure";

class CContextBasedComputationProcedure : public IComputationProcedure, public IModule {
public:
	CContextBasedComputationProcedure();
	// Methods of IFilter
	void SetCallback( const IComputationCallback * cb );
	virtual void Run();
	virtual void SaveResult( const std::string& basePath );

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return ComputationProcedureModuleType;}
	static const char* const Name()
		{ return ContextBasedComputationProcedure; }
	static const char* const Desc();

private:
	static const CModuleRegistrar<CContextBasedComputationProcedure> registrar;
	// Callback for reporting the progress
	const IComputationCallback * callback;

	// Context processor that make the actual computations
	CSharedPtr<IContextProcessor> contextProcessor;
	// File path to context to process
	std::string contextFilePath;
	// Maximal number of objects to process
	DWORD maxObjectNumber;
};

#endif // CCONTEXTBASEDCOMPUTATIONPROCEDURE_H

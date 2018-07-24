// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2016, v0.7

// Author: Aleksey Buzmakov
// Description: Implementation of pattern enumeration interface. It is realy on the original code of the ALGO from an external library.
//  In order to rely on existing code of ALGO, the most simple thing is to find places in this code where patterns are reported
//  and then report the patterns by a callback. However in this case pattern reported by callback is hard to translate to the IPatternEnumerator interface
//  Accordingly, we run ALGO in parallel thread and are switching between threads when we want to get next pattern by the callback.

#ifndef PARALLELPATTERNENUMERATOR_H
#define PARALLELPATTERNENUMERATOR_H

#include <fcaps/PatternEnumerator.h>
#include <fcaps/PatternEnumeratorByCallback.h>

#include <fcaps/Module.h>
#include <ModuleTools.h>

////////////////////////////////////////////////////////////////////////

const char ParallelPatternEnumeratorModule [] = "ParallelPatternEnumeratorModule";

////////////////////////////////////////////////////////////////////////

class CParallelPatternEnumerator : public IPatternEnumerator, public IModule {
public:
    CParallelPatternEnumerator();
    ~CParallelPatternEnumerator();

	// Methods of IPatternEnumerator
	virtual void AddObject( DWORD objectNum, const JSON& intent );
    virtual bool GetNextPattern( TCurrentPatternUsage usage, CPatternImage& pattern );
    virtual void ClearMemory( CPatternImage& pattern );

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return PatternEnumeratorModuleType;}
	static const char* const Name()
		{ return ParallelPatternEnumeratorModule; }
	static const char* const Desc()
		{ return "{}"; }

	// A special function to run a new thread.
	void RunAlgoThread();

private:
		struct CSyncData;
private:
	static CModuleRegistrar<CParallelPatternEnumerator> registar;

	// A module that can generate patterns by a callback
    CSharedPtr<IPatternEnumeratorByCallback> peByCallback;
	
	// All data for syncronization
	CPtrOwner<CSyncData> syncData;
	// A flag to mark the fact the the algo is started.
	bool isAlgoRun;

	static bool callback( PECDataRef data, const CPatternImage& ptrn );

	void createAlgoThread();
	bool registerPattern( const CPatternImage& ptrn );
    bool getNextPattern( TCurrentPatternUsage usage, CPatternImage& pattern );
};

////////////////////////////////////////////////////////////////////////

#endif // PARALLELPATTERNENUMERATOR_H

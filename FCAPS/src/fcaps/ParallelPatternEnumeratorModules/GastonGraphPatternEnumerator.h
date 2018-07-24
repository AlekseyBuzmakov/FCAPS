// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2016, v0.7

// Author: Aleksey Buzmakov
// Description: Implementation of pattern enumeration interface. It is realy on the original code of Gaston from an external library.
//  In order to rely on existing code of Gaston, the most simple thing is to find places in this code where patterns are reported
//  and then report the patterns by a callback. However in this case pattern reported by callback is hard to translate to the IPatternEnumerator interface
//  Accordingly, we run Gaston in parallel thread and are switching between threads when we want to get next pattern by the callback.

#ifndef GASTONGRAPHPATTERNENUMERATOR_H
#define GASTONGRAPHPATTERNENUMERATOR_H

#include <fcaps/PatternEnumeratorByCallback.h>

#include <Library.h>
#include <LibGastonForSofia.h>

#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <boost/unordered_map.hpp>
#include <vector>

////////////////////////////////////////////////////////////////////////

const char GastonGraphPatternEnumeratorModule [] = "GastonGraphPatternEnumeratorModule";

////////////////////////////////////////////////////////////////////////

class CGastonGraphPatternEnumerator : public IPatternEnumeratorByCallback , public IModule {
public:
    CGastonGraphPatternEnumerator();
    ~CGastonGraphPatternEnumerator();

	// Methods of IPatternEnumeratorByCallback
	virtual void AddObject( DWORD objectNum, const JSON& intent );
	virtual void Run( PECReportPatternCallback callback, PECDataRef data = 0 );

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return PatternEnumeratorByCallbackModuleType;}
	static const char* const Name()
		{ return GastonGraphPatternEnumeratorModule; }
	static const char* const Desc()
		{ return "{}"; }

private:
	class CLabelMap {
	public:
		// Returns label by label id
		const std::string& GetLabel( DWORD id ) const;
		// Returns id by label
		const DWORD GetId( const std::string& label );

	private:
		typedef boost::unordered_map<std::string, DWORD> CLabelToIdMap;
	private:
		CLabelToIdMap labelToId;
		std::vector<std::string> idToLabel;
	};
private:
	static CModuleRegistrar<CGastonGraphPatternEnumerator> registar;

	// Callback and data for running return a new subgraph to a user.
	//  The values are passed within the Run(...).
	PECReportPatternCallback pecCallback;
	PECDataRef pecData;

	// The ID of the current subgraph
	int subgraphId;

	// Path to library with Gaston algorithm
	std::string libraryPath;
	// Library with Gaston algo.
	Library gastonLib;
	// Path to file to write the input graphs to
	std::string inputPath;
	// Should the input be removed
	bool removeInput;
	// To write the input file
	CDestStream inputStream;
	// Path for saving the patterns
	std::string patternPath;
	CDestStream patternStream;
	// Original object numbers
	std::vector<DWORD> origObjIDs;
	// Label maps for vertices and edges.
	CLabelMap vertexLabelMap;
	CLabelMap edgeLabelMap;
	// Function to run GastonAlgo
	RunGastonFunc runGaston;
	// Min support to be passed to Gaston
	int gastonMinSupport;
	// Max size of a pattern to be passed to Gaston
	int gastonMaxPtrnSize;
	// Running mode for Gaston
	TGastonRunningMode gastonMode;

	static bool LibGastonAPI gastonCallback(LibGastonDataRef data, const LibGastonGraph* graph);

	void loadLibrary();
	bool registerGraph( const LibGastonGraph* graph );
	void writePattern( const LibGastonGraph* graph );
};

////////////////////////////////////////////////////////////////////////

#endif // GASTONGRAPHPATTERNENUMERATOR_H

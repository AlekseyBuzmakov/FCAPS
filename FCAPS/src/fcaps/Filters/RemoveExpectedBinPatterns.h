// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CTRAINTESTCONTEXTSPLITTER_H
#define CTRAINTESTCONTEXTSPLITTER_H

#include <fcaps/Filter.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <fcaps/SharedModulesLib/VectorBinarySetDescriptor.h>

#include <fcaps/SharedModulesLib/StabilityCalculation.h>

#include <rapidjson/document.h>

#include <deque>

class CBinarySetDescriptorsComparator;

////////////////////////////////////////////////////////////////////

const char RemoveExpectedBinPatterns[] = "RemoveExpectedBinPatternsModule";

class CRemoveExpectedBinPatterns : public IFilter, public IModule {
public:
	CRemoveExpectedBinPatterns();
	// Methods of IFilter
	virtual const int GetResultFileCount() const 
		{ return results.size(); }
	virtual const char* GetResultFilePath( int index ) const 
		{ assert(0 <= index && index < results.size()); return results[index].c_str(); }
	virtual const char* GetDataFile() const
		 { return dataFile.c_str(); }
	virtual void SetDataFile( const char* path )
		{ dataFile = path; }
	virtual const char* GetInputFile() const
		 { return inputFile.c_str(); }
	virtual void SetInputFile( const char* path )
		{ inputFile = path; }
	virtual void Process();

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return ContextFilterModuleType; };
	virtual const char* const GetName() const
		{ return RemoveExpectedBinPatterns; };

private:
	static const CModuleRegistrar<CRemoveExpectedBinPatterns> registrar;
	// Resulting file (two files).
	std::vector<std::string> results;
	// The path to the data file.
	std::string dataFile;
	// The path to the input file.
	std::string inputFile;
	// Significance level
	double significance;
	// Output file suffix
	std::string outSuffix;

	// Temrorarily context
	std::deque< CList<DWORD> > tmpContext;
	// The number of objects
	DWORD objCount;
	// Comparator for extents
	CSharedPtr<CVectorBinarySetJoinComparator> extCmp;
	CPatternDeleter extDeleter;
	// Comparator for intents;
	CSharedPtr<CBinarySetDescriptorsComparator> intCmp;
	// The context in the form from attrs to objects
	CBinarySetCollection attrToTidsetMap;

	void convertContext();
	void addColumnToTable( DWORD columnNum, const CList<DWORD>& values, CBinarySetCollection& table );
};

#endif // CTRAINTESTCONTEXTSPLITTER_H

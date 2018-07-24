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

interface IPattern;

////////////////////////////////////////////////////////////////////

const char RemoveExpectedIntPatterns[] = "RemoveExpectedIntPatternsModule";

class CRemoveExpectedIntPatterns : public IFilter, public IModule {
public:
	CRemoveExpectedIntPatterns();
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
	virtual void SetInputFile( const char* path );
	virtual void Process();

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return ContextFilterModuleType;}
	static const char* const Name()
		{ return RemoveExpectedIntPatterns; }
	static const char* const Desc()
		{ return "{}"; }

private:
	static const CModuleRegistrar<CRemoveExpectedIntPatterns> registrar;
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
	// Should the order of filtered concepts be found
	bool findPartialOrder; 

	// Comparator for patterns
	CSharedPtr<IPatternManager> cmp;
	CPatternDeleter deleter;

	// Context and concepts
	std::deque< CSharedPtr<const IPatternDescriptor> > context;
	std::deque< CSharedPtr<const IPatternDescriptor> > concepts;	
	// The number of objects
	DWORD objCount;
	// Vector with p-values
	std::vector<double> pvals;
};

#endif // CTRAINTESTCONTEXTSPLITTER_H

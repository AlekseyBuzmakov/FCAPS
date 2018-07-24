// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CTRAINTESTCONTEXTSPLITTER_H
#define CTRAINTESTCONTEXTSPLITTER_H

#include <fcaps/Filter.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <rapidjson/document.h>

////////////////////////////////////////////////////////////////////

const char TrainTestContextSplitter[] = "TrainTestContextSplitterModule";

class CTrainTestContextSplitter : public IFilter, public IModule {
public:
	CTrainTestContextSplitter();
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
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return ContextFilterModuleType;}
	static const char* const Name()
		{ return TrainTestContextSplitter; }
	static const char* const Desc()
		{ return "{}"; }

private:
	static const CModuleRegistrar<CTrainTestContextSplitter> registrar;
	// Resulting file (two files).
	std::vector<std::string> results;
	// The path to the input file.
	std::string dataFile;
	// The path to the input file.
	std::string inputFile;
	// The proportion between train and test datasets. Default 90% is the train set.
	double proportion;
	// Seed for the random
	int seed;

	rapidjson::Value data;
	rapidjson::Document params;
	rapidjson::Value objNames;

	void writeDataset(
		std::vector<int>::const_iterator b, std::vector<int>::const_iterator e,
		const std::string& path );
	void initObjectNames(std::vector<int>::const_iterator b, std::vector<int>::const_iterator e );
};

#endif // CTRAINTESTCONTEXTSPLITTER_H

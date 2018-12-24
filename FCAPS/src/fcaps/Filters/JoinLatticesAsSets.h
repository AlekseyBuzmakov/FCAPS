// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CJOINLATTICESASSETS_H
#define CJOINLATTICESASSETS_H

#include <fcaps/Filter.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <fcaps/SharedModulesLib/VectorBinarySetDescriptor.h>

#include <rapidjson/document.h>

#include <deque>

////////////////////////////////////////////////////////////////////

const char JoinLatticesAsSets[] = "JoinLatticesAsSetsModule";

class CJoinLatticesAsSets : public IFilter, public IModule {
public:
	CJoinLatticesAsSets();
	// Methods of IFilter
	virtual const int GetResultFileCount() const 
		{ return results.size(); }
	virtual const char* GetResultFilePath( int index ) const 
		{ assert(0 <= index && index < results.size()); return results[index].c_str(); }
	virtual const char* GetDataFile() const
		{ assert(false); return 0; }
	virtual void SetDataFile( const char* path )
		{ assert(false); }
	virtual const char* GetInputFile() const
		 { return inputFile.c_str(); }
	virtual void SetInputFile( const char* path )
		{inputFile = path;}

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
		{ return LatticeFilterModuleType;}
	static const char* const Name()
		{ return JoinLatticesAsSets; }
	static const char* const Desc()
		{ return "{}"; }

private:
	static const CModuleRegistrar<CJoinLatticesAsSets> registrar;
	// Resulting file, one file
	std::vector<std::string> results;
	// The path to the input lattice file.
	std::string inputFile;
	// The patho to other lattice file.
	std::string otherFile;
	// Should the partial order be found
	bool shouldFindPartialOrder;

	// For comparison of extents
	CVectorBinarySetJoinComparator cmp;
	CPatternDeleter dlt;
};

#endif // CJOINLATTICESASSETS_H

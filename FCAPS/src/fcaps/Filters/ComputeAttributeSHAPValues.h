// Initial software, Aleksey Buzmakov

#ifndef COMPUTEATTRIBUTESHAPVALUES_H
#define COMPUTEATTRIBUTESHAPVALUES_H

#include <fcaps/Filter.h>

#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <fcaps/SharedModulesLib/VectorBinarySetDescriptor.h>

#include <rapidjson/document.h>

#include <deque>
#include <random>
#include <set>

////////////////////////////////////////////////////////////////////

const char ComputeAttributeShapValuesFilter[] = "ComputeAttributeShapValuesFilter";

class CComputeAttributeShapValues : public IFilter, public IModule {
public:
	CComputeAttributeShapValues();
	// Methods of IFilter
	virtual const int GetResultFileCount() const 
		{ return results.size(); }
	virtual const char* GetResultFilePath( int index ) const 
		{ assert(0 <= index && index < results.size()); return results[index].c_str(); }
	virtual const char* GetDataFile() const
		{ return contextFile.c_str(); }
	virtual void SetDataFile( const char* path )
		{ contextFile = path; }
	virtual const char* GetInputFile() const
		 { return latticeFile.c_str(); }
	virtual void SetInputFile( const char* path )
		{latticeFile = path;}

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
		{ return ComputeAttributeShapValuesFilter; }
	static const char* const Desc()
		{ return "{}"; }

private:
	static const CModuleRegistrar<CComputeAttributeShapValues> registrar;
	std::mt19937 rng;
	// Resulting file, one file
	std::vector<std::string> results;
	// The path to the input lattice file.
	std::string latticeFile;
	// The path to the context file.
	std::string contextFile;

	// The path to the result file
	std::string outSuffix;
	// The path to the result file
	std::string outFile;

	// The delta threshold to be used for  computing SHAP values
	int deltaThld;
	// The budget in number of rnd trials available for every concept
	int budgetRnd;
	// The budget in number of rnd trials available for every concept
	int budgetBF;
	// Indicator for reordering attributes w.r.t. their significance
	bool shouldReorderAttributes;

	// For comparison of extents
	CVectorBinarySetJoinComparator cmp;
	CPatternDeleter dlt;
	std::deque< CSharedPtr<const CVectorBinarySetDescriptor> > context;
	int objectNum;

	// Data for current concept
	// The size of the current extent
	int curExtSize;
	// The current extent, can be larger than the actual extent during the computation
	CSharedPtr<const CVectorBinarySetDescriptor> curExtent;	
	// The indiecs of the curent concept intent
	std::deque<int> curIntentInds;
	// The computed SHAP values of attributes for the current concept
	std::deque<double> curSHAPValues;

	// The indices of the current set of attributes being analyzed
	std::deque<int> analysedIntent;
	// The attributes that are involved in the curExtentComputation
	std::set<int> ignoredAttrs;

	void computeSHAPValues();
	double computeExactShap(int attr, int size);
	int computeExactShapGenIntent(int attrIntentIndex, int prevAttrIndex, int size);
	bool isAttrImportant(int attr);
	void computeExtent();
	void computeExtent(int attr);
	CSharedPtr<const CVectorBinarySetDescriptor> intersectAttrWithConcept(int attr);
	void closeExtent();
};

#endif // COMPUTEATTRIBUTESHAPVALUES_H

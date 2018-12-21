// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CSOFYACONCEPTBUILDER_H
#define CSOFYACONCEPTBUILDER_H

#include <fcaps/ContextProcessor.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <fcaps/ProjectionChain.h>

#include <fcaps/storages/CachedPatternStorage.h>

#include <unordered_map>

interface IComputationCallback;
interface IOptimisticEstimator;

template<typename T>
class CFindConceptOrder;

////////////////////////////////////////////////////////////////////

const char SofiaContextProcessor[] = "SofiaContextProcessorModule";

////////////////////////////////////////////////////////////////////

class CSofiaContextProcessor : public IContextProcessor, public IModule {
public:
	CSofiaContextProcessor();
	~CSofiaContextProcessor();

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return ContextProcessorModuleType;}
	static const char* const Name()
		{ return SofiaContextProcessor; }
	static const char* const Desc();

	// Methods of IContextProcessor
	virtual const std::vector<std::string>& GetObjNames() const;
	virtual void SetObjNames( const std::vector<std::string>& );
	virtual void PassDescriptionParams( const JSON& json );

	virtual void SetCallback( const IComputationCallback* cb )
		{callback = cb;};

	virtual void Prepare()
		{}
	virtual void AddObject( DWORD objectNum, const JSON& intent );
	virtual void ProcessAllObjectsAddition();

	virtual void SaveResult( const std::string& path );

private:
	class CHasher{
	public:
		CHasher() {}
		CHasher(const CSharedPtr<IProjectionChain>& c) :
			chain(c) {}
		void Init(const CSharedPtr<IProjectionChain>& c)
			{ chain = c;}

		// boost Hasher
		bool operator()(
				const IPatternDescriptor* x, const IPatternDescriptor* y) const
			{ return chain->AreEqual(x,y); }
		size_t operator()( const IPatternDescriptor* x) const
			{return x->Hash();}
		// Removing of a pattern
		void Free( const IPatternDescriptor* x)
			{return chain->FreePattern(x);}

	private:
		CSharedPtr<IProjectionChain> chain;
	};
	struct COEstQuality {
		// The objective value of the pattern
		double OEstMeasure;
		// The potential value of a more specific pattern
		double OEstPotential;

		COEstQuality():
			OEstMeasure(0),
			OEstPotential(0)
		{}
	};
	typedef std::pair<const IPatternDescriptor*,double> CPatternMeasurePair;
	class CConceptsForOrder;

	struct COutputParams {
		bool OutExtent;
		bool OutIntent;

		COutputParams() :
			OutExtent( true ), OutIntent( true ) {}
	};
	struct CBestPattern{
		const IPatternDescriptor* Pattern;
		bool IsProjectionPattern;
		double Q;

		CBestPattern():
			Pattern(0), IsProjectionPattern(false), Q(0) {}
	};

private:
	static CModuleRegistrar<CSofiaContextProcessor> registrar;
	// Chain of projections for SOFYA algo
	CSharedPtr<IProjectionChain> pChain;
	// Callback for progess reporting
	const IComputationCallback* callback;
	// An object that evaluates every extent and provides the upperbound estimate for possible values on extent subsets
	CSharedPtr<IOptimisticEstimator> oest;

	// Current thld for a measure
	double thld;
	// Maximal number of patterns
	DWORD mpn;
	// Should find partial order of the resulting concepts
	bool shouldFindPartialOrder;
	// Should adjust thld to have a polynomial algo
	bool shouldAdjustThld;
	// The params of the output
	COutputParams outParams;
	// The path to the file with aleady known concepts
	std::string knownConceptsPath;
	DWORD maxKnownConceptSize;

	// The number of added objects
	DWORD objectNumber;

	// TODO: move hashed storage to projections.
	//  then it would be possible to check if a stability for a pattern should be computed
	//  and if a pattern has been nicely created.
	CCachedPatternStorage<CHasher> storage;
	// The set of patterns that are used for passing through projection.
	CList<const IPatternDescriptor*> projectionPatterns;
	// A correspondance between patterns and their quality
	std::unordered_map<const IPatternDescriptor*,COEstQuality> oestQuality;

	// The best found pattern by OEst
	CBestPattern bestPattern;
	// The minimal and maximal potential in the search space.
	double minPotential;
	double maxPotential;

	// The of known concepts that are used to filter the result and the computation
	std::vector< const IPatternDescriptor* > knownConcepts;

	void loadKnownConcepts();
	void addNewPatterns( const IProjectionChain::CPatternList& newPatterns );
	bool isUnderKnownPatterns(const IPatternDescriptor* p) const;
	void computeOEstimate(const IPatternDescriptor* p, COEstQuality& q);
	void removeProjectionPattern(const IPatternDescriptor* p);
	void removeBestPattern();
	void removeInpotentialPatterns();
	void adjustThreshold();
	void reportProgress() const;

	void saveToFile( const std::vector<CPatternMeasurePair>& concepts, const CFindConceptOrder<CConceptsForOrder>& order, const std::string& path );
	void printConceptToJson( const CPatternMeasurePair& c, std::ostream& dst );
};

#endif // CSOFYACONCEPTBUILDER_H

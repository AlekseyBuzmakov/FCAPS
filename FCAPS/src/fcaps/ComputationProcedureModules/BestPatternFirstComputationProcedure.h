// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8
// A module that pass over a pattern space in breath first order expanding in the direction of the most promissing patterns

#ifndef BESTPATTERNFIRSTCOMPUTATIONPROCEDURE_H
#define BESTPATTERNFIRSTCOMPUTATIONPROCEDURE_H

#include <fcaps/ComputationProcedure.h>
#include <fcaps/LocalProjectionChain.h>
#include <fcaps/ComputationProcedureModules/details/ThldBestPatternMap.h>
#include <ListWrapper.h>
#include <ModuleTools.h>

#include <rapidjson/document.h>

#include <set>

////////////////////////////////////////////////////////////////////

interface IComputationProcedure;
interface IPatternDescriptor;
interface IOptimisticEstimator;

////////////////////////////////////////////////////////////////////

const char BestPatternFirstComputationProcedure[] = "BestPatternFirstComputationProcedureModule";

class CBestPatternFirstComputationProcedure : public IComputationProcedure, public IModule {
public:
	CBestPatternFirstComputationProcedure();
	// Methods of IComputationProcedure
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
		{ return BestPatternFirstComputationProcedure; }
	static const char* const Desc();

private:
	struct CPatternDeleter{
		CPatternDeleter(const CSharedPtr<ILocalProjectionChain>& _c) : c(_c) {}
		void operator()(const IPatternDescriptor* p ){ c->FreePattern(p); }

	private:
		const CSharedPtr<ILocalProjectionChain>& c;
	};

	struct CPattern {
		CSharedPtr<const IPatternDescriptor> Pattern;
		double Quality;
		double Potential;
		
		CPattern() : Quality(-1),Potential(0) {}
		CPattern( const double & potential) : Quality(-1), Potential(potential) {}
	};
	struct CBestPattern {
		CSharedPtr<const IPatternDescriptor> Pattern;
		double Quality;

		CBestPattern() : Quality(-1e10) {}
		CBestPattern(const CPattern& p) : Pattern(p.Pattern), Quality(p.Quality) {}

		const double& operator()() const
			{ return Quality; }
	};

	// Class for comparing potential of patterns
	class CPatternPotentialComparator {
	public:
		CPatternPotentialComparator(const CSharedPtr<ILocalProjectionChain>& lpc) :
			lpChain(lpc) {}

		// Compare potential of patterns
		bool operator()(const CPattern& a, const CPattern& b);
	private:
		const CSharedPtr<ILocalProjectionChain>& lpChain;
	};

	// The queue class
	typedef typename std::set<CPattern,CPatternPotentialComparator> TQueue;

private:
	static const CModuleRegistrar<CBestPatternFirstComputationProcedure> registrar;
	// Callback for reporting the progress
	const IComputationCallback * callback;

	// Context processor that make the actual computations
	CSharedPtr<ILocalProjectionChain> lpChain;
	// A deleter that removes patterns by means of lpChain;
	CPatternDeleter deleter;
	// An object that evaluates every extent and provides the upperbound estimate for possible values on extent subsets
	CSharedPtr<IOptimisticEstimator> oest;

	// Current thld for a measure
	double thld;
	// Maximal number of patterns
	DWORD mpn;
	// Maximal RAM consumption
	size_t maxRAMConsumption;
	// Should adjust thld to have a polynomial algo
	bool shouldAdjustThld;
	// Should stop the computations on the first found subgroup
	bool shouldBreakOnFirst;
	// Should compute best SD for every possible Thld, or only for the first found one?
	bool shouldComputeForAllThlds;
	// The number of beams to expand every concept from the queue.
	DWORD beamsNum;

	// The correspondnce between stability (interest of a pattern) and the best quality for patterns of at least certain interest
	CThldBestPatternMap<double,CBestPattern> bestMap;
	// Class for comparing patterns
	CPatternPotentialComparator potentialCmp;
	// The priority queue (with operations for random access removal)
	TQueue queue;
	// A flag indicating if the patterns are Swappable. -1 is not known, 0 -- not Swappable, 1 -- are swappable
	int arePatternsSwappable;
	// The number of patterns to be remaind in memory in the case of swapping
	DWORD numInMemoryPatterns;

	void convertPattern(const IPatternDescriptor* d, CPattern& p) const;
	void addPatternToQueue(const CPattern& p,TQueue& queue);
	void addNewPatterns( const ILocalProjectionChain::CPatternList& newPatterns, TQueue& queue );
	void startBeamSearch(const CPattern& p);
	void checkForBestConcept(const CPattern& p);
	void adjustThreshold();
};

#endif // BESTPATTERNFIRSTCOMPUTATIONPROCEDURE_H

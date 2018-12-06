// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8
// A module that pass over a pattern space in breath first order expanding in the direction of the most promissing patterns

#ifndef BESTPATTERNFIRSTCOMPUTATIONPROCEDURE_H
#define BESTPATTERNFIRSTCOMPUTATIONPROCEDURE_H

#include <fcaps/ComputationProcedure.h>
#include <fcaps/LocalProjectionChain.h>

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
		double Potential;
		
		CPattern() : Potential(0) {}
		CPattern( const double & p) : Potential(p) {}
	};
	struct CBestPattern {
		CSharedPtr<const IPatternDescriptor> Pattern;
		double Quality;

		CBestPattern() : Quality(-1e10) {}
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

	// Flag indicating if the minimal pattern quality is not known
	bool isBestQualityKnown;
	// The best pattern and its info is stored here
	CBestPattern best;
	// Class for comparing patterns
	CPatternPotentialComparator potentialCmp;
	// The priority queue (with operations for random access removal)
	std::set<CPattern,CPatternPotentialComparator> queue;

	void addNewPatterns( const ILocalProjectionChain::CPatternList& newPatterns );
	void adjustThreshold();
};

#endif // BESTPATTERNFIRSTCOMPUTATIONPROCEDURE_H

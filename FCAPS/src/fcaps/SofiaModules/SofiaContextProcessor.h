// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CSOFYACONCEPTBUILDER_H
#define CSOFYACONCEPTBUILDER_H

#include <fcaps/ContextProcessor.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <fcaps/ProjectionChain.h>

#include <fcaps/storages/CachedPatternStorage.h>

interface IOptimisticEstimator;

template<typename T>
class CFindConceptOrder;

////////////////////////////////////////////////////////////////////

const char SofiaContextProcessor[] = "SofiaContextProcessorModule";

////////////////////////////////////////////////////////////////////

class CSofiaContextProcessor : public IContextProcessor, public IModule {
public:
	CSofiaContextProcessor();

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return ContextProcessorModuleType; };
	virtual const char* const GetName() const
		{ return SofiaContextProcessor; };

	// Methods of IContextProcessor
	virtual const std::vector<std::string>& GetObjNames() const;
	virtual void SetObjNames( const std::vector<std::string>& );
	virtual void PassDescriptionParams( const JSON& json );

	virtual void SetCallback( const IContextProcessorCallback* cb )
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
	typedef std::pair<const IPatternDescriptor*,double> CPatternMeasurePair;
	class CConceptsForOrder;

	struct COutputParams {
		bool OutExtent;
		bool OutIntent;

		COutputParams() :
			OutExtent( true ), OutIntent( true ) {}
	};

private:
	static CModuleRegistrar<CSofiaContextProcessor> registrar;
	// Chain of projections for SOFYA algo
	CSharedPtr<IProjectionChain> pChain;
	// Callback for progess reporting
	const IContextProcessorCallback* callback;
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

	// TODO: move hashed storage to projections.
	//  then it would be possible to check if a stability for a pattern should be computed
	//  and if a pattern has been nicely created.
	CCachedPatternStorage<CHasher> storage;

	void addNewPatterns( const IProjectionChain::CPatternList& newPatterns );
	void adjustThreshold();
	static bool compareCachedPatterns(
		const CPatternMeasurePair& p1, const CPatternMeasurePair& p2);
	void reportProgress() const;

	void saveToFile( const std::vector<CPatternMeasurePair>& concepts, const CFindConceptOrder<CConceptsForOrder>& order, const std::string& path );
	void printConceptToJson( const CPatternMeasurePair& c, std::ostream& dst );
};

#endif // CSOFYACONCEPTBUILDER_H

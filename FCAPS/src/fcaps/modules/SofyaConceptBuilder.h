#ifndef CSOFYACONCEPTBUILDER_H
#define CSOFYACONCEPTBUILDER_H

#include <fcaps/ConceptBuilder.h>
#include <fcaps/Module.h>
#include <fcaps/ModuleTools.h>

#include <fcaps/ProjectionChain.h>

#include <fcaps/storages/CachedPatternStorage.h>

template<typename T>
class CFindConceptOrder;

////////////////////////////////////////////////////////////////////

const char SofyaConceptBuilder[] = "SofyaConceptBuilderModule";

////////////////////////////////////////////////////////////////////

class CSofyaConceptBuilder : public IConceptBuilder, public IModule {
public:
	CSofyaConceptBuilder();

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;

	// Methods of IConceptBuilder
	virtual const std::vector<std::string>& GetObjNames() const;
	virtual void SetObjNames( const std::vector<std::string>& );

	virtual void SetCallback( const IConceptBuilderCallback* cb )
		{callback = cb;};

	virtual void AddObject( DWORD objectNum, const JSON& intent );

	virtual void ProcessAllObjectsAddition();
	virtual void SaveLatticeToFile( const std::string& path );

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

private:
	static CModuleRegistrar<CSofyaConceptBuilder> registrar;
	// Chain of projections for SOFYA algo
	CSharedPtr<IProjectionChain> pChain;
	// Callback for progess reporting
	const IConceptBuilderCallback* callback;
	// Current thld for a measure
	double thld;
	// Maximal number of patterns
	DWORD mpn;
	// Should find partial order of the resulting concepts
	bool shouldFindPartialOrder;

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

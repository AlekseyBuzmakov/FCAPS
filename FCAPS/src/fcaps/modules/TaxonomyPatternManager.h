#ifndef CTAXONOMYPATTERNMANAGER_H
#define CTAXONOMYPATTERNMANAGER_H

#include <fcaps/PatternManager.h>
#include <fcaps/Module.h>
#include <fcaps/ModuleTools.h>
#include <fcaps/modules/details/TaxonomyJsonReader.h>

#include <Lca.h>

#include <boost/unordered_map.hpp>

////////////////////////////////////////////////////////////////////

const char TaxonomyPatternManager[] = "TaxonomyPatternManagerModule";

////////////////////////////////////////////////////////////////////

class CTaxonomyElementDescriptor : public IPatternDescriptor {
	friend class CTaxonomyPatternManager;
public:
	CTaxonomyElementDescriptor( DWORD _id ) :
		id( _id ) {};

	// Methods of IPatternDescriptor
	virtual TPatternType GetType() const
		{ return PT_TaxonomyElement; }
	virtual bool IsMostGeneral() const
		{ return id == 0; }
	virtual size_t Hash() const
		{ return id; }

private:
	DWORD id;
};

////////////////////////////////////////////////////////////////////

class CTaxonomyPatternManager : public IPatternManager, public IModule {
public:
	CTaxonomyPatternManager();

	// Methods of IPatternManager
	virtual TPatternType GetPatternsType() const
		{ return PT_TaxonomyElement; }

	virtual const CTaxonomyElementDescriptor* LoadObject( const JSON& json );
	virtual JSON SavePattern( const IPatternDescriptor* ptrn ) const;
	virtual const CTaxonomyElementDescriptor* LoadPattern( const JSON& json );

	virtual const CTaxonomyElementDescriptor* CalculateSimilarity(
		const IPatternDescriptor* first, const IPatternDescriptor* second );
	virtual TCompareResult Compare(
		const IPatternDescriptor* first, const IPatternDescriptor* second,
		DWORD interestingResults = CR_AllResults, DWORD possibleResults = CR_AllResults | CR_Incomparable );
	virtual void FreePattern( const IPatternDescriptor * ptrn )
		{ delete ptrn; }

	virtual void Write( const IPatternDescriptor* ptrn, std::ostream& dst ) const;

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;

private:
	typedef CTaxonomyJsonReader::CTree CTree;
	typedef CTaxonomyJsonReader::CNameToIndexMap CNameToIndexMap;
private:
	static const CModuleRegistrar<CTaxonomyPatternManager> registrar;

	// Path to taxonomy tree
	std::string pathToTree;
	// Taxonomy tree
	CTree tree;
	// The map from string IDs of vertices to the node index.
	CNameToIndexMap nameToIndexMap;
	// Least common ancestor in a tree.
	CPtrOwner< CLcaAlgorithm<CTaxonomyJsonReader::CNodeInfo, DWORD> > lca;

	static const CTaxonomyElementDescriptor& getTaxonomyElement( const IPatternDescriptor* );
	static std::string parseJsonString( const std::string& str );
};

#endif // CTAXONOMYPATTERNMANAGER_H

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CTREESETPATTERNDESCRIPTOR_H
#define CTREESETPATTERNDESCRIPTOR_H

#include <fcaps/PatternManager.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <fcaps/SharedModulesLib/BinarySetPatternManager.h> // Used for pattern

#include <fcaps/PS-Modules/details/TaxonomyJsonReader.h>

#include <StaticTree.h>
#include <Rmq.h>

#include <rapidjson/document.h>

#include <boost/unordered_map.hpp>

////////////////////////////////////////////////////////////////////

const char TreeSetPatternManager[] = "TreeSetPatternManagerModule";

////////////////////////////////////////////////////////////////////

class CTreeSetDescriptorsComparator;

class CTreeSetPatternDescriptor : public IPatternDescriptor {
	friend class CTreeSetDescriptorsComparator;
public:
	typedef CBinarySetPatternDescriptor::CAttrsList CAttrsList;

public:
	CTreeSetPatternDescriptor() {}

	//Methods of IPatternDescriptor
	virtual bool IsMostGeneral() const
		{ return data.IsMostGeneral(); /*TODO*/ }
	virtual size_t Hash() const
		{ return data.Hash(); }

private:
	CBinarySetPatternDescriptor data;

	CTreeSetPatternDescriptor( const CTreeSetPatternDescriptor& other ) :
		data( other.data ) {}

	// Methods of class
	// Add attribute to the set.
	void AddNextAttribNumber( DWORD attribNum )
		{ data.AddNextAttribNumber( attribNum ); }
	//  only in the case of sorted array.
	void AddSortedNextAttribNumber( DWORD attribNum )
		{ data.AddSortedNextAttribNumber( attribNum ); }
	// Add a set of attributes to the set.
	void AddList( const CAttrsList& listToAdd )
		{ data.AddList( listToAdd ); }

	const CAttrsList& GetAttribs() const
		{ return data.GetAttribs(); }
	CAttrsList& GetAttribs()
		{ return data.GetAttribs(); }
};

////////////////////////////////////////////////////////////////////

class CTreeSetDescriptorsComparator : public IPatternManager, public IModule {
public:
	CTreeSetDescriptorsComparator();

	// Methods of IPatternManager
	virtual const CTreeSetPatternDescriptor* LoadObject( const JSON& json );
	virtual JSON SavePattern( const IPatternDescriptor* ptrn ) const;
	virtual const CTreeSetPatternDescriptor* LoadPattern( const JSON& json );

	virtual const CTreeSetPatternDescriptor* CalculateSimilarity(
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
	virtual const char* const GetType() const
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return PatternManagerModuleType;}
	static const char* const Name()
		{ return TreeSetPatternManager; }
	static const char* const Desc()
		{ return "{}"; }

private:
	typedef CTaxonomyJsonReader::CTree CTree;
	typedef CTaxonomyJsonReader::CNameToIndexMap CNameToIndexMap;
private:
	static const CModuleRegistrar<CTreeSetDescriptorsComparator> registrar;

	// Path to taxonomy tree
	std::string pathToTree;
	// Taxonomy tree
	CTree tree;
	// Correspandance between string name and the index in the 'tree'.
	CNameToIndexMap nameToIndexMap;
	// RMQ: range minimum query algorithm for realising multiple LCA.
	CPtrOwner< CRmqAlgorithmAuto<DWORD> > rmq;
	// Depth array in the tree, for working with RMQ.
	std::vector<DWORD> depthArray;

	std::vector<DWORD> indexToNode;

	// Max diff in a description between different elements
	DWORD maxDiff;

	CTreeSetDescriptorsComparator( const CTreeSetDescriptorsComparator& );

	static const CTreeSetPatternDescriptor& getDescriptor( const IPatternDescriptor* );

	const CTreeSetPatternDescriptor* loadPattern( const JSON& json ) const;
	JSON savePattern( const IPatternDescriptor* ptrn ) const;

	void initMultiLca();

	void filterAntichain( CList<DWORD>& chain ) const;
	TCompareResult compareIndexes( DWORD first, DWORD second ) const;
	void projectIndexes( CList<DWORD>& inds ) const;
	DWORD projectIndex( DWORD ind, DWORD maxWeight ) const;
	static void changePossibleFlags( TCompareResult attrsPairCmp, DWORD& possibleFlag );
};

////////////////////////////////////////////////////////////////////

#endif // CTREESETPATTERNDESCRIPTOR_H

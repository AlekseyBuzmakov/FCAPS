// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.7

#include <fcaps/Filters/JoinLatticesAsSets.h>

#include <fcaps/SharedModulesLib/VectorBinarySetDescriptor.h>
#include <fcaps/SharedModulesLib/FindConceptOrder.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/writer.h>

#include <boost/sort/spinsort/spinsort.hpp>

using namespace std;

////////////////////////////////////////////////////////////////////

class CConceptsForOrder {
public:
	CConceptsForOrder( const CVectorBinarySetJoinComparator& _cmp, const std::deque< CSharedPtr<const CVectorBinarySetDescriptor> >& _concepts ) :
		cmp( _cmp ), concepts( _concepts ) {}

	// Methods of TConcepts requiring by CFindConceptOrder
	DWORD Size() const
		{ return concepts.size(); }
	bool IsTopologicallyLess( DWORD c1, DWORD c2 ) const
	{
		assert( c1 < concepts.size() );
		assert( c2 < concepts.size() );
		const bool res = concepts[c1]->Size() > concepts[c2]->Size() || concepts[c1]->Size() == concepts[c2]->Size() && concepts[c1]->Hash() < concepts[c2]->Hash();
		return res;
	}
	bool operator()( DWORD c1, DWORD c2 ) const {
		return IsTopologicallyLess(c1,c2);
	}

	bool IsLess( DWORD c1, DWORD c2 ) const
	{
		assert( c1 < concepts.size() );
		assert( c2 < concepts.size() );
		const bool res = cmp.Compare(*concepts[c1],*concepts[c2], CR_LessGeneral, CR_LessGeneral | CR_Incomparable ) == CR_LessGeneral; 
		return res;
	}
private:
	const CVectorBinarySetJoinComparator& cmp;
	const std::deque< CSharedPtr<const CVectorBinarySetDescriptor> >& concepts;
};

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CJoinLatticesAsSets> CJoinLatticesAsSets::registrar;

CJoinLatticesAsSets::CJoinLatticesAsSets() :
	dlt(cmp),
	shouldFindPartialOrder(true)
{
}

void CJoinLatticesAsSets::Process()
{
	static const char place[] = "CJoinLatticesAsSets::Process";
	CJsonError error(inputFile,"");

	rapidjson::Document doc;

	// Loading data
	{
		CJsonFile file(inputFile);
		if( !file.Read(  doc, error ) ) {
			throw new CJsonException( place, error );
		}
	}
	if( !doc.IsArray() || doc.Size() < 2 || !doc[1].IsObject()
		|| !doc[1].HasMember("Nodes") || !doc[1]["Nodes"].IsArray() )
	{
		error.Error = "InputLattice[1].Nodes not found. Not a valid context";
		throw new CJsonException( place, error );
	}
	rapidjson::Value& inputNodes = doc[1]["Nodes"];

	rapidjson::Document otherDoc;

	{
		// Loading lattice
		CJsonFile file( otherFile );
		if( !file.Read( otherDoc, error ) ) {
			throw new CJsonException( place, error );
		}
	}
	if( !otherDoc.IsArray() || otherDoc.Size() < 2 
		|| !otherDoc[1].IsObject() || !otherDoc[1].HasMember("Nodes") || !otherDoc[1]["Nodes"].IsArray()
	  )
	{
		error.Error = "OtherLattice[1].Nodes not found. Not a valid lattice";
		throw new CJsonException( place, error );
	}
	rapidjson::MemoryPoolAllocator<>& alloc = doc.GetAllocator();
	rapidjson::Value& otherNodes = otherDoc[1]["Nodes"];
	for( int i = 0; i < otherNodes.Size(); ++i ) {
		inputNodes.PushBack(otherNodes[i].Move(),alloc);
	}

	// Loading concepts
	std::deque< CSharedPtr<const CVectorBinarySetDescriptor> > concepts;	
	std::deque<DWORD> conceptInds;
	concepts.resize( inputNodes.Size() );
	conceptInds.resize(concepts.size(), -1);
	std::vector<bool> conceptsToRemove;
	conceptsToRemove.resize(concepts.size(), false);

	DWORD objectNum = 0;
	for( int i = 0; i < inputNodes.Size(); ++i ) {
		const rapidjson::Value& ext = inputNodes[i]["Ext"]["Inds"];
		assert(ext.Size() > 0);
		objectNum = max<DWORD>(objectNum, ext[ext.Size() - 1].GetUint() + 1);
	}
	cmp.SetMaxAttrNumber(objectNum);

	for( int i = 0; i < inputNodes.Size(); ++i ) {
		JSON extent;
		CreateStringFromJSON( inputNodes[i]["Ext"], extent );
		concepts[i]= CSharedPtr<const CVectorBinarySetDescriptor>( cmp.LoadPattern( extent ), dlt);
		conceptInds[i] = i;
	}

	// Finding duplicates
	CConceptsForOrder conceptsForDupplicateRemoval( cmp, concepts  );
	boost::sort::spinsort(conceptInds.begin(), conceptInds.end(), conceptsForDupplicateRemoval );

	for( int i = 1; i < conceptInds.size(); ++i ) {
		TCompareResult res = 
			cmp.Compare(concepts[conceptInds[i]].get(), concepts[conceptInds[i-1]].get(), CR_Equal, CR_AllResults | CR_Incomparable );
		if (res == CR_Equal) {
			conceptsToRemove[conceptInds[i]] = true;
		}
	}

	rapidjson::Value newNodes;
	newNodes.SetArray();
	std::deque<CSharedPtr<const CVectorBinarySetDescriptor> > newConcepts;
	for( int i = 0; i < conceptsToRemove.size(); ++i ) {
		if(conceptsToRemove[i]) {
			continue;
		}
		newNodes.PushBack(inputNodes[i].Move(), doc.GetAllocator());
		newConcepts.push_back(concepts[i]);
	}

	doc[1]["Nodes"] = newNodes;
	doc[0]["NodesCount"] = rapidjson::Value().SetUint(doc[1]["Nodes"].Size());

	// Updating arcs
	if( !shouldFindPartialOrder ) {
		doc[2]=rapidjson::Value().Move();
		doc[0]["ArcsCount"] = rapidjson::Value().SetInt(0);
	} else {
		CConceptsForOrder conceptsForOrder( cmp, newConcepts  );
		CFindConceptOrder<CConceptsForOrder> order( conceptsForOrder );
		order.Compute();

		CList<DWORD> tops;
		vector<bool> bottoms;
		bottoms.resize( newConcepts.size(), true );
		doc[2]["Arcs"].SetArray();
		for( DWORD i = 0; i < newConcepts.size(); ++i ) {
			const CList<DWORD>& parents = order.GetParents( i );
			if( parents.Size() == 0 ) {
				tops.PushBack( i );
			}
			CStdIterator<CList<DWORD>::CConstIterator,false> p( parents );
			for( ; !p.IsEnd(); ++p ) {
				bottoms[*p] = false;
				doc[2]["Arcs"].PushBack( 
					rapidjson::Value().SetObject()
						.AddMember("S", rapidjson::Value().SetUint(*p), alloc)
						.AddMember("D", rapidjson::Value().SetUint(i), alloc)
					,alloc);	
			}
		}

		doc[0]["ArcsCount"].SetInt(doc[2]["Arcs"].Size());
		assert(tops.Size() > 0 );

		doc[0]["Top"].SetArray();
		CStdIterator<CList<DWORD>::CConstIterator, false> top(tops);
		for( ;!top.IsEnd(); ++top ) {
			doc[0]["Top"].PushBack(rapidjson::Value().SetUint(*top), alloc);
		}

		doc[0]["Bottom"].SetArray();
		for( DWORD i = 0; i < bottoms.size(); ++i ) {
			if(!bottoms[i]) {
				continue;
			}
			doc[0]["Bottom"].PushBack(rapidjson::Value().SetUint(i), alloc);
		}
		assert(doc[0]["Bottom"].Size() > 0);
 	}


	// Updating params of the lattice
	JSON json = SaveParams();
	rapidjson::Document thisParams;
	if( !ReadJsonString( json, thisParams, error ) ) {
		assert(false);
	}
	if( !doc[0].HasMember("Filters") ) {
		doc[0].AddMember( "Filters", rapidjson::Value().SetArray(), alloc );
	}
	doc[0]["Filters"].PushBack(thisParams, alloc );

	JSON outputStr;
	CreateStringFromJSON( doc, outputStr );
	CDestStream dst(results[0]);
	dst << outputStr;
}

void CJoinLatticesAsSets::LoadParams( const JSON& json )
{
	static const char place[] = "CJoinLatticesAsSets::LoadParams";
	CJsonError error;
	error.Data = json;
	rapidjson::Document doc;
	if( !ReadJsonString( json, doc, error ) ) {
		throw new CJsonException( place, error );
	}
	assert( string( doc["Type"].GetString() ) == LatticeFilterModuleType );
	assert( string( doc["Name"].GetString() ) == JoinLatticesAsSets );
	if( !(doc.HasMember( "Params" ) && doc["Params"].IsObject()) ) {
		error.Error = "Params is not found. Necessary for input files.";
		throw new CJsonException(place, error);
	}
	const rapidjson::Value& params=doc["Params"];

	if( !params.HasMember("Lattice") || !params["Lattice"].IsString() ) {
		error.Error = "'Lattice' not found";
		throw new CJsonException(place, error);
	} else {
		otherFile = params["Lattice"].GetString();
	}

	if( params.HasMember("FindPartialOrder") && params["FindPartialOrder"].IsBool() ) {
		shouldFindPartialOrder = params["FindPartialOrder"].GetBool();
	}

	results.clear();
	if( params.HasMember("OutFile") && params["OutFile"].IsString() ) {
		results.push_back(params["OutFile"].GetString());
	}
}

JSON CJoinLatticesAsSets::SaveParams() const
{
	assert( results.size() == 1 );
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", LatticeFilterModuleType, alloc )
		.AddMember( "Name", JoinLatticesAsSets, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "InputFile", rapidjson::Value().SetString(
				rapidjson::StringRef(inputFile.c_str())), alloc )
			.AddMember( "Lattice", rapidjson::Value().SetString(
				rapidjson::StringRef(results[0].c_str())), alloc )
			.AddMember( "FindPartialOrder", rapidjson::Value().SetBool(shouldFindPartialOrder), alloc)
			.AddMember( "Out", rapidjson::Value().SetString(
				rapidjson::StringRef(results[0].c_str())), alloc ),
		alloc );
	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

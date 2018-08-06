// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/PS-Modules/BinarySetWithDependenciesPatternManager.h>

#include <JSONTools.h>
#include <StdTools.h>
#include <TopologicalSorting.h>

#include <rapidjson/document.h>


using namespace std;

////////////////////////////////////////////////////////////////////

const CModuleRegistrar<CBinarySetDescriptorsWithDependenciesComparator> CBinarySetDescriptorsWithDependenciesComparator::registrar(
	PatternManagerModuleType, BinarySetDescriptorsWithDependenciesComparator );
const char CBinarySetDescriptorsWithDependenciesComparator::jsonAttrPosetPath[] = "AttrPosetPath";

CBinarySetDescriptorsWithDependenciesComparator::CBinarySetDescriptorsWithDependenciesComparator()
{
	//ctor
}

CBinarySetDescriptorsWithDependenciesComparator::~CBinarySetDescriptorsWithDependenciesComparator()
{
	//dtor
}

void CBinarySetDescriptorsWithDependenciesComparator::LoadParams( const JSON& json )
{
	// Loading params of binary sets
	CBinarySetDescriptorsComparator::LoadParams( json );
	// If the params are firstly loaded we should check that they have all necesarry info.
	//  The complementary calls to LoadParams are possible.
	const bool areMainParams = posetPath.empty();

	rapidjson::Document dParams;
	CJsonError errorText;

	if( !ReadJsonString( json, dParams, errorText ) ) {
		throw new CJsonException( "CBinarySetDescriptorsWithDependenciesComparator", errorText );
	}

	if( !dParams.HasMember("Params") || !dParams["Params"].IsObject() ) {
		if( areMainParams ) {
			throw new CTextException( "CBinarySetDescriptorsWithDependenciesComparator", "Partial order of attributes is obligatory in Params");
		} else {
			return;
		}
	}
	const rapidjson::Value& params = dParams["Params"];

	if( !params.HasMember( jsonAttrPosetPath ) || !params[jsonAttrPosetPath].IsString() ) {
		if( areMainParams ) {
			throw new CTextException( "CBinarySetDescriptorsWithDependenciesComparator", "Partial order of attributes is obligatory in Params");
		} else {
			return;
		}
	}

	posetPath = params[jsonAttrPosetPath].GetString();
	loadPoset();
}

JSON CBinarySetDescriptorsWithDependenciesComparator::SaveParams() const
{
	JSON result = CBinarySetDescriptorsComparator::SaveParams();

	rapidjson::Document dParams;
	CJsonError errorText;
	if( !ReadJsonString( result, dParams, errorText ) ) {
		assert( false );
		throw new CJsonException( "CBinarySetDescriptorsWithDependenciesComparator", errorText );
	}

	if( dParams.HasMember("Params") && !dParams["Params"].IsObject() ) {
		assert( false );
		dParams.RemoveMember( "Params" );
	}
	if( !dParams.HasMember("Params") ) {
		dParams.AddMember( "Params", rapidjson::Value().SetObject(), dParams.GetAllocator() );
	}
	rapidjson::Value& params = dParams["Params"];

	// Add new params to BinarySetComparator
	params.AddMember( jsonAttrPosetPath, rapidjson::Value().SetString( posetPath.c_str(), posetPath.length() ), dParams.GetAllocator() );

	result.clear();
	CreateStringFromJSON( dParams, result );
	return result;
}

const CBinarySetPatternDescriptor* CBinarySetDescriptorsWithDependenciesComparator::LoadObject( const JSON& json )
{
	unique_ptr<CBinarySetPatternDescriptor> result( LoadRWPattern( json ) );

	convertAttributes( *result, toNewAttrs );
	projectDescription( *result );

	return result.release();
}

JSON CBinarySetDescriptorsWithDependenciesComparator::SavePattern( const IPatternDescriptor* ptrnInt ) const
{
	assert( ptrnInt != 0 && dynamic_cast<const CBinarySetPatternDescriptor*>(ptrnInt) != 0  );

	CBinarySetPatternDescriptor ptrn( debug_cast<const CBinarySetPatternDescriptor&>( *ptrnInt ) );
	convertAttributes( ptrn, toOldAttrs );
	return CBinarySetDescriptorsComparator::SavePattern( &ptrn );
}

const CBinarySetPatternDescriptor* CBinarySetDescriptorsWithDependenciesComparator::LoadPattern( const JSON& json )
{
	unique_ptr<CBinarySetPatternDescriptor> result( LoadRWPattern( json ) );

	convertAttributes( *result, toNewAttrs );

	return result.release();
}

const CBinarySetPatternDescriptor* CBinarySetDescriptorsWithDependenciesComparator::CalculateSimilarity(
	const IPatternDescriptor* firstPattern, const IPatternDescriptor* secondPattern )
{
	assert( firstPattern != 0 && dynamic_cast<const CBinarySetPatternDescriptor*>(firstPattern) != 0  );
	assert( secondPattern != 0 && dynamic_cast<const CBinarySetPatternDescriptor*>(secondPattern) != 0  );

	const CBinarySetPatternDescriptor& first = debug_cast<const CBinarySetPatternDescriptor&>( *firstPattern );
	const CBinarySetPatternDescriptor& second = debug_cast<const CBinarySetPatternDescriptor&>( *secondPattern );


	unique_ptr<CBinarySetPatternDescriptor> result( IntersectSets( first, second ) );

	projectDescription( *result );

	return result.release();
}

// Loads dependency poset of attributes from the file given by posetPath.
void CBinarySetDescriptorsWithDependenciesComparator::loadPoset()
{
	static char errorPlace[]="CBinarySetDescriptorsWithDependenciesComparator::loadPoset";

	CJsonError errorText;
	rapidjson::Document jsonDoc;
	CJsonFile file(posetPath);
	if( !file.Read(  jsonDoc, errorText ) ) {
		throw new CJsonException( errorPlace, errorText );
	}

	// Read nodes
	if( !jsonDoc.IsArray() || jsonDoc.Size() < 2 || !jsonDoc[1].IsObject() || !jsonDoc[1].HasMember( "Nodes" ) ) {
		throw new CTextException( errorPlace, "Wrong Poset JSON: no 'Nodes' found" );
	}
	const rapidjson::Value& nodes = jsonDoc[1]["Nodes"];

	if( !nodes.IsArray() ) {
		throw new CTextException( errorPlace, "Wrong Poset JSON: 'Nodes' should be an array" );
	}

	const int nodeCount = nodes.Size();
	vector<int> toSingletones;
	toSingletones.resize( nodeCount, -1 );
	int singletoneCount = 0;

	for( int i = 0; i < nodes.Size(); ++i ) {
		if( !nodes[i].HasMember("Count") || !nodes[i]["Count"].IsUint() ) {
			throw new CTextException( errorPlace,
				"Wrong Node Description in the Poset: 'Count' member not found in node " + StdExt::to_string(i) );
		}
		if( nodes[i]["Count"].GetUint() == 1 ) {
			toSingletones[i] = singletoneCount++;
		}
	}

	// Read edges
	if( !jsonDoc.IsArray() || jsonDoc.Size() < 3 || !jsonDoc[2].IsObject() || !jsonDoc[2].HasMember( "Arcs" ) ) {
		throw new CTextException( errorPlace, "Wrong Poset JSON: no 'Arcs' found" );
	}
	const rapidjson::Value& arcs = jsonDoc[2]["Arcs"];

	if( !arcs.IsArray() ) {
		throw new CTextException( errorPlace, "Wrong Poset JSON: 'Arcs' should be an array" );
	}

	vector< pair<int,int> > edges;

	for( int i = 0; i < arcs.Size(); ++i ) {
		if( !arcs[i].HasMember("S") || !arcs[i]["S"].IsUint()
			|| !arcs[i].HasMember("D") || !arcs[i]["D"].IsUint() )
		{
			throw new CTextException( errorPlace, string("Wrong Edge Description in the Poset: 'S' or 'D' member not found in arc ") + StdExt::to_string(i) );
		}
		pair<int,int> newEdge(toSingletones[arcs[i]["S"].GetUint()],toSingletones[arcs[i]["D"].GetUint()]);
		if( newEdge.first < 0 || newEdge.second < 0 ) {
			// not a singletone
			continue;
		}
		edges.push_back(newEdge);
	}

////TODO: we should be cleaver with circles, but how?
//
//	CTopologicalSorting topoOrder( singletoneCount, edges );
//	topoOrder.Compute();
//	const vector<int>& topoPlaces = topoOrder.GetPlaces();
////Just a %HACK without a sorting. TOKILL
	vector<int> topoPlaces;
	topoPlaces.resize(singletoneCount);
	for( int i = 0; i < topoPlaces.size(); ++i ) {
		topoPlaces[i]=i;
	}
////ENDOF %HACK

	// Creating toNew and toOld arrays.
	for( int i = 0; i < nodes.Size(); ++i ) {
		if( nodes[i]["Count"].GetUint() != 1 ) {
			continue;
		}
		if( !nodes[i].HasMember("Inds") || !nodes[i]["Inds"].IsArray() ) {
			throw new CTextException( errorPlace,
				"Wrong Node Description in the Poset: 'Inds' member not found in node " + StdExt::to_string(i) );
		}
		if( nodes[i]["Inds"].Size() != 1 ) {
			throw new CTextException( errorPlace,
				"Wrong Node Description in the Poset: 'Inds' and 'Count' are not coherent in node " + StdExt::to_string(i) );
		}
		const DWORD attr = nodes[i]["Inds"][0u].GetUint();
		addMappings( attr, (DWORD)topoPlaces[toSingletones[i]] );
	}
	assert( toOldAttrs.size() == toNewAttrs.size() );

	// Loading dependencies.
	//  We use method LoadObject here, which can be used only if toAldAttrs and toNewAttrs are created.
	dependencies.reserve( nodes.Size() );
	for( int i = 0; i < nodes.Size(); ++i ) {
		string json;
		CreateStringFromJSON( nodes[i], json );
		// const_cast is needed because we work with vector_ptr, that cannot store const pointers.
		dependencies.push_back(const_cast<CBinarySetPatternDescriptor*>(LoadPattern(json)));
	}

	// Linking new attributes and dependencies
	attrsToDependencies.resize(toNewAttrs.size());
	for( int i = 0; i < arcs.Size(); ++i ) {
		pair<int,int> edge(arcs[i]["S"].GetUint(),arcs[i]["D"].GetUint());
		if( toSingletones[edge.first] < 0 ) {
			assert( false );
			continue;
		}
		const int newAttr = topoPlaces[toSingletones[edge.first]];
		attrsToDependencies[newAttr].push_back(edge.second);
	}
}

// Adds mappings from old attributes to new attributes
void CBinarySetDescriptorsWithDependenciesComparator::addMappings( DWORD oldAttr, DWORD newAttr )
{
	if( toOldAttrs.size() <= newAttr ) {
		toOldAttrs.resize( newAttr+1, -1 );
	}
	if( toNewAttrs.size() <= oldAttr ) {
		toNewAttrs.resize( oldAttr+1, -1 );
	}
	toOldAttrs[newAttr]=oldAttr;
	toNewAttrs[oldAttr]=newAttr;
}

// Chagnges attributes
void CBinarySetDescriptorsWithDependenciesComparator::convertAttributes(
	CBinarySetPatternDescriptor& p, const std::vector<int>& mapping )
{
	CBinarySetPatternDescriptor::CAttrsList& attrList = p.GetAttribs();
	CStdIterator<CBinarySetPatternDescriptor::CAttrsList::CIterator, false> itr( attrList );
	for( ; !itr.IsEnd(); ++itr ) {
		assert( *itr < mapping.size() );
		*itr = mapping[*itr];
	}
	attrList.Sort();
}

// Projects a pattern descriptor in accordance with the poset.
void CBinarySetDescriptorsWithDependenciesComparator::projectDescription( CBinarySetPatternDescriptor& p ) const
{
	CBinarySetPatternDescriptor::CAttrsList& attrs = p.GetAttribs();
	CStdIterator<CBinarySetPatternDescriptor::CAttrsList::CIterator, false> itr( attrs );
	CStdIterator<CBinarySetPatternDescriptor::CAttrsList::CIterator, false> itr2;

//Because of %HACK we should have many loops, and the removedAll stuff
	bool removedAll = false;
	while( !removedAll ) {
		removedAll = true;
		itr.Reset( attrs );
		for( ; !itr.IsEnd(); ) {
			itr2 = itr;
			++itr;
			const DWORD attr = *itr2;
			// Verifying that for 'attr' all conditions hold.
			assert( attr < attrsToDependencies.size() );
			CStdIterator<list<int>::const_iterator> cond( attrsToDependencies[attr] );
			// If no conditions, then should be removed:
			bool shouldBeRemoved = true;
			for( ; !cond.IsEnd(); ++cond ) {
				const TCompareResult rslt = Compare( dependencies[*cond], p, CR_MoreOrEqual, CR_AllResults | CR_Incomparable );
				if( rslt != CR_Incomparable ) {
					// At least one condition holds
					shouldBeRemoved = false;
					break;
				}
			}

			if( shouldBeRemoved ) {
				removedAll = false;
				attrs.Erase( itr2 );
			}
		}

	}
}



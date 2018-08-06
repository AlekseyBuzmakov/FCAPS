// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/PS-Modules/TaxonomyPatternManager.h>

#include <JSONTools.h>

using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////

const CModuleRegistrar<CTaxonomyPatternManager> CTaxonomyPatternManager::registrar( PatternManagerModuleType, TaxonomyPatternManager );

CTaxonomyPatternManager::CTaxonomyPatternManager()
{
	//ctor
}

const CTaxonomyElementDescriptor* CTaxonomyPatternManager::LoadObject( const JSON& json )
{
	const string str = parseJsonString( json );
	if( str.empty() ) {
		throw new CTextException( "CTaxonomyPatternManager::LoadPattern",
			string("Cannot parse json string ('") + json + "')");
	}
	CNameToIndexMap::const_iterator itr = nameToIndexMap.find( str );
	if( itr == nameToIndexMap.end() ) {
		throw new CTextException( "CTaxonomyPatternManager::LoadPattern",
			string("Name not found in the taxonomy ('") + str + "')");
	}
	return new CTaxonomyElementDescriptor( itr->second );
}
JSON CTaxonomyPatternManager::SavePattern( const IPatternDescriptor* pattern ) const
{
	const CTaxonomyElementDescriptor& ptrn = getTaxonomyElement( pattern );
	return "\"" + tree.GetNode( ptrn.id ).Data.ID + "\"";
}
const CTaxonomyElementDescriptor* CTaxonomyPatternManager::LoadPattern( const JSON& json )
{
	const string str = parseJsonString( json );
	if( str.empty() ) {
		throw new CTextException( "CTaxonomyPatternManager::LoadPattern",
			string("Cannot parse json string ('") + json + "')");
	}
	CNameToIndexMap::const_iterator itr = nameToIndexMap.find( str );
	if( itr == nameToIndexMap.end() ) {
		throw new CTextException( "CTaxonomyPatternManager::LoadPattern",
			string("Name not found in the taxonomy ('") + str + "')");
	}
	return new CTaxonomyElementDescriptor( itr->second );
}

const CTaxonomyElementDescriptor* CTaxonomyPatternManager::CalculateSimilarity(
	const IPatternDescriptor* first, const IPatternDescriptor* second )
{
	assert( lca != 0 );
	const CTaxonomyElementDescriptor& frst = getTaxonomyElement( first );
	const CTaxonomyElementDescriptor& scnd = getTaxonomyElement( second );
	unique_ptr<CTaxonomyElementDescriptor> rslt( new CTaxonomyElementDescriptor( lca->GetParent( frst.id, scnd.id ) ) );
	return rslt.release();
}

TCompareResult CTaxonomyPatternManager::Compare(
	const IPatternDescriptor* first, const IPatternDescriptor* second,
	DWORD interestingResults, DWORD /*possibleResults*/ )
{
	assert( lca != 0 );
	const CTaxonomyElementDescriptor& frst = getTaxonomyElement( first );
	const CTaxonomyElementDescriptor& scnd = getTaxonomyElement( second );
	if( frst.id == scnd.id ) {
		return HasAllFlags( interestingResults, CR_Equal ) ? CR_Equal : CR_Incomparable;
	}
	const DWORD rslt = lca->GetParent( frst.id, scnd.id );
	if( rslt == frst.id ) {
		return HasAllFlags( interestingResults, CR_MoreGeneral ) ? CR_MoreGeneral : CR_Incomparable;
	}
	if( rslt == scnd.id ) {
		return HasAllFlags( interestingResults, CR_LessGeneral ) ? CR_LessGeneral : CR_Incomparable;
	}
	return CR_Incomparable;
}

void CTaxonomyPatternManager::Write( const IPatternDescriptor* pattern, std::ostream& dst ) const
{
	const CTaxonomyElementDescriptor& ptrn = getTaxonomyElement( pattern );
	dst << tree.GetNode(ptrn.id).Data.ID;
}

void CTaxonomyPatternManager::LoadParams( const JSON& json )
{
	CJsonError errorText;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "TaxonomyPatternManager", errorText );
	}
	assert( string( params["Type"].GetString() ) == PatternManagerModuleType );
	assert( string( params["Name"].GetString() ) == TaxonomyPatternManager );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()
		&& params["Params"].HasMember( "TreePath" ) && params["Params"]["TreePath"].IsString() ) )
	{
		throw new CTextException( "TaxonomyPatternManager",
			"No 'TreePath' is found in <<\n" + json + "\n>>" );
	}
	pathToTree = params["Params"]["TreePath"].GetString();

	CTaxonomyJsonReader(tree, nameToIndexMap).ReadTree( pathToTree );
	lca.reset( new CLcaAlgorithm<CTaxonomyJsonReader::CNodeInfo, DWORD>( tree ) );
	lca->Initialize();
}

JSON CTaxonomyPatternManager::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", PatternManagerModuleType, alloc )
		.AddMember( "Name", TaxonomyPatternManager, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "TreePath", rapidjson::StringRef( pathToTree.c_str() ), alloc ),
		alloc );

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

inline const CTaxonomyElementDescriptor& CTaxonomyPatternManager::getTaxonomyElement( const IPatternDescriptor* ptrn )
{
	assert( ptrn != 0 && dynamic_cast<const CTaxonomyElementDescriptor*>(ptrn) != 0  );
	return debug_cast<const CTaxonomyElementDescriptor&>( *ptrn );
}

inline std::string CTaxonomyPatternManager::parseJsonString( const string& str )
{
	if( str.length() < 2 || str[0] != '\"' || str[str.length() - 1] != '\"' ) {
		return "";
	}
	return str.substr( 1, str.length() - 2 );
}


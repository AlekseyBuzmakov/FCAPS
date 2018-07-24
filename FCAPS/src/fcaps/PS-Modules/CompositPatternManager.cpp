// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/PS-Modules/CompositPatternManager.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/document.h>

#include <sstream>

using namespace std;

////////////////////////////////////////////////////////////////////

CCompositePatternDescriptor::~CCompositePatternDescriptor()
{
	for( size_t i = 0; i < ptrns.size(); ++i ) {
		assert( ptrns[i] != 0 );
		delete ptrns[i];
	}
	ptrns.clear();
}
bool CCompositePatternDescriptor::IsMostGeneral() const
{
	for( size_t i = 0; i < ptrns.size(); ++i ) {
		if( !ptrns[i]->IsMostGeneral() ) {
			return false;
		}
	}
	return true;
}
size_t CCompositePatternDescriptor::Hash() const
{
	size_t result = 0;
	for( size_t i = 0; i < ptrns.size(); ++i ) {
		result ^= ptrns[i]->Hash();
	}
	return result;
}

////////////////////////////////////////////////////////////////////

const CModuleRegistrar<CCompositPatternManager> CCompositPatternManager::registrar( PatternManagerModuleType, CompositPatternManager );

CCompositPatternManager::CCompositPatternManager()
{
	//ctor
}

const CCompositePatternDescriptor* CCompositPatternManager::LoadObject( const JSON& json )
{
	return loadPattern( json );
}
JSON CCompositPatternManager::SavePattern( const IPatternDescriptor* ptrn ) const
{
	return savePattern( ptrn );
}
const CCompositePatternDescriptor* CCompositPatternManager::LoadPattern( const JSON& json )
{
	return loadPattern( json );
}

const CCompositePatternDescriptor* CCompositPatternManager::CalculateSimilarity(
	const IPatternDescriptor* first, const IPatternDescriptor* second )
{
	const CCompositePatternDescriptor& frst = getCompositPattern( first );
	assert( frst.ptrns.size() == cmps.size() );
	const CCompositePatternDescriptor& scnd = getCompositPattern( second );
	assert( scnd.ptrns.size() == cmps.size() );

	unique_ptr<CCompositePatternDescriptor> result( new CCompositePatternDescriptor );
	result->ptrns.reserve( cmps.size() );
	for( size_t i = 0; i < cmps.size(); ++i ) {
		result->ptrns.push_back( cmps[i].CalculateSimilarity( frst.ptrns[i], scnd.ptrns[i] ) );
	}

	return result.release();
}

TCompareResult CCompositPatternManager::Compare(
	const IPatternDescriptor* first, const IPatternDescriptor* second,
	DWORD interestingResults /*= CR_AllResults*/, DWORD possibleResults /*= CR_AllResults | CR_Incomparable*/ )
{
	const CCompositePatternDescriptor& frst = getCompositPattern( first );
	assert( frst.ptrns.size() == cmps.size() );
	const CCompositePatternDescriptor& scnd = getCompositPattern( second );
	assert( scnd.ptrns.size() == cmps.size() );

	interestingResults &= possibleResults;
	const DWORD internalInterestingResults = interestingResults | CR_Equal;
	const DWORD internalPossibleResults = possibleResults | CR_Equal;

	for( size_t i = 0; i < cmps.size(); ++i ) {
		const TCompareResult cmp = cmps[i].Compare( frst.ptrns[i], scnd.ptrns[i],
			internalInterestingResults, internalPossibleResults );
		switch( cmp ) {
			case CR_Incomparable:
				return CR_Incomparable;
			case CR_Equal:
				break;
			case CR_LessGeneral:
			case CR_MoreGeneral:
				possibleResults &= (cmp | CR_Incomparable);
				if( possibleResults == cmp || possibleResults == CR_Incomparable ) {
					return static_cast<TCompareResult>( possibleResults );
				}
				break;
			default:
				assert( false );
		}
	}
	TCompareResult result = CR_Incomparable;
	if( HasAllFlags( possibleResults, CR_Equal ) ) {
		result = CR_Equal;
	} else if ( HasAllFlags( possibleResults, CR_LessGeneral | CR_MoreGeneral ) ) {
		result = CR_Incomparable;
	} else {
		result = static_cast<TCompareResult>( possibleResults & (CR_LessGeneral | CR_MoreGeneral) );
	}
	if( HasAllFlags( interestingResults, result ) ) {
		return result;
	} else {
		return CR_Incomparable;
	}
}

void CCompositPatternManager::FreePattern( const IPatternDescriptor* pattern )
{
	const CCompositePatternDescriptor& ptrn = getCompositPattern( pattern );
	delete &ptrn;
}

void CCompositPatternManager::Write( const IPatternDescriptor* pattern, std::ostream& dst ) const
{
	const CCompositePatternDescriptor& ptrn = getCompositPattern( pattern );
	assert( ptrn.ptrns.size() == cmps.size() );
	dst << "<";
	for( size_t i = 0; i < cmps.size(); ++i ) {
		cmps[i].Write( ptrn.ptrns[i], dst );
		if( i != cmps.size() - 1 ) {
			dst << ";";
		}
	}
	dst << ">";
}

void CCompositPatternManager::LoadParams( const JSON& json )
{
	CJsonError errorText;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "CompositPatternManager", errorText );
	}
	assert( string( params["Type"].GetString() ) == PatternManagerModuleType );
	assert( string( params["Name"].GetString() ) == CompositPatternManager );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()
		&& params["Params"].HasMember( "PMs" ) && params["Params"]["PMs"].IsArray() ) ) {
		throw new CTextException( "CompositPatternManager",
			"No 'PMs' found in module params <<\n" + json + "\n>>");
	}
	rapidjson::Value& pms = params["Params"]["PMs"];
	cmps.clear();
	cmps.reserve( pms.Size() );
	for( size_t i = 0; i < pms.Size(); ++i ) {
		string errorText;
		if( !pms[i].IsObject() ) {
			continue;
		}
		unique_ptr<IPatternManager> newPM(
			dynamic_cast<IPatternManager*>( CreateModuleFromJSON( pms[i], errorText ) ) );
		if( newPM.get() == 0 ) {
			stringstream destStr;
			destStr << "Cannot create the module " << i;
			if( !errorText.empty() ) {
				destStr << "with an error <<\n" << errorText << "\n>>";
			}
			throw new CTextException( "CompositPatternManager", destStr.str() );
		}
		cmps.push_back( newPM.release() );
	}
}
JSON CCompositPatternManager::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", PatternManagerModuleType, alloc )
		.AddMember( "Name", CompositPatternManager, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "PMs", rapidjson::Value().SetArray(), alloc ),
		alloc );
	rapidjson::Value& pms = params["Params"]["PMs"];
	for( size_t i = 0; i < cmps.size(); ++i ) {
		const IModule& module = dynamic_cast<const IModule&>( cmps[i] );
		rapidjson::Document internalParams;
		internalParams.Parse( module.SaveParams().c_str() );
		pms.PushBack( internalParams.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

inline const CCompositePatternDescriptor& CCompositPatternManager::getCompositPattern( const IPatternDescriptor* ptrn )
{
	assert( ptrn != 0 );
	return debug_cast<const CCompositePatternDescriptor&>( *ptrn );
}

JSON CCompositPatternManager::savePattern( const IPatternDescriptor* pattern ) const
{
	const CCompositePatternDescriptor& ptrn = getCompositPattern( pattern );
	assert( ptrn.ptrns.size() == cmps.size() );

	CJsonError errorText;
	rapidjson::Document patternJSON;
	patternJSON.SetArray();
	patternJSON.Reserve( cmps.size(), patternJSON.GetAllocator() );
	for( size_t i = 0; i < cmps.size(); ++i ) {
		rapidjson::Document internalPattern( &patternJSON.GetAllocator() );
		const bool rslt = ReadJsonString( cmps[i].SavePattern( ptrn.ptrns[i] ), internalPattern, errorText );
		assert( rslt );
		rapidjson::Value& intPattern = internalPattern;
		patternJSON.PushBack( intPattern, patternJSON.GetAllocator() );
	}
	JSON result;
	CreateStringFromJSON( patternJSON, result );
	return result;
}

const CCompositePatternDescriptor* CCompositPatternManager::loadPattern( const JSON& json )
{
	rapidjson::Document patternJSON;
	patternJSON.Parse( json.c_str() );
	if( !patternJSON.IsArray() || patternJSON.Size() != cmps.size() ) {
		new CTextException( "CCompositPatternManager::loadPattern",
			string("The JSON is not an array or does not contain ")
			 + StdExt::to_string( cmps.size() ) + "elements " + "('" + json + "')" );
	}

	unique_ptr<CCompositePatternDescriptor> compositPattern( new CCompositePatternDescriptor );
	compositPattern->ptrns.reserve( cmps.size() );
	for( size_t i = 0; i < cmps.size(); ++i ) {
		JSON internalPattern;
		CreateStringFromJSON( patternJSON[i], internalPattern );
		unique_ptr<const IPatternDescriptor> internalPatternObj( cmps[i].LoadPattern( internalPattern ) );
		if( internalPatternObj.get() == 0 ) {
			throw new CTextException( "CCompositPatternManager::loadPattern",
				string("The ") + StdExt::to_string( cmps.size() ) + "th component of the JSON cannot be parsed"
				 + "('" + json + "')");
		}
		compositPattern->ptrns.push_back( internalPatternObj.release() );
	}

	return compositPattern.release();
}

#include "StabilityEstimatorContextProcessor.h"

#include <fcaps/modules/VectorBinarySetDescriptor.h>

#include <JSONTools.h>
#include <fcaps/modules/details/JsonBinaryPattern.h>

using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CStabilityEstimatorContextProcessor> CStabilityEstimatorContextProcessor::registrar(
	ContextProcessorModuleType, StabilityEstimatorContextProcessorModule );

CStabilityEstimatorContextProcessor::CStabilityEstimatorContextProcessor() :
	cmp( new CVectorBinarySetJoinComparator ),
	stabApprox(*cmp)
{
	//ctor
}

void CStabilityEstimatorContextProcessor::AddObject( DWORD objectNum, const JSON& intent )
{
	JsonBinaryPattern::CIndices indices;
	JsonBinaryPattern::CNames names;
	JsonBinaryPattern::LoadPattern( intent, indices, names );

	if( indices.Size() == 0 ) {
		throw new CTextException( "CStabilityEstimatorContextProcessor::AddObject", "No indices found in the intent" );
	}

	CSharedPtr<CVectorBinarySetDescriptor> ptrn( cmp->NewPattern(), CPatternDeleter( cmp ) );

	cmp->AddList( indices, *ptrn );

	CList<DWORD> dummyIntent;
	stabApprox.InitComputation( *ptrn, dummyIntent );
	stabApprox.ComputeLowerBound();
	stabApprox.ComputeUpperBound();

	CResult res;
	res.Id = objectNum;
	res.LBound = stabApprox.GetStabilityLeftLimit();
	res.UBound = stabApprox.GetStabilityRightLimit();

	results.push_back( res );
}
void CStabilityEstimatorContextProcessor::SaveResult( const std::string& path )
{
	CDestStream dst(path);

	dst << "[\n";
	for( DWORD i = 0; i < results.size(); ++i ) {
		const CResult& res = results[i];
		dst << "{\"ID\":" << res.Id << ",\"L\":" << res.LBound << ",\"U\":" << res.UBound << "}";
		if( i < results.size() - 1 ) {
			dst << ",";
		}
		dst << "\n";
	}

	dst << "]\n";
}

void CStabilityEstimatorContextProcessor::LoadParams( const JSON& json )
{
	static char place[]="CStabilityEstimatorContextProcessor::LoadParams";
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( place, error );
	}
	assert( string( params["Type"].GetString() ) == ContextProcessorModuleType );
	assert( string( params["Name"].GetString() ) == StabilityEstimatorContextProcessorModule );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for Context";
		throw new CJsonException(place, error);
	}
	const rapidjson::Value& p = params["Params"];

	if( !p.HasMember( "Context") || !p["Context"].IsString() ) {
		error.Data = json;
		error.Error = "THIS.Params.Context is not found.";
		throw new CJsonException(place, error);
	}

	contextPath = p["Context"].GetString();
	loadContext();
	stabApprox.SetContext( attrToTidsetMap );

	if( p.HasMember("alpha") && p["alpha"].IsDouble() ) {
		alpha = p["alpha"].GetDouble();
		if ( 0 > alpha || alpha >= 1.0 ) {
			alpha = 0.5;
		}
		stabApprox.SetBase( 1/(1-alpha) );
	}
}
JSON CStabilityEstimatorContextProcessor::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();

	params.SetObject()
		.AddMember( "Type", ContextProcessorModuleType, alloc )
		.AddMember( "Name", StabilityEstimatorContextProcessorModule, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "Context", rapidjson::Value().SetString(rapidjson::StringRef(contextPath.c_str())), alloc)
			.AddMember( "alpha", rapidjson::Value().SetDouble( alpha ),
		alloc ),
	alloc );

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

void CStabilityEstimatorContextProcessor::loadContext()
{
	static char place[]="CStabilityEstimatorContextProcessor::loadContext";
	CJsonError jsonError;
	rapidjson::Document data;
	if( !ReadJsonFile( contextPath, data, jsonError ) ) {
		throw new CJsonException( place, jsonError );
	}
	if( !data.IsArray() || data.Size() < 2 ) {
		throw new CTextException( place, "JSON data are not in an 2-sized json-array" );
	}
	if( !data[1].HasMember( "Data") ) {
		throw new CTextException( place, "No DATA[1].Data found" );
	}
	if( !data[1]["Data"].IsArray() ) {
		throw new CTextException( place, "DATA[1].Data should be an array" );
	}
	const rapidjson::Value& dataBody=data[1]["Data"];
	cmp->SetMaxAttrNumber( dataBody.Size() );

	string objectJson;
	JsonBinaryPattern::CIndices indices;
	JsonBinaryPattern::CNames names;
	for( size_t i = 0; i < dataBody.Size(); ++i ) {
		CreateStringFromJSON( dataBody[i], objectJson );
		JsonBinaryPattern::LoadPattern( objectJson, indices, names );
		AddColumnToCollection( cmp, i, indices, attrToTidsetMap );
	}
}

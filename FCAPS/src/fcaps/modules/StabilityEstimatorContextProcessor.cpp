// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

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
	type( DT_Tidset ),
	alpha( 0.5 ),
	thld( 3 ),
	computeUBound( true ),
	computeLBound( true ),

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

	CSharedPtr<const CVectorBinarySetDescriptor> ptrn;

	CList<DWORD> dummyIntent;
	CList<DWORD>* attrs = &dummyIntent;

	if( type == DT_Tidset ) {
		CSharedPtr<CVectorBinarySetDescriptor> holder( cmp->NewPattern(), CPatternDeleter( cmp ) );
		cmp->AddList( indices, *holder );
		ptrn = holder;
	} else {
		computeExtent( indices, ptrn );
		attrs = &indices;
	}

	CResult res;
	res.Id = objectNum;
	res.LBound = -1;
	res.UBound = -1;

	stabApprox.InitComputation( *ptrn, *attrs );

	if( computeLBound ) {
		stabApprox.ComputeLowerBound();
		res.LBound = stabApprox.GetStabilityLeftLimit();
	}

	if( computeUBound ) {
		stabApprox.ComputeUpperBound();
		res.UBound = stabApprox.GetStabilityRightLimit();
	}

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

	const bool isMainLoad=attrToTidsetMap.empty();
	const bool neededParams=loadParams(json);
	if( isMainLoad && !neededParams ) {
		CJsonError error;
		error.Data = json;
		error.Error = "THIS.Params.Context is not found.";
		throw new CJsonException(place, error);
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
			.AddMember( "DataType", rapidjson::Value().SetString( rapidjson::StringRef( type == DT_Itemset ? "Itemset" : "Tidset") ), alloc )
			.AddMember( "alpha", rapidjson::Value().SetDouble( alpha ), alloc )
			.AddMember( "Thld", rapidjson::Value().SetDouble( thld ), alloc )
			.AddMember( "ComputeLBound", rapidjson::Value().SetBool( computeLBound ), alloc )
			.AddMember( "ComputeUBound", rapidjson::Value().SetBool( computeUBound ), alloc ),
		alloc );

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

bool CStabilityEstimatorContextProcessor::loadParams( const JSON& json )
{
	static char place[]="CStabilityEstimatorContextProcessor::LoadParams";
	bool neededFound = true;
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

	if( p.HasMember( "Context") && p["Context"].IsString() ) {
		contextPath = p["Context"].GetString();
		loadContext();
		stabApprox.SetContext( attrToTidsetMap );
	} else {
		neededFound = false;
	}

	if( p.HasMember( "DataType" ) && p["DataType"].IsString() ) {
		if( p["DataType"] == "Itemset" ) {
			type = DT_Itemset;
		} else {
			type = DT_Tidset;
		}
	}

	if( p.HasMember("alpha") && p["alpha"].IsDouble() ) {
		alpha = p["alpha"].GetDouble();
		if ( 0 > alpha || alpha >= 1.0 ) {
			alpha = 0.5;
		}
		stabApprox.SetBase( 1/(1-alpha) );
	}

	if( p.HasMember( "Thld" ) && p["Thld"].IsNumber() ) {
		thld = p["Thld"].GetDouble();
		stabApprox.SetStableThreshold( thld );
	}

	if( p.HasMember( "ComputeUBound" ) && p["ComputeUBound"].IsBool() ) {
		computeUBound = p["ComputeUBound"].GetBool();
	}
	if( p.HasMember( "ComputeLBound" ) && p["ComputeLBound"].IsBool() ) {
		computeLBound = p["ComputeLBound"].GetBool();
	}

	return neededFound;
}

void CStabilityEstimatorContextProcessor::loadContext()
{
	static char place[]="CStabilityEstimatorContextProcessor::loadContext";
	CJsonError jsonError;
	rapidjson::Document data;
	CJsonFile file(contextPath);
	if( !file.Read(  data, jsonError ) ) {
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

void CStabilityEstimatorContextProcessor::computeExtent(
	const CList<DWORD>& attrs, CSharedPtr<const CVectorBinarySetDescriptor>& result ) const
{
	assert( attrs.Size() > 0 );

	const CVectorBinarySetDescriptor* last = 0;

	CStdIterator<CList<DWORD>::CConstIterator, false > attr( attrs );
	result = attrToTidsetMap[*attr];
	++attr;

	for( ; !attr.IsEnd(); ++attr ) {
		result.reset(
			cmp->CalculateSimilarity( *result, *attrToTidsetMap[*attr] ),
			CPatternDeleter( cmp ) );
	}
}

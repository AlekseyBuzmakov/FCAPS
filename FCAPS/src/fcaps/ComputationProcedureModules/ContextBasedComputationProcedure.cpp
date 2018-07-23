// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8

#include <fcaps/ComputationProcedureModules/ContextBasedComputationProcedure.h>

#include <fcaps/ContextProcessor.h>

#include <JSONTools.h>
#include <ModuleJSONTools.h>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(x...) #x

const char description[] =
STR(
	{
	"Name":"Context Based Computation Procedure",
	"Description":"This computation procedures parses the dataset and passed it object by object to context processor",
	"Params": {
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of Context Based Computation Procedure",
			"type": "object",
			"properties": {
				"ContextFilePath":{
					"description": "The path to a file, containing the context",
					"type": "file-path"
				},
				"MaxObjectNumber" :{
					"description": "The maximal number of objects to process",
					"type":"integer"
					"minimum":1,
				},
				"ContextProcessor":{
					"description": "The object that defines the context processor that performs the actual computations.",
					"type": "@ContextProcessorModuleType"
				}
			}
		}
	}
);


////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CContextBasedComputationProcedure> CContextBasedComputationProcedure::registrar(
	                                 ComputationProcedureModuleType,ContextBasedComputationProcedure);

const char* const CContextBasedComputationProcedure::Desc()
{
	return "{}";
}
////////////////////////////////////////////////////////////////////

CContextBasedComputationProcedure::CContextBasedComputationProcedure() :
	callback(0),
	maxObjectNumber(0)
{
}

void CContextBasedComputationProcedure::SetCallback( const IComputationCallback * cb )
{
	callback = cb;
	assert(contextProcessor != 0);
	contextProcessor->SetCallback(callback);
}
void CContextBasedComputationProcedure::Run()
{
	
}
void CContextBasedComputationProcedure::SaveResult( const std::string& basePath )
{
	
}

void CContextBasedComputationProcedure::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CContextBasedComputationProcedure::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == Type() );
	assert( string( params["Name"].GetString() ) == Name() );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for Context Processor and context file path";
		throw new CJsonException("CContextBasedComputationProcedure::LoadParams", error);
	}

	const rapidjson::Value& p = params["Params"];

	if(!(p.HasMember("ContextFilePath") && (p["ContextFilePath"].IsString()))) {
		error.Data = json;
		error.Error = "Params.ContextFilePath is not found or is not a file name.";
		throw new CJsonException("CContextBasedComputationProcedure::LoadParams", error);
	}
	contextFilePath = p["ContextFilePath"].GetString();

	if(!(p.HasMember("ContextProcessor") && (p["ContextProcessor"].IsObject()))) {
		error.Data = json;
		error.Error = "Params.ContextProcessor is not found or is not an object.";
		throw new CJsonException("CContextBasedComputationProcedure::LoadParams", error);
	}
	if(p.HasMember("MinObjNum") && p["MinObjNum"].IsInt()) {
		maxObjectNumber=p["MaxObjectNumber"].GetInt();
	}
	const rapidjson::Value& cp = params["Params"]["ContextProcessor"];
	string errorText;
	contextProcessor.reset( CreateModuleFromJSON<IContextProcessor>(cp,errorText) );
	if( contextProcessor == 0 ) {
		throw new CJsonException( "CContextBasedComputationProcedure::LoadParams", CJsonError( json, errorText ) );
	}
}

JSON CContextBasedComputationProcedure::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", rapidjson::StringRef(Type()), alloc )
		.AddMember( "Name", rapidjson::StringRef(Name()), alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "ContextFilePath", rapidjson::Value().SetString( rapidjson::StringRef(contextFilePath.c_str()) ), alloc )
			.AddMember( "MaxObjectNumber", rapidjson::Value().SetInt( maxObjectNumber ), alloc ),
		alloc );

	IModule* m = dynamic_cast<IModule*>(contextProcessor.get());

	JSON cpParams;
	rapidjson::Document cpParamsDoc;
    assert( m!=0);
	if( m != 0 ) {
		cpParams = m->SaveParams();
		CJsonError error;
		const bool rslt = ReadJsonString( cpParams, cpParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("ContextProcessor", cpParamsDoc.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;

}



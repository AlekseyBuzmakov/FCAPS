// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8

#include <fcaps/ComputationProcedureModules/ContextBasedComputationProcedure.h>

#include <fcaps/ContextProcessor.h>

#include <JSONTools.h>
#include <ModuleJSONTools.h>
#include <StdTools.h>
#include <RelativePathes.h>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(...) #__VA_ARGS__

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
					"type":"integer",
					"minimum":1
				},
				"Indices" :{
					"description": "The zero-based indices of objects that should be processed",
					"type":"array",
					"items": {
						"type": "integer",
						"minimum":0
					}
				},
				"ContextProcessor":{
					"description": "The object that defines the context processor that performs the actual computations.",
					"type": "@ContextProcessorModules"
				}
			}
		}
	}
);


////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CContextBasedComputationProcedure> CContextBasedComputationProcedure::registrar;

const char* const CContextBasedComputationProcedure::Desc()
{
	return description;
}
////////////////////////////////////////////////////////////////////

CContextBasedComputationProcedure::CContextBasedComputationProcedure() :
	callback(0),
	maxObjectNumber(-1)
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
	assert(contextProcessor != 0);
	assert(callback != 0);

	rapidjson::Document data;
	readDataJson( data );
	extractObjectNames( data );

	string dataParams;
	if( data[0].HasMember("Params") ) {
		CreateStringFromJSON(data[0]["Params"], dataParams);
		contextProcessor->PassDescriptionParams( dataParams );
	}

	callback->ReportNextStage("Preparation");
	contextProcessor->Prepare();

	callback->ReportNextStage("Object Addition");
	string objectJson;
	const rapidjson::Value& dataBody=data[1]["Data"];

	CStdIterator<CList<DWORD>::CConstIterator, false> index( indexes );
	DWORD objNum = 0;
	for( size_t i = 0; i < dataBody.Size(); ++i ) {
		// Select objects with good indices.
		if( !indexes.IsEmpty() ) {
			if( index.IsEnd() ) {
				break;
			}
			i = *index;
			++index;
		}
		// Cut if have processed to much.
		if( objNum >= maxObjectNumber ) {
			break;
		}

		CreateStringFromJSON( dataBody[i], objectJson );
		try{
			contextProcessor->AddObject( i, objectJson );
			++objNum;
		} catch( CException* e ) {
			callback->Warning( string("Object ") + StdExt::to_string(i) + " has a bad description -> IGNORED. (" + e->GetPlace() + ", " + e->GetText() + ")"); 
			delete e;
			continue;
		}

		callback->ReportProgress( static_cast<double>(objNum) / max(indexes.Size(), dataBody.Size()),
		                          string("Added ") + StdExt::to_string(objNum) + "th object" );
	}
	callback->ReportProgress( 1.0, string("All objects have been added."));

	/////////////////////////////////////////

	callback->ReportNextStage("Main Routine");
	contextProcessor->ProcessAllObjectsAddition();
}
void CContextBasedComputationProcedure::SaveResult( const std::string& basePath )
{
	callback->ReportNextStage("Writting result");
	contextProcessor->SaveResult( basePath );
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
	if(p.HasMember("MaxObjectNumber") && p["MaxObjectNumber"].IsInt()) {
		maxObjectNumber=p["MaxObjectNumber"].GetInt();
	}
	if(p.HasMember("Indices") && p["Indices"].IsArray()) {
		indexes.Clear();
		const rapidjson::Value& inds = p["Indices"];
		for( int i = 0; i < inds.Size(); ++i) {
			if(!inds[i].IsUint()) {
				callback->Warning("Indices are not unsigned numbers");
				indexes.Clear();
				break;
			}
			indexes.PushBack(inds[i].GetUint());
		}
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

// Reads the file with the context
void CContextBasedComputationProcedure::readDataJson( rapidjson::Document& data ) const
{
	CJsonError jsonError;
	string path;
	RelativePathes::GetFullPath( contextFilePath, path);
	if( !ReadJsonFile( path, data, jsonError ) ) {
		throw new CJsonException( "readDataJson", jsonError );
	}
	if( !data.IsArray() || data.Size() < 2 ) {
		throw new CTextException( "readDataJson", "JSON data are not in an 2-sized json-array" );
	}
	if( !data[1].HasMember( "Data") ) {
		throw new CTextException( "readDataJson", "No DATA[1].Data found" );
	}
	if( !data[1]["Data"].IsArray() ) {
		throw new CTextException( "readDataJson", "DATA[1].Data should be an array" );
	}
}

// Extracts object names form the context
void CContextBasedComputationProcedure::extractObjectNames( rapidjson::Document& data )
{
	assert(contextProcessor != 0);
	assert(callback != 0);
	const rapidjson::Value& dataParams = data[0u];
	if( !dataParams.IsObject() ) {
		callback->Warning("DATA[0] is not an object");
		return;
	}

	if( !dataParams.HasMember( "ObjNames" ) || !dataParams["ObjNames"].IsArray() ) {
		return;
	}

	vector<string> objNames;
	const rapidjson::Value& objNamesArray = dataParams["ObjNames"];
	objNames.reserve( objNamesArray.Size() );
	for( int i = 0; i < objNamesArray.Size(); ++i) {
		const rapidjson::Value& name = objNamesArray[i];
		if( !name.IsString() ) {
			callback->Warning( string("The ") + StdExt::to_string(i) + "th object name is not a string -- ignored." );
			objNames.push_back( StdExt::to_string(i) );
			continue;
		}
		objNames.push_back( name.GetString() );
	}

	if( data[1]["Data"].Size() != objNames.size() ) {
		callback->Warning( "The number of objects (" + StdExt::to_string(data[1]["Data"].Size()) + ")"
		                   " does not correspond to the number of object names (" + StdExt::to_string(objNames.size()) + ").");
	}

	contextProcessor->SetObjNames( objNames );
}


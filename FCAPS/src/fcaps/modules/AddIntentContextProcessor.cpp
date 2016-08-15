// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include "AddIntentContextProcessor.h"

#include <fcaps/modules/details/AddIntentLatticeBuilder.h>
#include <fcaps/modules/details/ExtentImpl.h>
#include <fcaps/storages/VectorIntentStorage.h>
#include <fcaps/PatternManager.h>

#include <ModuleTools.h>
#include <ModuleJSONTools.h>
#include <JSONTools.h>

using namespace std;

////////////////////////////////////////////////////////////////////

const CModuleRegistrar<CAddIntentContextProcessor> CAddIntentContextProcessor::registrar(
	ContextProcessorModuleType, AddIntentContextProcessorModule );

CAddIntentContextProcessor::CAddIntentContextProcessor() :
	callback( 0 ),
	intStorage( new CVectorIntentStorage ),
	extStorage( new CDequeExtentStorage ),
	builder( new CAddIntentLatticeBuilder ),
	objectCount(0)
{
	//ctor
}

const std::vector<std::string>& CAddIntentContextProcessor::GetObjNames() const
{
	assert(extStorage != 0 );
	return extStorage->GetNames();
}
void CAddIntentContextProcessor::SetObjNames( const std::vector<std::string>& names )
{
	assert(extStorage != 0 );
	extStorage->SetNames( names );
}


void CAddIntentContextProcessor::PassDescriptionParams( const JSON& json )
{
	assert( cmp != 0 );
	IModule& cmpModule = dynamic_cast<IModule&>(*cmp);
	const JSON params = string("{") +
		"\"Type\":\"" + cmpModule.GetType() + "\","
		"\"Name\":\"" + cmpModule.GetName() + "\","
		"\"Params\":" + json +
		"}";
	cmpModule.LoadParams( params );
}

void CAddIntentContextProcessor::AddObject( DWORD objectNum, const JSON& intent )
{
	assert(cmp != 0);
	const DWORD intentID = intStorage->LoadObject( intent );
	assert( intentID != NotFound );
	builder->AddObject( objectNum, intentID );
	if( callback != 0 ) {
		callback->ReportProgress( 1, "Lattice Size is " + StdExt::to_string( lattice.Size() ) );
	}
	++objectCount;
}
void CAddIntentContextProcessor::ProcessAllObjectsAddition()
{
	// Counting edge number
	CStdIterator<CLatticeNodes::const_iterator> node( lattice.GetNodes() );
	DWORD edgeCount = 0;
	for( ; !node.IsEnd(); ++node ) {
		edgeCount += (*node).Parents.size();
	}

	if( callback != 0 ) {
		callback->ReportProgress( 1,
			"Lattice Size = " + StdExt::to_string( lattice.Size() ) + " "
			"Edges Count = " + StdExt::to_string( edgeCount ) );
	}
}
void CAddIntentContextProcessor::SaveResult( const std::string& path )
{
	const string outFullDataPath( path );

	string outSelectedDataPath( path );
	const size_t ext = outSelectedDataPath.find_last_of( "." );
	if( ext != string::npos ) {
		outSelectedDataPath = outSelectedDataPath.substr(0,ext);
	}
	outSelectedDataPath += ".selected.json";

	outputParams.PercentageBase = objectCount;

	CLatticeWriter( lattice, intStorage, extStorage, outputParams )
		.Write( outFullDataPath, outSelectedDataPath );
}

void CAddIntentContextProcessor::LoadParams( const JSON& json )
{
	static char place[]="CAddIntentContextProcessor::LoadParams";
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( place, error );
	}
	assert( string( params["Type"].GetString() ) == ContextProcessorModuleType );
	assert( string( params["Name"].GetString() ) == AddIntentContextProcessorModule );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for PatternManager";
		throw new CJsonException(place, error);
	}
	const rapidjson::Value& p = params["Params"];

	if( !p.HasMember( "PatternManager") || !p["PatternManager"].IsObject() ) {
		error.Data = json;
		error.Error = "THIS.Params.PatternManager is not found.";
		throw new CJsonException(place, error);
	}
	const rapidjson::Value& pm = params["Params"]["PatternManager"];
	string errorText;
	cmp.reset( CreateModuleFromJSON<IPatternManager>(pm,errorText) );
	if( cmp == 0 ) {
		throw new CJsonException( place, CJsonError( json, errorText ) );
	}
	intStorage->Initialize( cmp );
	builder->Initialize( extStorage, intStorage );
	builder->SetResultLattice( lattice );
	objectCount = 0;

	if( p.HasMember( "OutputParams") && p["OutputParams"].IsObject() ) {
		const rapidjson::Value& opJson = p["OutputParams"];
#define	getFromJson( name, opp ) \
		if( opJson.HasMember( #name ) && opJson[#name].Is##opp() ) { \
			outputParams.name = opJson[#name].Get##opp(); \
		}
		getFromJson( MinExtentSize, Uint );
		getFromJson( MinLift, Uint );
		// We should check IsNumber rather than IsDouble here.
		if( opJson.HasMember( "MinStab" ) && opJson["MinStab"].IsNumber() ) {
			outputParams.MinStab = opJson["MinStab"].GetDouble();
		}
		getFromJson( OutExtent, Bool );
		getFromJson( OutSupport, Bool );
		getFromJson( OutOrder, Bool );
		getFromJson( OutStabEstimation, Bool );
		getFromJson( OutStability, Bool );
		getFromJson( IsStabilityInLog, Bool );
#undef	getFromJson
	}
}


JSON CAddIntentContextProcessor::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();

#define	addToJson( name, opp ) \
		.AddMember( #name , rapidjson::Value().Set##opp( outputParams.name ), alloc )
	params.SetObject()
		.AddMember( "Type", ContextProcessorModuleType, alloc )
		.AddMember( "Name", AddIntentContextProcessorModule, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "OutputParams", rapidjson::Value().SetObject()
				addToJson( MinExtentSize, Uint )
				addToJson( MinLift, Uint )
				addToJson( MinStab, Double )
				addToJson( OutExtent, Bool )
				addToJson( OutSupport, Bool )
				addToJson( OutOrder, Bool )
				addToJson( OutStabEstimation, Bool )
				addToJson( OutStability, Bool )
				addToJson( IsStabilityInLog, Bool ),
			alloc ),
		alloc );
#undef addToJson

	IModule* m = dynamic_cast<IModule*>(cmp.get());
	assert( m!=0);
	if( m != 0 ) {
		JSON cmpParams = m->SaveParams();
		rapidjson::Document cmpParamsDoc;
		CJsonError error;
		const bool rslt = ReadJsonString( cmpParams, cmpParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("PatternManager", cmpParamsDoc.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

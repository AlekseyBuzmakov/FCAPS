// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/OptimisticEstimators/BinaryClassificationOEst.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/document.h>

#include <sstream>

using namespace std;

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CBinaryClassificationOEst> CBinaryClassificationOEst::registrar(
	                 OptimisticEstimatorModuleType, BinaryClassificationOptimisticEstimator );

CBinaryClassificationOEst::CBinaryClassificationOEst()
{
	
}

double CBinaryClassificationOEst::GetValue(const IExtent* ext) const
{
	return 0;
}
double CBinaryClassificationOEst::GetBestSubsetEstimate(const IExtent* ext) const
{
	return 0;
}

void CBinaryClassificationOEst::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CBinaryClassificationOEst::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == OptimisticEstimatorModuleType );
	assert( string( params["Name"].GetString() ) == BinaryClassificationOptimisticEstimator );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for Classification Information";
		throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
	}

	const rapidjson::Value& p = params["Params"];

	// Reading Levels that are considered as positive
	if(!(p.HasMember("TargetLevels") && (p["TargetLevels"].IsArray() || p["TargetLevels"].IsString()))) {
		error.Data = json;
		error.Error = "Params.TargetLevels is not found or is neither 'Array' nor 'String'.";
		throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
	}
	for( int i = 0 ; p["TargetLevels"].Size(); ++i ) {
		if(!p["TargetLevels"][i].IsString()) {
			error.Data = json;
			error.Error = "Params.TargetLevels has nonstring values.";
			throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
		}
		targetClasses.insert(p["TargetLevels"][i].GetString());
	}

	// Reading class labels information
	if(!(p.HasMember("Classes") && (p["Classes"].IsArray() || p["Classes"].IsString()))) {
		error.Data = json;
		error.Error = "Params.Classes is not found or is neither 'Array' nor 'String'.";
		throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
	}
	const rapidjson::Value* cl_ptr = 0;
	if( p["Classes"].IsArray() ) {
		cl_ptr = &p["Classes"];
	} else {
		// TODO: read class info from a separte file
		rapidjson::Document classesDocument;
		if( !ReadJsonFile( json, params, error ) ) {
			throw new CJsonException( "CBinaryClassificationOEst::LoadParams", error );
		}
		cl_ptr=&classesDocument;
	}
	assert(cl_ptr != 0);
	
	const rapidjson::Value& cl = *cl_ptr;
	classes.resize(cl.Size(),false);
	strClasses.resize(cl.Size());
	for( int i = 0 ; cl.Size(); ++i ) {
		if(!cl[i].IsString()) {
			error.Data = json;
			error.Error = "Params.Classes has nonstring values.";
			throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
		}
		strClasses[i]=cl[i].GetString();
		classes[i]=targetClasses.find(strClasses[i]) != targetClasses.end();
	}
}

JSON CBinaryClassificationOEst::SaveParams() const
{
	// TODO
	return "";	
}



// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/OptimisticEstimatorModules/LocalTreatmentEffectOEst.h>
#include <fcaps/Extent.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/document.h>

#include <boost/math/distributions/chi_squared.hpp>
#include <sstream>

using namespace std;

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CLocalTreatmentEffectOEst> CLocalTreatmentEffectOEst::registrar(
	                 OptimisticEstimatorModuleType, LocalTreatmentEffectOptimisticEstimator );

CLocalTreatmentEffectOEst::CLocalTreatmentEffectOEst() 
{
	
}

double CLocalTreatmentEffectOEst::GetValue(const IExtent* ext) const
{
	assert(ext!=0);
	assert(false);
	return 0;
}
double CLocalTreatmentEffectOEst::GetBestSubsetEstimate(const IExtent* ext) const
{
	assert(ext!=0);
	assert(false);
	return 0;
}

JSON CLocalTreatmentEffectOEst::GetJsonQuality(const IExtent* ext) const
{
	std::stringstream rslt;
	assert(false);
	return rslt.str();
}
void CLocalTreatmentEffectOEst::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CLocalTreatmentEffectOEst::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == OptimisticEstimatorModuleType );
	assert( string( params["Name"].GetString() ) == LocalTreatmentEffectOptimisticEstimator );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for Object Information";
		throw new CJsonException("CLocalTreatmentEffectOEst::LoadParams", error);
	}

	const rapidjson::Value& p = params["Params"];

	// Reading Levels that are considered as positive
	if(!(p.HasMember("ControlLevels") && (p["ControlLevels"].IsArray()))) {
		error.Data = json;
		error.Error = "Params.ControlLevels is not found or is not an 'Array'.";
		throw new CJsonException("CLocalTreatmentEffectOEst::LoadParams", error);
	}
	for( int i = 0 ; i < p["ControlLevels"].Size(); ++i ) {
		if(!p["ControlLevels"][i].IsString()) {
			error.Data = json;
			error.Error = "Params.ControlLevels has nonstring values.";
			throw new CJsonException("CLocalTreatmentEffectOEst::LoadParams", error);
		}
		cntrlLevels.insert(p["ControlLevels"][i].GetString());
	}

	// Reading class labels information
	if(!(p.HasMember("ObjInfoFile") && p["ObjInfoFile"].IsString())) {
		error.Data = json;
		error.Error = "Params.ObjInfoFile is not found or is not a file name.";
		throw new CJsonException("CLocalTreatmentEffectOEst::LoadParams", error);
	}

	rapidjson::Document objInfoDocument;
	objInfoFilePath = p["ObjInfoFile"].GetString();
	if( !ReadJsonFile( objInfoFilePath, objInfoDocument, error ) ) {
		throw new CJsonException( "CLocalTreatmentEffectOEst::LoadParams", error );
	}

	if(!objInfoDocument.IsObject()
	   || !objInfoDocument.HasMember("Y") || !objInfoDocument["Y"].IsArray()
	   || !objInfoDocument.HasMember("Trt") || !objInfoDocument["Trt"].IsArray()
	   || objInfoDocument["Y"].Size() != objInfoDocument["Trt"].Size())
	{
		throw new CTextException( "CLocalTreatmentEffectOEst::LoadParams", "Object Info is not an object; or does not contains 'Y' or 'Trt' arrays; or their sizes are different" );
	}

	const rapidjson::Value& y = objInfoDocument["Y"];
	const rapidjson::Value& trt = objInfoDocument["Trt"];
	assert(y.Size() == trt.Size());

	objY.resize(y.Size());
	objTrt.resize(trt.Size());
	for( int i = 0; i < trt.Size(); ++i ) {
		if(!y[i].IsNumber()) {
			error.Data = json;
			error.Error = "Params.ObjInfoFile.Y has nonnumeric values.";
			throw new CJsonException("CLocalTreatmentEffectOEst::LoadParams", error);
		}
		if(!trt[i].IsString()) {
			error.Data = json;
			error.Error = "Params.ObjInfoFile.Trt has nonstring values.";
			throw new CJsonException("CLocalTreatmentEffectOEst::LoadParams", error);
		}
		objTrt[i]=cntrlLevels.find(trt[i].GetString()) != cntrlLevels.end();
		objY[i]=y[i].GetDouble();
	}
}

JSON CLocalTreatmentEffectOEst::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", OptimisticEstimatorModuleType, alloc )
		.AddMember( "Name", LocalTreatmentEffectOptimisticEstimator, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject(), alloc );

	rapidjson::Value& p = params["Params"];
	p.AddMember("ObjInfoFile", rapidjson::StringRef(objInfoFilePath.c_str()), alloc);

	rapidjson::Value& controlLevels = p.AddMember( "ControlLevels", rapidjson::Value().SetArray(), alloc )["ControlLevels"];
	for(auto it = cntrlLevels.begin(); it != cntrlLevels.end(); ++it) {
		controlLevels.PushBack(rapidjson::StringRef((*it).c_str()), alloc);
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

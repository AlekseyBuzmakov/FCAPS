// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/OptimisticEstimatorModules/BinaryClassificationOEst.h>
#include <fcaps/Extent.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/document.h>

#include <sstream>

using namespace std;

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CBinaryClassificationOEst> CBinaryClassificationOEst::registrar(
	                 OptimisticEstimatorModuleType, BinaryClassificationOptimisticEstimator );

CBinaryClassificationOEst::CBinaryClassificationOEst() :
	nPlus(0)
{
	
}

double CBinaryClassificationOEst::GetValue(const IExtent* ext) const
{
	assert(ext!=0);
	const DWORD curNPlus=getPositiveObjectsCount(ext);
	assert(curNPlus <= ext->Size());

	
	return static_cast<double>(ext->Size()) / classes.size()
		* (1.0L * curNPlus / ext->Size() - 1.0L * nPlus / classes.size());
}
double CBinaryClassificationOEst::GetBestSubsetEstimate(const IExtent* ext) const
{
	assert(ext!=0);
	const DWORD curNPlus=getPositiveObjectsCount(ext);
	assert(curNPlus <= ext->Size());

	// Here we should find maximum of locNPlus/n - nPlus*locN/n^2, the first one is maximized when locNPlus == curNPLus,
	//   the second one is maximized when locN is minimized, i.e., it should be equal curNPlus.
	
	const double result =
		static_cast<double>( curNPlus) / classes.size() - 1.0L * nPlus * curNPlus / (classes.size() * classes.size());
	
	assert( result - GetValue(ext) > -1e-10);
	return result;
}

JSON CBinaryClassificationOEst::GetJsonQuality(const IExtent* ext) const
{
	std::stringstream rslt;
	const DWORD curNPlus=getPositiveObjectsCount(ext);
	rslt << "{"
		<< "\"Positiveness\":" << static_cast<double>(curNPlus) / ext->Size() << ","
		<< "\"BasePositiveness\":" << static_cast<double>(nPlus) / classes.size() << ","
		<< "\"Value\":" << GetValue(ext)
		<<"}";
	return rslt.str();
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
	if(!(p.HasMember("TargetClasses") && (p["TargetClasses"].IsArray()))) {
		error.Data = json;
		error.Error = "Params.TargetClasses is not found or is neither an 'Array'.";
		throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
	}
	for( int i = 0 ; i < p["TargetClasses"].Size(); ++i ) {
		if(!p["TargetClasses"][i].IsString()) {
			error.Data = json;
			error.Error = "Params.TargetClasses has nonstring values.";
			throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
		}
		targetClasses.insert(p["TargetClasses"][i].GetString());
	}

	// Reading class labels information
	if(!(p.HasMember("Classes") && (p["Classes"].IsArray() || p["Classes"].IsString()))) {
		error.Data = json;
		error.Error = "Params.Classes is not found or is neither 'Array' nor 'String'.";
		throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
	}

	const rapidjson::Value* cl_ptr = 0;
	rapidjson::Document classesDocument;
	if( p["Classes"].IsArray() ) {
		cl_ptr = &p["Classes"];
	} else {
		classesFilePath = p["Classes"].GetString();
		if( !ReadJsonFile( classesFilePath, classesDocument, error ) ) {
			throw new CJsonException( "CBinaryClassificationOEst::LoadParams", error );
		}
		cl_ptr=&classesDocument;
	}
	assert(cl_ptr != 0);
	const rapidjson::Value& cl = *cl_ptr;

	assert(nPlus==0);
	classes.resize(cl.Size(),false);
	strClasses.resize(cl.Size());
	for( int i = 0; i < cl.Size(); ++i ) {
		if(!cl[i].IsString()) {
			error.Data = json;
			error.Error = "Params.Classes has nonstring values.";
			throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
		}
		strClasses[i]=cl[i].GetString();
		classes[i]=targetClasses.find(strClasses[i]) != targetClasses.end();
		nPlus += classes[i];
	}
}

JSON CBinaryClassificationOEst::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", OptimisticEstimatorModuleType, alloc )
		.AddMember( "Name", BinaryClassificationOptimisticEstimator, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject(), alloc );

	rapidjson::Value& p = params["Params"];

	rapidjson::Value& targets = p.AddMember( "TargetClasses", rapidjson::Value().SetArray(), alloc )["TargetClasses"];
	for(auto it = targetClasses.begin(); it != targetClasses.end(); ++it) {
		targets.PushBack(rapidjson::StringRef((*it).c_str()), alloc);
	}

	if(classesFilePath != "") {
		p.AddMember("Classes", rapidjson::StringRef(classesFilePath.c_str()), alloc);
	} else {
		// TODO How should we do that??
		//   skipping for now
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

// Returns the number of positive objects in the extent @param ext
DWORD CBinaryClassificationOEst::getPositiveObjectsCount(const IExtent* ext) const
{
	assert(ext!=0);
	
	CPatternImage img;
	ext->GetExtent(img);
	assert(img.ImageSize != 0 && img.Objects != 0);
	assert(ext->Size() == img.ImageSize);
	
	DWORD nPlus = 0;
	for( int i = 0; i < img.ImageSize; ++i) {
		const DWORD objNum = img.Objects[i];
		assert(objNum < classes.size());
		nPlus += classes[objNum];
	}
	return nPlus;
}

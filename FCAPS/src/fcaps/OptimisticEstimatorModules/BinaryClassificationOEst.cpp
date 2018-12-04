// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/OptimisticEstimatorModules/BinaryClassificationOEst.h>
#include <fcaps/Extent.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/document.h>

#include <boost/math/distributions/chi_squared.hpp>
#include <sstream>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(x...) #x

const char description[] =
STR(
	{
	"Name":"Local Treatment Effect Optimistic Estimator",
	"Description":"An optimistic estimator for computing upper bound of Local Treatment Effect given as a linear function of difference between confidence intervals of Test and Control groups and the total number of objects in the extent of a pattern.",
	"Params": {
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of LTE OEst",
			"type": "object",
			"properties": {
				"TargetClasses":{
					"description": "Array fo strings that gives the classes that would be all together considered as a traget class",
					"type":"array",
					"items": {
						"type":"string"
					}
				},
				"Classes":{
					"description": "The path to a file, containing the class labels for every object",
					"type": "file-path",
					"example": {
						"description":"The file contains an array of string labels for every object",
						"json":["+","-","-","+","-"]
					}
				}
			}
		}
	}
);

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CBinaryClassificationOEst> CBinaryClassificationOEst::registrar(
	                 OptimisticEstimatorModuleType, BinaryClassificationOptimisticEstimator );

const char* const CBinaryClassificationOEst::Desc()
{
	return description;
}

CBinaryClassificationOEst::CBinaryClassificationOEst() :
	nPlus(0)
{
	
}

double CBinaryClassificationOEst::GetValue(const IExtent* ext) const
{
	assert(ext!=0);
	if(ext->Size() == 0) {
		return 0;
	}

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

	// Computing P-value
	const DWORD o1 = curNPlus;
	const DWORD o0 = ext->Size() - o1;
	//  expected counts
	const double probability = static_cast<double>(nPlus)/classes.size();
	const double e1 = ext->Size() * probability;  
	const double e0 = ext->Size() - e1;
	const double T = (e1-o1)*(e1-o1)/e1 + (e0-o0)*(e0-o0)/e0;
	boost::math::chi_squared chi2(1);
	const double pValue = 1-boost::math::cdf(chi2,T);

	rslt << "{"
		<< "\"Positiveness\":" << static_cast<double>(curNPlus) / ext->Size() << ","
		<< "\"BasePositiveness\":" << static_cast<double>(nPlus) / classes.size() << ","
		<< "\"Size\":" << static_cast<double>(ext->Size()) / classes.size() << ","
		<< "\"p-Value\":" << pValue << ","
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
	assert(img.ImageSize >= 0 && img.Objects != 0);
	assert(ext->Size() == img.ImageSize);
	
	DWORD nPlus = 0;
	for( int i = 0; i < img.ImageSize; ++i) {
		const DWORD objNum = img.Objects[i];
		assert(objNum < classes.size());
		nPlus += classes[objNum];
	}
	return nPlus;
}

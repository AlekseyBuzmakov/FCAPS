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
#define STR(...) #__VA_ARGS__

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
						"description":"The file contains a list of 'Class' with an array of string labels for every object and optional 'Weights' with weights of the objects",
						"json":{"Class":["+","-","-","+","-"],"Weight":[0.9,1,1,0.9,0.2]}
					}
				},
				"FreqWeight": {
					"description": "A weight for the frequence part in the resulting formula  should be less than 1. If 1 frequency part is considered with the same weight to probability part.",
					"type": "number",
					"minimum": 0,
					"maximum": 1
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
	wPlus(0),
	wAll(0),
	freqWeight(1)
{
	
}

void CBinaryClassificationOEst::GetValue(const IExtent* ext, COEstValue& val ) const
{
	assert(ext!=0);
	double curWPlus = 0;
	double curWAll = 0;
	getObjectsWeight(ext, curWPlus, curWAll);
	assert(curWPlus <= curWAll);

	val.Value = getValue(curWPlus,curWAll);

	// Here we should find maximum of locNPlus/n - nPlus*locN/n^2, the first one is maximized when locNPlus == curNPLus,
	//   the second one is maximized when locN is minimized, i.e., it should be equal curNPlus.
	
	val.BestSubsetEstimate = pow(curWPlus / wAll, freqWeight)
		* (1 - wPlus / wAll);

	assert( val.BestSubsetEstimate - val.Value > -1e-10);
}

JSON CBinaryClassificationOEst::GetJsonQuality(const IExtent* ext) const
{
	std::stringstream rslt;
	double curWPlus = 0;
	double curWAll = 0;
	getObjectsWeight(ext, curWPlus, curWAll);
	assert(curWPlus <= curWAll);

	// Computing P-value
	const double o1 = curWPlus;
	const double o0 = curWAll - o1;
	//  expected counts
	const double probability = wPlus/wAll;
	const double e1 = curWAll * probability;  
	const double e0 = curWAll - e1;
	const double T = e1*e0 == 0 ? 0 : (e1-o1)*(e1-o1)/e1 + (e0-o0)*(e0-o0)/e0;
	boost::math::chi_squared chi2(1);
	const double pValue = 1-boost::math::cdf(chi2,T);

	rslt << "{"
		<< "\"Positiveness\":" << curWPlus / curWAll << ","
		<< "\"BasePositiveness\":" << wPlus / wAll << ","
		<< "\"Size\":" << curWAll / wAll << ","
		<< "\"p-Value\":" << pValue << ","
		<< "\"Value\":" << getValue(curWPlus, curWAll)
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
	if(!(p.HasMember("Classes") && p["Classes"].IsString())) {
		error.Data = json;			                
		error.Error = "Params.Classes is not found or is not an 'String'.";
		throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
	}

	rapidjson::Document classesDocument;
	classesFilePath = p["Classes"].GetString();
	if( !ReadJsonFile( classesFilePath, classesDocument, error ) ) {
		throw new CJsonException( "CBinaryClassificationOEst::LoadParams", error );
	}
	if( !classesDocument.IsObject() || !classesDocument.HasMember("Class") || !classesDocument["Class"].IsArray() ) {
		error.Data = json;			                
		error.Error = "Params.Classes.Class is not found or is not an 'Array'.";
		throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
	}
	const rapidjson::Value& cl = classesDocument["Class"];
	const rapidjson::Value* w = 0;
	if( classesDocument.HasMember("Weight") && classesDocument["Weight"].IsArray() && classesDocument["Weight"].Size() == cl.Size() ) {
		w = &classesDocument["Weight"];
	}

	assert(wPlus == 0);
	assert(wAll == 0);
	classes.resize(cl.Size(),false);
	weights.resize(classes.size(),0);
	strClasses.resize(cl.Size());

	for( int i = 0; i < cl.Size(); ++i ) {
		if(!cl[i].IsString()) {
			error.Data = json;
			error.Error = "Params.Classes has nonstring values.";
			throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
		}
		strClasses[i]=cl[i].GetString();
		classes[i]=targetClasses.find(strClasses[i]) != targetClasses.end();
		weights[i] = 1;
		if(w != 0) {
			assert(i < w->Size());
			if(!(*w)[i].IsDouble() || (*w)[i].GetDouble() < 0 ) {
				error.Data = json;
				error.Error = "Params.Classes.Weight should be a positive number.";
				throw new CJsonException("CBinaryClassificationOEst::LoadParams", error);
			}
			weights[i] = (*w)[i].GetDouble();
		}
		wPlus += classes[i] * weights[i];
		wAll += weights[i];
	}

	if(p.HasMember("FreqWeight") && p["FreqWeight"].IsNumber()) {
		const double a =p["FreqWeight"].GetDouble();
		if( 0 <= a && a <= 1) {
			freqWeight = a;
		}
	}
	
}

JSON CBinaryClassificationOEst::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", OptimisticEstimatorModuleType, alloc )
		.AddMember( "Name", BinaryClassificationOptimisticEstimator, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
		            .AddMember("FreqWeight",rapidjson::Value().SetDouble(freqWeight),alloc)
		            , alloc );

	rapidjson::Value& p = params["Params"];

	rapidjson::Value& targets = p.AddMember( "TargetClasses", rapidjson::Value().SetArray(), alloc )["TargetClasses"];
	for(auto it = targetClasses.begin(); it != targetClasses.end(); ++it) {
		targets.PushBack(rapidjson::StringRef((*it).c_str()), alloc);
	}

	p.AddMember("Classes", rapidjson::StringRef(classesFilePath.c_str()), alloc);

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

// Returns the number of positive objects in the extent @param ext
void CBinaryClassificationOEst::getObjectsWeight(const IExtent* ext, double& wPlus, double& wAll) const
{
	assert(ext!=0);
	
	CPatternImage img;
	ext->GetExtent(img);
	assert(img.ImageSize >= 0 && img.Objects != 0);
	assert(ext->Size() == img.ImageSize);
	
	wPlus = 0;
	wAll = 0;
	for( int i = 0; i < img.ImageSize; ++i) {
		const DWORD objNum = img.Objects[i];
		assert(objNum < classes.size());
		assert(classes.size() == weights.size());

		wPlus += classes[objNum] * weights[objNum];
		wAll += weights[objNum];
	}

	ext->ClearMemory(img);

	assert(wPlus <= wAll);
}

// Returns the value given the number of positive objects and the extent size
double CBinaryClassificationOEst::getValue( const double& curWPlus, const double& curWAll ) const
{
	return pow(curWAll / wAll, freqWeight)
		* (curWPlus / curWAll - wPlus / wAll);
}

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/OptimisticEstimatorModules/FisherBinClassificationOEst.h>
#include <fcaps/Extent.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/document.h>

#include <boost/math/distributions/chi_squared.hpp>
#include <sstream>
#include <bits/stdc++.h>
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
const CModuleRegistrar<CFisherBinClassificationOEst> CFisherBinClassificationOEst::registrar;

const char* const CFisherBinClassificationOEst::Desc()
{
	return description;
}


CFisherBinClassificationOEst::CFisherBinClassificationOEst() :
	wPlus(0),
	wAll(0),
	freqWeight(1)
{
	log10_2 = log10(2.0);
	log_10 = log(10);
	log_method = 1;
	log_inv_binom_N_n = 0;
	log_method = -1;
	loggamma = 0;
}

void CFisherBinClassificationOEst::GetValue(const IExtent* ext, COEstValue& val ) const
{
	assert(ext!=0);
	double curWPlus = 0;
	double curWAll = 0;
	getObjectsWeight(ext, curWPlus, curWAll);
	assert(curWPlus <= curWAll);

/********************************************************************************************/
//Method 1
/*******************************************************************************************/
	//BestSubsetEstimate defined in 2018 paper
	/*
	val.Value = getValue(curWPlus,curWAll);	
        double maxQuality;
	int N=(int)wAll;
	int n1=(int)wPlus;	
	int x=(int)curWAll;
	
	if(x<=N&&x>n1)
	{
		val.BestSubsetEstimate=-1*(log_inv_binom_N_n/log_10);
	}
	else
	{
		int a_min = ((n1+x-N) > 0) ? (n1+x-N) : 0;//max(0,n+x-N)
		int a_max = (x > n1) ? n1 : x;//min(x,n)
		maxQuality=getValue(a_min,x);
		for(int u=a_min+1;u<=a_max;u++)
		{
			double curQuality=getValue(u,x);
			if(curQuality>maxQuality)
				maxQuality=curQuality;
		}
		val.BestSubsetEstimate = maxQuality;
        
	}
	*/
/******************************************************************************************/
/********************************************************************************************/
//Method 2
/*******************************************************************************************/
	
	int N=(int)wAll;
	int n1=(int)wPlus;	
	int x=(int)curWAll;
	int a=(int)curWPlus;
	if( x<=N && x>n1 )
	{
		val.BestSubsetEstimate=-1*(log_inv_binom_N_n/log_10);
		val.Value = getValue(curWPlus,curWAll);
	}
	else
	{
		double pre_comp_xterms, pval, p_left, p_right , p_left_log, p_right_log , pval_log;
		int a_min, a_max;
		// Compute the contribution of all terms depending on x but not on a
		pre_comp_xterms = loggamma[x] + loggamma[N-x];
		a_min = ((n1+x-N) > 0) ? (n1+x-N) : 0;//max(0,n+x-N)
		a_max = (x > n1) ? n1 : x;//min(x,n)

		double maxQuality=INT_MAX;
		// log values computation
		pval_log = 0;
	
		while(a_min<a_max){
			p_left_log = (pre_comp_xterms + log_inv_binom_N_n - (loggamma[a_min] + loggamma[n1-a_min] + loggamma[x-a_min] + loggamma[(N-n1)-(x-a_min)]))/log_10;
			p_right_log = (pre_comp_xterms + log_inv_binom_N_n - (loggamma[a_max] + loggamma[n1-a_max] + loggamma[x-a_max] + loggamma[(N-n1)-(x-a_max)]))/log_10;

			if(p_left_log == p_right_log) {

				// log increment
				if(pval_log == 0){
				pval_log = p_left_log + log10_2;
				}
				else{
					double temp = p_left_log + log10_2;
					//pval_log = pval_log + log10(1 + pow(10.0,(temp - pval_log)));
					pval_log = pval_log + (log1p(exp((temp - pval_log) * log_10)) / log_10);
					//pval_log = sumlogs(pval_log , temp);
				}
				if(a_min==a)
					val.Value=pval_log;
				if(pval_log<maxQuality)
					maxQuality=pval_log;
				// movement of indices
				a_min++;
				a_max--;

			}
			else
			if(p_left_log < p_right_log){

				// log increment
				if(pval_log == 0){
					pval_log = p_left_log;
				}
				else{
					//pval_log = pval_log + log10(1 + pow(10.0,(p_left_log - pval_log)));
					pval_log = pval_log + (log1p(exp((p_left_log - pval_log) * log_10)) / log_10);
					//pval_log = sumlogs(pval_log , p_left_log);
				}

				if(a_min==a)
					val.Value=pval_log;
				if(pval_log<maxQuality)
					maxQuality=pval_log;

				// move index
				a_min++;
			}
			else{

				// log increment
				if(pval_log == 0){
					pval_log = p_right_log;
				}
				else{
					//pval_log = pval_log + log10(1 + pow(10.0,(p_right_log - pval_log)));
					pval_log = pval_log + (log1p(exp((p_right_log - pval_log) * log_10)) / log_10);
					//pval_log = sumlogs(pval_log , p_right_log);
				}

				if(a_max==a)
					val.Value=pval_log;
				if(pval_log<maxQuality)
					maxQuality=pval_log;

				// move index
				a_max--;
			}

		}
		// In this case a_min=a_max is the mode of the distribution and its p-value is 1 by definition
		if(a_min==a_max){
			if(a_min==a)
				val.Value=0;
			if(maxQuality>0)
				maxQuality=0;
        	}
		val.Value=-1*val.Value;
		val.BestSubsetEstimate=-1*maxQuality;
	}
/********************************************************************************************/
/*******************************************************************************************/
	assert( val.BestSubsetEstimate - val.Value > -1e-10);
}


void CFisherBinClassificationOEst::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CFisherBinClassificationOEst::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == OptimisticEstimatorModuleType );
	assert( string( params["Name"].GetString() ) == FisherBinClassificationOptimisticEstimator );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for Classification Information";
		throw new CJsonException("CFisherBinClassificationOEst::LoadParams", error);
	}

	const rapidjson::Value& p = params["Params"];

	// Reading Levels that are considered as positive
	if(!(p.HasMember("TargetClasses") && (p["TargetClasses"].IsArray()))) {
		error.Data = json;
		error.Error = "Params.TargetClasses is not found or is neither an 'Array'.";
		throw new CJsonException("CFisherBinClassificationOEst::LoadParams", error);
	}
	for( int i = 0 ; i < p["TargetClasses"].Size(); ++i ) {
		if(!p["TargetClasses"][i].IsString()) {
			error.Data = json;
			error.Error = "Params.TargetClasses has nonstring values.";
			throw new CJsonException("CFisherBinClassificationOEst::LoadParams", error);
		}
		targetClasses.insert(p["TargetClasses"][i].GetString());
	}

	// Reading class labels information
	if(!(p.HasMember("Classes") && p["Classes"].IsString())) {
		error.Data = json;			                
		error.Error = "Params.Classes is not found or is not an 'String'.";
		throw new CJsonException("CFisherBinClassificationOEst::LoadParams", error);
	}

	rapidjson::Document classesDocument;
	classesFilePath = p["Classes"].GetString();
	if( !ReadJsonFile( classesFilePath, classesDocument, error ) ) {
		throw new CJsonException( "CFisherBinClassificationOEst::LoadParams", error );
	}
	if( !classesDocument.IsObject() || !classesDocument.HasMember("Class") || !classesDocument["Class"].IsArray() ) {
		error.Data = json;			                
		error.Error = "Params.Classes.Class is not found or is not an 'Array'.";
		throw new CJsonException("CFisherBinClassificationOEst::LoadParams", error);
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
			throw new CJsonException("CFisherBinClassificationOEst::LoadParams", error);
		}
		strClasses[i]=cl[i].GetString();
		classes[i]=targetClasses.find(strClasses[i]) != targetClasses.end();
		weights[i] = 1;
		if(w != 0) {
			assert(i < w->Size());
			if(!(*w)[i].IsDouble() || (*w)[i].GetDouble() < 0 ) {
				error.Data = json;
				error.Error = "Params.Classes.Weight should be a positive number.";
				throw new CJsonException("CFisherBinClassificationOEst::LoadParams", error);
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
	/**********************************************************************************/
	//Initialization 
	/**********************************************************************************/
	int N=(int)wAll;
	int n1=(int)wPlus;
	int x;
	// Allocate memory for log-gamma cache, raising error if it fails
	loggamma = (double *)malloc((N+1)*sizeof(double));
	if(!loggamma){
		fprintf(stderr,"Error in function loggamma_init: couldn't allocate memory for array loggamma\n");
		exit(1);
	}
	// Initialise cache with appropriate values
	for(x=0;x<=N;x++) loggamma[x] = lgamma(x+1);//Gamma(x) = (x-1)!
	// Initialise log_inv_binom_N_n
		log_inv_binom_N_n = loggamma[n1] + loggamma[N-n1] - loggamma[N];
}

JSON CFisherBinClassificationOEst::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", OptimisticEstimatorModuleType, alloc )
		.AddMember( "Name", FisherBinClassificationOptimisticEstimator, alloc )
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
void CFisherBinClassificationOEst::getObjectsWeight(const IExtent* ext, double& wPlus, double& wAll) const
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
double CFisherBinClassificationOEst::getValue( const double& curWPlus, const double& curWAll ) const
{
	//cout<<"Inside function getValue..";
	//cout<<"\n curWPlus: "<<curWPlus<<" curWAll: "<<curWAll;
	double pre_comp_xterms, pval, p_left, p_right , p_left_log, p_right_log , pval_log;
	int a, a_min, a_max;
	int N=(int)wAll;
	int n1=(int)wPlus;	
	int x=(int)curWAll;

	pre_comp_xterms = loggamma[x] + loggamma[N-x];
	a_min = ((n1+x-N) > 0) ? (n1+x-N) : 0;//max(0,n+x-N)
	a_max = (x > n1) ? n1 : x;//min(x,n)
	a=(int)curWPlus;
	// Precompute the hypergeometric PDF in the range of interest
	//double hypergeom_pvals_a = exp(pre_comp_xterms + log_inv_binom_N_n - (loggamma[a] + loggamma[n1-a] + loggamma[x-a] + loggamma[(N-n1)-(x-a)]));
	double temp_value = pre_comp_xterms + log_inv_binom_N_n - (loggamma[a] + loggamma[n1-a] + loggamma[x-a] + loggamma[(N-n1)-(x-a)]);
	double hypergeom_pvals_log_a = temp_value / log_10;
	//pval = 0;
	pval_log = 0;
	for(a=a_min; a<=a_max; a++) {
		//double hypergeom_pvals_a_curr = exp(pre_comp_xterms + log_inv_binom_N_n - (loggamma[a] + loggamma[n1-a] + loggamma[x-a] + loggamma[(N-n1)-(x-a)]));
		temp_value = pre_comp_xterms + log_inv_binom_N_n - (loggamma[a] + loggamma[n1-a] + loggamma[x-a] + loggamma[(N-n1)-(x-a)]);
		double temp = temp_value / log_10;
		if(hypergeom_pvals_log_a>=temp){
		//if(hypergeom_pvals_a>=hypergeom_pvals_a_curr){
			 //pval+=hypergeom_pvals_a_curr;
			 if(pval_log == 0){
				pval_log = temp;
			 }
			 else
			 	pval_log = pval_log + (log1p(exp((temp - pval_log) * log_10)) / log_10);
		}
	}
	//cout<<"pval: "<<pval<<"pval_log: "<<pval_log;
	return -1*pval_log;
}

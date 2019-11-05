// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/WestfallYoungOptimisticEstimatorModules/WYOEst.h>
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
	"Name":"Westfall-Young statistical Test for OEst",
	"Description":"Class verifies the statistical significance of the found patterns by W-Y statistical test",
	"Params": {
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of Westfall-Young statistical test for OEst",
			"type": "object",
			"properties": {
				"OptimisticEstimator":{
					"description": "The object that defines the optimistic estimator for the goal function",
					"type": "@OptimisticEstimatorModules"
				},
				"PermutationCount": {
					"description": "A number of permutations to be computed.",
					"type": "integer",
					"minimum": 0
				}
			}
		}
	}
);

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CWYOEst> CWYOEst::registrar;

const char* const CWYOEst::Desc()
{
	return description;
}

CWYOEst::CWYOEst() :
	permutationCount(0)
{
}

void CWYOEst::GetValue(const IExtent* ext, CWYOEstValue& val ) const
{
	// TODO
	//   Normally just iterate over all permutations and compute the quality
	//   Permutations should be computed just once at initialization then they should be reused
}

void CWYOEst::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		// If JSON is incorrect should terminate
		throw new CJsonException( "CWYOEst::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == Type() );
	assert( string( params["Name"].GetString() ) == Name() );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params needed for internal Optimistic Estimate and for the number of computations are missed";
		throw new CJsonException("CWYOEst::LoadParams", error);
	}

	// Reading params for number of permutations
	const rapidjson::Value& p = params["Params"];
	if(!p.HasMember("PermutationCount") || !p["PermutationCount"].IsUint()) {
		error.Data = json;
		error.Error = "Params.PermutationCount is not found";
		throw new CJsonException("CWYOEst::LoadParams", error);
	}
	permutationCount =p["PermutationCount"].GetUint();

	// Reading object with Optimistic Estimator
	if( !p.HasMember("OptimisticEstimator") || !p["OptimisticEstimator"].IsObject()) {
		error.Data = json;
		error.Error = "Params.OptimisticEstimator is not found or is not an object.";
		throw new CJsonException("CBestPatternFirstComputationProcedure::LoadParams", error);
	}
	const rapidjson::Value& pc = params["Params"]["OptimisticEstimator"];
	string errorText;
	oest.reset( CreateModuleFromJSON<IOptimisticEstimator>(pc,errorText) );	
	if( oest == 0 ) {
		throw new CJsonException( "CSofiaContextProcessor::LoadParams", CJsonError( json, errorText ) );
	}

	// TODO: normally here initialization should be done
}

JSON CWYOEst::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", WestfallYoungOptimisticEstimatorModuleType, alloc )
		.AddMember( "Name", WYOptimisticEstimator, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
		            .AddMember("PermutationCount",rapidjson::Value().SetUint(permutationCount),alloc)
		            , alloc );

	IModule* m = dynamic_cast<IModule*>(oest.get());
	if( m != 0 ) { 
		rapidjson::Document cpParamsDoc;
		JSON cpParams = m->SaveParams();
		CJsonError error;
		const bool rslt = ReadJsonString( cpParams, cpParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("OptimisticEstimator", cpParamsDoc.Move(), alloc );
	} else {
		// It is not critical but strange
		assert(false);
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

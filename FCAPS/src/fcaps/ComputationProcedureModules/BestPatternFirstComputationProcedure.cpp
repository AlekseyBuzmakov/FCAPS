// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8

#include <fcaps/ComputationProcedureModules/BestPatternFirstComputationProcedure.h>

#include <fcaps/OptimisticEstimator.h>
#include <fcaps/PatternDescriptor.h>

#include <JSONTools.h>
#include <ModuleJSONTools.h>
#include <StdTools.h>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(x...) #x

const char description[] =
STR(
	{
	"Name":"Best Pattern First Computation Procedure",
	"Description":"This computation procedures expand patterns in a breath first order taking the most promissing patterns and expanding them first",
	"Params": {
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of Best Pattern First Computation Procedure",
			"type": "object",
			"properties": {
				"LocalProjectionChain":{
					"description": "The projection chain that allows for finding preimages for any pattern with its special projection chain.",
					"type": "@LocalProjectionChainModules"
				},
				"DefualtThld":{
					"description": "The initial threshold for DELTA-measure (or any other projection-antimonotonic measure).",
					"type":"number"
				},
				"MaxPatternNumber" :{
					"description": "The maximal number of patterns that should be stored at any iteration of the algorithm. Used only when 'AdjustThreshold' is true.",
					"type":"number",
					"minimumu":0,
					"exclusiveMinimum":true
				},
				"AdjustThreshold":{
					"description": "A flag indicating if the threshold should be adjusted in orderder to ensure polynomiality and memory finity",
					"type":"boolean"
				},
				"OptimisticEstimator":{
					"description": "The object that defines the optimistic estimator for the goal function",
					"type": "@OptimisticEstimatorModules"
				},
				"OEstMinQuality":{
					"description": "The minimal quality of the pattern that is considederd interesting to be found",
					"type": "number"
				}
			}
		}
	}
);


////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CBestPatternFirstComputationProcedure> CBestPatternFirstComputationProcedure::registrar;

const char* const CBestPatternFirstComputationProcedure::Desc()
{
	return description;
}

////////////////////////////////////////////////////////////////////
CBestPatternFirstComputationProcedure::CBestPatternFirstComputationProcedure() :
	deleter(lpChain),
	callback(0),
	thld(0),
	mpn(-1),
	shouldAdjustThld(false),
	isBestQualityKnown( false ),
	potentialCmp(lpChain),
	queue( potentialCmp )
{
}

void CBestPatternFirstComputationProcedure::SetCallback( const IComputationCallback * cb )
{
	callback = cb;
	assert(lpChain != 0);
}
void CBestPatternFirstComputationProcedure::Run()
{
	assert(callback != 0);
	assert(lpChain != 0);
	assert(oest != 0);
	
	if( !oest->CheckObjectNumber(lpChain->GetObjectNumber()) ) {
		throw CTextException("CBestPatternFirstComputationProcedure::Run", "Number of objects in the projection chain and optimistic estimator are different");
	}

	lpChain->UpdateInterestThreshold( thld );

	ILocalProjectionChain::CPatternList newPatterns;
	lpChain->ComputeZeroProjection( newPatterns );
	addNewPatterns( newPatterns );

	callback->ReportNextStage("Expansion");

	DWORD expansionCount = 0;
	// Just takes one by one the most promissing patterns and expand them
	while( !callback->IsInterrupted() && !queue.empty() && queue.begin()->Potential > best.Quality ) {
		const CPattern& p = *queue.begin();
		if( !lpChain->Preimages(p.Pattern.get(), newPatterns) ) { // The expansion of the pattern
			queue.erase(queue.begin()); // If no more expansion is possible than pattern is removed
		}
		newPatterns.Clear();
		addNewPatterns( newPatterns );
		++expansionCount;

		adjustThreshold();
		if( expansionCount % 1000 == 0 ) { // Just to report not too often
			callback->ReportProgress( expansionCount, string("Border size is") + StdExt::to_string(queue.size())
			                          + ". Quality: " + StdExt::to_string(best.Quality) + " / " + StdExt::to_string(queue.begin()->Potential)
			                          + ". Delta: " + StdExt::to_string(lpChain->GetInterestThreshold()));
		}
	}
	// Last report of the progress
	callback->ReportProgress( expansionCount, string("Border size is") + StdExt::to_string(queue.size())
								+ ". Quality: " + StdExt::to_string(best.Quality) 
	                          + ". Delta: " + StdExt::to_string(lpChain->GetInterestThreshold()) + "                                 ");
}

void CBestPatternFirstComputationProcedure::SaveResult( const std::string& basePath )
{
	callback->ReportNextStage("Producing output");
	CDestStream dst(basePath);
	dst << "[\n"
	    << "{"
			"\"NodesCount\":1,\"ArcsCount\":0,"
			"\"Params\":" << SaveParams()
	    << "},{ \"Nodes\":[\n"
	    << "{" "\"ExtSize\":" << lpChain->GetExtentSize( best.Pattern.get() ) << ",\"Ext\":" << lpChain->SaveExtent( best.Pattern.get() ) << ",\n\"Int\":" << lpChain->SaveIntent( best.Pattern.get() )
	    << ",\n\"Thld\":" << thld << ", \"Value\":" << best.Quality  << "}" 
		"]}"
		"]";
}

void CBestPatternFirstComputationProcedure::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CBestPatternFirstComputationProcedure::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == Type() );
	assert( string( params["Name"].GetString() ) == Name() );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for Local Projection Chain object";
		throw new CJsonException("CBestPatternFirstComputationProcedure::LoadParams", error);
	}

	const rapidjson::Value& p = params["Params"];

	if(!(p.HasMember("LocalProjectionChain") && (p["LocalProjectionChain"].IsObject()))) {
		error.Data = json;
		error.Error = "Params.LocalProjectionChain is not found or is not an object.";
		throw new CJsonException("CBestPatternFirstComputationProcedure::LoadParams", error);
	}
	const rapidjson::Value& cp = params["Params"]["LocalProjectionChain"];
	string errorText;
	lpChain.reset( CreateModuleFromJSON<ILocalProjectionChain>(cp,errorText) );
	if( lpChain == 0 ) {
		throw new CJsonException( "CBestPatternFirstComputationProcedure::LoadParams", CJsonError( json, errorText ) );
	}
	if( p.HasMember( "DefualtThld" )) {
		const rapidjson::Value& thldJson = params["Params"]["DefualtThld"];
		if( thldJson.IsNumber() ) {
			thld = thldJson.GetDouble();
		}
	}
	if( p.HasMember( "MaxPatternNumber" ) ) {
		const rapidjson::Value& mpnJson = params["Params"]["MaxPatternNumber"];
		if( mpnJson.IsUint() ) {
			mpn = mpnJson.GetUint();
		}
	}

	if( p.HasMember("AdjustThreshold")) {
		const rapidjson::Value& atJson = params["Params"]["AdjustThreshold"];
		if( atJson.IsBool() ) {
			shouldAdjustThld = atJson.GetBool();
		}
	}

	if( !p.HasMember("OptimisticEstimator") || !p["OptimisticEstimator"].IsObject()) {
		error.Data = json;
		error.Error = "Params.OptimisticEstimator is not found or is not an object.";
		throw new CJsonException("CBestPatternFirstComputationProcedure::LoadParams", error);
	} else {
		const rapidjson::Value& pc = params["Params"]["OptimisticEstimator"];
		string errorText;
		oest.reset( CreateModuleFromJSON<IOptimisticEstimator>(pc,errorText) );	
		if( oest == 0 ) {
			throw new CJsonException( "CSofiaContextProcessor::LoadParams", CJsonError( json, errorText ) );
		}
	}
	if( p.HasMember( "OEstMinQuality" )) {
		const rapidjson::Value& minQ = params["Params"]["OEstMinQuality"];
		if( minQ.IsNumber() ) {
			best.Quality = minQ.GetDouble();
			isBestQualityKnown = true;
		}
	}
}

JSON CBestPatternFirstComputationProcedure::SaveParams() const
{
	// TODO
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", rapidjson::StringRef(Type()), alloc )
		.AddMember( "Name", rapidjson::StringRef(Name()), alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "MaxObjectNumber", rapidjson::Value().SetInt( mpn ), alloc ),
		alloc );

	IModule* m = dynamic_cast<IModule*>(lpChain.get());

	JSON cpParams;
	rapidjson::Document cpParamsDoc;
    assert( m!=0);
	if( m != 0 ) {
		cpParams = m->SaveParams();
		CJsonError error;
		const bool rslt = ReadJsonString( cpParams, cpParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("LocalProjectionChain", cpParamsDoc.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

// Add new patterns to the queue and to the best found pattern. Removes unecessary patterns
void CBestPatternFirstComputationProcedure::addNewPatterns( const ILocalProjectionChain::CPatternList& newPatterns )
{
	assert(oest != 0);
	
	auto itr = newPatterns.Begin();
	if( itr == newPatterns.End()) {
		return;
	}
	if( !isBestQualityKnown ) {
		const IExtent* ext = dynamic_cast<const IExtent*>(*itr);
		if( ext == 0) {
			throw new CTextException("CBestPatternFirstComputationProcedure::computeOEstimate",
									 "Cannot extract extent from pattern. Local projection chain does not support it.");
		}

		best.Quality = oest->GetValue( ext );
		best.Pattern.reset(*itr, deleter);
		CPattern p;
		p.Pattern = best.Pattern;
		p.Potential = oest->GetBestSubsetEstimate( ext );
		queue.insert(p);
		++itr;
	}
	for( ;itr != newPatterns.End(); ) {
		auto currItr = itr;
		++itr;
		const IExtent* ext = dynamic_cast<const IExtent*>(*currItr);
		if( ext == 0) {
			throw new CTextException("CBestPatternFirstComputationProcedure::computeOEstimate",
									 "Cannot extract extent from pattern. Local projection chain does not support it.");
		}

		CPattern p;
		p.Pattern.reset(*currItr, deleter);
		p.Potential = oest->GetBestSubsetEstimate( ext );
		if( oest->GetBestSubsetEstimate(ext) <= best.Quality ) {
			continue;
		}
		queue.insert(p);
	}
}

// Adjusting threshold in order to maintain the limitted number of pattern candidates
void CBestPatternFirstComputationProcedure::adjustThreshold()
{
	auto itr = queue.lower_bound(CPattern(best.Quality));
	if( itr != queue.end() ) {
		queue.erase(itr, queue.end());
	}

	if( !shouldAdjustThld || queue.size() < mpn * 2 ) {
		return;
	}

	// Should remove patterns such that there are at most @var mpn patterns.
	vector<double> interests;
	interests.reserve(queue.size());
	for(auto itr = queue.begin(); itr != queue.end(); ++itr) {
		interests.push_back(lpChain->GetPatternInterest(itr->Pattern.get()));
	}

	sort(interests.begin(),interests.end(),std::greater<double>() );

	thld = interests[mpn+1]+0.001; // A constant that can be bad for some interests
	for(auto itr = queue.begin(); itr != queue.end();) {
		auto currItr = itr;
		++itr;

		if(lpChain->GetPatternInterest(currItr->Pattern.get()) >= thld) {
			continue;
		}
		queue.erase(currItr);
	}
}

///////////////////////////////////////////////////////////////////

// compares patterns first patterns with larger potential are first (true), second patterns are given in topolgical order
bool CBestPatternFirstComputationProcedure::CPatternPotentialComparator::operator()(const CPattern& a, const CPattern& b)
{
	assert(lpChain != 0);

	// Difference is evident from potential
	if( a.Potential - b.Potential > 1e-10 ) {
		return true;
	}

	// Special case if a pattern is zero
	if(a.Pattern == 0) {
		return true;
	} else if(b.Pattern == 0) {
		return false;
	}

	return lpChain->IsTopoSmaller(a.Pattern.get(), b.Pattern.get());
}

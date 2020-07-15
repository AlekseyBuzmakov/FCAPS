// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8

#include <fcaps/ComputationProcedureModules/BestPatternFirstComputationProcedure.h>

#include <fcaps/OptimisticEstimator.h>
#include <fcaps/PatternDescriptor.h>
#include <fcaps/Swappable.h>

#include <JSONTools.h>
#include <ModuleJSONTools.h>
#include <StdTools.h>

#include <boost/math/special_functions/round.hpp>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(...) #__VA_ARGS__

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
					"type":"integer",
					"minimum":0,
					"exclusiveMinimum":true
				},
				"MaxRAMConsumption" :{
					"description": "The maximal amount of RAM to be used to store the patterns. The number is approximate.",
					"type":"integer",
					"minimum":0,
					"exclusiveMinimum":true
				},
				"AdjustThreshold":{
					"description": "A flag indicating if the threshold should be adjusted in order to ensure polynomiality and memory finity",
					"type":"boolean"
				},
				"OptimisticEstimator":{
					"description": "The object that defines the optimistic estimator for the goal function",
					"type": "@OptimisticEstimatorModules"
				},
				"OEstMinQuality":{
					"description": "The minimal quality of the pattern that is considederd interesting to be found",
					"type": "number"
				},
				"BreakOnFirstSD":{
				"description": "Stop the computation when the first subgroup with sufficient quality is found",
					"type":"boolean"
				},
				"ComputeForAllThlds":{
					"description": "If false, then the computation stops when it understand that for the current threshold the best pattern is known. Otherwise it computes for all thlds, starting from the first one.",
					"type":"boolean"
				},
				"BeamsNumber":{
					"description": "The number of beams to be used for expansion of a concept from the queue",
					"type": "integer",
					"minimum": 1
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
	maxRAMConsumption(-1),
	shouldAdjustThld(false),
	shouldBreakOnFirst(false),
	shouldComputeForAllThlds(false),
	beamsNum(1),
	potentialCmp(lpChain),
	queue( potentialCmp ),
	arePatternsSwappable(-1),
	numInMemoryPatterns(1000),
	conceptPreimagesCount(0)
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
		throw new CTextException("CBestPatternFirstComputationProcedure::Run", "Number of objects in the projection chain and optimistic estimator are different");
	}

	lpChain->UpdateInterestThreshold( thld );

	ILocalProjectionChain::CPatternList newPatterns;
	lpChain->ComputeZeroProjection( newPatterns );
	addNewPatterns( newPatterns, queue );

	callback->ReportNextStage("Expansion");

	// Just takes one by one the most promissing patterns and expand them
	while( !callback->IsInterrupted() // requested by the user
	       && !queue.empty() // nothing to process
	       // Searching for just one patern mean that for the found stability
	       // we cannot improve the found quality, then only front is reported
	       // TODO: if we want to report all correspondnce of stability and quality,
	       //   then we should continue
	       && (shouldComputeForAllThlds || queue.begin()->Potential > bestMap.GetFrontQuality() ) )
	{
		auto beginItr = queue.begin();
		CPattern p = *beginItr;
		queue.erase(beginItr);

		startBeamSearch(p);

		// newPatterns.Clear();
		// // The expansion of the pattern
		// const ILocalProjectionChain::TPreimageResult res = lpChain->Preimages(p.Pattern.get(), newPatterns);
		// addNewPatterns( newPatterns, queue );
		
		// if( res == ILocalProjectionChain::PR_Finished ) {
		// 	checkForBestConcept(p); // The expansion is finished and the concept is stable, should check for best quality
		// }
		// if( res != ILocalProjectionChain::PR_Expandable ) {
		// 	queue.erase(beginItr); // If no more expansion is possible than pattern is removed
		// }


		if( queue.size() == 0 || shouldBreakOnFirst && bestMap.HasValues() ) {
			break;
		}

		adjustThreshold();
	}
	// Last report of the progress
	callback->ReportProgress( conceptPreimagesCount, string("Border size is ") + StdExt::to_string(queue.size())
								+ ". Quality: " + StdExt::to_string(bestMap.GetFrontQuality()) 
	                          + ". Delta: " + StdExt::to_string(lpChain->GetInterestThreshold()) + "                                 ");
}

void CBestPatternFirstComputationProcedure::SaveResult( const std::string& basePath )
{
	callback->ReportNextStage("Producing output");

	CDestStream dst(basePath);
	if( !bestMap.HasValues() ) {
		// No pattern is found w.r.t. the input
		dst << "[\n"
			<< "{"
				"\"NodesCount\":0,\"ArcsCount\":0,"
				"\"Params\":" << SaveParams()
			<< "},{ \"Nodes\":[\n"
			"]}"
			"]";
	} else {
		dst << "[\n"
			<< "{"
				"\"NodesCount\":1,\"ArcsCount\":0,"
				"\"Params\":" << SaveParams()
		    << "},{ \"Nodes\":[\n";

		bool shouldAddComma = false;
		for( auto itr = bestMap.Begin(); itr != bestMap.End();++itr ) {
			if( shouldAddComma) {
				dst << ",";
			}
			shouldAddComma = true;
			const CBestPattern& best = itr->second;
			dst	<< "{" "\"ExtSize\":" << lpChain->GetExtentSize( best.Pattern.get() )
				<< ",\n\"Ext\":" << lpChain->SaveExtent( best.Pattern.get() )
				<< ",\n\"Int\":" << lpChain->SaveIntent( best.Pattern.get() )
				<< ",\n\"Thld\":" << thld << ", \"Value\":" << best.Quality << ", \"Interest\":" << itr->first
				<< ",\n\"Quality\":" << oest->GetJsonQuality( dynamic_cast<const IExtent*>(best.Pattern.get()) )
				<< "}\n" ;
		}

		dst << "]}"
			"]";
	}
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
	if( p.HasMember( "MaxRAMConsumption" ) ) {
	   	const rapidjson::Value& mrcJson = params["Params"]["MaxRAMConsumption"];
		if( mrcJson.IsUint64() ) {
			maxRAMConsumption = mrcJson.GetUint64();
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
			bestMap.SetMinAcceptableQuality(minQ.GetDouble());
		}
	}
	if( p.HasMember("BreakOnFirstSD")) {
		const rapidjson::Value& atJson = params["Params"]["BreakOnFirstSD"];
		if( atJson.IsBool() ) {
			shouldBreakOnFirst = atJson.GetBool();
		}
	}
	if( p.HasMember("ComputeForAllThlds")) {
		const rapidjson::Value& atJson = params["Params"]["ComputeForAllThlds"];
		if( atJson.IsBool() ) {
			shouldComputeForAllThlds = atJson.GetBool();
		}
	}
	if( p.HasMember( "BeamsNumber" )) {
		const rapidjson::Value& beamsNumVal = params["Params"]["OEstMinQuality"];
		if( beamsNumVal.IsUint() ) {
			beamsNum = max(1,beamsNumVal.GetInt());
		}
	}

	maxRAMConsumption = max( maxRAMConsumption, 2 * lpChain->GetTotalConsumedMemory());
}

JSON CBestPatternFirstComputationProcedure::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", rapidjson::StringRef(Type()), alloc )
		.AddMember( "Name", rapidjson::StringRef(Name()), alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "DefualtThld", rapidjson::Value().SetDouble( thld ), alloc )
			.AddMember( "MaxObjectNumber", rapidjson::Value().SetInt( mpn ), alloc )
			.AddMember( "MaxRAMConsumption", rapidjson::Value().SetUint64( maxRAMConsumption ), alloc )
			.AddMember( "AdjustThreshold", rapidjson::Value().SetBool( shouldAdjustThld ), alloc )
			.AddMember( "BeamsNum", rapidjson::Value().SetUint( beamsNum ), alloc )
			.AddMember( "OEstMinQuality", rapidjson::Value().SetDouble( bestMap.GetFrontQuality() ), alloc ),
		alloc );

	JSON cpParams;
	rapidjson::Document cpParamsDoc;

	IModule* m = dynamic_cast<IModule*>(lpChain.get());
    assert( m!=0);
	if( m != 0 ) {
		cpParams = m->SaveParams();
		CJsonError error;
		const bool rslt = ReadJsonString( cpParams, cpParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("LocalProjectionChain", cpParamsDoc.Move(), alloc );
	}

	m = dynamic_cast<IModule*>(oest.get());
    assert( m!=0);
	if( m != 0 ) {
		cpParams = m->SaveParams();
		CJsonError error;
		const bool rslt = ReadJsonString( cpParams, cpParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("OptimisticEstimator", cpParamsDoc.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

// Converts the original format of the pattern to CPattern
void CBestPatternFirstComputationProcedure::convertPattern(const IPatternDescriptor* d, CPattern& p) const
{
	assert(oest != 0);

	const IExtent* ext = dynamic_cast<const IExtent*>(d);
	if( ext == 0) {
		throw new CTextException("CBestPatternFirstComputationProcedure::convertPattern",
									"Cannot extract extent from pattern. Local projection chain does not support it.");
	}

	IOptimisticEstimator::COEstValue val;
	oest->GetValue(ext, val);

	p.Pattern.reset(d, deleter);
	p.Potential = val.BestSubsetEstimate;
	p.Quality = val.Value;
	assert( p.Potential >= p.Quality );
}

// Adding a pattern to the queue
void CBestPatternFirstComputationProcedure::addPatternToQueue(const CPattern& p,TQueue& queue)
{
	checkForBestConcept(p);

	if( p.Potential <= max(
			// We are not interested at all in smaller quality
			bestMap.GetMinAcceptableQuality(),
			// For the given interest (or better) the already found pattern is of better quality
			bestMap.GetQuality(lpChain->GetPatternInterest(p.Pattern.get()))) )
	{
		return;
	}

	if( lpChain->IsExpandable(p.Pattern.get()) ) {
		queue.insert(p);
	}
	
}

// Add new patterns to the queue and to the best found pattern. Removes unecessary patterns
void CBestPatternFirstComputationProcedure::addNewPatterns( const ILocalProjectionChain::CPatternList& newPatterns, TQueue& queue )
{
	assert(oest != 0);
	assert(lpChain != 0);
	
	auto itr = newPatterns.Begin();
	if( itr == newPatterns.End()) {
		return;
	}

	for( ;itr != newPatterns.End(); ) {
		auto currItr = itr;
		++itr;

		CPattern p;
		convertPattern(*currItr,p);

		addPatternToQueue(p,queue);
	}
}

// Starts a beams search for the most interesting pattern at p
void CBestPatternFirstComputationProcedure::startBeamSearch(const CPattern& p)
{
	TQueue bsQueues[2] = {TQueue(potentialCmp), TQueue(potentialCmp)};
	addPatternToQueue(p, bsQueues[0]);
	int q = 0; // index of the currently expanded queue
	ILocalProjectionChain::CPatternList newPatterns;

	while(!bsQueues[q].empty()) {
		// Iterating over all elements expanding them and adding queues to the other queue
		auto itr = bsQueues[q].begin();
		for(; itr != bsQueues[q].end(); ++itr ) {
			newPatterns.Clear();

			// cerr << endl << "===============================================" << endl;
			// cerr << lpChain->SaveIntent(itr->Pattern.get()) << endl;
			// cerr << "\nQuality: " << itr->Quality << " Potential:" << itr->Potential << endl ;

			// The expansion of the pattern
			const ILocalProjectionChain::TPreimageResult res = lpChain->Preimages(itr->Pattern.get(), newPatterns);
			conceptPreimagesCount += newPatterns.Size();

			addNewPatterns( newPatterns, bsQueues[1-q] );

			// cerr << "RES:" << res << " Stab: " << lpChain->GetPatternInterest(itr->Pattern.get()) << endl;

			if( res != ILocalProjectionChain::PR_Uninteresting) {
				// It will check if it is expandable and register it either for expnasion or as the best pattern
				addPatternToQueue(*itr, bsQueues[1-q]);
			}
			// if( res == ILocalProjectionChain::PR_Finished ) {
			// 	checkForBestConcept(*itr); // The expansion is finished and the concept is stable, should check for best quality
			// }
			// if( res == ILocalProjectionChain::PR_Expandable ) {
			// 	bsQueues[1-q].insert(*itr);
			// }
		}
		// Switching queues
		bsQueues[q].clear();
		q = 1-q;
		// Only beamsNum numbers of elements is preserved
		int passedItems = 0;
		itr = bsQueues[q].begin();
		while(itr != bsQueues[q].end()) {
			auto curItr = itr;

			if( p.Potential < curItr->Potential -  abs(curItr->Potential) * 1e-6 || curItr->Potential < curItr->Quality) {
				cerr << endl << "OEst is WRONG!" << endl;
				cerr << "P Potential: " << p.Potential << " Ch Potential: " << curItr->Potential << " Ch Quality: " << curItr->Quality << endl;
				cerr << lpChain->SaveIntent(p.Pattern.get()) << endl;
				cerr << oest->GetJsonQuality( dynamic_cast<const IExtent*>(p.Pattern.get()) ) << endl;
				cerr << lpChain->SaveIntent(curItr->Pattern.get()) << endl;
				cerr << oest->GetJsonQuality( dynamic_cast<const IExtent*>(curItr->Pattern.get()) ) << endl;
				assert(p.Potential >= curItr->Potential);
				assert(p.Potential >= curItr->Quality);
			}
			++passedItems;
			++itr;
			if( passedItems <= beamsNum ) {
				continue;
			}

			CPattern p = *curItr;
			queue.insert(p);

			bsQueues[q].erase(curItr);
		}

		string progress = string("Border: ") + StdExt::to_string(queue.size());
		if( queue.size() > 0) {
			progress += ". Quality: " + StdExt::to_string(bestMap.GetFrontQuality()) + " / ["
				+ StdExt::to_string(queue.rbegin()->Potential) + "; " + StdExt::to_string(queue.begin()->Potential) + "]";
		}
		progress += ". Delta: " + StdExt::to_string(lpChain->GetInterestThreshold())
			+ ". Memory: " + StdExt::to_string(boost::math::round(lpChain->GetTotalConsumedMemory() / (1024.0*1024))) + "Mb.   ";
		callback->ReportProgress( conceptPreimagesCount, progress );
	}
}
// If pattern is finished checks its quality and update the best concept
void CBestPatternFirstComputationProcedure::checkForBestConcept(const CPattern& p)
{
	assert(lpChain != 0);

	if( lpChain->IsExpandable(p.Pattern.get()) ) {
		return; // Cannot yet take its quality
	}
	
	const double& interest = lpChain->GetPatternInterest(p.Pattern.get()); // since the pattern is not expandable, it is the final interest
	const bool res = bestMap.Insert(interest,CBestPattern(p));

	// if( arePatternsSwappable == -1 ) {
	// 	arePatternsSwappable  = ( dynamic_cast<const ISwappable*>(queue.begin()->Pattern.get()) != 0 ? 1 : 0);
	// }

	// const ISwappable* swp = dynamic_cast<const ISwappable*>(p.Pattern.get());
	// assert(swp != 0);
	// swp->Swap();
}

// Adjusting threshold in order to maintain the limitted number of pattern candidates
void CBestPatternFirstComputationProcedure::adjustThreshold()
{
	////Cannot be done, since when adjusting the treshold currently stable pattern can became unstable
	// auto itr = queue.lower_bound(CPattern(best.Quality));
	// if( itr != queue.end() ) {
	// 	queue.erase(itr, queue.end());
	// }

	// Nothing is wory about...
	if( (!shouldAdjustThld || queue.size() < mpn * 2) && lpChain->GetTotalConsumedMemory() < maxRAMConsumption  ) {
		return;
	}

	for(auto itr = queue.begin(); itr != queue.end(); ) {
		auto currItr = itr;
		++itr;
		if( currItr->Potential <= max(
				// We are not interested at all in smaller quality
				bestMap.GetMinAcceptableQuality(),
				// For the given interest (or better) the already found pattern is of better quality
				bestMap.GetQuality(lpChain->GetPatternInterest(currItr->Pattern.get()))) )
		{
			queue.erase(currItr);
		}
	}

	if( arePatternsSwappable == -1 ) {
		arePatternsSwappable  = ( dynamic_cast<const ISwappable*>(queue.begin()->Pattern.get()) != 0 ? 1 : 0);
	}

	if( arePatternsSwappable == 1 && lpChain->GetTotalConsumedMemory() >= maxRAMConsumption ) {
		// Will try to swap pattrns to disk
		int nPatterns = 0;
		auto itr = queue.begin();
		for( ; itr != queue.end(); ++itr, ++nPatterns) {
			if( nPatterns < numInMemoryPatterns ) {
				continue;
			}
			const ISwappable* swp = dynamic_cast<const ISwappable*>(itr->Pattern.get());
			assert(swp != 0);
			swp->Swap();
		}
		// If the memory is freed, then we will try to remove patterns
	}

	// Yes, now there is nothing to worry about
	if( !shouldAdjustThld || (queue.size() < mpn * 2 && lpChain->GetTotalConsumedMemory() < maxRAMConsumption )) {
		return;
	}

	const DWORD firstPatternToRemove = min<DWORD>(mpn + 1, boost::math::round<DWORD>(queue.size() * 1.0 * maxRAMConsumption / lpChain->GetTotalConsumedMemory()) );
	// Should remove patterns such that there are at most @var mpn patterns.
	vector<double> interests;
	interests.reserve(queue.size());
	for(auto itr = queue.begin(); itr != queue.end(); ++itr) {
		interests.push_back(lpChain->GetPatternInterest(itr->Pattern.get()));
	}

	sort(interests.begin(),interests.end(),std::greater<double>() );

	assert(0 <= firstPatternToRemove && firstPatternToRemove < interests.size());

	thld = interests[firstPatternToRemove]+0.001; // A constant that can be bad for some interests
	for(auto itr = queue.begin(); itr != queue.end();) {
		auto currItr = itr;
		++itr;

		if(lpChain->GetPatternInterest(currItr->Pattern.get()) >= thld) {
			continue;
		}
		queue.erase(currItr);
	}
	lpChain->UpdateInterestThreshold( thld );
	bestMap.SetMinKey(thld);
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
	if( a.Potential - b.Potential < -1e-10 ) {
		return false;
	}

	// Special case if a pattern is zero
	if(a.Pattern == 0) {
		return true;
	} else if(b.Pattern == 0) {
		return false;
	}

	// return a.Potential - b.Potential > 0;
	return lpChain->IsTopoSmaller(a.Pattern.get(), b.Pattern.get());
}

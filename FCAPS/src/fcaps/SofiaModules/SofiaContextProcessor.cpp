// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/SofiaModules/SofiaContextProcessor.h>

#include <fcaps/ComputationProcedure.h>
#include <fcaps/OptimisticEstimator.h>

#include <fcaps/storages/CachedIntentStorage.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>

#include <fcaps/SharedModulesLib/FindConceptOrder.h>

#include <rapidjson/document.h>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(x...) #x

const char description[] =
STR(
	{
	"Name":"Algorithm SOFIA for finding DELTA-stable patterns",
	"Description":"An algorithm that finds DELTA-stable patterns by passing from smaller projections of the dataset to larger ones. Algorithm is also able to automatically adjust the threshold of DELTA-measure in order to limit the requiered memory and have polynomial computational complexity. Optimistic estimators can also be used with SOFIA. In this case only the best found is reported.",
	"Params": {
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of SOFIA Context processor",
			"type": "object",
			"properties": {
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
				"FindPartialOrder":{
					"description": "A flag indicating if the partial order of patterns should be found",
					"type":"boolean"
				},
				"AdjustThreshold":{
					"description": "A flag indicating if the threshold should be adjusted in orderder to ensure polynomiality and memory finity",
					"type":"boolean"
				},
				"OutputParams":{
					"description": "The set of parameters controlling what should be printed out as the result",
					"type": "object",
					"properties": {
						"OutExtent":{
							"description": "A flag indicating if the extent should be printed or (if false) only the support is reported",
							"type":"boolean"
						},
						"OutIntent":{
							"description": "A flag indicating if the intent should be reported",
							"type":"boolean"
						}
					}
				},
				"OptimisticEstimator":{
					"description": "The object that defines the optimistic estimator for the goal function",
					"type": "@OptimisticEstimatorModules"
				},
				"OEstMinQuality":{
					"description": "The minimal quality of the pattern that is considederd interesting to be found",
					"type": "number"
				},
				"ProjectionChain":{
					"description": "The object that defines the projection chain that is related to both the patterns and the measure to compute.",
					"type": "@ProjectionChainModules"
				}
			}
		}
	}
);

////////////////////////////////////////////////////////////////////

CModuleRegistrar<CSofiaContextProcessor> CSofiaContextProcessor::registrar;
const char* const CSofiaContextProcessor::Desc()
{
	return description;
}

////////////////////////////////////////////////////////////////////

CSofiaContextProcessor::CSofiaContextProcessor() :
	callback(0),
	thld( 0 ),
	mpn( 1000 ),
	shouldFindPartialOrder(false),
	shouldAdjustThld(true),
	objectNumber(0),
	minPotential(1),
	maxPotential(-1)
{
	storage.Reserve( mpn );
}

void CSofiaContextProcessor::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CSofiaContextProcessor::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == ContextProcessorModuleType );
	assert( string( params["Name"].GetString() ) == SofiaContextProcessor );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for ProjectionChain";
		throw new CJsonException("CSofiaContextProcessor::LoadParams", error);
	}

	const rapidjson::Value& p = params["Params"];
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
	if( p.HasMember( "FindPartialOrder" ) ) {
		const rapidjson::Value& poJson = params["Params"]["FindPartialOrder"];
		if( poJson.IsBool() ) {
			shouldFindPartialOrder = poJson.GetBool();
		}
	}

	if( p.HasMember("AdjustThreshold")) {
		const rapidjson::Value& atJson = params["Params"]["AdjustThreshold"];
		if( atJson.IsBool() ) {
			shouldAdjustThld = atJson.GetBool();
		}
	}
	if( p.HasMember("OutputParams") && p["OutputParams"].IsObject() ) {
		const rapidjson::Value& outJson = p["OutputParams"];
		if( outJson.HasMember("OutExtent") && outJson["OutExtent"].IsBool() ) {
			outParams.OutExtent = outJson["OutExtent"].GetBool();
		}
		if( outJson.HasMember("OutIntent") && outJson["OutIntent"].IsBool() ) {
			outParams.OutIntent = outJson["OutIntent"].GetBool();
		}
	}

	if( p.HasMember("OptimisticEstimator") && p["OptimisticEstimator"].IsObject()) {
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
			bestPattern.Q = minQ.GetDouble();
		}
	}

	if( !p.HasMember("ProjectionChain") ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for ProjectionChain";
		throw new CJsonException("CSofiaContextProcessor::LoadParams", error);
	}

	const rapidjson::Value& pc = params["Params"]["ProjectionChain"];
	string errorText;
	pChain.reset( CreateModuleFromJSON<IProjectionChain>(pc,errorText) );
	if( pChain == 0 ) {
		throw new CJsonException( "CSofiaContextProcessor::LoadParams", CJsonError( json, errorText ) );
	}
	storage.Initialize(CHasher(pChain));
	storage.Reserve( mpn );
}
JSON CSofiaContextProcessor::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", ContextProcessorModuleType, alloc )
		.AddMember( "Name", SofiaContextProcessor, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "Thld", rapidjson::Value().SetDouble( thld ), alloc )
			.AddMember( "MaxPatternNumber", rapidjson::Value().SetUint( mpn ), alloc )
			.AddMember( "AdjustThreshold", rapidjson::Value().SetBool( shouldAdjustThld ), alloc )
			.AddMember( "FindPartialOrder", rapidjson::Value().SetBool( shouldFindPartialOrder ), alloc )
			.AddMember( "OutputParams", rapidjson::Value().SetObject()
				.AddMember( "OutExtent",rapidjson::Value().SetBool(outParams.OutExtent), alloc)
				.AddMember( "OutIntent",rapidjson::Value().SetBool(outParams.OutIntent), alloc),
			alloc ),
		alloc );
	IModule* m = dynamic_cast<IModule*>(pChain.get());

	JSON pChainParams;
	rapidjson::Document pChainParamsDoc;
    assert( m!=0);
	if( m != 0 ) {
		pChainParams = m->SaveParams();
		CJsonError error;
		const bool rslt = ReadJsonString( pChainParams, pChainParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("ProjectionChain", pChainParamsDoc.Move(), alloc );
	}

	m = dynamic_cast<IModule*>(oest.get());
	JSON oestParams;
	rapidjson::Document oestParamsDoc;
	if( m != 0 ) {
		oestParams = m->SaveParams();
		CJsonError error;
		const bool rslt = ReadJsonString( oestParams, oestParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("OptimisticEstimator", oestParamsDoc.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

const std::vector<std::string>& CSofiaContextProcessor::GetObjNames() const
{
	assert(pChain != 0);
	return pChain->GetObjNames();
}
void CSofiaContextProcessor::SetObjNames( const std::vector<std::string>& names )
{
	assert(pChain != 0);
	pChain->SetObjNames( names );
}

void CSofiaContextProcessor::PassDescriptionParams( const JSON& json )
{
	assert( pChain != 0 );
	IModule& pChainModule = dynamic_cast<IModule&>(*pChain);
	const JSON params = string("{") +
		"\"Type\":\"" + pChainModule.GetType() + "\","
		"\"Name\":\"" + pChainModule.GetName() + "\","
		"\"Params\":" + json +
		"}";
	pChainModule.LoadParams( params );
}

void CSofiaContextProcessor::AddObject( DWORD objectNum, const JSON& intent )
{
	assert(pChain != 0);
	pChain->AddObject(objectNum, intent);
	++objectNumber;
}

void CSofiaContextProcessor::ProcessAllObjectsAddition()
{
	assert( pChain != 0 );
	pChain->UpdateInterestThreshold( thld );
	assert(minPotential == 1 && maxPotential == -1);

	if(oest != 0 && !oest->CheckObjectNumber(objectNumber)) {
		throw new CTextException("CSofiaContextprocessor::ProcessAllObjectsAddition", "Optimistic estimator is based on different number of objects that are found in the data");
	}

	IProjectionChain::CPatternList newPatterns;
	pChain->ComputeZeroProjection( newPatterns );
	addNewPatterns( newPatterns );

	while( pChain->NextProjection() ) {
		newPatterns.Clear();
		auto itr = projectionPatterns.Begin();
		while( itr != projectionPatterns.End()) {
			auto currItr = itr;
			++itr;

			pChain->Preimages( *currItr, newPatterns );
			if( pChain->GetPatternInterest( *currItr ) < thld ) {
				// Normally the iterator should not be invalidated here
				// Sure?
				projectionPatterns.Erase(currItr);
				removeProjectionPattern(*currItr);
			}
		}
		addNewPatterns( newPatterns );
		if(minPotential < bestPattern.Q) {
			// there are some inpotential patterns
			removeInpotentialPatterns();
		}
		adjustThreshold();
		reportProgress();
	}
	reportProgress();
}

class CSofiaContextProcessor::CConceptsForOrder {
public:
	CConceptsForOrder( const IProjectionChain& _chain, const vector<CPatternMeasurePair>& _concepts ) :
		cmp( _chain ), concepts( _concepts ) {}

	// Methods of TConcepts requiring by CFindConceptOrder
	DWORD Size() const
		{ return concepts.size(); }
	bool IsTopologicallyLess( DWORD c1, DWORD c2 ) const
	{
		assert( c1 < concepts.size() );
		assert( c2 < concepts.size() );
		return cmp.IsTopoSmaller(concepts[c1].first, concepts[c2].first);
	}
	bool IsLess( DWORD c1, DWORD c2 ) const
	{
		assert( c1 < concepts.size() );
		assert( c2 < concepts.size() );
		const bool res =
			cmp.IsSmaller( concepts[c1].first, concepts[c2].first);
		return res;
	}
private:
	const IProjectionChain& cmp;
	const vector<CPatternMeasurePair>& concepts;
};
template<typename T>
static inline bool compareCachedPatterns(const T& p1, const T& p2)
{
	return p1.second > p2.second;
}
void CSofiaContextProcessor::SaveResult( const std::string& path )
{
	// TODO save best pattern if available
	vector<CPatternMeasurePair> concepts;
	if( oest == 0 ) {
		concepts.reserve(projectionPatterns.Size());
		auto pItr = projectionPatterns.Begin();
		for( ; pItr != projectionPatterns.End(); ++pItr ) {
			const IPatternDescriptor* p = *pItr;
			concepts.push_back( CPatternMeasurePair( p, pChain->GetPatternInterest(p) ) );
		}
		sort( concepts.begin(), concepts.end(), compareCachedPatterns<CPatternMeasurePair> );
		assert( concepts.size() == 0 || concepts.back().second >= thld );

		CConceptsForOrder conceptsForOrder( *pChain, concepts );
		CFindConceptOrder<CConceptsForOrder> conceptOrderFinder( conceptsForOrder );
		if( shouldFindPartialOrder ) {
			conceptOrderFinder.Compute();
		}

		saveToFile( concepts, conceptOrderFinder, path );
	} else {
		if(bestPattern.Pattern != 0) {
			concepts.push_back( CPatternMeasurePair( bestPattern.Pattern, pChain->GetPatternInterest(bestPattern.Pattern) ) );
		} else {
			callback->ReportProgress( 1, "Nothing is found" );
		}
		CConceptsForOrder conceptsForOrder( *pChain, concepts );
		CFindConceptOrder<CConceptsForOrder> conceptOrderFinder( conceptsForOrder );
		saveToFile( concepts, conceptOrderFinder, path );
	}
}

// Adds new patterns to
void CSofiaContextProcessor::addNewPatterns( const IProjectionChain::CPatternList& newPatterns )
{
	CStdIterator<IProjectionChain::CPatternList::CConstIterator, false> itr(newPatterns);
	DWORD i = 0;
	for( ; !itr.IsEnd(); ++itr, ++i ) {
        const IPatternDescriptor* origP = *itr;
        const IPatternDescriptor* p = storage.AddPattern( origP ); // Patterns are stored here
		assert( p!=0 );
		if(p==origP) {
			projectionPatterns.PushBack(p);
		}
#ifdef _DEBUG
		if( !(pChain->GetPatternInterest(p) - thld >= -0.00001*thld) ) {
			std::cerr << "M(origP)" <<  pChain->GetPatternInterest(origP) << " M(p)=" <<  pChain->GetPatternInterest(p) << " Thld=" << thld << endl;
			assert(false);
		}
#endif
		// Computing Optimistic estimates
		if( p == origP && oest != 0) {
			COEstQuality q;
			computeOEstimate(p,q);

			if(minPotential == 1 && maxPotential == -1) {
				// It is starting point so it is normal. Just initialization.
				minPotential = q.OEstPotential;
				maxPotential = q.OEstPotential;
				if(bestPattern.Q == 0) {
					// To be sure that this quality is worse
					bestPattern.Q = q.OEstMeasure - 1;
				}
			} else {
				// Should check if the new pattern is better than the existing ones
				assert(minPotential <= maxPotential);
				minPotential = min(minPotential,q.OEstPotential);
				maxPotential = max(maxPotential,q.OEstPotential);
			}
			if(q.OEstPotential <= bestPattern.Q) {
				// The potential of the pattern is less then already found pattern
				//  no sense to store it neither to expand it
				projectionPatterns.PopBack();
				removeProjectionPattern(p);
			} else {
				// The pattern should be preserved
				oestQuality.insert(std::pair<const IPatternDescriptor*,COEstQuality>(p,q));
			}
			if(q.OEstMeasure > bestPattern.Q) {
				removeBestPattern();
				// Should change the best pattern
				bestPattern.Q = q.OEstMeasure;
				bestPattern.Pattern = p;
				bestPattern.IsProjectionPattern = true;
			}
		}
	}
}
// Computes OEstimate for pattern @param p
void CSofiaContextProcessor::computeOEstimate(const IPatternDescriptor* p, COEstQuality& q)
{
	assert(p != 0);
	assert(oest != 0);
	const IExtent* ext = dynamic_cast<const IExtent*>(p);
	if( ext == 0) {
		throw new CTextException("CSofiaContextProcessor::computeOEstimate",
		                         "Cannot extract extent from pattern. Projection chain does not support it.");
	}
	q.OEstMeasure = oest->GetValue(ext);
	q.OEstPotential = oest->GetBestSubsetEstimate(ext);
}
// Removes projection pattern from storage if it is removed from potentially best pattern
void CSofiaContextProcessor::removeProjectionPattern(const IPatternDescriptor* p)
{
	assert(p != 0);
	if(bestPattern.Pattern == p) {
		assert(bestPattern.IsProjectionPattern);
		bestPattern.IsProjectionPattern = false;
	} else {
		// not best pattern, just remove
		storage.RemovePattern(p);
	}
}
// Removes best pattern from storage if it is not in projection
void CSofiaContextProcessor::removeBestPattern()
{
	if(!bestPattern.IsProjectionPattern) {
		if(bestPattern.Pattern != 0) {
			storage.RemovePattern(bestPattern.Pattern);
		}
	}
	bestPattern.Pattern = 0;
}
// Removes all patternss from chain that cannot produce interesting results
void CSofiaContextProcessor::removeInpotentialPatterns()
{
	assert(minPotential < bestPattern.Q);
	minPotential = maxPotential;
	auto itr = projectionPatterns.Begin();
	while( itr != projectionPatterns.End() ) {
		auto currItr = itr;
		++itr;
		const IPatternDescriptor* p = *currItr;
		assert(p != 0);
		auto res = oestQuality.find(p);
		assert(res != oestQuality.end());
		if(res->second.OEstPotential < bestPattern.Q) {
			// should be removed
			assert( p != bestPattern.Pattern);
			projectionPatterns.Erase(currItr);
			removeProjectionPattern(p);
		} else {
			minPotential = min(minPotential, res->second.OEstPotential);
		}
	}
}
////////////////////////////////////////////////////////////////////
// CSofiaContextProcessor::adjustThreshold stuff

typedef pair<CList<const IPatternDescriptor*>::CIterator,double> CPtrnIteratorMeasurePair;

// Adjusts threshold in order to be in memory
void CSofiaContextProcessor::adjustThreshold()
{
	assert(projectionPatterns.Size() <= storage.Size());
	assert(storage.Size() <= projectionPatterns.Size()+1);

	if( !shouldAdjustThld || projectionPatterns.Size() <= mpn ) {
		return;
	}

	vector< CPtrnIteratorMeasurePair > measures;
	measures.reserve(projectionPatterns.Size() );
	auto pItr = projectionPatterns.Begin();
	for( ; pItr != projectionPatterns.End(); ++pItr ) {
		measures.push_back( CPtrnIteratorMeasurePair( pItr, pChain->GetPatternInterest(*pItr) ) );
	}
	sort( measures.begin(), measures.end(), compareCachedPatterns<CPtrnIteratorMeasurePair> );
	assert( measures.front().second > thld );

	thld = measures[mpn].second + 0.001; // TODO: what if not integer measure?
	// Finding the final threshold.
	// Too slow?
	// What if Zero?s
	//DWORD finalSize = mpn;
	//while( abs((measures[finalSize-1].second - measures[mpn].second)/measures[mpn].second) < 0.00001 ) {
		//--finalSize;
	//}
	//thld = (measures[finalSize-1].second + measures[finalSize].second)/2;

	int i = measures.size() - 1;
	for( ; i >= 0 && measures[i].second < thld ; --i ) {
		projectionPatterns.Erase(measures[i].first);
		removeProjectionPattern(*(measures[i].first));
	}
	assert( i < mpn );
	assert( projectionPatterns.Size() <= mpn );
	pChain->UpdateInterestThreshold( thld );
}

void CSofiaContextProcessor::reportProgress() const
{
	if( callback == 0 ) {
		return;
	}
	string oestStr;
	if(oest != 0){
		oestStr = " Best: " + StdExt::to_string(bestPattern.Q)
			+ " Min: " + StdExt::to_string(minPotential)
			+ " Max: " + StdExt::to_string(maxPotential);
	}
	callback->ReportProgress( pChain->GetProgress(),
		"Concepts: " + StdExt::to_string(storage.Size())
		+ " Thld: " + StdExt::to_string(thld) + oestStr );
}

void CSofiaContextProcessor::saveToFile(
	const std::vector<CPatternMeasurePair>& concepts,
	const CFindConceptOrder<CConceptsForOrder>& order,
	const std::string& path )
{
	CDestStream dst(path);
	CList<DWORD> tops;
	vector<bool> bottoms;
	bottoms.resize( concepts.size(), true );
	DWORD arcsCount = 0;
	for( DWORD i = 0; i < concepts.size(); ++i ) {
		const CList<DWORD>& parents = order.GetParents( i );
		arcsCount += parents.Size();
		if( parents.Size() == 0 ) {
			tops.PushBack( i );
		}
		CStdIterator<CList<DWORD>::CConstIterator,false> p( parents );
		for( ; !p.IsEnd(); ++p ) {
			bottoms[*p] = false;
		}
	}

	bool isFirst = true;

	dst << "[";

	// Params of the lattice
	dst << "{\n";
		dst << "\"NodesCount\":" << concepts.size() << ",";
		dst << "\"ArcsCount\":" << arcsCount << ",";

        if( shouldFindPartialOrder ) {
            dst << "\"Top\":[";
            CStdIterator<CList<DWORD>::CConstIterator, false> top(tops);
            for( ;!top.IsEnd(); ++top ) {
                if(!isFirst) {
                    dst << ",";
                }
                isFirst = false;
                dst << *top;
            }
            dst << "],";

            dst << "\"Bottom\":[";
            isFirst=true;
            for( DWORD i = 0; i < bottoms.size(); ++i ) {
                if(!bottoms[i]) {
                    continue;
                }
                if(!isFirst) {
                    dst << ",";
                }
                isFirst=false;
                dst << i;
            }
            dst << "],";
		}
        dst << "\n";
		dst << "\"Params\":" << SaveParams();
	dst << "\n},";

	// Nodes of the lattice
	dst << "{";
		dst << "\"Nodes\":[\n";
		isFirst=true;
		for( DWORD i = 0; i < concepts.size(); ++i ) {
			if(!isFirst){
				dst << ",\n";
			}
			isFirst=false;
			dst << "  ";
			printConceptToJson( concepts[i], dst );
		}
		dst << "\n]";
	dst << "},";

	// Arcs of the poset
	dst << "{";
		dst << "\"Arcs\":[\n";
		isFirst = true;
		if( shouldFindPartialOrder ) {
			for( DWORD i = 0; i < concepts.size(); ++i ) {
				CStdIterator<CList<DWORD>::CConstIterator,false> p( order.GetParents( i ) );
				for( ;!p.IsEnd(); ++p ) {
					if(!isFirst) {
						dst << ",";
					}
					isFirst=false;
					dst << "  {\"S\":" << *p << ",\"D\":" << i << "}";
				}
			}
		}
		dst << "]";
	dst << "}";

	dst << "]\n";
}

void CSofiaContextProcessor::printConceptToJson( const CPatternMeasurePair& c, std::ostream& dst )
{
dst << "{";

	if( outParams.OutExtent ) {
		dst << "\"Ext\":" << pChain->SaveExtent( c.first ) << ",";
	} else {
		dst << "\"Ext\":" << pChain->GetExtentSize( c.first ) << ",";
	}
	if( outParams.OutIntent ) {
		dst << "\"Int\":" << pChain->SaveIntent( c.first ) << ",";
	}
	if(oest != 0) {
		dst << "\"PatternQuality\":" << oest->GetJsonQuality(dynamic_cast<const IExtent*>(c.first)) << ",";
	}

	dst << "\"Interest\":" << c.second;

dst << "}";
}

// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.7

#include <fcaps/Filters/RemoveExpectedIntPatterns.h>

#include <fcaps/SharedModulesLib/FindConceptOrder.h>

#include <fcaps/SharedModulesLib/details/JsonIntervalPattern.h>
#include <ModuleTools.h>
#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/writer.h>

#include <boost/container/map.hpp>
#include <boost/math/distributions/chi_squared.hpp>

#include <time.h>

using namespace std;


////////////////////////////////////////////////////////////////////

class CConceptsForOrder {
public:
	CConceptsForOrder( IPatternManager& _cmp, const std::deque< CSharedPtr<const IPatternDescriptor> >& _concepts, const vector<DWORD>& _extSize ) :
		cmp( _cmp ), concepts( _concepts ), extSize(_extSize) {}

	// Methods of TConcepts requiring by CFindConceptOrder
	DWORD Size() const
		{ return concepts.size(); }
	bool IsTopologicallyLess( DWORD c1, DWORD c2 ) const
	{
		assert( c1 < concepts.size() );
		assert( c2 < concepts.size() );
		const bool res = extSize[c1] >= extSize[c2];
		return res;
	}
	bool IsLess( DWORD c1, DWORD c2 ) const
	{
		assert( c1 < concepts.size() );
		assert( c2 < concepts.size() );
		const bool res = cmp.Compare(concepts[c1].get(),concepts[c2].get(), CR_MoreGeneral, CR_MoreGeneral | CR_Incomparable ) == CR_MoreGeneral; 
		return res;
	}
private:
	IPatternManager& cmp;
	const std::deque< CSharedPtr<const IPatternDescriptor> >& concepts;
	const vector<DWORD>& extSize;
};

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CRemoveExpectedIntPatterns> CRemoveExpectedIntPatterns::registrar(
	LatticeFilterModuleType, RemoveExpectedIntPatterns );

CRemoveExpectedIntPatterns::CRemoveExpectedIntPatterns() :
	cmp(dynamic_cast<IPatternManager*>(CreateModule(PatternManagerModuleType,"IntervalPatternManagerModule",
					string("{\"Type\":\"") + PatternManagerModuleType +"\",\"Name\":\"IntervalPatternManagerModule\"}"))),
	deleter(cmp),
	significance(-1),
	outSuffix(".FILTERED"),
	findPartialOrder(false)
{
	assert(cmp != 0);
}

void CRemoveExpectedIntPatterns::SetInputFile( const char* path )
{ 
	inputFile = path; 

	string resultFile = inputFile;
	const size_t ext = resultFile.find_last_of( "." );
	if( ext != string::npos ) {
		resultFile = resultFile.substr(0,ext);
	}
	resultFile += outSuffix + ".json";
	results.push_back( resultFile );
}

static bool areEqual(const double& x, const double& y)
{
	return abs(x-y) <= std::numeric_limits<double>::epsilon() * std::abs(x+y) * 2
		|| std::abs(x-y) < std::numeric_limits<double>::min();
}

void CRemoveExpectedIntPatterns::Process()
{
	static const char place[] = "CRemoveExpectedIntPatterns::Process";
	CJsonError error(dataFile,"");

	rapidjson::Document doc;

	// Loading data
	CJsonFile file(dataFile);
	if( !file.Read(  doc, error ) ) {
		throw new CJsonException( place, error );
	}
	if( !doc.IsArray() || doc.Size() < 2 || !doc[1].IsObject()
		|| !doc[1].HasMember("Data") || !doc[1]["Data"].IsArray() )
	{
		error.Error = "DATA[1].Data not found. Not a valid context";
		throw new CJsonException( place, error );
	}
	const rapidjson::Value& data = doc[1]["Data"];
	const DWORD objCount = data.Size();
	
	context.resize( data.Size() );
	for( DWORD objectNum = 0; objectNum < data.Size(); ++objectNum ) {
		JSON intent;
		CreateStringFromJSON( data[objectNum], intent );
		CSharedPtr<const IPatternDescriptor> p( cmp->LoadObject( intent ), deleter );
		context[objectNum] = p;
	}

	// Loading lattice
	CJsonFile inputFileObj( inputFile );
	if( !inputFileObj.Read(  doc, error ) ) {
		throw new CJsonException( place, error );
	}
	if( !doc.IsArray() || doc.Size() < 3 
		|| !doc[1].IsObject() || !doc[1].HasMember("Nodes") || !doc[1]["Nodes"].IsArray()
		|| !doc[2].IsObject() || !doc[2].HasMember("Arcs") || !doc[2]["Arcs"].IsArray()
	  )
	{
		error.Error = "DATA[1].Nodes or DATA[2].Arcs not found. Not a valid lattice";
		throw new CJsonException( place, error );
	}
	rapidjson::MemoryPoolAllocator<>& alloc = doc.GetAllocator();
	boost::container::multimap<DWORD,DWORD> parents;
	// Reading lattice structure
	const rapidjson::Value& arcs=doc[2]["Arcs"];
	for( int i = 0; i < arcs.Size(); ++i ) {
		const DWORD from = arcs[i]["D"].GetUint();
		const DWORD to = arcs[i]["S"].GetUint();
		parents.insert(pair<DWORD,DWORD>( from, to ));
	}
	
	// Loading concepts
	rapidjson::Value& nodes=doc[1]["Nodes"];
	concepts.resize( nodes.Size() );
	for( int i = 0; i < nodes.Size(); ++i ) {
		JSON intent;
		CreateStringFromJSON( nodes[i]["Int"], intent );
		concepts[i]= CSharedPtr<const IPatternDescriptor>( cmp->LoadPattern( intent ), deleter );
	}

	// To stare p-values
	pvals.resize(concepts.size());
	boost::math::chi_squared chi2(1);
	// Computing P-value for every found concept
	for( int i = 0; i < concepts.size(); ++i ) {
		std::cout << "Concept " << i << " \r";
		const DWORD support = nodes[i]["Ext"].GetUint();
		double pvalue = 1;
		JsonIntervalPattern::CPattern current;
		JsonIntervalPattern::LoadPattern(cmp->SavePattern( concepts[i].get() ), current);

		CStdIterator<boost::container::multimap<DWORD,DWORD>::const_iterator>  prnt( parents.find(i), parents.end() );
		// Verifying if there any parent concept that could generate this one by chance.
		for( ; !prnt.IsEnd() && (*prnt).first == i; ++prnt ) {
			const DWORD parentIndex = (*prnt).second;
			assert(parentIndex < concepts.size());

			const DWORD supportParent = nodes[parentIndex]["Ext"].GetUint();
			assert(supportParent > support);

			// Need to find the attributes that has changed between them.
			JsonIntervalPattern::CPattern parent;
			JsonIntervalPattern::LoadPattern(cmp->SavePattern( concepts[parentIndex].get() ), parent);
			assert(current.size() == parent.size());

			JsonIntervalPattern::CPattern generalParent;
			JsonIntervalPattern::CPattern generalChild;
			generalParent.resize(current.size());
			generalChild.resize(current.size());

			// Now we remove any part of the pattern that is not changed
			const double pos_inf = std::numeric_limits<double>::max();
			const double neg_inf = -std::numeric_limits<double>::max();

			bool hasNonModifiedAttribute = false;
			for( int i = 0; i < current.size(); ++i ) {
				assert(neg_inf < current[i].first && current[i].second < pos_inf);
				if(areEqual(current[i].first,parent[i].first) && areEqual(current[i].second,parent[i].second)) {
					hasNonModifiedAttribute = true;	
					generalParent[i].first=neg_inf;
					generalChild[i].first=neg_inf;
					generalParent[i].second=pos_inf;
					generalChild[i].second=pos_inf;
				} else {
					generalParent[i]=parent[i];
					generalChild[i]=current[i];
				}
			}
			if(!hasNonModifiedAttribute) {
				// Not by chance, at least we cannot check it
				continue;
			}
			// Let us find the support of geneal patterns
			CSharedPtr<const IPatternDescriptor> generalParentPattern( cmp->LoadPattern(JsonIntervalPattern::SavePattern(generalParent)), deleter);
			CSharedPtr<const IPatternDescriptor> generalChildPattern( cmp->LoadPattern(JsonIntervalPattern::SavePattern(generalChild)), deleter);
			DWORD generalChildSupport = 0;
			DWORD generalParentSupport = 0;
			for(DWORD objNum = 0; objNum < context.size(); ++objNum ) {
				if(cmp->Compare(context[objNum].get(),generalChildPattern.get(), 
							CR_LessGeneral | CR_Equal, CR_AllResults | CR_Incomparable) != CR_Incomparable) 
				{
					assert(cmp->Compare(context[objNum].get(),generalParentPattern.get(), 
							CR_LessGeneral | CR_Equal, CR_AllResults | CR_Incomparable) != CR_Incomparable);
					++generalChildSupport;
					++generalParentSupport;
				} else 	if(cmp->Compare(context[objNum].get(),generalParentPattern.get(), 
							CR_LessGeneral | CR_Equal, CR_AllResults | CR_Incomparable) != CR_Incomparable) 
				{
					++generalParentSupport;
				}
			}
			assert( generalParentSupport > generalChildSupport );
			if(generalParentSupport == supportParent) {
				assert(generalChildSupport == support);
				continue;
			}

			// Computing pvalue by chi2
			//
			//  observed counts
			const DWORD o1 = support;
			const DWORD o0 = supportParent - o1;
			//  expected counts
			const double probability = static_cast<double>(generalChildSupport)/generalParentSupport;
			const double e1 = supportParent * probability;  
			const double e0 = supportParent - e1;
			const double T = (e1-o1)*(e1-o1)/e1 + (e0-o0)*(e0-o0)/e0;
			const double currPValue = boost::math::cdf(chi2,T);
			pvalue *= currPValue ;
			if( 1 - pvalue > significance ) {
				pvalue = 0;
				break;
			}

		}

		pvals[i]= 1 - pvalue;
	}	

	//// Removing insignificant nodes from initial lattice
	std::deque< CSharedPtr<const IPatternDescriptor> > fltrConcepts;	
	rapidjson::Value newNodeArray;
	newNodeArray.SetArray();
	vector<DWORD> extSize;
	for( int i = 0; i < pvals.size(); ++i ) {
		if( pvals[i] < significance ) {
			const DWORD support = nodes[i]["Ext"].GetUint();
			nodes[i].AddMember("P-Val",rapidjson::Value().SetDouble(pvals[i]),alloc);
			newNodeArray.PushBack(nodes[i].Move(), alloc);
			fltrConcepts.push_back(concepts[i]);
			extSize.push_back(support);
		}	
	}
	doc[1]["Nodes"] = newNodeArray;
	doc[0]["NodesCount"] = rapidjson::Value().SetUint(doc[1]["Nodes"].Size());

	// Updating arcs
	if( !findPartialOrder ) {
		doc[2]=rapidjson::Value().Move();
		doc[0]["ArcsCount"] = rapidjson::Value().SetInt(0);
	} else {
		CConceptsForOrder conceptsForOrder( *cmp, fltrConcepts, extSize );
		CFindConceptOrder<CConceptsForOrder> order( conceptsForOrder );
		order.Compute();

		CList<DWORD> tops;
		vector<bool> bottoms;
		bottoms.resize( fltrConcepts.size(), true );
		doc[2]["Arcs"].SetArray();
		for( DWORD i = 0; i < fltrConcepts.size(); ++i ) {
			const CList<DWORD>& parents = order.GetParents( i );
			if( parents.Size() == 0 ) {
				tops.PushBack( i );
			}
			CStdIterator<CList<DWORD>::CConstIterator,false> p( parents );
			for( ; !p.IsEnd(); ++p ) {
				bottoms[*p] = false;
				doc[2]["Arcs"].PushBack( 
					rapidjson::Value().SetObject()
						.AddMember("S", rapidjson::Value().SetUint(*p), alloc)
						.AddMember("D", rapidjson::Value().SetUint(i), alloc)
					,alloc);	
			}
		}

		doc[0]["ArcsCount"].SetInt(doc[2]["Arcs"].Size());
		assert(tops.Size() > 0 );

		doc[0]["Top"].SetArray();
		CStdIterator<CList<DWORD>::CConstIterator, false> top(tops);
		for( ;!top.IsEnd(); ++top ) {
			doc[0]["Top"].PushBack(rapidjson::Value().SetUint(*top), alloc);
		}

		doc[0]["Bottom"].SetArray();
		for( DWORD i = 0; i < bottoms.size(); ++i ) {
			if(!bottoms[i]) {
				continue;
			}
			doc[0]["Bottom"].PushBack(rapidjson::Value().SetUint(i), alloc);
		}
		assert(doc[0]["Bottom"].Size() > 0);
	 }


	// Updating params of the lattice
	JSON json = SaveParams();
	rapidjson::Document thisParams;
	if( !ReadJsonString( json, thisParams, error ) ) {
		assert(false);
	}
	if( !doc[0].HasMember("Filters") ) {
		doc[0].AddMember( "Filters", rapidjson::Value().SetArray(), alloc );
	}
	doc[0]["Filters"].PushBack(thisParams, alloc );

	JSON outputStr;
	CreateStringFromJSON( doc, outputStr );
	CDestStream dst(results[0]);
	dst << outputStr;
}

void CRemoveExpectedIntPatterns::LoadParams( const JSON& json )
{
	static const char place[] = "CRemoveExpectedIntPatterns::LoadParams";
	CJsonError error;
	error.Data = json;
	rapidjson::Document doc;
	if( !ReadJsonString( json, doc, error ) ) {
		throw new CJsonException( place, error );
	}
	assert( string( doc["Type"].GetString() ) == LatticeFilterModuleType );
	assert( string( doc["Name"].GetString() ) == RemoveExpectedIntPatterns );
	if( !(doc.HasMember( "Params" ) && doc["Params"].IsObject()) ) {
		error.Error = "Params is not found. Necessary for significance level.";
		throw new CJsonException(place, error);
	}
	const rapidjson::Value& params=doc["Params"];

	results.clear();
	if( !params.HasMember("Significance") || !params["Significance"].IsDouble() ) {
		error.Error = "'Significance' not found";
		throw new CJsonException(place, error);
	} else {
		significance = params["Significance"].GetDouble();
	}

	if( params.HasMember("FindPartialOrder") && params["FindPartialOrder"].IsBool() ) {
		findPartialOrder = params["FindPartialOrder"].GetBool();
	}

	if( params.HasMember("OutSuffix") && params["OutSuffix"].IsString() ) {
		outSuffix = params["OutSuffix"].GetString();
	}
}

JSON CRemoveExpectedIntPatterns::SaveParams() const
{
	assert( results.size() == 1 );
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", LatticeFilterModuleType, alloc )
		.AddMember( "Name", RemoveExpectedIntPatterns, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "Significance", rapidjson::Value().SetDouble(significance), alloc)
			.AddMember( "FindPartialOrder", rapidjson::Value().SetBool(findPartialOrder), alloc)
			.AddMember( "OutSuffix", rapidjson::Value().SetString(
				rapidjson::StringRef(outSuffix.c_str())), alloc ),
		alloc );
	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}


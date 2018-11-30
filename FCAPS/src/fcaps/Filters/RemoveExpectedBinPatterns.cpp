// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.7

#include <fcaps/Filters/RemoveExpectedBinPatterns.h>

#include <fcaps/SharedModulesLib/BinarySetPatternManager.h>
#include <fcaps/SharedModulesLib/FindConceptOrder.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/writer.h>

#include <boost/container/map.hpp>
#include <boost/math/distributions/chi_squared.hpp>

#include <time.h>

using namespace std;


////////////////////////////////////////////////////////////////////

#include <fcaps/SharedModulesLib/ParallelListIteration.inl>

////////////////////////////////////////////////////////////////////

class CConceptsForOrder {
public:
	CConceptsForOrder( const CBinarySetDescriptorsComparator& _cmp, const std::deque< CSharedPtr<const CBinarySetPatternDescriptor> >& _concepts ) :
		cmp( _cmp ), concepts( _concepts ) {}

	// Methods of TConcepts requiring by CFindConceptOrder
	DWORD Size() const
		{ return concepts.size(); }
	bool IsTopologicallyLess( DWORD c1, DWORD c2 ) const
	{
		assert( c1 < concepts.size() );
		assert( c2 < concepts.size() );
		const bool res = concepts[c1]->GetAttribs().Size() < concepts[c2]->GetAttribs().Size();
		return res;
	}
	bool IsLess( DWORD c1, DWORD c2 ) const
	{
		assert( c1 < concepts.size() );
		assert( c2 < concepts.size() );
		const bool res = cmp.Compare(*concepts[c1],*concepts[c2], CR_MoreGeneral, CR_MoreGeneral | CR_Incomparable ) == CR_MoreGeneral; 
		return res;
	}
private:
	const CBinarySetDescriptorsComparator& cmp;
	const std::deque< CSharedPtr<const CBinarySetPatternDescriptor> >& concepts;
};

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CRemoveExpectedBinPatterns> CRemoveExpectedBinPatterns::registrar(
	LatticeFilterModuleType, RemoveExpectedBinPatterns );

CRemoveExpectedBinPatterns::CRemoveExpectedBinPatterns() :
	extCmp(new CVectorBinarySetJoinComparator),
	extDeleter(extCmp),
	intCmp(new CBinarySetDescriptorsComparator),
	significance(-1),
	outSuffix(".FILTERED"),
	findPartialOrder(false)
{
}

void CRemoveExpectedBinPatterns::SetInputFile( const char* path )
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

void CRemoveExpectedBinPatterns::Process()
{
	static const char place[] = "CRemoveExpectedBinPatterns::Process";
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
	if( !data[0].HasMember("Inds") && !data[0].HasMember("Names") ) {
		error.Error = "Not a binary context. Only binary context is allowed";
		throw new CJsonException( place, error );
	}


	const DWORD objCount = data.Size();
	tmpContext.resize( data.Size() );
	for( DWORD objectNum = 0; objectNum < data.Size(); ++objectNum ) {
		JSON intent;
		CreateStringFromJSON( data[objectNum], intent );
		CSharedPtr<const CBinarySetPatternDescriptor> p( intCmp->LoadObject( intent ), CPatternDeleter(intCmp) );
		tmpContext[objectNum] = p->GetAttribs();
	}
	// Converting data
	convertContext();
	tmpContext.clear();

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
	std::deque< CSharedPtr<const CBinarySetPatternDescriptor> > concepts;	
	concepts.resize( nodes.Size() );
	for( int i = 0; i < nodes.Size(); ++i ) {
		JSON intent;
		CreateStringFromJSON( nodes[i]["Int"], intent );
		concepts[i]= CSharedPtr<const CBinarySetPatternDescriptor>( intCmp->LoadObject( intent ), CPatternDeleter(intCmp) );
	}

	// To stare p-values
	vector<double> pvals;
	pvals.resize(concepts.size());
	boost::math::chi_squared chi2(1);
	// Computing P-value for every found concept
	for( int i = 0; i < concepts.size(); ++i ) {
		std::cout << "Concept " << i << " \r";
		if(!nodes[i].HasMember("Ext")
		   || (!nodes[i]["Ext"].IsObject() || !nodes[i]["Ext"].HasMember("Count") || !nodes[i]["Ext"]["Count"].IsUint())
			&& !nodes[i]["Ext"].IsUint())
		{
			throw CTextException(place, "Extent is not found in the concept");	
		}
		DWORD support = 0;
		if(nodes[i]["Ext"].IsUint()) {
			support = nodes[i]["Ext"].GetUint();
		} else {
			support = nodes[i]["Ext"]["Count"].GetUint();
		}
		double pvalue = 1;

		const CList<DWORD>& curr = concepts[i]->GetAttribs();
		CStdIterator<boost::container::multimap<DWORD,DWORD>::const_iterator>  itr( parents.find(i), parents.end() );
		// Verifying if there any parent concept that could generate this one by chance.
		for( ; !itr.IsEnd() && (*itr).first == i; ++itr ) {
			const DWORD parentIndex = (*itr).second;
			assert(parentIndex < concepts.size());
			const CList<DWORD>& parent = concepts[parentIndex]->GetAttribs();

			assert(parent.Size() < curr.Size());
			DWORD supportParent = 0;
			if(nodes[i]["Ext"].IsUint()) {
				supportParent = nodes[parentIndex]["Ext"].GetUint();
			} else {
				supportParent = nodes[parentIndex]["Ext"]["Count"].GetUint();
			}
			assert(supportParent > support);

			// To save additional attributes in the current node
			CList<DWORD> diff;

			PARALLEL_LIST_ITERATION_BEGIN( 
					CBinarySetPatternDescriptor::CAttrsList, CBinarySetPatternDescriptor::CAttrsList::CConstIterator, 
					curr, parent, currItr, parentItr );

				assert( parentItr == parent.End() || *parentItr == *currItr || *parentItr > *currItr );

				if( *currItr < *parentItr ) {
					diff.PushBack(*currItr );
				}

			PARALLEL_LIST_ITERATION_END( currItr, parentItr );
			assert( parentItr == parent.End() );
			for(; currItr != curr.End(); ++currItr ) {
				diff.PushBack(*currItr );
			}
			assert( diff.Size() > 0 );

			CStdIterator<CList<DWORD>::CConstIterator,false> diffAttrItr( diff );

			// Computing the tidset for the new attributes in the concept in question
			const DWORD currAttr = *diffAttrItr;
			assert( currAttr < attrToTidsetMap.size() );
			CSharedPtr<const CVectorBinarySetDescriptor> newAttrsIntent = attrToTidsetMap[currAttr];
			for( ++diffAttrItr; !diffAttrItr.IsEnd(); ++diffAttrItr ) {
				const DWORD currAttr = *diffAttrItr;
				assert( currAttr < attrToTidsetMap.size() );
				newAttrsIntent.reset(
						extCmp->CalculateSimilarity( *newAttrsIntent, *attrToTidsetMap[currAttr] ), extDeleter );
			}

			// Computing pvalue by chi2
			//
			//  observed counts
			const DWORD o1 = support;
			const DWORD o0 = supportParent - o1;
			//  expected counts
			const double probability = static_cast<double>(newAttrsIntent->Size())/objCount;
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

		pvals[i]=1 - pvalue;
	}	

	// Removing insignificant nodes from initial lattice
	std::deque< CSharedPtr<const CBinarySetPatternDescriptor> > fltrConcepts;	
	rapidjson::Value newNodeArray;
	newNodeArray.SetArray();
	for( int i = 0; i < pvals.size(); ++i ) {
		if( pvals[i] < significance ) {
			nodes[i].AddMember("P-Val",rapidjson::Value().SetDouble(pvals[i]),alloc);
			newNodeArray.PushBack(nodes[i].Move(), alloc);
			fltrConcepts.push_back(concepts[i]);
		}	
	}
	doc[1]["Nodes"] = newNodeArray;
	doc[0]["NodesCount"] = rapidjson::Value().SetUint(doc[1]["Nodes"].Size());

	// Updating arcs
	if( !findPartialOrder ) {
		doc[2]=rapidjson::Value().Move();
		doc[0]["ArcsCount"] = rapidjson::Value().SetInt(0);
	} else {
		CConceptsForOrder conceptsForOrder( *intCmp, fltrConcepts );
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

void CRemoveExpectedBinPatterns::LoadParams( const JSON& json )
{
	static const char place[] = "CRemoveExpectedBinPatterns::LoadParams";
	CJsonError error;
	error.Data = json;
	rapidjson::Document doc;
	if( !ReadJsonString( json, doc, error ) ) {
		throw new CJsonException( place, error );
	}
	assert( string( doc["Type"].GetString() ) == LatticeFilterModuleType );
	assert( string( doc["Name"].GetString() ) == RemoveExpectedBinPatterns );
	if( !(doc.HasMember( "Params" ) && doc["Params"].IsObject()) ) {
		error.Error = "Params is not found. Necessary for significance level and Context.";
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
	if( !params.HasMember("Context") || !params["Context"].IsString() ) {
		error.Error = "'Context' not found";
		throw new CJsonException(place, error);
	} else {
		dataFile = params["Context"].GetString();
	}

	if( params.HasMember("FindPartialOrder") && params["FindPartialOrder"].IsBool() ) {
		findPartialOrder = params["FindPartialOrder"].GetBool();
	}

	if( params.HasMember("OutSuffix") && params["OutSuffix"].IsString() ) {
		outSuffix = params["OutSuffix"].GetString();
	}
}

JSON CRemoveExpectedBinPatterns::SaveParams() const
{
	assert( results.size() == 1 );
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", LatticeFilterModuleType, alloc )
		.AddMember( "Name", RemoveExpectedBinPatterns, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "Significance", rapidjson::Value().SetDouble(significance), alloc)
			.AddMember( "Context", rapidjson::Value().SetString(
				rapidjson::StringRef(dataFile.c_str())), alloc )
			.AddMember( "FindPartialOrder", rapidjson::Value().SetBool(findPartialOrder), alloc)
			.AddMember( "OutSuffix", rapidjson::Value().SetString(
				rapidjson::StringRef(outSuffix.c_str())), alloc ),
		alloc );
	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

// Converts a context from ObjToAttrs form to AttrToObjs form.
void CRemoveExpectedBinPatterns::convertContext()
{
	objCount = tmpContext.size();
	extCmp->SetMaxAttrNumber( objCount );
	for( int i = 0; i < objCount; ++i ) {
		const CList<DWORD>& obj = tmpContext[i];
		addColumnToTable( i, obj, attrToTidsetMap );
	}
	tmpContext.clear();
}

// Add a new vertical line to the table, i.e. add the columnNum to every horisontal line in values.
void CRemoveExpectedBinPatterns::addColumnToTable(
	DWORD columnNum, const CList<DWORD>& values, CBinarySetCollection& table )
{
	AddColumnToCollection( extCmp, columnNum, values, table );
}



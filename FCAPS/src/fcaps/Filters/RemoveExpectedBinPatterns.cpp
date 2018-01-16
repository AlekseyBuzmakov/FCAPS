// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.7

#include <fcaps/Filters/RemoveExpectedBinPatterns.h>

#include <fcaps/SharedModulesLib/BinarySetPatternManager.h>

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
const CModuleRegistrar<CRemoveExpectedBinPatterns> CRemoveExpectedBinPatterns::registrar(
	LatticeFilterModuleType, RemoveExpectedBinPatterns );

CRemoveExpectedBinPatterns::CRemoveExpectedBinPatterns() :
	extCmp(new CVectorBinarySetJoinComparator),
	extDeleter(extCmp),
	intCmp(new CBinarySetDescriptorsComparator),
	significance(-1),
	outSuffix(".FILTERED")
{
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

	boost::container::multimap<DWORD,DWORD> parents;
	// Reading lattice structure
	const rapidjson::Value& arcs=doc[2]["Arcs"];
	for( int i = 0; i < arcs.Size(); ++i ) {
		const DWORD from = arcs[i]["D"].GetUint();
		const DWORD to = arcs[i]["S"].GetUint();
		parents.insert(pair<DWORD,DWORD>( from, to ));
	}
	
	// Loading concepts
	const rapidjson::Value& nodes=doc[1]["Nodes"];
	tmpContext.resize( nodes.Size() );
	for( int i = 0; i < nodes.Size(); ++i ) {
		JSON intent;
		CreateStringFromJSON( nodes[i]["Int"], intent );
		CSharedPtr<const CBinarySetPatternDescriptor> p( intCmp->LoadObject( intent ), CPatternDeleter(intCmp) );
		tmpContext[i] = p->GetAttribs();
	}

	// To stare p-values
	vector<double> pvals;
	pvals.resize(tmpContext.size());
	boost::math::chi_squared chi2(1);
	// Computing P-value for every found concept
	for( int i = 0; i < tmpContext.size(); ++i ) {
		const DWORD support = nodes[i]["Ext"].GetUint();
		double pvalue = 1;

		const CList<DWORD>& curr = tmpContext[i];
		CStdIterator<boost::container::multimap<DWORD,DWORD>::const_iterator>  itr( parents.find(i), parents.end() );
		// Verifying if there any parent concept that could generate this one by chance.
		for( ; !itr.IsEnd(); +itr ) {
			const DWORD parentIndex = (*itr).second;
			assert(parentIndex < tmpContext.size());
			const CList<DWORD>& parent = tmpContext[parentIndex];
			assert(parent.Size() < curr.Size());
			const DWORD supportParent = nodes[parentIndex]["Ext"].GetUint();
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
			//  expected probabilities
			const double e1 = static_cast<double>(newAttrsIntent->Size())/objCount;
			const double e0 = 1 - e1;
			//  observed probabilities
			const double o1 = static_cast<double>(support)/supportParent;
			const double o0 = 1 - o1;
			const double T = (e1-o1)*(e1-o1)/e1 + (e0-o0)*(e0-o0)/o0;
			pvalue *= boost::math::cdf(chi2,T);
			if( 1 - pvalue > significance ) {
				pvalue = 0;
				break;
			}

		}

		pvals[i]=1 - pvalue;
	}	


	/////////////////////////////////////////////////////////
	// Just to save params of the filter to the output
	//if( !doc[0].IsObject() ) {
		//doc[0].SetObject();
	//}
	//JSON json = SaveParams();
	//rapidjson::Document thisParams;
	//if( !ReadJsonString( json, thisParams, error ) ) {
		//assert(false);
	//}
	//if( !doc[0].HasMember("Filters") ) {
		//doc[0].AddMember( "Filters", rapidjson::Value().SetArray(), doc.GetAllocator());
	//}
	//doc[0]["Filters"].PushBack(thisParams, doc.GetAllocator());

	//params.SetObject() = doc[0];
	//if( params.HasMember("ObjNames") && params["ObjNames"].IsArray() ) {
		//objNames = params["ObjNames"];
		//params.RemoveMember("ObjNames");
	//}
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
		error.Error = "Params is not found. Necessary for significance level.";
		throw new CJsonException(place, error);
	}
	const rapidjson::Value& params=doc["Params"];

	results.clear();
	if( !params.HasMember("Significance") || !params["Significance"].IsDouble() ) {
		error.Error = "'Significance' not found";
		throw new CJsonException(place, error);
	} else {
		significance = !params["Significance"].GetDouble();
	}

	if( params.HasMember("OutSuffix") && params["OutSuffix"].IsString() ) {
		outSuffix = params["OutSuffix"].GetString();
	}
	string resultFile = inputFile;
	const size_t ext = resultFile .find_last_of( "." );
	if( ext != string::npos ) {
		resultFile = resultFile.substr(0,ext);
	}
	resultFile += outSuffix + ".json";
	results.push_back( resultFile );
}

JSON CRemoveExpectedBinPatterns::SaveParams() const
{
	assert( results.size() == 2 );
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", LatticeFilterModuleType, alloc )
		.AddMember( "Name", RemoveExpectedBinPatterns, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "Significance", rapidjson::Value().SetDouble(significance), alloc)
			.AddMember( "OutSuffix", rapidjson::Value().SetString(
				rapidjson::StringRef(results[0].c_str())), alloc ),
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



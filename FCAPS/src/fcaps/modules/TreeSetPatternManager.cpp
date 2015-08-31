// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/modules/TreeSetPatternManager.h>

#include <fcaps/modules/details/TaxonomyJsonReader.h>
#include <JSONTools.h>

#include "ParallelListIteration.inl"

using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define DEBUG_CMP 1
#endif

#ifdef DEBUG_CMP
#define debug_return(res) \
	cout << #res << "\n"; \
	cout << "--------------------------------------\n"; \
	return res;
#else
#define debug_return(res) return res;
#endif

const CModuleRegistrar<CTreeSetDescriptorsComparator> CTreeSetDescriptorsComparator::registrar(
	PatternManagerModuleType, TreeSetPatternManager );

CTreeSetDescriptorsComparator::CTreeSetDescriptorsComparator() :
	maxDiff(-1)
{
}

const CTreeSetPatternDescriptor* CTreeSetDescriptorsComparator::LoadObject( const JSON& json )
{
	return loadPattern( json );
}
JSON CTreeSetDescriptorsComparator::SavePattern( const IPatternDescriptor* ptrn ) const
{
	return savePattern( ptrn );
}
const CTreeSetPatternDescriptor* CTreeSetDescriptorsComparator::LoadPattern( const JSON& json )
{
	return loadPattern( json );
}

const CTreeSetPatternDescriptor* CTreeSetDescriptorsComparator::CalculateSimilarity(
	const IPatternDescriptor* firstPattern, const IPatternDescriptor* secondPattern )
{
#ifdef DEBUG_CMP
	// Debuging
	cout << "\n-------TreeSetCmp::Similarity--------\n";
	Write( firstPattern, cout );
	cout << "\n";
	Write( secondPattern, cout );
	cout << "\n";
#endif


	const CTreeSetPatternDescriptor& first = getDescriptor( firstPattern );
	const CTreeSetPatternDescriptor& second = getDescriptor( secondPattern );
	CList<DWORD> resultIndexes;
	const DWORD noPrevElement = 2 << 31;
	DWORD last = noPrevElement;
	bool isLastFromFirst = false;

	// Find possible result antichain.
	PARALLEL_LIST_ITERATION_BEGIN( CTreeSetPatternDescriptor::CAttrsList, CTreeSetPatternDescriptor::CAttrsList::CConstIterator,
			first.GetAttribs(), second.GetAttribs(), firstItr, secondItr );
		assert( last == noPrevElement || *firstItr >= last && *secondItr >= last );
		if( *firstItr < *secondItr ) {
			if( last == noPrevElement ) {
				// Nothing yet to intersect with
				last = *firstItr;
			} else if( isLastFromFirst ) {
				// Just renew,because there is no need to intersect with itself
				last = *firstItr;
			} else {
				resultIndexes.PushBack( rmq->GetMinIndexOnRange( last, *firstItr ) );

				last = *firstItr;
			}
			isLastFromFirst = true;
		} else if( *firstItr > *secondItr ) {
			if( last == noPrevElement ) {
				// Nothing yet to intersect with
				last = *secondItr;
			} else if( !isLastFromFirst ) {
				// Just renew,because there is no need to intersect with itself
				last = *secondItr;
			} else {
				resultIndexes.PushBack( rmq->GetMinIndexOnRange( last, *secondItr ) );

				last = *secondItr;
			}
			isLastFromFirst = false;
		} else {
			resultIndexes.PushBack( *firstItr );
			last = noPrevElement;
		}
	PARALLEL_LIST_ITERATION_END( firstItr, secondItr );

	// Process the rest of the "longest" anti-chain
	if( last != noPrevElement ) {
		if( isLastFromFirst && secondItr != second.GetAttribs().End() ) {
			resultIndexes.PushBack( rmq->GetMinIndexOnRange( last, *secondItr ) );
		} else {
			resultIndexes.PushBack( rmq->GetMinIndexOnRange( last, *firstItr ) );
		}
	}

	filterAntichain( resultIndexes );

// TOKILL?
// It is not a projection, so it cannot be applied.
//	if( maxDiff != NotFound ) {
//		projectIndexes( resultIndexes );
//	}

	// In resultIndexes now result antichain. We should create new pattern.
	auto_ptr<CTreeSetPatternDescriptor> patternRW( new CTreeSetPatternDescriptor );
	patternRW->AddList( resultIndexes );

#ifdef DEBUG_CMP
	// Debuging
	cout << "Result:\n";
	Write( patternRW.get(), cout );
	cout << "\n--------------------------------------\n";
#endif

	assert( HasAllFlags(CR_MoreOrEqual, Compare(patternRW.get(), firstPattern ) ) );
	assert( HasAllFlags(CR_MoreOrEqual, Compare(patternRW.get(), secondPattern) ) );

	return patternRW.release();
}

TCompareResult CTreeSetDescriptorsComparator::Compare(
	const IPatternDescriptor* firstPattern, const IPatternDescriptor* secondPattern,
	DWORD interestingResults/* = CR_AllResults*/, DWORD possibleResults/* = CR_AllResults | CR_Incomparable*/ )
{
#ifdef DEBUG_CMP
	// Debuging
	cout << "\n-------TreeSetCmp::Compare--------\n";
	Write( firstPattern, cout );
	cout << "\n";
	Write( secondPattern, cout );
	cout << "\n";
#endif

	const CTreeSetPatternDescriptor& first = getDescriptor( firstPattern );
	const CTreeSetPatternDescriptor& second = getDescriptor( secondPattern );

	interestingResults &= possibleResults & CR_AllResults;

	if( first.GetAttribs().Size() < second.GetAttribs().Size() ){
		// Could not be equal.
		interestingResults &= ~CR_Equal;
		// Could not be less general because if it is less general then for every path from node to root can exist only one node in second
		interestingResults &= ~CR_LessGeneral;
	} else if( first.GetAttribs().Size() > second.GetAttribs().Size() ){
		// Could not be equal.
		interestingResults &= ~CR_Equal;
		// Could not be less general because if it is less general then for every path from node to root can exist only one node in second
		interestingResults &= ~CR_MoreGeneral;
	}

	PARALLEL_LIST_ITERATION_BEGIN( CTreeSetPatternDescriptor::CAttrsList, CTreeSetPatternDescriptor::CAttrsList::CConstIterator,
			first.GetAttribs(), second.GetAttribs(), firstItr, secondItr );

		// Check the comparison between indices

		TCompareResult result = compareIndexes( *firstItr, *secondItr );
		while( firstItr != first.GetAttribs().End()
			&& secondItr != second.GetAttribs().End()
			&& result == CR_Equal )
		{
			++firstItr;
			++secondItr;
			result = compareIndexes( *firstItr, *secondItr );
		}
		if(firstItr == first.GetAttribs().End()
			|| secondItr == second.GetAttribs().End())
		{
			break;
		}
		changePossibleFlags( result, interestingResults );
		if( result == CR_Incomparable ) {
			// The smallest is uncovered
			//	i.e. 'the corresponding pattern' can be only uncovered
			changePossibleFlags(
				*firstItr < *secondItr ? CR_LessGeneral : CR_MoreGeneral, interestingResults );
		}
		if( interestingResults == 0 ) {
			debug_return(CR_Incomparable);
		}

		// 'Eat' all second that should be more specific
		while( result == CR_MoreGeneral ) {
			++secondItr;
			if( secondItr == second.GetAttribs().End() ) {
				break;
			}
			result = compareIndexes( *firstItr, *secondItr );
			assert( result == CR_MoreGeneral
				|| (result == CR_Incomparable && *firstItr < *secondItr ) );
		}

		// 'Eat' all first that should be more specific
		while( result == CR_LessGeneral ) {
			++firstItr;
			if( firstItr == first.GetAttribs().End() ) {
				break;
			}
			result = compareIndexes( *firstItr, *secondItr );
			assert( result == CR_LessGeneral
				|| result == CR_Incomparable && *firstItr > *secondItr );
		}
		// If we found 'END' it is done by one of the while,
		//  i.e. the other element have been "eaten".
		if( firstItr == first.GetAttribs().End() ) {
			++secondItr;
			break;
		}
		if( secondItr == second.GetAttribs().End() ) {
			++firstItr;
			break;
		}
	PARALLEL_LIST_ITERATION_END( firstItr, secondItr );

	if( firstItr != first.GetAttribs().End() ) {
		changePossibleFlags( CR_LessGeneral, interestingResults );
	}
	if( secondItr != second.GetAttribs().End() ) {
		changePossibleFlags( CR_MoreGeneral, interestingResults );
	}

	if( interestingResults == 0 ) {
		debug_return( CR_Incomparable );
	}

	if( HasAllFlags( interestingResults, CR_Equal ) ) {
		debug_return( CR_Equal );
	}
	switch( interestingResults ) {
		case CR_MoreGeneral:
			debug_return( CR_MoreGeneral );
		case CR_LessGeneral:
			debug_return( CR_LessGeneral );
		case 0:
			debug_return( CR_Incomparable );
		default:
			assert( false );
	}
	debug_return( CR_Incomparable );
}

void CTreeSetDescriptorsComparator::Write( const IPatternDescriptor* iPtrn, std::ostream& dst ) const
{
	const CTreeSetPatternDescriptor& ptrn = getDescriptor( iPtrn );
	CStdIterator<CTreeSetPatternDescriptor::CAttrsList::CConstIterator, false> itr( ptrn.GetAttribs() );
	dst << "{";
	for( ; !itr.IsEnd();  ) {
		dst << tree.GetNode( indexToNode[*itr] ).Data.ID;
		++itr;
		if( itr.IsEnd() ) {
			break;
		}
		dst << ",";
	}
	dst << "}";
}

void CTreeSetDescriptorsComparator::LoadParams( const JSON& json )
{
	CJsonError errorText;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "TaxonomyPatternManager", errorText );
	}
	assert( string( params["Type"].GetString() ) == PatternManagerModuleType );
	assert( string( params["Name"].GetString() ) == TreeSetPatternManager );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()
		&& params["Params"].HasMember( "TreePath" ) && params["Params"]["TreePath"].IsString() ) )
	{
		throw new CTextException( "TaxonomyPatternManager",
			"No 'TreePath' is found in <<\n" + json + "\n>>" );
	}
	pathToTree = params["Params"]["TreePath"].GetString();
	if( params["Params"].HasMember("MaxWeightDifference") && params["Params"]["MaxWeightDifference"].IsUint() ) {
		maxDiff = params["Params"]["MaxWeightDifference"].GetUint();
	}
	CTaxonomyJsonReader( tree, nameToIndexMap ).ReadTree( pathToTree );
	initMultiLca();
}
JSON CTreeSetDescriptorsComparator::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", PatternManagerModuleType, alloc )
		.AddMember( "Name", TreeSetPatternManager, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "TreePath", rapidjson::StringRef( pathToTree.c_str() ), alloc ),
		alloc );

	if( maxDiff != -1 ) {
		params.AddMember( "MaxWeightDifference", rapidjson::Value().SetUint( maxDiff ), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

// Converts the interface to the pattern.
const CTreeSetPatternDescriptor& CTreeSetDescriptorsComparator::getDescriptor( const IPatternDescriptor* ptrn )
{
	assert( ptrn != 0 );
	return *debug_cast<const CTreeSetPatternDescriptor*>( ptrn );
}

// Loads pattern from JSON
const CTreeSetPatternDescriptor* CTreeSetDescriptorsComparator::loadPattern( const JSON& json ) const
{
	CJsonError error;
	rapidjson::Document doc;
	if( !ReadJsonString( json, doc, error ) ) {
		throw new CJsonException( "CTreeSetDescriptorsComparator::loadPattern", error );
	};
	if( !( doc.IsObject() && doc.HasMember( "Names" ) && doc["Names"].IsArray() ) ) {
		throw new CTextException( "CTreeSetDescriptorsComparator::loadPattern", "Member 'Names' of type 'Array' not found" );
	}
	const rapidjson::Value& names = doc["Names"];
	assert( names.IsArray() );

	auto_ptr<CTreeSetPatternDescriptor> ptrn( new CTreeSetPatternDescriptor );
	for( DWORD i = 0; i < names.Size(); ++i ) {
		if( !names[i].IsString() ) {
			throw new CTextException( "CTreeSetDescriptorsComparator::loadPattern", "A name in 'Names' is not a string" );
		}
		CNameToIndexMap::const_iterator itr = nameToIndexMap.find( names[i].GetString() );
		if( itr == nameToIndexMap.end() ) {
			throw new CTextException( "CTreeSetDescriptorsComparator::loadPattern", string("The name '") + names[i].GetString() + "' is not found in the taxonomy" );
		}
		ptrn->AddNextAttribNumber( itr->second );
	}

	filterAntichain( ptrn->GetAttribs() );

	return ptrn.release();
}

// Saves the pattern to JSON
inline JSON CTreeSetDescriptorsComparator::savePattern( const IPatternDescriptor* iPtrn ) const
{
	const CTreeSetPatternDescriptor& ptrn = getDescriptor( iPtrn );
	rapidjson::Document doc;
	rapidjson::MemoryPoolAllocator<>& alloc = doc.GetAllocator();
	doc.SetObject();
	doc.AddMember( "Count", rapidjson::Value().SetInt( ptrn.GetAttribs().Size() ), alloc )
		.AddMember( "Names", rapidjson::Value().SetArray(), alloc );

	rapidjson::Value& arr = doc["Names"];
	CStdIterator<CList<DWORD>::CConstIterator, false> itr( ptrn.GetAttribs() );
	for( ; !itr.IsEnd(); ++itr ) {
		arr.PushBack( rapidjson::Value().SetString( tree.GetNode( indexToNode[*itr] ).Data.ID.c_str(), alloc), alloc );
	}

	JSON result;
	CreateStringFromJSON( doc, result );
	return result;
}

// Initialize array for multiple LCA by the tree
void CTreeSetDescriptorsComparator::initMultiLca()
{
	const size_t size = tree.GetSize();
	depthArray.resize( size * 2 - 1 );
	indexToNode.resize( depthArray.size() );

	CDeepFirstStaticTreeIterator<CTaxonomyJsonReader::CNodeInfo> node( tree.GetRoot(), tree );
	DWORD depth = -1;
	size_t index = 0;
	for( ; !node.IsEnd(); ++node, ++index ) {
		assert( index <= depthArray.size() );

		const CTree::TNodeIndex nodeIndex = *node;
		assert( 0 <= nodeIndex );
		indexToNode[index] = nodeIndex;

		if( node.IsForward() ) {
			depth++;
			// Save the only one index for every label in the tree
			nameToIndexMap[tree.GetNode( nodeIndex ).Data.ID] = index;
		} else {
			depth--;
		}

		depthArray[index] = depth;
	}
	rmq.reset( new CRmqAlgorithmAuto<DWORD>( depthArray ) );
	rmq->Initialize();
}

// filters a set of attributes to form an antichain
void CTreeSetDescriptorsComparator::filterAntichain( CList<DWORD>& chain ) const
{
	// Filter antichain comparing neighbourhoods
	CStdIterator<CList<DWORD>::CIterator, false> i( chain );
	DWORD last = *i;
	++i;
	CStdIterator<CList<DWORD>::CIterator, false> itr;

	for( ; !i.IsEnd(); ) {
		assert( last <= *i );
		const TCompareResult result = compareIndexes( last, *i );
		itr = i;
		++i;

		switch( result ) {
			case CR_Equal:
				// We already have this one
				chain.Erase( itr );
				break;
			case CR_MoreGeneral:
				// Prev attrib is more general, so remove it
				last = *itr;
				--itr;
				chain.Erase( itr );
				break;
			case CR_LessGeneral:
				// This is more general, remove.
				chain.Erase( itr );
				break;
			case CR_Incomparable:
				// New candidate to antichain
				last = *itr;
				break;
			default:
				assert( false );
		}
	}
}

// Compare to elements by their indexes.
inline TCompareResult CTreeSetDescriptorsComparator::compareIndexes( DWORD first, DWORD second ) const
{
	if( indexToNode[first] == indexToNode[second] ) {
		return CR_Equal;
	}

	const DWORD minIndex = first <= second ? rmq->GetMinIndexOnRange( first, second ) : rmq->GetMinIndexOnRange( second, first );
	if( indexToNode[first] == indexToNode[minIndex] ) {
		return CR_MoreGeneral;
	}
	if( indexToNode[minIndex] == indexToNode[second] ) {
		return CR_LessGeneral;
	}
	return CR_Incomparable;
}

// Project indecis in such a way that diff in weight between any two indexes is smaller than maxDiff.
// inds -- the in/out indecis
void CTreeSetDescriptorsComparator::projectIndexes( CList<DWORD>& inds ) const
{
	// Finds min weight value
	DWORD minWeight = -1;
	CStdIterator<CList<DWORD>::CIterator,false> i(inds);
	for( ; !i.IsEnd(); ++i ) {
		const DWORD weight = tree.GetNode( indexToNode[*i] ).Data.Weight;
		minWeight = min(minWeight, weight);
	}

	DWORD prevMinWeight = -1;
	while( minWeight != prevMinWeight ) {
		prevMinWeight = minWeight;
		i.Reset(inds);
		for( ; !i.IsEnd(); ++i ) {
			*i = projectIndex( *i, minWeight + maxDiff );
			minWeight = min( minWeight, tree.GetNode( indexToNode[*i] ).Data.Weight );
		}
	}
}

// Finds the closest ancestor with weight <= maxWeight.
// ind -- index to start from
// maxWeight -- max allowed weight
DWORD CTreeSetDescriptorsComparator::projectIndex( DWORD ind, DWORD maxWeight ) const
{
	DWORD treeNode = indexToNode[ind];
	while( tree.GetNode(treeNode).Data.Weight > maxWeight ) {
		const DWORD parentNode = tree.GetNode(treeNode).GetParent();
		assert( tree.GetNode(treeNode).Data.Weight >= tree.GetNode(parentNode).Data.Weight );
		treeNode = parentNode;
	}
	return nameToIndexMap.at(tree.GetNode( treeNode ).Data.ID);
}

// Correct flags for possible values of comparation
//  considering on the result of comparation of two attrs from different antichains.
// attrsPairCmp - result of next attrs pair comparation.
// possibleFlag - corrent possible flags for changing
inline void CTreeSetDescriptorsComparator::changePossibleFlags( TCompareResult attrsPairCmp, DWORD& possibleFlag )
{
	switch( attrsPairCmp ) {
		case CR_Equal:
			// Any variant still possible.
			break;
		case CR_MoreGeneral:
			// Could not be equal.
			possibleFlag &= ~CR_Equal;
			// Could not be less general in other case in first should be attr
			//  less general but comparable with second, so comparable with first,
			//  so first is not antichain.
			possibleFlag &= ~CR_LessGeneral;
			break;
		case CR_LessGeneral:
			// Could not be equal.
			possibleFlag &= ~CR_Equal;
			// Could not be more see above description.
			possibleFlag &= ~CR_MoreGeneral;
			break;
		case CR_Incomparable:
			// means nothing we could consider here not coinsiding attrs.
			break;
		default:
			assert( false );
	}
}

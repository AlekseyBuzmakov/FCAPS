// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Implementation of AddIntent algorithm.

#include <fcaps/modules/details/AddIntentLatticeBuilder.h>
#include <fcaps/modules/details/Extent.h>
#include <fcaps/storages/IntentStorage.h>

#include <StdTools.h>


using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////

CAddIntentLatticeBuilder::CAddIntentLatticeBuilder() :
	lattice( 0 ), processingObject( 0 ), currObjDescrID( -1 )
{

}

void CAddIntentLatticeBuilder::Initialize(
		const CSharedPtr<IExtentStorage>& _extStorage,
		const CSharedPtr<IIntentStorage>& cmp )
{
	assert( _extStorage != 0 );
	assert( cmp != 0 );

	extStorage = _extStorage;
	comparator = cmp;
}

void CAddIntentLatticeBuilder::SetResultLattice( CLattice& result )
{
	lattice = &result;

	lessConcept = 0;
}

void CAddIntentLatticeBuilder::SetPrunner( const CSharedPtr<IPatternPrunner>& _prunner )
{
	if( _prunner == 0 ) {
		prunner.reset();
	} else {
		prunner = _prunner;
	}
}

void CAddIntentLatticeBuilder::AddObject( DWORD objectNum, DWORD descriptionID )
{
	assert( descriptionID != -1 );

	assert( extStorage != 0 );
	assert( comparator != 0 );
	assert( lattice != 0 );

	if( lattice->Size() == 0 ) {
		TLatticeNodeId bottomNode = lattice->AddNewNode();
		getConcept( bottomNode ).Extent = -1; // -1 is empty read-only extent
		getConcept( bottomNode ).Intent = -1; // -1 is the most specific descriptor
	}
	processingObject = objectNum;

//	comparator->PreprocessObjectDescription( description );
	currObjDescrID = descriptionID;
	addIntent( descriptionID );
}

bool CAddIntentLatticeBuilder::FindMaxConcept(
	DWORD intent, TLatticeNodeId generatorConcept,
	TLatticeNodeId& maxGeneratorConcept )
{
	// Поднимаемся под диаграмме до тех пор пока есть предки generatorConcept, которые меньше intent.
	//	доказанo что если идти по любому пути верз результат будет один.
	bool areIntMaxGenIntEqual = false;
	maxGeneratorConcept = generatorConcept;
	bool parentIsMaximal = true;
	while( parentIsMaximal ) {
		parentIsMaximal = false;
		CLatticeNode& node = getNode( maxGeneratorConcept );
		CStdIterator<CEdges::const_iterator> parent( node.Parents );
		for( ; !parent.IsEnd(); ++parent ) {
			const TLatticeNodeId& parentNode = *parent;
			const TCompareResult rslt = comparator->Compare( intent, getConcept(parentNode).Intent, CR_MoreOrEqual );
			if( rslt == CR_Incomparable ) {
				continue;
			}
			maxGeneratorConcept = parentNode;
			areIntMaxGenIntEqual = rslt == CR_Equal;
			if( !areIntMaxGenIntEqual ) { // We need to search further.
				parentIsMaximal = true;
			}
			break;
		}
	}
	return areIntMaxGenIntEqual;
}

DWORD CAddIntentLatticeBuilder::FindGeneratedIntentByParent(
	DWORD intent, const TLatticeNodeId& parentCandidate )
{
	// The "equal" and "less" result are not possible because we are in canonical generator.
	const DWORD parentIntent = getConcept(parentCandidate).Intent;
	const bool isMore = comparator->Compare( parentIntent, intent, CR_MoreGeneral,
		CR_MoreGeneral | CR_Incomparable ) == CR_MoreGeneral;
	if( !isMore ) {
		return comparator->CalculateSimilarity( intent, parentIntent );
	} else {
		return -1;
	}
}

inline void CAddIntentLatticeBuilder::InitializeNewConcept(
	DWORD extent, DWORD intent,
	const TLatticeNodeId& node )
{
	getConcept(node).Intent = intent;
	getConcept(node).Extent = extent;
}

inline CLatticeConcept& CAddIntentLatticeBuilder::getConcept( TLatticeNodeId id )
{
	return lattice->GetNode( id ).Data;
}
inline CLatticeNode& CAddIntentLatticeBuilder::getNode( TLatticeNodeId id )
{
	return lattice->GetNode( id );
}

void CAddIntentLatticeBuilder::addIntent( DWORD description )
{
	statesForProcess.Clear();
	lessConcept = 0;

	addState( description, 0 );
	// Now there is only one state to process.
	processStates();

	if( lessConcept != 0 ) {
		addNewObjectToConceptTree( description, lessConcept );
		if( prunner != 0 ) {
			filterLattice();
		}
	}
}


// Add the next state to statesForProcess if neseccary.
// intent - the intent to be processed;
// generatorConcept - one of the generators for the intent;
void CAddIntentLatticeBuilder::addState( DWORD intent, TLatticeNodeId generatorConcept )
{
	TLatticeNodeId maxConceptGenerator = generatorConcept;
	const bool isEqual = FindMaxConcept( intent, generatorConcept, maxConceptGenerator );

	if( isEqual ) {
		addConcept( maxConceptGenerator );
		if( intent != getConcept(maxConceptGenerator).Intent // At least one of them should be preserved
			&& intent != currObjDescrID ) // External descriptions should not be tauched.
		{
			// i.e. different objects with the same meaning
			comparator->DeletePattern( intent );
		}
		return;
	}

	CAddIntentState newState;
	newState.MaxGeneratorConcept = maxConceptGenerator;
	newState.Intent = intent;

	// Copy parents to state.
	newState.ParentCandidates.Clear();
	CStdIterator<CEdges::iterator> i( getNode(maxConceptGenerator).Parents );
	for( ; !i.IsEnd(); ++i ) {
		newState.ParentCandidates.PushBack( *i );
	}
	statesForProcess.PushBack( newState );
}

// Add the result as  parent to the needed state
void CAddIntentLatticeBuilder::addConcept( TLatticeNodeId resultConcept )
{
 	if( statesForProcess.IsEmpty() ) {
		lessConcept = resultConcept;
		return;
	}

	statesForProcess.Back().ParentCandidates.PushBack( resultConcept );
}

// Process all possible states like in recursion.
void CAddIntentLatticeBuilder::processStates()
{
	while( !statesForProcess.IsEmpty() ) {
		CAddIntentState& currentState = statesForProcess.Back();

		if( currentState.ParentCandidates.Size() == 0 ) {
			// The concept is processed. Add to the lattice.
			addConceptToLattice();
			continue;
		}

		// Here is one iteration of cycle biginning on the line 08 of initial algorithm
		// Lines 09-11
		TLatticeNodeId parentCandidate = currentState.ParentCandidates.Back();
		currentState.ParentCandidates.PopBack();

		const DWORD generatedParentIntent = FindGeneratedIntentByParent( currentState.Intent, parentCandidate );
		if( generatedParentIntent != -1 ) {
			addState( generatedParentIntent, parentCandidate );
			continue;
		}

		// Check parentCandidate on influince with other parents of the new concept
		// Lines 12-24
		bool addParent = true;
		const CLatticeConcept& parentCandidateConcept = getConcept(parentCandidate);
		CLatticeNodesList::CIterator parent = currentState.NewParents.Begin();
		while( parent != currentState.NewParents.End() ) {
			const TCompareResult cmpCandidateToNewParent =
				comparator->Compare( parentCandidateConcept.Intent, getConcept(*parent).Intent );
			switch( cmpCandidateToNewParent ) {
				case CR_MoreGeneral:
				case CR_Equal:
					// There is the bond on the lattice.
					addParent = false;
					// Go to the next parrent
					++parent;
					break;
				case CR_Incomparable:
					// Go to the next parrent
					++parent;
					break;
				case CR_LessGeneral:
					// The parent in question is more general than new one.
					parent = currentState.NewParents.Erase( parent );
					break;
				default:
					assert( false );
			}
			if( !addParent ) {
				break;
			}
		}
		if( addParent ) {
			currentState.NewParents.PushBack( parentCandidate );
		}
	}
}

// Add new concept to lattice. All parents should be already added.
// Lines 25-32 of original algorithm
void CAddIntentLatticeBuilder::addConceptToLattice()
{
	CAddIntentState& currentState = statesForProcess.Back();
	TLatticeNodeId newNode = lattice->AddNewNode();
	InitializeNewConcept(
		extStorage->Clone(getConcept(currentState.MaxGeneratorConcept).Extent), currentState.Intent, newNode );

	// Now we should remove all links to new parents
	CLatticeNodesList::CConstIterator parent = currentState.NewParents.Begin();
	for( ; parent != currentState.NewParents.End(); ++parent ) {
		lattice->RemoveLink( *parent, currentState.MaxGeneratorConcept );
		lattice->AddLink( *parent, newNode );
	}
	lattice->AddLink( newNode, currentState.MaxGeneratorConcept );

	statesForProcess.PopBack();
	addConcept( newNode );
}

void CAddIntentLatticeBuilder::addNewObjectToConceptTree(
	DWORD newObjectDescription, TLatticeNodeId resultConcept )
{
	if( !extStorage->AddObject( processingObject, getConcept(resultConcept).Extent ) ) {
		return;
	}

	CStdIterator<CEdges::iterator> itr( getNode(resultConcept).Parents );
	for( ; !itr.IsEnd(); ++itr ) {
		TLatticeNodeId parent = *itr;
		addNewObjectToConceptTree( newObjectDescription, parent );
	}
}

void CAddIntentLatticeBuilder::filterLattice()
{
// TODO

//	assert( prunner != 0 );
//
//	CLatticeNodes& nodes = lattice->GetNodes();
//	CStdIterator<CLatticeNodes::CIterator, false> node( nodes );
//	for( ; !node.IsEnd(); ) {
//		TLatticeNodeId concept( node );
//		node++;
//		if( prunner->IsUseful( PS_PatternWithParents, concept->Extent, concept->Intent ) ) {
//			continue;
//		}
//		CStdIterator<CEdgeToNodeMap::iterator> itr( concept.ChildrenBegin(), concept.ChildrenEnd() );
//		for( ; !itr.IsEnd(); ++itr ) {
//			if( prunner->IsUseful( PS_PatternWithParents, (*itr)->Extent, (*itr)->Intent ) ){
//				break;
//			}
//		}
//		if( !itr.IsEnd() ) {
//			continue;
//		}
//		lattice->Delete( concept );
//	}
}

////////////////////////////////////////////////////////////////////

struct CCacheNode {
	// The object Num, which generates the Pattern with the corrsponding node.
	DWORD ObjNum;
	// The generated Pattern, if ZERO, then it was visited once.
	DWORD Pattern;

	CCacheNode() : ObjNum( -1 ), Pattern( 0 )
		{}
};

void CAddIntentLatticeBuilderExt::ProcessAllObjectsAddition()
{
	CStdIterator<CLatticeNodes::iterator> node( lattice->GetNodes() );
	for( ; !node.IsEnd(); ++node ) {
		delete reinterpret_cast<CCacheNode*>((*node).UserData);
	}
}

//CCacheNode* getCache( const TLatticeNodeId& parentNode )
//{
//	return reinterpret_cast<CCacheNode*>( parentNode.GetUserData() );
//}

bool CAddIntentLatticeBuilderExt::FindMaxConcept(
	DWORD intent, TLatticeNodeId generatorConcept,
	TLatticeNodeId& maxGeneratorConcept )
{
	assert( intent != 0 );
	// Go down-up manner to find the maximal concept less then the 'intent'
	//  It is proven that any way is aceptable.
	bool areIntMaxGenIntEqual = getConcept(generatorConcept).Intent != 0 &&
		comparator->Compare( intent, getConcept(generatorConcept).Intent, CR_Equal, CR_MoreOrEqual ) == CR_Equal;
	maxGeneratorConcept = generatorConcept;
	bool parentIsMaximal = !areIntMaxGenIntEqual;
	while( parentIsMaximal ) {
		parentIsMaximal = false;
		CStdIterator<CEdges::iterator> parent( getNode( maxGeneratorConcept ).Parents );
		for( ; !parent.IsEnd(); ++parent ) {
			const TLatticeNodeId& parentNode = *parent;
			TCompareResult rslt = isIntentMoreEqGeneral( intent, parentNode );
			if( rslt == CR_Incomparable ) {
				continue;
			}
			maxGeneratorConcept = parentNode;
			areIntMaxGenIntEqual = rslt == CR_Equal;
			if( !areIntMaxGenIntEqual ) { // We need to search further.
				parentIsMaximal = true;
			}
			break;
		}
	}
	return areIntMaxGenIntEqual;
}

DWORD CAddIntentLatticeBuilderExt::FindGeneratedIntentByParent(
	DWORD intent, const TLatticeNodeId& parentCandidateId )
{
	const CLatticeNode& parentCandidateNode = getNode(parentCandidateId);
	const CLatticeConcept& parentCandidate = parentCandidateNode.Data;
	CCacheNode* cache = reinterpret_cast<CCacheNode*>( parentCandidateNode.UserData );
	// The "equal" and "less" result are not possible because we are in canonical generator.
	if( cache != 0 && cache->ObjNum == processingObject && cache->Pattern != 0 ) {
		DWORD parentGenIntent = cache->Pattern;

		if( parentCandidate.Intent == parentGenIntent ) {
			// if intent is equal to its generator the pointers should be the same
			return -1;
		} else {
			assert( comparator->Compare( parentCandidate.Intent, intent, CR_MoreGeneral,
			CR_MoreGeneral | CR_Incomparable ) == CR_Incomparable );
			return parentGenIntent;
		}
	} else {
		assert( cache != 0 );
		cache->ObjNum = processingObject;
		cache->Pattern = 0;
		const bool isMore = comparator->Compare( parentCandidate.Intent, intent, CR_MoreGeneral,
			CR_MoreGeneral | CR_Incomparable ) == CR_MoreGeneral;
		if( !isMore ) {
			cache->Pattern = comparator->CalculateSimilarity( intent, parentCandidate.Intent );
			return cache->Pattern;
		} else {
			cache->Pattern = parentCandidate.Intent;
			return -1;
		}
	}
}

void CAddIntentLatticeBuilderExt::InitializeNewConcept(
	DWORD extent, DWORD intent,
	const TLatticeNodeId& nodeId )
{
	CLatticeNode& node = getNode( nodeId );
	node.Data.Extent = extent;
	node.Data.Intent = intent;
	node.UserData = new CCacheNode;
}

TCompareResult CAddIntentLatticeBuilderExt::isIntentMoreEqGeneral(
	DWORD intent, const TLatticeNodeId& parentNodeId )
{
	const CLatticeNode& parentNode = getNode(parentNodeId);
	CCacheNode* cache = reinterpret_cast<CCacheNode*>( parentNode.UserData );
	assert( cache != 0 );
	if( cache->ObjNum != processingObject ) {
		// First visit, just compare
		cache->ObjNum = processingObject;
		cache->Pattern = 0;
		return isIntentMoreEqGeneralSmpl( intent, parentNodeId );
	} else if ( cache->Pattern == 0 ) {
		return isIntentMoreEqGeneralSmpl( intent, parentNodeId );
	}

	// Not the first time in the concept, need the cache.
	return isIntentMoreEqGeneralExt( intent, parentNodeId );
}

//#define SIMPLE_CMP
#ifdef SIMPLE_CMP
TCompareResult CAddIntentLatticeBuilderExt::isIntentMoreEqGeneralSmpl(
	const CSharedPtr<IPatternDescriptor>& intent, const TLatticeNodeId& parent )
{
	return comparator->Compare( intent, parent->Intent, CR_MoreOrEqual );
}
#else
TCompareResult CAddIntentLatticeBuilderExt::isIntentMoreEqGeneralSmpl(
	DWORD intent, const TLatticeNodeId& parentNodeId )
{
	const CLatticeNode& parentNode = getNode(parentNodeId);
	const CLatticeConcept& parentConcept = parentNode.Data;
	CCacheNode* cache = reinterpret_cast<CCacheNode*>( parentNode.UserData );
	assert( cache != 0 );
	assert( cache->ObjNum == processingObject );

	const TCompareResult rslt = comparator->Compare( intent, parentConcept.Intent );
	if( rslt == CR_Incomparable ) {
		return CR_Incomparable;
	}
	if( rslt == CR_MoreGeneral ) {
		cache->Pattern = intent;
		return CR_MoreGeneral;
	}
	if ( rslt == CR_Equal ) {
		cache->Pattern = parentConcept.Intent;
		return CR_Equal;
	}

	assert( rslt == CR_LessGeneral );
	cache->Pattern = parentConcept.Intent;
	return CR_Incomparable;
}
#endif

TCompareResult CAddIntentLatticeBuilderExt::isIntentMoreEqGeneralExt(
	DWORD intent, const TLatticeNodeId& parentNodeId )
{
	const CLatticeNode& parentNode = getNode(parentNodeId);
	const CLatticeConcept& parentConcept = parentNode.Data;
	CCacheNode* cache = reinterpret_cast<CCacheNode*>( parentNode.UserData );
	assert( cache != 0 );
	assert( cache->ObjNum == processingObject );
	DWORD parIntent = parentConcept.Intent;

	if( cache->Pattern == 0 ) {
		// It is the second time we are in that concept, probably we need to cache it.
		// Creating cache...
		const TCompareResult rslt =	comparator->Compare( intent, parIntent );
		if( rslt == CR_Incomparable ) {
			cache->Pattern = comparator->CalculateSimilarity( intent, parIntent );
			return CR_Incomparable;
		} else if ( rslt == CR_MoreGeneral ) {
			cache->Pattern = intent;
			return CR_MoreGeneral;
		} else if ( rslt == CR_Equal ) {
			cache->Pattern = parIntent;
			return CR_Equal;
		} else {
			assert( rslt == CR_LessGeneral );
			cache->Pattern = parIntent;
			return CR_Incomparable;
		}
	}
	const TCompareResult rslt =
		comparator->Compare( intent, cache->Pattern, CR_Equal, CR_LessOrEqual );
	if( rslt != CR_Equal ) {
		assert( comparator->Compare( intent, parIntent, CR_MoreOrEqual ) == CR_Incomparable );
		return CR_Incomparable;
	}

	if( cache->Pattern == parIntent ) {
		assert( comparator->Compare( intent, parIntent, CR_Equal ) == CR_Equal );
		return CR_Equal;
	} else {
		return CR_MoreGeneral;
	}
}

////////////////////////////////////////////////////////////////////

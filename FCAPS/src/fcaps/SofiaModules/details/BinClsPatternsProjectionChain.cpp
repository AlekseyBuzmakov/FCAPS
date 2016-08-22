// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/SofiaModules/details/BinClsPatternsProjectionChain.h>

#include <fcaps/SharedModulesLib/BinarySetPatternManager.h>

#include <JSONTools.h>

#include <rapidjson/document.h>

using namespace std;

////////////////////////////////////////////////////////////////////
CBinClsPatternsProjectionChain::CBinClsPatternsProjectionChain() :
	extCmp(new CVectorBinarySetJoinComparator),
	extDeleter(extCmp),
	intCmp(new CBinarySetDescriptorsComparator),
	thld( 0 ),
	objCount( 0 ),
	currAttr(-1),
	order( AO_None ),
	useConditionalDB( false ),
	turnOnConditionalDB( false ),
	hasNewConcept( true ),
	noNewConceptProjectionCount(0)
{
	//ctor
}

const std::vector<std::string>& CBinClsPatternsProjectionChain::GetObjNames() const
{
	return extCmp->GetNames();
}
void CBinClsPatternsProjectionChain::SetObjNames( const std::vector<std::string>& names )
{
	extCmp->SetNames( names );
}

void CBinClsPatternsProjectionChain::UpdateInterestThreshold( const double& t )
{
	thld = t;
}

void CBinClsPatternsProjectionChain::AddObject( DWORD objectNum, const JSON& intent )
{
	CSharedPtr<const CBinarySetPatternDescriptor> p( intCmp->LoadObject( intent ), CPatternDeleter(intCmp) );
	if( tmpContext.size() <= objectNum ) {
		tmpContext.resize( objectNum + 1 );
	}
	tmpContext[objectNum] = p->GetAttribs();
}

bool CBinClsPatternsProjectionChain::AreEqual(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return extCmp->Compare(Pattern(p).Extent(),Pattern(q).Extent(), CR_Equal) != CR_Incomparable;
}
bool CBinClsPatternsProjectionChain::IsSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return extCmp->Compare(Pattern(p).Extent(),Pattern(q).Extent(), CR_LessGeneral) != CR_Incomparable;
}
bool CBinClsPatternsProjectionChain::IsTopoSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return Pattern(p).Extent().Size() >= Pattern(q).Extent().Size();
}
void CBinClsPatternsProjectionChain::FreePattern(const IPatternDescriptor* p ) const
{
	delete &Pattern(p);
}

void CBinClsPatternsProjectionChain::ComputeZeroProjection( CPatternList& ptrns )
{
	convertContext();
	currAttr = -1;
	initAttrOrder();

	CSharedPtr<CVectorBinarySetDescriptor> ptrn(extCmp->NewPattern(), extDeleter);
	for( DWORD i = 0; i < objCount; ++i ) {
		extCmp->AddValue(i,*ptrn);
	}
	ptrns.PushBack( NewPattern( ptrn ) );

	hasNewConcept = true;
}

bool CBinClsPatternsProjectionChain::NextProjection()
{
	if( turnOnConditionalDB ) {
		turnOnConditionalDB = false;
	}
	if(!hasNewConcept ) {
        ++noNewConceptProjectionCount;
	} else {
        noNewConceptProjectionCount = 0;
	}
	if( noNewConceptProjectionCount > attrOrder.size() / 10 && !useConditionalDB && false ) { // No conditional DB
		useConditionalDB = true;
		turnOnConditionalDB = true;
		cout << "\nTurning on conditional DB after Attr="<< currAttr << "\n";
	}
	hasNewConcept = false;

	++currAttr;
	return currAttr < attrOrder.size();
}
void CBinClsPatternsProjectionChain::Preimages(
	const IPatternDescriptor* d, CPatternList& preimages )
{
	assert(currAttr < attrOrder.size());

	const CPatternDescription* preimage = Preimage( Pattern(d) );
	if( preimage == 0 ) {
		return;
	}
	auto_ptr<const CPatternDescription> newPtrn( preimage );

	if( GetPatternInterest( newPtrn.get() ) >= thld ) {
		NewConceptCreated( *newPtrn );
		preimages.PushBack( newPtrn.release() );
	}
}

JSON CBinClsPatternsProjectionChain::SaveExtent( const IPatternDescriptor* d ) const
{
	return extCmp->SavePattern( &Pattern(d).Extent() );
}
JSON CBinClsPatternsProjectionChain::SaveIntent( const IPatternDescriptor* d ) const
{
	CList<DWORD> intent;
	computeIntent( Pattern(d), intent );
	CBinarySetPatternDescriptor intentDescr;
	intentDescr.AddList( intent );
	return intCmp->SavePattern( &intentDescr );
}

void CBinClsPatternsProjectionChain::LoadCommonParams( const JSON& json )
{
	CJsonError errorText;
	rapidjson::Document params;
	const bool res = ReadJsonString( json, params, errorText );
	assert( res );

	if( params.HasMember("IntentWritingMode") && params["IntentWritingMode"].IsString() ) {
        const string mode( params["IntentWritingMode"].GetString() );
        if( mode == "Indices" ) {
            intCmp->SetFlags( BSDC_UseInds );
        } else if( mode == "Names" ) {
            intCmp->SetFlags( BSDC_UseNames );
        } else if( mode == "Both" ) {
            intCmp->SetFlags( BSDC_UseNames | BSDC_UseInds );
        }
	}
	if( params.HasMember( "AttrNames" ) && params["AttrNames"].IsArray() ) {
		const rapidjson::Value& namesJson = params["AttrNames"];
		DWORD indsSize = namesJson.Size();

		vector<string> names;
		names.resize( indsSize );
		for( size_t i = 0; i < indsSize; ++i ) {
			if( namesJson[i].IsString() ) {
				names[i] = namesJson[i].GetString();
			}
		}
		intCmp->SetNames(names);
	}

}
JSON CBinClsPatternsProjectionChain::SaveCommonParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject();

	const DWORD f = intCmp->GetFlags();
	if( HasAllFlags( f, BSDC_UseInds|BSDC_UseNames ) ) {
        params.AddMember( "IntentWritingMode", "Both", alloc );
	} else if( HasAllFlags( f, BSDC_UseInds ) ) {
        params.AddMember( "IntentWritingMode", "Indices", alloc );
	} else if( HasAllFlags( f, BSDC_UseNames ) ) {
        params.AddMember( "IntentWritingMode", "Names", alloc );
	}
    const vector<string>& names=intCmp->GetNames();
	if( !names.empty() ) {
		params.AddMember( "AttrNames", rapidjson::Value().SetArray(), alloc );
		rapidjson::Value& namesJson = params["AttrNames"];
		namesJson.Reserve( names.size(), alloc );
		for( size_t i = 0; i < names.size(); ++i ) {
            namesJson.PushBack( rapidjson::Value().SetString( rapidjson::StringRef( names[i].c_str() ) ), alloc );
		}
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

CBinClsPatternsProjectionChain::CPatternDescription* CBinClsPatternsProjectionChain::NewPattern(const CSharedPtr<const CVectorBinarySetDescriptor>& ext)
{
	return new CPatternDescription(ext);
}
CBinClsPatternsProjectionChain::CPatternDescription* CBinClsPatternsProjectionChain::Preimage( const CPatternDescription& ptrn )
{
	if( turnOnConditionalDB ) {
		updateConditionalDB( ptrn );
	}

	// Removing unnecessry conditional info
	CStdIterator<CList<DWORD>::CIterator, false> itr( ptrn.AttrsInClosure() );
	while( !itr.IsEnd() && *itr < currAttr ) {
		CStdIterator<CList<DWORD>::CIterator, false> itrTmp = itr;
		++itr;
		ptrn.AttrsInClosure().Erase( itrTmp );
	}
	if( !itr.IsEnd() && *itr == currAttr ) {
		ptrn.Intent().PushBack( currAttr );
		return 0;
	}
	itr.Reset( ptrn.EmptyAttrs() );
	while( !itr.IsEnd() && *itr < currAttr ) {
		CStdIterator<CList<DWORD>::CIterator, false> itrTmp = itr;
		++itr;
		ptrn.EmptyAttrs().Erase( itrTmp );
	}
	if( !itr.IsEnd() && *itr == currAttr ) {
		return 0;
	}

	CSharedPtr<const CVectorBinarySetDescriptor> res(
		extCmp->CalculateSimilarity( ptrn.Extent(), *attrToTidsetMap[attrOrder[currAttr]] ), extDeleter );
	const DWORD extDiff = ptrn.Extent().Size() - res->Size();
	if(extDiff == 0 ) {
		ptrn.Intent().PushBack( currAttr );
		return 0;
	}

	auto_ptr<CPatternDescription> holder( NewPattern( res ) );

	holder->Intent()=ptrn.Intent();
	holder->Intent().PushBack(currAttr);

	holder->AttrsInClosure()=ptrn.AttrsInClosure();
	holder->EmptyAttrs()=ptrn.EmptyAttrs();

	return holder.release();
}

void CBinClsPatternsProjectionChain::NewConceptCreated( const CPatternDescription& p )
{
	hasNewConcept=true;
	updateConditionalDB(p);
}

// Converts a context from ObjToAttrs form to AttrToObjs form.
void CBinClsPatternsProjectionChain::convertContext()
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
void CBinClsPatternsProjectionChain::addColumnToTable(
	DWORD columnNum, const CList<DWORD>& values, CBinarySetCollection& table )
{
	AddColumnToCollection( extCmp, columnNum, values, table );
}

////////////////////////////////////////////////////////////////////
// CBinClsPatternsProjectionChain::initAttrOrder stuff
class CAttrSorter {
public:
	CAttrSorter( CBinClsPatternsProjectionChain::TAttributeOrder _order, const CBinarySetCollection& _attrToTidsetMap ) :
		order(_order), attrToTidsetMap( _attrToTidsetMap )
	{
		if( order == CBinClsPatternsProjectionChain::AO_Random ) {
			initRandomOrder();
		}
	}
	bool operator()( DWORD i1, DWORD i2 ) const
	{
		switch( order ) {
		case CBinClsPatternsProjectionChain::AO_None:
			return true;
		case CBinClsPatternsProjectionChain::AO_AscesndingSize:
			return attrToTidsetMap[i1]->Size() < attrToTidsetMap[i2]->Size();
		case CBinClsPatternsProjectionChain::AO_DescendingSize:
			return attrToTidsetMap[i1]->Size() > attrToTidsetMap[i2]->Size();
		case CBinClsPatternsProjectionChain::AO_Random:
			assert( i1 < randomOrder.size() );
			assert( i2 < randomOrder.size() );
            return randomOrder[i1] < randomOrder[i2];
			break;
		default:
			assert( false );
			return true;
		}
	}
private:
	const CBinClsPatternsProjectionChain::TAttributeOrder order;
	const CBinarySetCollection& attrToTidsetMap;
	vector<DWORD> randomOrder;

	void initRandomOrder();
};

void CAttrSorter::initRandomOrder() {
	randomOrder.resize( attrToTidsetMap.size(), -1 );
	for(DWORD i= 0; i < randomOrder.size(); ++i) {
		DWORD position = rand() % (randomOrder.size() - i);
		DWORD j = 0;
		for( ; position > 0; position--) {
			while( randomOrder[j] != -1 ) {
				++j;
				assert( j < randomOrder.size() );
			}
		}
		randomOrder[j] = i;
	}
}

// Initialize the order of attributes in which they are added to projections
void CBinClsPatternsProjectionChain::initAttrOrder() {
	attrOrder.resize( attrToTidsetMap.size() );
	for( DWORD i = 0; i < attrOrder.size(); ++i ) {
		attrOrder[i] = i;
	}

	if( order != AO_None ) {
		CAttrSorter sorter( order, attrToTidsetMap );
		sort( attrOrder.begin(), attrOrder.end(), sorter );
	}
}

// Updates the idices of conditional DB
void CBinClsPatternsProjectionChain::updateConditionalDB(const CPatternDescription& p)
{
	if( !useConditionalDB ) {
		return;
	}

	CStdIterator<CList<DWORD>::CIterator, false> nextInClosure( p.AttrsInClosure() );
	CStdIterator<CList<DWORD>::CIterator, false> nextEmpty( p.EmptyAttrs() );
	for( DWORD i = currAttr + 1; i < attrOrder.size(); ++i ) {
		if( !nextInClosure.IsEnd() && *nextInClosure == i ) {
			assert( nextEmpty.IsEnd() || *nextEmpty != i );
			++nextInClosure;
			continue;
		}
		if( !nextEmpty.IsEnd() && *nextEmpty == i ) {
			assert( nextInClosure.IsEnd() || *nextInClosure != i );
			++nextEmpty;
			continue;
		}
		CSharedPtr<const CVectorBinarySetDescriptor> res(
			extCmp->CalculateSimilarity( p.Extent(), *attrToTidsetMap[attrOrder[i]] ), extDeleter );
		ReportAttrSimilarity( p, i, *res );
		if( res->Size() < GetMinSupport() ) {
			// insert in Empty
			p.EmptyAttrs().Insert( nextEmpty, i );
		} else if (res->Size() == p.Extent().Size() ) {
			// insert in Closure
			p.AttrsInClosure().Insert( nextInClosure, i );
		}
	}
}
// Compute the intent for the concept.
void CBinClsPatternsProjectionChain::computeIntent(
	const CPatternDescription& d, CList<DWORD>& intent ) const
{
	for( DWORD attr = 0; attr < attrToTidsetMap.size() ; ++attr ) {
		if( extCmp->Compare( d.Extent(), *attrToTidsetMap[attr], CR_MoreGeneral | CR_Equal ) == CR_Incomparable ) {
			continue;
		}
		intent.PushBack( attr );
	}
}



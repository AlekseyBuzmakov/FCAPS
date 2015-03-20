#include <fcaps/modules/details/BinClsPatternsProjectionChain.h>

#include <fcaps/modules/BinarySetPatternManager.h>

using namespace std;

////////////////////////////////////////////////////////////////////

CBinClsPatternsProjectionChain::CBinClsPatternsProjectionChain() :
	extCmp(new CVectorBinarySetJoinComparator),
	extDeleter(extCmp),
	intCmp(new CBinarySetDescriptorsComparator),
	thld( 0 ),
	objCount( 0 ),
	currAttr(-1),
	order( AO_None )
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
}

bool CBinClsPatternsProjectionChain::NextProjection()
{
	++currAttr;
	return currAttr < attrOrder.size();
}
void CBinClsPatternsProjectionChain::Preimages(
	const IPatternDescriptor* d, CPatternList& preimages )
{
	assert(currAttr < attrOrder.size());

	auto_ptr<const CPatternDescription> newPtrn(Preimage( Pattern(d) ));

	if( GetPatternInterest( newPtrn.get() ) >= thld ) {
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

CBinClsPatternsProjectionChain::CPatternDescription* CBinClsPatternsProjectionChain::NewPattern(const CSharedPtr<const CVectorBinarySetDescriptor>& ext)
{
	return new CPatternDescription(ext);
}
CBinClsPatternsProjectionChain::CPatternDescription* CBinClsPatternsProjectionChain::Preimage( const CPatternDescription& p )
{
	CSharedPtr<const CVectorBinarySetDescriptor> res(
		extCmp->CalculateSimilarity( p.Extent(), *attrToTidsetMap[attrOrder[currAttr]] ), extDeleter );
	return NewPattern( res );
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
	CList<DWORD>::CConstReverseIterator i = values.RBegin();
	for( ; i!= values.REnd(); ++i ) {
		expandTable( *i, table );
		assert( table.size() > *i );
		extCmp->AddValue( columnNum, *table[*i] );
	}
}

// Increase size of the table if neccesary, to include at least minimalSize elements.
void CBinClsPatternsProjectionChain::expandTable( DWORD minimalSize, CBinarySetCollection& table )
{
	for( size_t i = table.size(); i <= minimalSize; ++i ) {
		table.push_back( CSharedPtr<CVectorBinarySetDescriptor>(
			extCmp->NewPattern(), extDeleter ) );
	}

	assert( table.size() > minimalSize );
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



#include <fcaps/modules/StabBinClsPatternsProjectionChain.h>

////////////////////////////////////////////////////////////////////

CModuleRegistrar<CStabBinClsPatternsProjectionChain> CStabBinClsPatternsProjectionChain::registar
	(ProjectionChainModuleType, StabBinClsPatternsProjectionChain);

CStabBinClsPatternsProjectionChain::CStabBinClsPatternsProjectionChain() :
	stabApprox( ExtCmp() )
{
	OrderType()=AO_DescendingSize;
	Thld()=5;
}

void CStabBinClsPatternsProjectionChain::UpdateInterestThreshold( const double& thld )
{
	CBinClsPatternsProjectionChain::UpdateInterestThreshold(thld);
	stabApprox.SetStableThreshold( thld );
}
double CStabBinClsPatternsProjectionChain::GetPatternInterest( const IPatternDescriptor* p )
{
	const CStabPatternDescription& ptrn = StabPattern(Pattern(p));
	if( ptrn.StabAttrNum() == CurrAttr()
		|| ptrn.StabAttrNum() == Order().size() - 1 )
	{
		return ptrn.Stability();
	} else {
		static const CList<DWORD> emptyIntent;
		stabApprox.InitComputation( ptrn.Extent(), emptyIntent );
		stabApprox.ComputeUpperBound();

		// Not more than concept.Extent->Size() ?
		ptrn.Stability()=stabApprox.GetStabilityRightLimit(),
		ptrn.StabAttrNum()=CurrAttr();
		ptrn.MinAttr() = stabApprox.GetMinDiffAttr();
		return ptrn.Stability();
	}
}
void CStabBinClsPatternsProjectionChain::ComputeZeroProjection( CPatternList& ptrns )
{
	CBinClsPatternsProjectionChain::ComputeZeroProjection(ptrns);
	const CStabPatternDescription& p = StabPattern(Pattern(ptrns.Front()));
	p.MinAttr()= -1;
	p.Stability() = ObjCount();
	p.StabAttrNum() = CurrAttr();

	stabApprox.SetContext( AttrToTidsetMap() );
	stabApprox.SetAttrOrder( &Order() );
}
bool CStabBinClsPatternsProjectionChain::NextProjection()
{
	const bool result = CBinClsPatternsProjectionChain::NextProjection();
	stabApprox.SetMaxAttrNumber( CurrAttr() + 1 );
	return result;
}
void CStabBinClsPatternsProjectionChain::Preimages( const IPatternDescriptor* d, CPatternList& preimages )
{
	assert(CurrAttr() < Order().size());

	const CStabPatternDescription& p = StabPattern(Pattern(d));
	std::auto_ptr<const CStabPatternDescription> newPtrn(
		&StabPattern(*Preimage( Pattern(d)) ) );
	assert(newPtrn.get() != 0 );

	const DWORD extDiff = p.Extent().Size() - newPtrn->Extent().Size();
	if( extDiff == 0 ) {
		p.StabAttrNum() = CurrAttr();
		return;
	}
	if( canBeStable( newPtrn->Extent(), p.MinAttr() ) ) {
		if( GetPatternInterest( newPtrn.get() ) >= Thld() ) {
			preimages.PushBack( newPtrn.release() );
		}
	}
	p.StabAttrNum() = CurrAttr();
	if( p.Stability() > extDiff ) {
		p.Stability() = extDiff;
		p.MinAttr() = CurrAttr();
	}
}

void CStabBinClsPatternsProjectionChain::LoadParams( const JSON& )
{

}
JSON CStabBinClsPatternsProjectionChain::SaveParams() const
{
	return JSON("")
		+ "{\"Type\":\"" + ProjectionChainModuleType + "\","
		+ "\"Name\":\"" + StabBinClsPatternsProjectionChain + "\"}";
}

const CStabBinClsPatternsProjectionChain::CStabPatternDescription&
CStabBinClsPatternsProjectionChain::StabPattern( const CPatternDescription& d)
{
	return debug_cast<const CStabPatternDescription&>(d);
}

bool CStabBinClsPatternsProjectionChain::canBeStable(
	const CVectorBinarySetDescriptor& extent, DWORD attr )
{
	if( attr == NotFound ) {
		return true;
	}
	//return true;
	CSharedPtr<const CVectorBinarySetDescriptor> tmp(
		ExtCmp().CalculateSimilarity( extent, *AttrToTidsetMap()[Order()[attr]] ), ExtDeleter() );

	const DWORD diff = tmp->Size() - extent.Size();
	return diff >= Thld();
}

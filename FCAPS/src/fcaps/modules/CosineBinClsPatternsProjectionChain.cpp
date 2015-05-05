#include "CosineBinClsPatternsProjectionChain.h"

CCosineBinClsPatternsProjectionChain::CCosineBinClsPatternsProjectionChain()
{
	Thld() = 0;
	OrderType() = AO_AscesndingSize;
}

void CCosineBinClsPatternsProjectionChain::ComputeZeroProjection( CPatternList& ptrns )
{
	computeItemValues();
	CBinClsPatternsProjectionChain::ComputeZeroProjection( ptrns );
}

double CCosineBinClsPatternsProjectionChain::GetPatternInterest( const IPatternDescriptor* p )
{
	const CPatternDescription& ptrn = Pattern( p );
	DWORD size = 0;
	double sum = 0;
	for( DWORD i = 0; i <= CurrAttr(); ++i ) {
		const TCompareResult rslt = ExtCmp().Compare(
			ptrn.Extent(), GetTidset( i ),
			CR_MoreOrEqual, CR_AllResults |CR_Incomparable );
		if( rslt == CR_Incomparable ) {
			continue;
		}
		++size;
		sum += itemValues[i];
	}
	assert (0 < size && size <= CurrAttr() );
	return ptrn.Extent().Size() / exp(sum/size);
}

void CCosineBinClsPatternsProjectionChain::computeItemValues()
{
	itemValues.resize( AttrToTidsetMap().size() );
	for( int i = 0; i < itemValues.size(); ++i ) {
		itemValues[i]=log( GetTidset(i).Size() );
	}
}

void CCosineBinClsPatternsProjectionChain::LoadParams( const JSON& )
{

}
JSON CCosineBinClsPatternsProjectionChain::SaveParams() const
{
	return JSON("{\"Type\":\"") + GetType() + "\","
		"\"Name\":\"" + GetName() + "\"}";
}

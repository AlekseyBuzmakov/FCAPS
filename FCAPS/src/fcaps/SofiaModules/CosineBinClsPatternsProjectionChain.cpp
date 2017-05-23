// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include "CosineBinClsPatternsProjectionChain.h"

////////////////////////////////////////////////////////////////////

CModuleRegistrar<CCosineBinClsPatternsProjectionChain> CCosineBinClsPatternsProjectionChain::registar(
	ProjectionChainModuleType, CosineBinClsPatternsProjectionChain );

CCosineBinClsPatternsProjectionChain::CCosineBinClsPatternsProjectionChain()
{
	Thld() = 0;
	OrderType() = AO_AscendingSize;
}

void CCosineBinClsPatternsProjectionChain::ComputeZeroProjection( CPatternList& ptrns )
{
	CBinClsPatternsProjectionChain::ComputeZeroProjection( ptrns );
	computeItemValues();
}

double CCosineBinClsPatternsProjectionChain::GetPatternInterest( const IPatternDescriptor* p )
{
	const CPatternDescription& ptrn = Pattern( p );
	DWORD size = 0;
	double sum = 0;
	for( DWORD i = 0; i < std::min( CurrAttr() + 1, (DWORD)Order().size() ); ++i ) {
		const TCompareResult rslt = ExtCmp().Compare(
			ptrn.Extent(), GetTidset( i ),
			CR_MoreOrEqual, CR_AllResults |CR_Incomparable );
		if( rslt == CR_Incomparable ) {
			continue;
		}
		++size;
		sum += itemValues[i];
	}
	assert ( size <= CurrAttr() + 1 );
	if( size == 0 ) {
		return 1;
	}
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

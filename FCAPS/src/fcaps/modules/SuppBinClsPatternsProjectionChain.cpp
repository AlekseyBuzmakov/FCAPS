#include <fcaps/modules/SuppBinClsPatternsProjectionChain.h>

////////////////////////////////////////////////////////////////////

CModuleRegistrar<CSuppBinClsPatternsProjectionChain> CSuppBinClsPatternsProjectionChain::registar
	(ProjectionChainType,SuppBinClsPatternsProjectionChain);

CSuppBinClsPatternsProjectionChain::CSuppBinClsPatternsProjectionChain()
{
	OrderType()=AO_DescendingSize;
	Thld()=0;
}
double CSuppBinClsPatternsProjectionChain::GetPatternInterest( const IPatternDescriptor* p )
{
	return Pattern(p).Extent().Size();
}
void CSuppBinClsPatternsProjectionChain::LoadParams( const JSON& )
{
}
JSON CSuppBinClsPatternsProjectionChain::SaveParams() const
{
	return JSON("")
		+ "{\"Type\":\"" + ProjectionChainType + "\","
		+ "\"Name\":\"" + SuppBinClsPatternsProjectionChain + "\"}";
}

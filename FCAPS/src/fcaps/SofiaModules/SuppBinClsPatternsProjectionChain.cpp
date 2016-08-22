// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/SofiaModules/SuppBinClsPatternsProjectionChain.h>

////////////////////////////////////////////////////////////////////

CModuleRegistrar<CSuppBinClsPatternsProjectionChain> CSuppBinClsPatternsProjectionChain::registar
	(ProjectionChainModuleType,SuppBinClsPatternsProjectionChain);

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
		+ "{\"Type\":\"" + ProjectionChainModuleType + "\","
		+ "\"Name\":\"" + SuppBinClsPatternsProjectionChain + "\"}";
}

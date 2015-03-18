#include <GeneralStringsPartialOrder.h>

////////////////////////////////////////////////////////////////////

void CGeneralStringsPartialOrderComparator::PreprocessStrElem( const TElem& el ) const
{
	assert( elemsCmp != 0 );
	elemsCmp->PreprocessObjectDescription( el );
}

TCompareResult CGeneralStringsPartialOrderComparator::CompareElems( DWORD cmpMask, const TElem& first, const TElem& last ) const
{
	assert( elemsCmp != 0 );
	return elemsCmp->Compare( first, last, cmpMask );
}

CGeneralStringsPartialOrderComparator::TElem CGeneralStringsPartialOrderComparator::CalculateElemsSimilarity( const TElem& first, const TElem& last ) const
{
	assert( elemsCmp != 0 );
	return elemsCmp->CalculateSimilarity( first, last );
}

bool CGeneralStringsPartialOrderComparator::IsMostGeneralElem( const TElem& el ) const
{
	assert( el != 0 );
	return el->IsMostGeneral();
}

void CGeneralStringsPartialOrderComparator::WriteElem( const TElem& el, std::ostream& dst ) const
{
	elemsCmp->Write( el, dst );
}

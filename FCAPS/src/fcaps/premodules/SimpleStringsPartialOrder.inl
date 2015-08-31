// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

////////////////////////////////////////////////////////////////////

#define DEBUG_INFO
#undef DEBUG_INFO

template<typename TElem>
void CStringsPartialOrderComparator<TElem>::PreprocessElement(
	 const CSharedPtr<IPartialOrderElement>& elInt, CElementSet& parts ) const
{
	assert( elInt!= 0 && elInt->GetType() == POT_SimpleStrings );
	CStringsPartialOrderElement<TElem>& el = debug_cast<CStringsPartialOrderElement<TElem>&>( *elInt );

#ifdef DEBUG_INFO
	std::cout << "StringsPartialOrderComparator<TElem>::PreprocessElement:\n";
	write( el.GetString(), std::cout );
	std::cout << "\n";
#endif // DEBUG_INFO

	// The minimal length projection
	if( el.GetString().size() < minStrLength ) {
		el.GetString().clear();
	}
	// Preprocessing of elements
	for( size_t i = 0; i < el.GetString().size(); ++i ) {
		PreprocessStrElem( el.GetString()[i] );
	}
#ifdef DEBUG_INFO
	write( el.GetString(), std::cout );
	std::cout << "\n";
#endif // DEBUG_INFO
	if( cutOnEmptyElems ) {
		splitString( el.GetString(), parts );
	}
}

template<typename TElem>
void CStringsPartialOrderComparator<TElem>::FindAllParents(
	const CSharedPtr<IPartialOrderElement>& el1, const CSharedPtr<IPartialOrderElement>& el2,
	CElementSet& parents ) const
{
	assert( el1 != 0 && el1->GetType() == POT_SimpleStrings );
	assert( el2 != 0 && el2->GetType() == POT_SimpleStrings );
	const CString& str1 = debug_cast<CStringsPartialOrderElement<TElem>&>( *el1 ).GetString();
	const CString& str2 = debug_cast<CStringsPartialOrderElement<TElem>&>( *el2 ).GetString();
	parents.Clear();

#ifdef DEBUG_INFO
	std::cout << "CStringsPartialOrderComparator<TElem>::FindAllParents:\n";
	write( str1, std::cout );
	std::cout << "\n";
	write( str2, std::cout );
	std::cout << "\nvvvvvvvvvvvv\n";
#endif // DEBUG_INFO

	const int str1size = str1.size();
	const int str2size = str2.size();
	const int lastIntersection = str1size - minStrLength;
	int j = -str2size + minStrLength;
	for( ; j <= lastIntersection; ++j ) {
		CSharedPtr< CStringsPartialOrderElement<TElem> > newElem( new CStringsPartialOrderElement<TElem> );
		CString& intersection = newElem->GetString();
		intersectStrings( str1, str2, j, intersection );
#ifdef DEBUG_INFO
		std::cout << j << ": ";
		write( intersection, std::cout );
		std::cout << "\n";
#endif // DEBUG_INFO
		if( intersection.size() < minStrLength ) {
			continue;
		}
		if( !cutOnEmptyElems ) {
			parents.PushBack( newElem );
		} else {
			splitString( intersection, parents );
		}
	}
}

template<typename TElem>
TCompareResult CStringsPartialOrderComparator<TElem>::Compare( DWORD interestingResults,
	const CSharedPtr<IPartialOrderElement>& first, const CSharedPtr<IPartialOrderElement>& second ) const
{
	assert( first != 0 && first->GetType() == POT_SimpleStrings );
	assert( second != 0 && second->GetType() == POT_SimpleStrings );
	const CString& str1 = debug_cast<CStringsPartialOrderElement<TElem>&>( *first ).GetString();
	const CString& str2 = debug_cast<CStringsPartialOrderElement<TElem>&>( *second ).GetString();

	if( str1.size() == str2.size() ) {
		DWORD possibleResult = interestingResults;
		for( DWORD i = 0; i < str1.size(); ++i ) {
			const TCompareResult rslt =
				CompareElems( possibleResult | CR_Equal, str1[i], str2[i] );
			switch( rslt ) {
				case CR_Incomparable:
					return CR_Incomparable;
				case CR_Equal:
					break;
				case CR_LessGeneral:
				case CR_MoreGeneral:
					possibleResult &= rslt;
					break;
				default:
					assert( false );
			}
			if( possibleResult == 0 ) {
				return CR_Incomparable;
			}
		}
		if( (possibleResult & CR_Equal) != 0 ) {
			return CR_Equal;
		}
		assert( possibleResult == CR_LessGeneral || possibleResult == CR_MoreGeneral );
		return static_cast<TCompareResult>( possibleResult );
	}

	////////////////////////////////////////////////////////////////////
	// str1.size() != str2.size()
	const bool isFirstSmaller = str1.size() < str2.size();
	if( isFirstSmaller && (interestingResults & CR_MoreGeneral) == 0
		|| !isFirstSmaller && (interestingResults & CR_LessGeneral) == 0 )
	{
		return CR_Incomparable;
	}
	const CString& small = isFirstSmaller ? str1 : str2;
	const CString& big = isFirstSmaller ? str2 : str1;
	const DWORD sizeDiff = big.size() - small.size();
	for( DWORD i = 0; i <= sizeDiff; ++i ) {
		DWORD j = 0;
		for( ; j < small.size(); ++j ) {
			const TCompareResult rslt = CompareElems(  CR_MoreOrEqual, small[j], big[i+j] );
			if( rslt == CR_Incomparable ) {
				break;
			}
		}
		if( j >= small.size() ) {
			return isFirstSmaller ? CR_MoreGeneral : CR_LessGeneral;
		}
	}
	return CR_Incomparable;
}

template<typename TElem>
void CStringsPartialOrderComparator<TElem>::Write( const CSharedPtr<IPartialOrderElement>& strInt, std::ostream& dst ) const
{
	assert( strInt != 0 && strInt->GetType() == POT_SimpleStrings );
	const CString& str = debug_cast<CStringsPartialOrderElement<TElem>&>( *strInt ).GetString();

	write( str, dst );
}

template<typename TElem>
void CStringsPartialOrderComparator<TElem>::intersectStrings(
	const CString& str1, const CString& str2, int str2Shift, CString& result ) const
{
	assert( str2Shift + str2.size() > 0 );
	const DWORD startIndex = std::max( 0, str2Shift );
	const DWORD endIndex = std::min( str2Shift + str2.size(), str1.size() );
	assert( endIndex - startIndex > 0 );
	result.reserve( endIndex-startIndex );
	DWORD firstMostGeneral = result.size();
	for( DWORD i = startIndex; i < endIndex; ++i ) {
		const TElem similarity = CalculateElemsSimilarity( str1[i], str2[i-str2Shift] );
		if( result.empty() && IsMostGeneralElem( similarity ) ) {
			continue;
		}
		result.push_back( similarity );
		if( !IsMostGeneralElem( similarity ) ) {
			firstMostGeneral = result.size();
		}
	}

	result.resize( firstMostGeneral );
}

template<typename TElem>
void CStringsPartialOrderComparator<TElem>::splitString( const CString& str, CElementSet& parents ) const
{
	CList<TElem> tmpStr;
	bool isFinish = false;
	for( DWORD i = 0; !isFinish ; ++i ) {
		isFinish = i >= str.size();
		if( !isFinish && !IsMostGeneralElem( str[i] ) ) {
			tmpStr.PushBack( str[i] );
		} else if( tmpStr.Size() >= minStrLength ) {
			CSharedPtr< CStringsPartialOrderElement<TElem> > next( new CStringsPartialOrderElement<TElem> );
			next->GetString().reserve( tmpStr.Size() );
			next->GetString().insert( next->GetString().begin(), tmpStr.Begin(), tmpStr.End() );
			parents.PushBack( next );
			tmpStr.Clear();
		} else {
			tmpStr.Clear();
		}
	}
}

template<typename TElem>
void CStringsPartialOrderComparator<TElem>::write( const CString& str, std::ostream& dst ) const
{
	CStdIterator<typename CString::const_iterator, true> elItr( str );
	DWORD counter = 0;
	TElem lastElement;

	dst << "< ";
	for( ; !elItr.IsEnd(); ++elItr ) {
		if( lastElement == 0 ) {
			WriteElem( *elItr, dst );
			lastElement = *elItr;
			counter = 1;
			continue;
		}
		const TCompareResult rslt = CompareElems( CR_Equal, lastElement, *elItr );
		if( rslt == CR_Equal ) {
			counter++;
		} else {
			if( counter > 1 ) {
				dst << "*" << counter;
			}
			dst << " ";
			WriteElem( *elItr, dst );
			lastElement = *elItr;
			counter = 1;
		}
	}
	if( counter > 1 ) {
		dst << "*" << counter;
	}
	dst << " >";
}

#undef DEBUG_INFO

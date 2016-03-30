// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

////////////////////////////////////////////////////////////////////

#define DEBUG_INFO
#undef DEBUG_INFO

template<typename TSymb>
JSON CStringsPartialOrderComparator<TSymb>::SaveElement( const IPartialOrderElement* e ) const
{
    assert( e != 0 );
    assert( strcmp( e->GetType(), GetElementType() ) == 0 );
    const CStringPartialOrderElement<TSymb>& el = dynamic_cast<const CStringPartialOrderElement<TSymb>&>( *e );

	CJsonError errorText;
	rapidjson::Document patternJSON;
	patternJSON.SetArray();
	patternJSON.Reserve( el.String().size(), patternJSON.GetAllocator() );
	for( size_t i = 0; i < el.String().size(); ++i ) {
        SaveSymb( el.String()[i], patternJSON, patternJSON.GetAllocator() );
	}

	assert( patternJSON.Size() == el.String().size() );

	JSON result;
	CreateStringFromJSON( patternJSON, result );
	return result;
}

template<typename TSymb>
void CStringsPartialOrderComparator<TSymb>::LoadElements( const JSON& json, CElementSet& parts )
{
	rapidjson::Document patternJSON;
	patternJSON.Parse( json.c_str() );
	if( !patternJSON.IsArray() ) {
		throw new CTextException( "CStringsPartialOrderComparator<TSymb>::LoadElements",
			std::string("The JSON is not an array") + "<<\n" + json + "\n>>" );
	}

	// The minimal length projection
	if( patternJSON.Size() < minStrLength ) {
        return;
	}
	// Loading elements
	CList<const CStringPartialOrderElement<TSymb>*> result;
	std::auto_ptr< CStringPartialOrderElement<TSymb> > nexTSymb( new CStringPartialOrderElement<TSymb> );
	for( size_t i = 0; i < patternJSON.Size(); ++i ) {
		const TSymb& el = LoadSymb( patternJSON[i] );
		if( cutOnEmptyElems && IsMostGeneralSymb( el ) ) {
            // We should cut on this element. Now check if the loaded str is needed
            if( nexTSymb->String().size() >= minStrLength ) {
                result.PushBack( nexTSymb.release() );
            }
            nexTSymb.reset( new CStringPartialOrderElement<TSymb> );
            continue;
		}
		nexTSymb->String().push_back( el );
	}
    if( nexTSymb->String().size() >= minStrLength ) {
        result.PushBack( nexTSymb.release() );
    }

    assert( parts.Count == 0 );
    parts.Elements=new const IPartialOrderElement*[result.Size()];
    parts.Count=result.Size();

    CStdIterator<typename CList<const CStringPartialOrderElement<TSymb>*>::CConstIterator, false> str( result );
    int i = 0;
    for(; !str.IsEnd(); ++str, ++i ) {
        parts.Elements[i] = *str;
    }
}

template<typename TSymb>
void CStringsPartialOrderComparator<TSymb>::FindAllParents(
    const IPartialOrderElement* first, const IPartialOrderElement* second,
	CElementSet& parents ) const
{
	assert( first != 0 && strcmp( first->GetType(), "SimpleStrings" ) == 0 );
	assert( second != 0 && strcmp( second->GetType(), "SimpleStrings" ) == 0 );
	assert( parents.Count == 0 );

	const CString& str1 = debug_cast<const CStringPartialOrderElement<TSymb>&>( *first ).String();
	const CString& str2 = debug_cast<const CStringPartialOrderElement<TSymb>&>( *second ).String();

#ifdef DEBUG_INFO
	std::cout << "CStringsPartialOrderComparator<TSymb>::FindAllParents:\n";
	std::cout << SaveElement( first );
	std::cout << "\n";
	std::cout << SaveElement( second );
	std::cout << "\nvvvvvvvvvvvv\n";
#endif // DEBUG_INFO

    CList<CStringPartialOrderElement<TSymb>*> tmpParents;

	const int str1size = str1.size();
	const int str2size = str2.size();
	const int lastIntersection = str1size - minStrLength;
	int j = -str2size + minStrLength;
	for( ; j <= lastIntersection; ++j ) {
		std::auto_ptr< CStringPartialOrderElement<TSymb> > newElem( new CStringPartialOrderElement<TSymb> );
		CString& intersection = newElem->String();
		intersectStrings( str1, str2, j, intersection );
#ifdef DEBUG_INFO
		std::cout << j << ": ";
        std::cout << SaveElement( newElem.get() );
		std::cout << "\n";
#endif // DEBUG_INFO
		if( intersection.size() < minStrLength ) {
			continue;
		}
		if( !cutOnEmptyElems ) {
			tmpParents.PushBack( newElem.release() );
		} else {
			splitString( intersection, tmpParents );
		}
	}
	parents.Count = tmpParents.Size();
	parents.Elements = new const IPartialOrderElement*[parents.Count];
	CStdIterator<typename CList<CStringPartialOrderElement<TSymb>*>::CConstIterator, false> p(tmpParents);
	for( int i = 0; !p.IsEnd(); ++p, ++i ) {
        parents.Elements[i]=*p;
	}
}

template<typename TSymb>
TCompareResult CStringsPartialOrderComparator<TSymb>::Compare( DWORD interestingResults,
    const IPartialOrderElement* first, const IPartialOrderElement* second ) const
{
	assert( first != 0 && strcmp( first->GetType(), "SimpleStrings" ) == 0 );
	assert( second != 0 && strcmp( second->GetType(), "SimpleStrings" ) == 0 );

	const CString& str1 = debug_cast<const CStringPartialOrderElement<TSymb>&>( *first ).String();
	const CString& str2 = debug_cast<const CStringPartialOrderElement<TSymb>&>( *second ).String();

	if( str1.size() == str2.size() ) {
		DWORD possibleResult = interestingResults;
		for( DWORD i = 0; i < str1.size(); ++i ) {
			const TCompareResult rslt =
				CompareSymbs( possibleResult | CR_Equal, str1[i], str2[i] );
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
			const TCompareResult rslt = CompareSymbs(  CR_MoreOrEqual, small[j], big[i+j] );
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

template<typename TSymb>
void CStringsPartialOrderComparator<TSymb>::intersectStrings(
	const CString& str1, const CString& str2, int str2Shift, CString& result ) const
{
	assert( str2Shift + str2.size() > 0 );
	const DWORD startIndex = std::max( 0, str2Shift );
	const DWORD endIndex = std::min( str2Shift + str2.size(), str1.size() );
	assert( endIndex - startIndex > 0 );
	result.reserve( endIndex-startIndex );
	DWORD firstMostGeneral = result.size();
	for( DWORD i = startIndex; i < endIndex; ++i ) {
		const TSymb similarity = CalculateSymbSimilarity( str1[i], str2[i-str2Shift] );
		if( result.empty() && IsMostGeneralSymb( similarity ) ) {
			continue;
		}
		result.push_back( similarity );
		if( !IsMostGeneralSymb( similarity ) ) {
			firstMostGeneral = result.size();
		}
	}

    // ?? Why?
	result.resize( firstMostGeneral );
}

template<typename TSymb>
void CStringsPartialOrderComparator<TSymb>::splitString( const CString& str,  CList<CStringPartialOrderElement<TSymb>*>& parents ) const
{
	CList<TSymb> tmpStr;
	bool isFinish = false;
	for( DWORD i = 0; !isFinish ; ++i ) {
		isFinish = i >= str.size();
		if( !isFinish && !IsMostGeneralSymb( str[i] ) ) {
			tmpStr.PushBack( str[i] );
		} else if( tmpStr.Size() >= minStrLength ) {
			std::auto_ptr< CStringPartialOrderElement<TSymb> > next( new CStringPartialOrderElement<TSymb> );
			next->String().reserve( tmpStr.Size() );
			next->String().insert( next->String().begin(), tmpStr.Begin(), tmpStr.End() );
			parents.PushBack( next.release() );
			tmpStr.Clear();
		} else {
			tmpStr.Clear();
		}
	}
}

#undef DEBUG_INFO

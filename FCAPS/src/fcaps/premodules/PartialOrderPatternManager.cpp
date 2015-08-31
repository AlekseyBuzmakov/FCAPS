// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <PartialOrderPatternManager.h>

//#define OUT_DEBUG_INFO

////////////////////////////////////////////////////////////////////

void CPartialOrderPatternDescriptor::AddElement(
	const CSharedPtr<IPartialOrderElement>& el )
{
	elements.PushBack( el );
}

////////////////////////////////////////////////////////////////////

void CPartialOrderPatternManager::PreprocessObjectDescription( const CSharedPtr<IPatternDescriptor>& desc ) const
{
	assert( desc != 0 && desc->GetType() == PT_PartialOrder );
	assert( elemsCmp != 0 );

	CPartialOrderPatternDescriptor& pattern = debug_cast<CPartialOrderPatternDescriptor&>( *desc );
	CStdIterator<CElementSet::CIterator, false> el( pattern.GetElements() );
	CElementSet parts;
	for( ; !el.IsEnd(); ) {
		CStdIterator<CElementSet::CIterator, false> curr = el;
		++el;
		elemsCmp->PreprocessElement( *curr, parts );
		if( (**curr).IsMostGeneral() ) {
			pattern.GetElements().Erase( curr );
		}
	}
	for( el.Reset( parts ); !el.IsEnd(); ++el ) {
		addElementToPattern( *el, pattern );
	}
}

CSharedPtr<IPatternDescriptor> CPartialOrderPatternManager::CalculateSimilarity(
	const CSharedPtr<IPatternDescriptor>& first, const CSharedPtr<IPatternDescriptor>& second ) const
{
	assert( first != 0 );
	assert( second != 0 );
	assert( first->GetType() == PT_PartialOrder );
	assert( second->GetType() == PT_PartialOrder );
	assert( elemsCmp != 0 );
	const CPartialOrderPatternDescriptor& ptrn1 = debug_cast<CPartialOrderPatternDescriptor&>( *first );
	const CPartialOrderPatternDescriptor& ptrn2 = debug_cast<CPartialOrderPatternDescriptor&>( *second );

	CSharedPtr<CPartialOrderPatternDescriptor> result( NewPattern() );

	CStdIterator<CElementSet::CConstIterator, false> el1( ptrn1.GetElements() );
	CStdIterator<CElementSet::CConstIterator, false> el2;
#ifdef OUT_DEBUG_INFO
	std::cout << "\n";
	Write( first, std::cout );
	Write( second, std::cout );
#endif // OUT_DEBUG_INFO
	DWORD el1Num = 0;
	for( ; !el1.IsEnd(); ++el1, ++el1Num ) {
		el2.Reset( ptrn2.GetElements() );
		for( ; !el2.IsEnd(); ++el2 ) {
			CElementSet parents;
			elemsCmp->FindAllParents( *el1, *el2, parents );
			CStdIterator<CElementSet::CConstIterator, false> parentEl( parents );
			for( ; !parentEl.IsEnd(); ++parentEl ) {
				addElementToPattern( *parentEl, *result );
			}
		}
	}

	return result;
}

TCompareResult CPartialOrderPatternManager::Compare(
	const CSharedPtr<IPatternDescriptor>& first, const CSharedPtr<IPatternDescriptor>& second,
	DWORD interestingResults, DWORD possibleResults ) const
{
	assert( first != 0 );
	assert( second != 0 );
	assert( first->GetType() == PT_PartialOrder );
	assert( second->GetType() == PT_PartialOrder );
	assert( elemsCmp != 0 );
	const CPartialOrderPatternDescriptor& ptrn1 = debug_cast<CPartialOrderPatternDescriptor&>( *first );
	const CPartialOrderPatternDescriptor& ptrn2 = debug_cast<CPartialOrderPatternDescriptor&>( *second );

	CElementSet elements1;
	CStdIterator<CElementSet::CConstIterator, false> ptrn1itr( ptrn1.GetElements() );
	for( ; !ptrn1itr.IsEnd(); ++ptrn1itr ) {
		elements1.PushBack( *ptrn1itr );
	}
	CElementSet elements2;
	CStdIterator<CElementSet::CConstIterator, false> ptrn2itr( ptrn2.GetElements() );
	for( ; !ptrn2itr.IsEnd(); ++ptrn2itr ) {
		elements2.PushBack( *ptrn2itr );
	}

	// Some knowledge about equality.
	DWORD possibleAnswers = possibleResults & interestingResults & CR_AllResults;
	if( ptrn1.GetElements().Size() != ptrn2.GetElements().Size() ) {
		possibleAnswers &= ~CR_Equal;
	}

	CStdIterator<CElementSet::CIterator, false> el1itr( elements1 );
	CStdIterator<CElementSet::CIterator, false> el2itr;

	for( ; !el1itr.IsEnd() && !elements2.IsEmpty(); ) {
		CStdIterator<CElementSet::CIterator, false> el1 = el1itr;
		++el1itr;
		el2itr.Reset( elements2 );
		for( ;!el2itr.IsEnd(); ) {
			CStdIterator<CElementSet::CIterator, false> el2 = el2itr;
			++el2itr;
			const TCompareResult elemsCmpRslt = elemsCmp->Compare( CR_AllResults, *el1, *el2 );

			switch( elemsCmpRslt ) {
				case CR_Incomparable:
					// Should continue find something.
					continue;
				case CR_LessGeneral:
					// el1 is more specific than el2,
					possibleAnswers &= CR_LessGeneral;
					if( possibleAnswers != 0 ) {
						// so, elements1 could be only less specific than eling2
						elements2.Erase( el2 );
						// we should find all other eling covered by el1.
						continue;
					}
					break;
				case CR_MoreGeneral:
					// el1 is less specific than el2
					possibleAnswers &= CR_MoreGeneral;
					elements1.Erase( el1 );
					break;
				case CR_Equal:
					elements1.Erase( el1 );
					elements2.Erase( el2 );
					break;
				default:
					assert( false );
			}
			// el2 is comparable to el1.
			break;
		}
		if( possibleAnswers == 0 ) {
			return CR_Incomparable;
		}
	}

	if( !elements1.IsEmpty() ) {
		possibleAnswers &= CR_LessGeneral;
	}
	if( !elements2.IsEmpty() ) {
		possibleAnswers &= CR_MoreGeneral;
	}

	if( possibleAnswers == 0 ) {
		return CR_Incomparable;
	}
	if( (possibleAnswers & CR_Equal) == CR_Equal ) {
		return CR_Equal;
	}
	assert( possibleAnswers == CR_Equal || possibleAnswers == CR_LessGeneral || possibleAnswers == CR_MoreGeneral );
	return static_cast<TCompareResult>( possibleAnswers );
}

void CPartialOrderPatternManager::Write( const CSharedPtr<IPatternDescriptor>& patternInt, std::ostream& dst ) const
{
	assert( patternInt != 0 );
	assert( patternInt->GetType() == PT_PartialOrder );
	assert( elemsCmp != 0 );
	const CPartialOrderPatternDescriptor& pattern = debug_cast<CPartialOrderPatternDescriptor&>( *patternInt );
	CStdIterator<CElementSet::CConstIterator, false> el( pattern.GetElements() );
	dst << "{\n";
	for( ; !el.IsEnd(); ++el ) {
		elemsCmp->Write( *el, dst );
		dst << "\n";
	}
	dst << "}";
}

void CPartialOrderPatternManager::Initialize( const CSharedPtr<IPartialOrderElementsComparator>& _elemsCmp )
{
	elemsCmp = _elemsCmp;
	assert( elemsCmp != 0 );
}

CPartialOrderPatternDescriptor* CPartialOrderPatternManager::NewPattern() const
{
	return new CPartialOrderPatternDescriptor;
}

void CPartialOrderPatternManager::addElementToPattern(
	const CSharedPtr<IPartialOrderElement>& el, CPartialOrderPatternDescriptor& ptrn ) const
{
	CElementSet::CIterator begin = ptrn.GetElements().Begin();
	CElementSet::CIterator end = ptrn.GetElements().End();
	bool theEnd = begin == end;
	--end;

	for( ;!theEnd; ) {
		theEnd = end == begin;
		CElementSet::CIterator patternEl = end;
		--end;

		const TCompareResult rslt = elemsCmp->Compare( CR_AllResults,  *patternEl, el );
		switch(rslt) {
			case CR_Incomparable:
				break;
			case CR_LessGeneral:
			case CR_Equal:
				// the pattern already has the eling
				return;
			case CR_MoreGeneral:
				ptrn.GetElements().Erase( patternEl );
				break;
			default:
				assert( false );
		}
	}
	ptrn.AddElement( el );
}

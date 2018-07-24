// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <AttrWithWeightPatternManager.h>

using namespace std;

void CAttrWithWeightPatternComparator::PreprocessObjectDescription( const IPatternDescriptor* desc ) const
{
	// Do Nothing
}

const IPatternDescriptor* CAttrWithWeightPatternComparator::CalculateSimilarity(
	const IPatternDescriptor* firstPtrn, const IPatternDescriptor* secondPtrn ) const
{
	assert( firstPtrn != 0 && firstPtrn->GetType() == PT_AttrWithWeight );
	assert( secondPtrn != 0 && secondPtrn->GetType() == PT_AttrWithWeight );

	const CAttrWithWeightPatternDescriptor& first = debug_cast<const CAttrWithWeightPatternDescriptor&>( *firstPtrn );
	const CAttrWithWeightPatternDescriptor& second = debug_cast<const CAttrWithWeightPatternDescriptor&>( *secondPtrn );

	unique_ptr<CAttrWithWeightPatternDescriptor> result( new CAttrWithWeightPatternDescriptor );
	if( first.GetAttr() != second.GetAttr() || first.GetWeight() == 0 || second.GetWeight() == 0 ) {
		result->SetWeight( 0 );
		return result.release();
	}
	result->SetAttr( first.GetAttr() );
	result->SetWeight( min( first.GetWeight(), second.GetWeight() ) );
	return result.release();
}

TCompareResult CAttrWithWeightPatternComparator::Compare(
	const IPatternDescriptor* firstPtrn,
	const IPatternDescriptor* secondPtrn,
	DWORD interestingResults, DWORD possibleResults ) const
{
	assert( firstPtrn != 0 && firstPtrn->GetType() == PT_AttrWithWeight );
	assert( secondPtrn != 0 && secondPtrn->GetType() == PT_AttrWithWeight );

	const CAttrWithWeightPatternDescriptor& first = debug_cast<const CAttrWithWeightPatternDescriptor&>( *firstPtrn );
	const CAttrWithWeightPatternDescriptor& second = debug_cast<const CAttrWithWeightPatternDescriptor&>( *secondPtrn );
	TCompareResult result = CR_Incomparable;
	if( first.GetWeight() == 0 || second.GetWeight() == 0 ) {
		if( first.GetWeight() == second.GetWeight() ) {
			result = CR_Equal;
		} else if( first.GetWeight() == 0 ) {
			result = CR_MoreGeneral;
		} else {
			result = CR_LessGeneral;
		}
	} else {
		if( first.GetAttr() != second.GetAttr() ) {
			result = CR_Incomparable;
		} else if( first.GetWeight() == second.GetWeight() ) {
			result = CR_Equal;
		} else if( first.GetWeight() < second.GetWeight() ) {
			result = CR_MoreGeneral;
		} else {
			result = CR_LessGeneral;
		}
	}
	if( (interestingResults & result) == 0 ) {
		return CR_Incomparable;
	} else {
		return result;
	}
}

void CAttrWithWeightPatternComparator::Write( const IPatternDescriptor* patternInt, std::ostream& dst ) const
{
	assert( patternInt != 0 && patternInt->GetType() == PT_AttrWithWeight );

	const CAttrWithWeightPatternDescriptor& pattern = debug_cast<const CAttrWithWeightPatternDescriptor&>( *patternInt );

	dst << pattern.GetAttr() << '(' << pattern.GetWeight() << ')';
}

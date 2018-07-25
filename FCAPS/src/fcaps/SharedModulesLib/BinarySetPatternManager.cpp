// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include "BinarySetPatternManager.h"

#include <JSONTools.h>
#include <rapidjson/document.h>

////////////////////////////////////////////////////////////////////

#include "ParallelListIteration.inl"

using namespace std;

////////////////////////////////////////////////////////////////////

CBinarySetPatternDescriptor::CBinarySetPatternDescriptor() :
	hash( 0 )
{
}

CBinarySetPatternDescriptor::~CBinarySetPatternDescriptor()
{

}

size_t CBinarySetPatternDescriptor::Hash() const
{
	if( hash != 0 ) {
		return hash;
	}
	CStdIterator<CList<DWORD>::CConstIterator, false> itr( attribsSet );
	char shift = 0;
	const char maxShift = sizeof( size_t ) << 3;
	for( ; !itr.IsEnd(); ++itr, shift = ((shift + 5) % maxShift )) {
		const DWORD value = *itr;
		const char headShift = shift + (sizeof(DWORD) << 3) - maxShift;
		hash ^= ((value << shift) | (headShift > 0 ? (value >> headShift) : 0 ) );
	}
	if( hash == 0 ) {
		// hash == 0 is not valable
		hash = 1;
	}
	return hash;
}

void CBinarySetPatternDescriptor::AddNextAttribNumber( DWORD attribNum )
{
	if( attribsSet.IsEmpty() ) {
		attribsSet.PushBack( attribNum );
		hash = 0;
		return;
	}
	if( attribNum == attribsSet.Back() ) {
//		assert( false ); // the same attribNum in the set;
		return;
	}
	if( attribNum > attribsSet.Back() ) {
		attribsSet.PushBack( attribNum );
		hash = 0;
		return;
	}
	CStdIterator<CList<DWORD>::CIterator, false> itr( attribsSet );
	for( ; !itr.IsEnd(); ++itr ) {
		if( *itr == attribNum ) {
//			assert( false ); // the same attribNum in the set;
			return;
		} else if ( attribNum < *itr ) {
			attribsSet.Insert( itr, attribNum );
			hash = 0;
			return;
		}
	}
	assert( false );
	return;
}

void CBinarySetPatternDescriptor::AddList( const CAttrsList& listToAdd )
{
	CAttrsList::CConstIterator i = listToAdd.Begin();
	for( ; i != listToAdd.End(); ++i ) {
		AddNextAttribNumber( *i );
	}
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
#define STR(x...) #x

#define PARAMS \
STR(\
	"$schema": "http://json-schema.org/draft-04/schema#",\
	"title": "Params of SOFIA Context processor",\
	"type": "object",\
	"properties": {\
		"AttrNames":{\
			"description": "A set of attribute names in the corresponding order",\
			"type":"array",\
			"items":{\
				"type":"string"\
			}\
		},\
		"UseInds":{\
			"description": "Should write pattern attributes by their indices",\
			"type":"boolean"\
		},\
		"UseNames":{\
			"description": "Should write pattern attributes by their names",\
			"type":"boolean"\
		}\
	}\
)

////////////////////////////////////////////////////////////////////
const char* const CBinarySetDescriptorsComparatorBase::ParamsDesc()
{
	return  PARAMS;
}

const char CBinarySetDescriptorsComparatorBase::jsonAttrNames[] = "AttrNames";
const char CBinarySetDescriptorsComparatorBase::jsonUseInds[] = "UseInds";
const char CBinarySetDescriptorsComparatorBase::jsonUseNames[] = "UseNames";
const char CBinarySetDescriptorsComparatorBase::jsonInds[] = "Inds";
const char CBinarySetDescriptorsComparatorBase::jsonNames[] = "Names";
const char CBinarySetDescriptorsComparatorBase::jsonCount[] = "Count";

CBinarySetDescriptorsComparatorBase::CBinarySetDescriptorsComparatorBase() :
	flags( BSDC_UseInds )
{

}

void CBinarySetDescriptorsComparatorBase::LoadParams( const JSON& json )
{
	rapidjson::Document dParams;
	CJsonError errorText;
	if( !ReadJsonString( json, dParams, errorText ) ) {
		throw new CJsonException( "CBinarySetDescriptorsComparator", errorText );
	}

	if( !dParams.HasMember("Params") || !dParams["Params"].IsObject() ) {
		return;
	}
	const rapidjson::Value& params = dParams["Params"];

	if( params.HasMember( jsonAttrNames ) && params[jsonAttrNames].IsArray() ) {
		const rapidjson::Value& namesJson = params[jsonAttrNames];
		DWORD indsSize = namesJson.Size();
		names.resize( indsSize );
		for( size_t i = 0; i < indsSize; ++i ) {
			if( namesJson[i].IsString() ) {
				names[i] = namesJson[i].GetString();
			}
		}
	}
	if( params.HasMember( jsonUseInds ) && params[jsonUseInds].IsBool() ) {
		if( params[jsonUseInds].GetBool()) {
			flags |= BSDC_UseInds;
		} else {
			flags &= ~BSDC_UseInds;
		}
	}
	if( params.HasMember( jsonUseNames ) && params[jsonUseNames].IsBool() ) {
		if( params[jsonUseNames].GetBool()) {
			flags |= BSDC_UseNames;
		} else {
			flags &= ~BSDC_UseNames;
		}
	}
}
JSON CBinarySetDescriptorsComparatorBase::SaveParams() const
{
	if( names.empty() ) {
		return "{}";
	}
	rapidjson::Document params;
	params.SetObject()
		.AddMember( "Type", PatternManagerModuleType, params.GetAllocator() )
		.AddMember( "Name", rapidjson::Value().SetString( rapidjson::StringRef( getModuleName() ) ), params.GetAllocator() )
		.AddMember( "Params", rapidjson::Value().SetObject(), params.GetAllocator() );
	params["Params"]
		.AddMember( jsonUseNames, rapidjson::Value().SetBool( HasAllFlags( flags, BSDC_UseNames ) ), params.GetAllocator() )
		.AddMember( jsonUseInds, rapidjson::Value().SetBool( HasAllFlags( flags, BSDC_UseInds ) ), params.GetAllocator() ) ;
	if( !names.empty() ) {
		params["Params"].AddMember( jsonAttrNames, rapidjson::Value().SetArray(), params.GetAllocator() );
		rapidjson::Value& namesJson = params["Params"][jsonAttrNames];
			;
		namesJson.Reserve( names.size(), params.GetAllocator() );
		for( size_t i = 0; i < names.size(); ++i ) {
			if( !names.empty() ) {
				namesJson.PushBack( rapidjson::Value().SetString( rapidjson::StringRef( names[i].c_str() ) ), params.GetAllocator() );
			} else {
				namesJson.PushBack( rapidjson::Value().SetNull(), params.GetAllocator() );
			}
		}
	}
	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

const CBinarySetPatternDescriptor* CBinarySetDescriptorsComparatorBase::LoadObject( const JSON& json )
{
	return LoadRWPattern( json );
}
JSON CBinarySetDescriptorsComparatorBase::SavePattern( const IPatternDescriptor* ptrn ) const
{
	return savePattern( ptrn );
}
const CBinarySetPatternDescriptor* CBinarySetDescriptorsComparatorBase::LoadPattern( const JSON& json )
{
	return LoadRWPattern( json );
}

TCompareResult CBinarySetDescriptorsComparatorBase::Compare(
	const IPatternDescriptor* firstPattern,
	const IPatternDescriptor* secondPattern,
	DWORD interestingResults, DWORD possibleResults )
{
	assert( firstPattern != 0 && firstPattern->GetType() == PT_BinarySet  );
	assert( secondPattern!= 0 && secondPattern->GetType() == PT_BinarySet );

	const CBinarySetPatternDescriptor& first = debug_cast<const CBinarySetPatternDescriptor&>( *firstPattern );
	const CBinarySetPatternDescriptor& second = debug_cast<const CBinarySetPatternDescriptor&>( *secondPattern );

	return Compare( first, second, interestingResults, possibleResults );
}

void CBinarySetDescriptorsComparatorBase::FreePattern( const IPatternDescriptor * patternInt )
{
	assert( patternInt != 0 && patternInt->GetType() == PT_BinarySet );
	const CBinarySetPatternDescriptor& pattern = debug_cast<const CBinarySetPatternDescriptor&>( *patternInt );
	delete &pattern;
}

void CBinarySetDescriptorsComparatorBase::Write( const IPatternDescriptor* patternInt, std::ostream& dst ) const
{
	assert( patternInt != 0 && patternInt->GetType() == PT_BinarySet );

	const CBinarySetPatternDescriptor& pattern = debug_cast<const CBinarySetPatternDescriptor&>( *patternInt );
	dst << "{";
	CStdIterator<CBinarySetPatternDescriptor::CAttrsList::CConstIterator, false> subElItr( pattern.GetAttribs() );
	bool isFirstSubEl = true;
	for( ; !subElItr.IsEnd(); ++subElItr ) {
		if( !isFirstSubEl ) {
			dst << ",";
		}
		const DWORD& el = *subElItr;
		if( el < names.size() ) {
			dst << names[el];
		} else {
			dst << el;
		}
		isFirstSubEl = false;
	}
	dst << "}";
}

inline CBinarySetPatternDescriptor* CBinarySetDescriptorsComparatorBase::UnionSets(
	 const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const
{
	CBinarySetPatternDescriptor* result = NewPattern();
	unique_ptr<CBinarySetPatternDescriptor> resultPattern( result );

	PARALLEL_LIST_ITERATION_BEGIN( CBinarySetPatternDescriptor::CAttrsList, CBinarySetPatternDescriptor::CAttrsList::CConstIterator, first.GetAttribs(), second.GetAttribs(), firstIter, lastIter );
		result->AddSortedNextAttribNumber( *firstIter < *lastIter ? *firstIter : *lastIter );
	PARALLEL_LIST_ITERATION_END( firstIter, lastIter );
	for( ; firstIter != first.GetAttribs().End(); ++firstIter ) {
		result->AddSortedNextAttribNumber( *firstIter );
	}
	for( ; lastIter != second.GetAttribs().End(); ++lastIter ) {
		result->AddSortedNextAttribNumber( *lastIter );
	}

	return resultPattern.release();
}

CBinarySetPatternDescriptor* CBinarySetDescriptorsComparatorBase::IntersectSets(
	 const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const
{
	CBinarySetPatternDescriptor* result = NewPattern();
	unique_ptr<CBinarySetPatternDescriptor> resultPattern( result );
	PARALLEL_LIST_ITERATION_BEGIN( CBinarySetPatternDescriptor::CAttrsList, CBinarySetPatternDescriptor::CAttrsList::CConstIterator, first.GetAttribs(), second.GetAttribs(), firstIter, lastIter );
		if( *firstIter == *lastIter ) {
			result->AddSortedNextAttribNumber( *firstIter );
		}
	PARALLEL_LIST_ITERATION_END( firstIter, lastIter );
	return resultPattern.release();
}

inline bool CBinarySetDescriptorsComparatorBase::IsEqualSets(
	 const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second  ) const
{
	PARALLEL_LIST_ITERATION_BEGIN( CBinarySetPatternDescriptor::CAttrsList, CBinarySetPatternDescriptor::CAttrsList::CConstIterator, first.GetAttribs(), second.GetAttribs(), firstItr, lastItr );
		if( *firstItr != *lastItr ) {
			return false;
		}
	PARALLEL_LIST_ITERATION_END( firstItr, lastItr );
	return true;
}

inline bool CBinarySetDescriptorsComparatorBase::IsSubsetOf(
	 const CBinarySetPatternDescriptor& subset, const CBinarySetPatternDescriptor& set ) const
{
	PARALLEL_LIST_ITERATION_BEGIN( CBinarySetPatternDescriptor::CAttrsList, CBinarySetPatternDescriptor::CAttrsList::CConstIterator, subset.GetAttribs(), set.GetAttribs(), subsetItr, setItr );
		if( *setItr > *subsetItr ) {
			return false;
		}
	PARALLEL_LIST_ITERATION_END( subsetItr, setItr );
	return subsetItr == subset.GetAttribs().End();
}

CBinarySetPatternDescriptor* CBinarySetDescriptorsComparatorBase::LoadRWPattern( const JSON& json )
{
	rapidjson::Document patternJson;
	CJsonError errorText;
	const bool hasError = ReadJsonString( json, patternJson, errorText );
	assert( hasError );
	if( !patternJson.HasMember( jsonInds ) || !patternJson[jsonInds].IsArray() ) {
		throw new CTextException( "BinarySetDescriptorsComparator",
			string( "No 'Inds' member found in pattern description <<\n" ) + json + "\n>>" );
	}

	unique_ptr<CBinarySetPatternDescriptor> pattern( NewPattern() );
	assert( pattern.get() != 0 );

	const rapidjson::Value& indsJson = patternJson[jsonInds];
	DWORD indsSize = indsJson.Size();
	for( size_t i = 0; i < indsSize; ++i ) {
		if( !indsJson[i].IsUint() ) {
			return 0;
		}
		pattern->AddNextAttribNumber( indsJson[i].GetUint() );
	}
	return pattern.release();
}

JSON CBinarySetDescriptorsComparatorBase::savePattern( const IPatternDescriptor* ptrn ) const
{
	assert( ptrn != 0 && ptrn->GetType() == PT_BinarySet );

	const CBinarySetPatternDescriptor& pattern = debug_cast<const CBinarySetPatternDescriptor&>( *ptrn );
	const CBinarySetPatternDescriptor::CAttrsList& attrs = pattern.GetAttribs();

	rapidjson::Document patternJson;

	patternJson.SetObject();
	patternJson
		.AddMember( jsonCount, rapidjson::Value().SetUint( attrs.Size() ), patternJson.GetAllocator() );


	if( HasAllFlags( flags, BSDC_UseInds ) ) {
		patternJson
			.AddMember( jsonInds, rapidjson::Value().SetArray(), patternJson.GetAllocator() );
		rapidjson::Value& indsJson = patternJson[jsonInds];
		indsJson.Reserve( attrs.Size(), patternJson.GetAllocator() );

		CStdIterator<CBinarySetPatternDescriptor::CAttrsList::CConstIterator, false> itr( attrs );
		for( ; !itr.IsEnd(); ++itr ) {
			indsJson.PushBack( rapidjson::Value().SetUint( *itr ), patternJson.GetAllocator() );
		}
	}
	if( HasAllFlags( flags, BSDC_UseNames ) && !names.empty() ) {
		patternJson
			.AddMember( jsonNames, rapidjson::Value().SetArray(), patternJson.GetAllocator() );
		rapidjson::Value& namesJson = patternJson[jsonNames];
		namesJson.Reserve( attrs.Size(), patternJson.GetAllocator() );

		CStdIterator<CBinarySetPatternDescriptor::CAttrsList::CConstIterator, false> itr( attrs );
		for( ; !itr.IsEnd(); ++itr ) {
			if( *itr < names.size() ) {
				namesJson.PushBack( rapidjson::Value().SetString(
					rapidjson::StringRef( names[*itr].c_str() ) ), patternJson.GetAllocator() );
			} else {
				namesJson.PushBack( rapidjson::Value().SetUint( *itr ), patternJson.GetAllocator() );
			}
		}
	}
	JSON result;
	CreateStringFromJSON( patternJson, result );
	return result;
}

////////////////////////////////////////////////////////////////////

const char* const CBinarySetDescriptorsComparator::Desc()
{
	return STR({
		"Name":"Join Binary Set Pattern Manager",
		"Description":"Defines a pattern manager based on binary attributes and intersection as similarity operation",
		"Params":{
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of SOFIA Context processor",
			"type": "object",
			"properties": {
				) PARAMS STR(
			}
		}
	});
}

const CBinarySetPatternDescriptor* CBinarySetDescriptorsComparator::CalculateSimilarity(
	const IPatternDescriptor* firstPattern, const IPatternDescriptor* secondPattern )
{
	assert( firstPattern != 0 && firstPattern->GetType() == PT_BinarySet  );
	assert( secondPattern!= 0 && secondPattern->GetType() == PT_BinarySet );

	const CBinarySetPatternDescriptor& first = debug_cast<const CBinarySetPatternDescriptor&>( *firstPattern );
	const CBinarySetPatternDescriptor& second = debug_cast<const CBinarySetPatternDescriptor&>( *secondPattern );


	return IntersectSets( first, second );
}

// Check patterns on equality.
bool CBinarySetDescriptorsComparator::isEqual(
	const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const
{
	return IsEqualSets( first, second );
}

// Check whether first pattern less general than second one.
bool CBinarySetDescriptorsComparator::isLessGeneral(
	const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const
{
	return IsSubsetOf( second, first );
}

TCompareResult CBinarySetDescriptorsComparator::fastCompare( DWORD firstSize, DWORD secondSize ) const
{
	if( firstSize == secondSize ) {
		return CR_Equal;
	} else if ( firstSize > secondSize ) {
		return CR_LessGeneral;
	} else {
		return CR_MoreGeneral;
	}
}

////////////////////////////////////////////////////////////////////

const char* const CUnionBinarySetDescriptorsComparator::Desc()
{
	return STR({
		"Name":"Union Binary Set Pattern Manager",
		"Description":"Defines a pattern manager based on binary attributes and union as the similarity operation",
		"Params":{
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of SOFIA Context processor",
			"type": "object",
			"properties": {
				) PARAMS STR(
			}
		}
	});
}
const CBinarySetPatternDescriptor* CUnionBinarySetDescriptorsComparator::CalculateSimilarity(
	const IPatternDescriptor* firstPattern, const IPatternDescriptor* secondPattern )
{
	assert( firstPattern->GetType() == PT_BinarySet && firstPattern->GetType() == PT_BinarySet );

	const CBinarySetPatternDescriptor& first = debug_cast<const CBinarySetPatternDescriptor&>( *firstPattern );
	const CBinarySetPatternDescriptor& second = debug_cast<const CBinarySetPatternDescriptor&>( *secondPattern );


	return UnionSets( first, second );
}

// Check patterns on equality.
bool CUnionBinarySetDescriptorsComparator::isEqual(
	const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const
{
	return IsEqualSets( first, second );
}

// Check whether first pattern less general than second one.
bool CUnionBinarySetDescriptorsComparator::isLessGeneral(
	const CBinarySetPatternDescriptor& first, const CBinarySetPatternDescriptor& second ) const
{
	return IsSubsetOf( first, second );
}

TCompareResult CUnionBinarySetDescriptorsComparator::fastCompare( DWORD firstSize, DWORD secondSize ) const
{
	if( firstSize == secondSize ) {
		return CR_Equal;
	} else if ( firstSize > secondSize ) {
		return CR_MoreGeneral;
	} else {
		return CR_LessGeneral;
	}
}

////////////////////////////////////////////////////////////////////

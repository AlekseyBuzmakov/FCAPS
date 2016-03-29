// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include "PartialOrderPatternManager.h"

#include <fcaps/ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/document.h>

using namespace std;

//#define OUT_DEBUG_INFO

////////////////////////////////////////////////////////////////////

void CPartialOrderPatternDescriptor::AddElement(
	const IPartialOrderElement* el )
{
    elements.PushBack( el );
}

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CPartialOrderPatternManager> CPartialOrderPatternManager::registrar( PatternManagerModuleType, PartialOrderPatternManager );

const CPartialOrderPatternDescriptor* CPartialOrderPatternManager::LoadObject( const JSON& json )
{
    return LoadPattern( json );
}
JSON CPartialOrderPatternManager::SavePattern( const IPatternDescriptor* p ) const
{
    assert( p != 0 );
    assert( p->GetType() == PT_PartialOrder );
    const CPartialOrderPatternDescriptor& d = debug_cast<const CPartialOrderPatternDescriptor&>( *p );
    JSON result;
    result += "[";
    CStdIterator<CElementSet::CConstIterator, false> itr( d.GetElements() );
    bool isFirst = true;
    for( ; !itr.IsEnd(); ++itr ) {
        if( !isFirst) {
            result += ",";
        }
        JSON elJson = elemsCmp->SaveElement( *itr );
        result += elJson;

        isFirst = false;
    }
    result += "]";
    return result;
}
const CPartialOrderPatternDescriptor* CPartialOrderPatternManager::LoadPattern( const JSON& json )
{
	rapidjson::Document ptrnJSON;
	ptrnJSON.Parse( json.c_str() );
	if( !ptrnJSON.IsArray() ) {
		throw new CTextException( "CPartialOrderPatternManager::LoadPattern",
			string("The JSON is not an array (\"") + json + "\")" );
	}

	auto_ptr<CPartialOrderPatternDescriptor> result( new CPartialOrderPatternDescriptor( elemsCmp ) );
	for( size_t i = 0; i < ptrnJSON.Size(); ++i ) {
		JSON elemJson;
		CreateStringFromJSON( ptrnJSON[i], elemJson );
		IPartialOrderElementsComparator::CElementSet elems;
		elemsCmp->LoadElements( elemJson, elems );

        for( int i = 0; i < elems.Count; ++i ) {
            if( !addElementToPattern( elems.Elements[i], *result ) ) {
                elemsCmp->FreeElement( elems.Elements[i] );
            };
        }
        elemsCmp->FreeElementSet( elems );
	}

	return result.release();
}

const CPartialOrderPatternDescriptor* CPartialOrderPatternManager::CalculateSimilarity(
	const IPatternDescriptor* first, const IPatternDescriptor* second )
{
	assert( first != 0 );
	assert( second != 0 );
	assert( first->GetType() == PT_PartialOrder );
	assert( second->GetType() == PT_PartialOrder );
	assert( elemsCmp != 0 );
	const CPartialOrderPatternDescriptor& ptrn1 = debug_cast<const CPartialOrderPatternDescriptor&>( *first );
	const CPartialOrderPatternDescriptor& ptrn2 = debug_cast<const CPartialOrderPatternDescriptor&>( *second );

	auto_ptr<CPartialOrderPatternDescriptor> result( NewPattern() );

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
			IPartialOrderElementsComparator::CElementSet parents;
			elemsCmp->FindAllParents( *el1, *el2, parents );
			for( int p = 0; p < parents.Count; ++p ) {
				addElementToPattern( parents.Elements[p], *result );
			}
		}
	}

	return result.release();
}

TCompareResult CPartialOrderPatternManager::Compare(
	const IPatternDescriptor* first, const IPatternDescriptor* second,
	DWORD interestingResults, DWORD possibleResults )
{
	assert( first != 0 );
	assert( second != 0 );
	assert( first->GetType() == PT_PartialOrder );
	assert( second->GetType() == PT_PartialOrder );
	assert( elemsCmp != 0 );
	const CPartialOrderPatternDescriptor& ptrn1 = debug_cast<const CPartialOrderPatternDescriptor&>( *first );
	const CPartialOrderPatternDescriptor& ptrn2 = debug_cast<const CPartialOrderPatternDescriptor&>( *second );

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

void CPartialOrderPatternManager::FreePattern( const IPatternDescriptor* p )
{
    delete p;
}

void CPartialOrderPatternManager::Write( const IPatternDescriptor* pattern, std::ostream& dst ) const
{
    dst << SavePattern( pattern );
}

void CPartialOrderPatternManager::LoadParams( const JSON& json )
{
	CJsonError errorText;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "CPartialOrderPatternManager::LoadParams", errorText );
	}
	assert( string( params["Type"].GetString() ) == PatternManagerModuleType );
	assert( string( params["Name"].GetString() ) == PartialOrderPatternManager );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()
		&& params["Params"].HasMember( "PartialOrder" ) && params["Params"]["PartialOrder"].IsObject() ) )
    {
		throw new CTextException( "CPartialOrderPatternManager::LoadParams",
			"No PartialOrder module found in params <<\n" + json + "\n>>");
	}

	const rapidjson::Value& po = params["Params"]["PartialOrder"];
	elemsCmp.reset(dynamic_cast<IPartialOrderElementsComparator*>( CreateModuleFromJSON( po, errorText.Error ) ));
	if( elemsCmp == 0 ) {
        errorText.Data = json;
        errorText.Offset = NotFound;
        throw new CJsonException("CPartialOrderPatternManager::LoadParams", errorText );
	}
}
JSON CPartialOrderPatternManager::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", PatternManagerModuleType, alloc )
		.AddMember( "Name", PartialOrderPatternManager, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject(),
		alloc );
	rapidjson::Value& paramsDoc = params["Params"];

    const IModule& module = dynamic_cast<const IModule&>( *elemsCmp );
    rapidjson::Document internalParams;
    internalParams.Parse( module.SaveParams().c_str() );
    paramsDoc.AddMember( "PartialOrder", internalParams.Move(), alloc );

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

void CPartialOrderPatternManager::Initialize( const CSharedPtr<IPartialOrderElementsComparator>& _elemsCmp )
{
	elemsCmp = _elemsCmp;
	assert( elemsCmp != 0 );
}

CPartialOrderPatternDescriptor* CPartialOrderPatternManager::NewPattern() const
{
	return new CPartialOrderPatternDescriptor( elemsCmp );
}

// Returns if pattern was added.
bool CPartialOrderPatternManager::addElementToPattern(
	const IPartialOrderElement* el, CPartialOrderPatternDescriptor& ptrn ) const
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
				return false;
			case CR_MoreGeneral:
				ptrn.GetElements().Erase( patternEl );
				break;
			default:
				assert( false );
		}
	}
	ptrn.AddElement( el );
	return true;
}

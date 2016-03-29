// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include "SimpleStringsPartialOrder.h"

///////////////////////////////////////////////////////////////

struct CParams {
    // Minimal allowed length of the string.
    DWORD MinStrLength;
    // Should cut a string an empty elements, e.g., after a projection.
    bool CutOnEmptySymbs;

    CParams() :
        MinStrLength( 0 ), CutOnEmptySymbs( true ) {}
    CParams( DWORD minLength, bool cut ) :
        MinStrLength( minLength ), CutOnEmptySymbs( cut ) {}
};

void loadParams( const JSON& json, rapidjson::Document& params, CParams& result )
{
	CJsonError errorText;
    if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "CStringPartialOrderComparator::LoadParams", errorText );
	}
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject() ) ) {
        return;
	}
	const rapidjson::Value& paramsObj = params["Params"];
	if( paramsObj.HasMember( "MinStrLength") && paramsObj["MinStrLength"].IsUint() ) {
        result.MinStrLength = paramsObj["MinStrLength"].GetUint();
	}
	if( paramsObj.HasMember( "CutOnEmptySymbs") && paramsObj["CutOnEmptySymbs"].IsBool() ) {
        result.CutOnEmptySymbs = paramsObj["CutOnEmptySymbs"].GetBool();
	}
}

void saveParams( const CParams& params, rapidjson::Document& doc )
{
	rapidjson::MemoryPoolAllocator<>& alloc = doc.GetAllocator();
	doc.SetObject()
		.AddMember( "Type", PartialOrderElementsComparatorModuleType, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "MinStrLength", rapidjson::Value().SetUint( params.MinStrLength ), alloc )
			.AddMember( "CutOnEmptySymbs", rapidjson::Value().SetUint( params.CutOnEmptySymbs), alloc ),
		alloc );
}

///////////////////////////////////////////////////////////////

const CModuleRegistrar<CDwordStringPartialOrderComparator> CDwordStringPartialOrderComparator::registrar(
    PartialOrderElementsComparatorModuleType, DwordStringPartialOrderComparator );

void CDwordStringPartialOrderComparator::LoadParams( const JSON& json )
{
    rapidjson::Document doc;
    CParams params;
    loadParams( json, doc, params );
    SetMinStrLength( params.MinStrLength );
    SetCutOnEmptyElems( params.CutOnEmptySymbs );

  	const rapidjson::Value& paramsObj = doc["Params"];
	if( paramsObj.HasMember( "GeneralSymbol") && paramsObj["GeneralSymbol"].IsUint() ) {
        MostGeneralElem() = paramsObj["GeneralSymbol"].GetUint();
	}
}
JSON CDwordStringPartialOrderComparator::SaveParams() const
{
    rapidjson::Document doc;
	rapidjson::MemoryPoolAllocator<>& alloc = doc.GetAllocator();
    saveParams( CParams( GetMinStrLength(), GetCutOnEmptyElems() ), doc );

    doc.AddMember( "Name", DwordStringPartialOrderComparator, alloc );
    doc["Params"]
        .AddMember( "GeneralSymbol", rapidjson::Value().SetUint(MostGeneralElem()), alloc );

	JSON result;
	CreateStringFromJSON( doc, result );
	return result;
}
void CDwordStringPartialOrderComparator::SaveSymb( const DWORD& s, rapidjson::Value& str, rapidjson::MemoryPoolAllocator<>& alloc ) const
{
    assert( str.IsArray() );
    str.PushBack( rapidjson::Value().SetUint(s), alloc );
}
const DWORD CDwordStringPartialOrderComparator::LoadSymb( const rapidjson::Value& val )
{
    return val.GetUint();
}

///////////////////////////////////////////////////////////////

const CModuleRegistrar<CCharStringPartialOrderComparator> CCharStringPartialOrderComparator::registrar(
    PartialOrderElementsComparatorModuleType, CharStringPartialOrderComparator );

void CCharStringPartialOrderComparator::LoadParams( const JSON& json )
{
    rapidjson::Document doc;
    CParams params;
    loadParams( json, doc, params );
    SetMinStrLength( params.MinStrLength );
    SetCutOnEmptyElems( params.CutOnEmptySymbs );
}
JSON CCharStringPartialOrderComparator::SaveParams() const
{
    rapidjson::Document doc;
	rapidjson::MemoryPoolAllocator<>& alloc = doc.GetAllocator();
    saveParams( CParams( GetMinStrLength(), GetCutOnEmptyElems() ), doc );

    doc.AddMember( "Name", CharStringPartialOrderComparator, alloc );

	JSON result;
	CreateStringFromJSON( doc, result );
	return result;
}
void CCharStringPartialOrderComparator::SaveSymb( const char& s, rapidjson::Value& str, rapidjson::MemoryPoolAllocator<>& alloc ) const
{
    assert( str.IsArray() );
    const std::string toStr= std::string()+s;
    str.PushBack( rapidjson::Value().SetString(toStr.c_str(),alloc), alloc );
}
const char CCharStringPartialOrderComparator::LoadSymb( const rapidjson::Value& val )
{
    return val.GetString()[0];
}






// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include "GeneralStringsPartialOrder.h"

#include <fcaps/ModuleJSONTools.h>

////////////////////////////////////////////////////////////////////

const CModuleRegistrar<CGeneralStringsPartialOrderComparator> CGeneralStringsPartialOrderComparator::registrar(
    PartialOrderElementsComparatorModuleType, GeneralStringPartialOrderComparator );

void CGeneralStringsPartialOrderComparator::LoadParams( const JSON& json)
{
    rapidjson::Document params;
	CJsonError errorText;
    if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "CGeneralStringsPartialOrderComparator::LoadParams", errorText );
	}
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()
        && params["Params"].HasMember( "SymbolComparator" ) && params["Params"]["SymbolComparator"].IsObject() ) )
    {
		throw new CTextException( "CGeneralStringsPartialOrderComparator::LoadParams", "'ElementComparator' not found in the setting file" );
	}
	const rapidjson::Value& paramsObj = params["Params"];
	std::string errorStr;
	elemsCmp.reset( dynamic_cast<IPatternManager*>(CreateModuleFromJSON( paramsObj["SymbolComparator"], errorStr )) );
    if( elemsCmp.get() == 0 ) {
        std::string error("Cannot create the ElementComparator module " );
        error += errorStr.empty() ? "" : "('" + errorStr + "')";
        throw new CTextException( "CGeneralStringsPartialOrderComparator::LoadParams", error );
    }

	if( paramsObj.HasMember( "MinStrLength") && paramsObj["MinStrLength"].IsUint() ) {
        SetMinStrLength( paramsObj["MinStrLength"].GetUint() );
	}
	if( paramsObj.HasMember( "CutOnEmptySymbs") && paramsObj["CutOnEmptySymbs"].IsBool() ) {
        SetCutOnEmptyElems( paramsObj["CutOnEmptySymbs"].GetBool() );
	}
}
JSON CGeneralStringsPartialOrderComparator::SaveParams() const
{
    rapidjson::Document doc;
	rapidjson::MemoryPoolAllocator<>& alloc = doc.GetAllocator();
	doc.SetObject()
		.AddMember( "Type", PartialOrderElementsComparatorModuleType, alloc )
		.AddMember( "Name", GeneralStringPartialOrderComparator, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "MinStrLength", rapidjson::Value().SetUint(GetMinStrLength()), alloc )
			.AddMember( "CutOnEmptySymbs", rapidjson::Value().SetBool(GetCutOnEmptyElems()), alloc ),
		alloc );

    const IModule& module = dynamic_cast<const IModule&>( *elemsCmp );
    rapidjson::Document internalParams;
    internalParams.Parse( module.SaveParams().c_str() );
    doc.AddMember( "ElementComparator", internalParams.Move(), alloc );

	JSON result;
	CreateStringFromJSON( doc, result );
	return result;
}

void CGeneralStringsPartialOrderComparator::SaveSymb( const TSymb& s, rapidjson::Value& str, rapidjson::MemoryPoolAllocator<>& alloc ) const
{
    assert( elemsCmp != 0 );
    const JSON json = elemsCmp->SavePattern( s.get() );
	CJsonError errorText;

	rapidjson::Document doc( &alloc );
	doc.Parse( json.c_str() );
	str.Swap( doc );
}
const CGeneralStringsPartialOrderComparator::TSymb CGeneralStringsPartialOrderComparator::LoadSymb( const rapidjson::Value& val )
{
    assert( elemsCmp != 0 );
    JSON strJson;
    CreateStringFromJSON( val, strJson );
    TSymb pattern( elemsCmp->LoadPattern( strJson ), CPatternDeleter( elemsCmp ) );
    if( pattern.get() == 0 ) {
        throw new CTextException( "CGeneralStringsPartialOrderComparator::LoadSymb",
            "Symbol cannot be parsed <<\n" + strJson + "\n>>" );
    }
    return pattern;
}

TCompareResult CGeneralStringsPartialOrderComparator::CompareSymbs( DWORD cmpMask, const TSymb& first, const TSymb& last ) const
{
	assert( elemsCmp != 0 );
	return elemsCmp->Compare( first.get(), last.get(), cmpMask );
}

CGeneralStringsPartialOrderComparator::TSymb
CGeneralStringsPartialOrderComparator::CalculateSymbSimilarity( const TSymb& first, const TSymb& last ) const
{
	assert( elemsCmp != 0 );
	return TSymb( elemsCmp->CalculateSimilarity( first.get(), last.get() ), CPatternDeleter( elemsCmp ) );
}

bool CGeneralStringsPartialOrderComparator::IsMostGeneralSymb( const TSymb& el ) const
{
	assert( el != 0 );
	return el->IsMostGeneral();
}

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/SofiaModules/SuppIntervalClsPatternsProjectionChain.h>

#include <JSONTools.h>

#include <rapidjson/document.h>

using namespace std;

////////////////////////////////////////////////////////////////////

CModuleRegistrar<CSuppIntervalClsPatternsProjectionChain> CSuppIntervalClsPatternsProjectionChain::registar(
	ProjectionChainModuleType, SuppIntervalClsPatternsProjectionChain );

CSuppIntervalClsPatternsProjectionChain::CSuppIntervalClsPatternsProjectionChain()
{
	Thld()=0;
}

double CSuppIntervalClsPatternsProjectionChain::GetPatternInterest( const IPatternDescriptor* p )
{
	return Pattern(p).Extent().Size();
}
void CSuppIntervalClsPatternsProjectionChain::LoadParams( const JSON& json )
{
	CJsonError errorText;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "CSuppIntervalClsPatternsProjectionChain::LoadParams", errorText );
	}
	assert( string( params["Type"].GetString() ) == ProjectionChainModuleType );
	assert( string( params["Name"].GetString() ) == SuppIntervalClsPatternsProjectionChain );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		return;
	}
	const rapidjson::Value& paramsObj = params["Params"];
	if( paramsObj.HasMember("Precisions") ) {
		if(paramsObj["Precisions"].IsNumber() ) {
			Precision()=paramsObj["Precisions"].GetDouble();
		} else if( paramsObj["Precisions"].IsArray() ) {
			Precisions().resize( paramsObj["Precisions"].Size() );
			for( DWORD i = 0; i < Precisions().size(); ++i ) {
				if( paramsObj["Precisions"][i].IsNumber() ) {
					Precisions()[i]=paramsObj["Precisions"][i].GetDouble();
				} else {
					Precisions()[i]=Precision();
				}
			}
		}
	} else if( paramsObj.HasMember( "ProportionalPrecisionThreshold" ) && paramsObj["ProportionalPrecisionThreshold"].IsNumber() ) {
		PropPrecisionThld() = paramsObj["ProportionalPrecisionThreshold"].GetDouble();
	}
}
JSON CSuppIntervalClsPatternsProjectionChain::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", ProjectionChainModuleType, alloc )
		.AddMember( "Name", SuppIntervalClsPatternsProjectionChain, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject(), alloc );
	rapidjson::Value& p = params["Params"];
	if( 0 < PropPrecisionThld() && PropPrecisionThld() < 1 ) {
		p.AddMember( "ProportionalPrecisionThreshold", rapidjson::Value().SetDouble(PropPrecisionThld()),alloc);
	}
	p.AddMember( "Precisions", rapidjson::Value().SetArray(), alloc );
	p["Precisions"].Reserve( Precisions().size(), alloc );
	for( DWORD i = 0; i < Precisions().size(); ++i ) {
		p["Precisions"].PushBack( rapidjson::Value().SetDouble(Precisions()[i]), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

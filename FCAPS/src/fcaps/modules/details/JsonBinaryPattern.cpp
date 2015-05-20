#include "JsonBinaryPattern.h"

#include <JSONTools.h>
#include <StdTools.h>
#include <ListWrapper.h>

#include<rapidjson/document.h>

using namespace std;

namespace JsonBinaryPattern {
////////////////////////////////////////////////////////////////////

JSON_VAR(Inds);
JSON_VAR(Names);
JSON_VAR(Count);

void LoadPattern( const JSON& json, CIndices& inds, CNames& names )
{
	static char place[]="JsonBinaryPattern::LoadPattern";
	rapidjson::Document patternJson;
	CJsonError errorText;
	if( !ReadJsonString( json, patternJson, errorText ) ) {
		throw new CJsonException( place, errorText );
	};

	inds.Clear();
	if( patternJson.HasMember( jsonInds ) && patternJson[jsonInds].IsArray() ) {
		const rapidjson::Value& indsJson = patternJson[jsonInds];
		DWORD indsSize = indsJson.Size();
		for( size_t i = 0; i < indsSize; ++i ) {
			if( !indsJson[i].IsUint() ) {
				errorText.Data=json;
				errorText.Error="Not an unsigned int value in THIS.Inds[" + StdExt::to_string( i ) + "].";
				throw new CJsonException( place, errorText );
			}
			inds.PushBack( indsJson[i].GetUint() );
		}
	}

	names.Clear();
	if( patternJson.HasMember( jsonNames ) && patternJson[jsonNames].IsArray() ) {
		const rapidjson::Value& namesJson = patternJson[jsonNames];
		DWORD indsSize = namesJson.Size();
		for( size_t i = 0; i < indsSize; ++i ) {
			if( !namesJson[i].IsString() ) {
				errorText.Data=json;
				errorText.Error="Not a string value in THIS.Names[" + StdExt::to_string( i ) + "].";
				throw new CJsonException( place, errorText );
			}
			names.PushBack( namesJson[i].GetString() );
		}
	}
}

JSON SavePattern( CIndices& inds, CNames& names )
{
	rapidjson::Document patternJson;

	patternJson.SetObject();
	patternJson
		.AddMember( jsonCount, rapidjson::Value().SetUint( inds.Size() > 0 ? inds.Size() : names.Size() ), patternJson.GetAllocator() );

	if( inds.Size() > 0 ) {
		patternJson
			.AddMember( jsonInds, rapidjson::Value().SetArray(), patternJson.GetAllocator() );
		rapidjson::Value& indsJson = patternJson[jsonInds];
		indsJson.Reserve( inds.Size(), patternJson.GetAllocator() );

		CStdIterator<CList<DWORD>::CConstIterator, false> itr( inds );
		for( ; !itr.IsEnd(); ++itr ) {
			indsJson.PushBack( rapidjson::Value().SetUint( *itr ), patternJson.GetAllocator() );
		}
	}
	if( names.Size() > 0 ) {
		patternJson
			.AddMember( jsonNames, rapidjson::Value().SetArray(), patternJson.GetAllocator() );
		rapidjson::Value& namesJson = patternJson[jsonNames];
		namesJson.Reserve( names.Size(), patternJson.GetAllocator() );

		CStdIterator<CList<std::string>::CConstIterator, false> itr( names );
		for( ; !itr.IsEnd(); ++itr ) {
			namesJson.PushBack( rapidjson::Value().SetString(
				rapidjson::StringRef( (*itr).c_str() ) ), patternJson.GetAllocator() );
		}
	}
	JSON result;
	CreateStringFromJSON( patternJson, result );
	return result;
}

////////////////////////////////////////////////////////////////////
} // JsonBinaryPattern

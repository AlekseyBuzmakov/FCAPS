#include "JsonClassifierClasses.h"

#include <JSONTools.h>
#include <StdTools.h>

using namespace std;

////////////////////////////////////////////////////////////////////

namespace JsonClassifierClasses {
////////////////////////////////////////////////////////////////////////

void Load( const std::string& classesPath, CObjToClassesMap& classes )
{
	CJsonError jsonError;
	string errorText;

	// Read PatternManager (PM) params.
	rapidjson::Document cl;
	if( !ReadJsonFile( classesPath, cl, jsonError ) ) {
		throw new CJsonException( "JsonClassifierClasses::Load", jsonError );
	}
	if( !cl.IsArray() ) {
		throw new CTextException( "JsonClassifierClasses::Load", "Classes JSON is not an json-array\n" );
	}

	for( int i = 0; i < cl.Size(); ++i ) {
		const rapidjson::Value& mVal = cl[i];
		if(!mVal.IsObject()
			|| !mVal.HasMember("N") || !mVal["N"].IsString()
			|| !mVal.HasMember("Cl") || !mVal["Cl"].IsString() )
		{
			throw new CTextException( "JsonClassifierClasses::Load", "Element " + StdExt::to_string( i ) + " is in a bad format (should be'{\"N\":...,\"Cl\":...}')");
		}
		classes.insert( pair<string,string>( mVal["N"].GetString(), mVal["Cl"].GetString() ) );
	}
}

////////////////////////////////////////////////////////////////////////
} // namespace JsonClassifierClasses

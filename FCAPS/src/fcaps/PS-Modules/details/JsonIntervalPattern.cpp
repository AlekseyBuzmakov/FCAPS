// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/PS-Modules/details/JsonIntervalPattern.h>

#include <JSONTools.h>
#include <StdTools.h>

#include<rapidjson/document.h>
#include <sstream>

using namespace std;

namespace JsonIntervalPattern {
////////////////////////////////////////////////////////////////////////

void LoadPattern( const JSON& json, CPattern& result )
{
	static const char place[]="JsonIntervalPattern::LoadPattern";
	rapidjson::Document doc;
	CJsonError error;
	if( !ReadJsonString( json, doc, error ) ) {
		throw new CJsonException(place, error);
	}
	if( !doc.IsArray() || doc.Size() == 0 ) {
		throw new CJsonException(place, CJsonError(json,"Not an array"));
	}
	result.resize(doc.Size());
	for( DWORD i = 0; i < doc.Size(); ++i ) {
		const rapidjson::Value& val = doc[i];
		if(val.IsArray()) {
			if(val.Size() != 2 || !val[0].IsNumber() || !val[1].IsNumber()) {
				throw new CJsonException(place, CJsonError(json,"The element " + StdExt::to_string(i) + " is not an interval"));
			}
			result[i].first = val[0].GetDouble();
			result[i].second = val[1].GetDouble();
			if( result[i].first > result[i].second + 0.0000001) {
				throw new CJsonException(place, CJsonError(json,"The element " + StdExt::to_string(i) + " is not an interval"));
			}
		} else if( val.IsNumber() ) {
			result[i].first = result[i].second = val.GetDouble();
		} else {
			throw new CJsonException(place, CJsonError(json,"The element " + StdExt::to_string(i) + " is not an interval"));
		}
	}
}
JSON SavePattern( const CPattern& result )
{
	stringstream ss;
	ss << "[";
	for( DWORD i = 0 ; i < result.size(); ++i ) {
		if( i!=0){
			ss << ",";
		}
		ss << "[" << result[i].first << "," << result[i].second << "]";
	}
	ss << "]";
	return ss.str();
}

////////////////////////////////////////////////////////////////////////
} // namespace JsonIntervalPattern

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Module interface for creting modules by the module name.

#include <JSONTools.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <boost/lexical_cast.hpp>

using namespace std;
using namespace rapidjson;

////////////////////////////////////////////////////////////////////

bool ReadJsonFile( const std::string& path, rapidjson::Document& doc, CJsonError& error )
{
	error.Clear();

	// Buffer for io operations.
	char buffer[4096];

	FILE* fp = fopen( path.c_str(), "r");
	if( fp == 0 ) {
		error.Data = path;
		error.Error = "Cannot open the file";
		return false;
	}
	rapidjson::FileReadStream is( fp, buffer, sizeof(buffer) );
	doc.ParseStream( is );
	if( doc.HasParseError () ) {
		error.Data = path;
		error.Error = rapidjson::GetParseError_En( doc.GetParseError() );
		error.Offset = doc.GetErrorOffset();
		fclose(fp);
		return false;
	}
	fclose(fp);
	return true;
}

class COneValueReader {
public:
	COneValueReader( Document& _result ) :
		result( _result ) {}
	bool Null()
		{ assert( result.IsNull() ); result.SetNull(); return true; }
	bool Bool(bool b)
		{ assert( result.IsNull() ); result.SetBool(b); return true; }
	bool Int(int i)
		{ assert( result.IsNull() ); result.SetInt(i); return true; }
	bool Uint(unsigned u)
		{ assert( result.IsNull() ); result.SetUint(u); return true; }
	bool Int64(int64_t i)
		{ assert( result.IsNull() ); result.SetInt64(i); return true; }
	bool Uint64(int64_t u)
		{ assert( result.IsNull() ); result.SetUint64(u); return true; }
	bool Double(double d)
		{ assert( result.IsNull() ); result.SetDouble(d); return true; }
	bool String(const char* str, SizeType length, bool copy)
	{
		assert( result.IsNull() );
		if( copy ) {
			result.SetString( str, length, result.GetAllocator() );
		} else {
			result.SetString( StringRef( str ) );
		}
		return true;
	}
	bool Key(const char* str, size_t length, bool copy) {
		assert( false ); return true;
	}
	bool StartObject()
		{ assert( false ); return true; }
	bool EndObject(SizeType memberCount)
		{ assert( false ); return true; }
	bool StartArray()
		{ assert( false ); return true; }
	bool EndArray(SizeType elementCount)
		{ assert( false ); return true; }

private:
	Document& result;
};

bool ReadJsonString( const std::string& str, rapidjson::Document& doc, CJsonError& error )
{
	error.Clear();

	if( str.length() == 0 ) {
		error.Data = str;
		error.Error="Empty input";
		return false;
	}
	if( str[0] == '{' || str[0] == '[' ) {
		// We are in the valid from rapidjson point of view JSON.
		doc.Parse( str.c_str() );
	} else {
		// Making back-door for stupid disicion in rapidjson.
		COneValueReader handler( doc );
		Reader reader;
		StringStream ss(str.c_str());
		reader.ParseValueOnly(ss, handler);
	}
	if( doc.HasParseError () ) {
		error.Data = str;
		error.Error = rapidjson::GetParseError_En( doc.GetParseError() );
		error.Offset = doc.GetErrorOffset();
		return false;
	}
	return true;
}

void CreateStringFromJSON( const rapidjson::Value& json, JSON& result )
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	json.Accept(writer);
	result = buffer.GetString();
}

////////////////////////////////////////////////////////////////////

CJsonException::CJsonException( const std::string& place, const CJsonError& _error ) :
	CException( place ),
	error( _error )
{
	stringstream destStr;
	destStr << "In: <<<""\n\t"
		<< error.Data << "\n"
		<< ">>> with an error <<<\n\t"
		<< error.Error << "\n"
		<< ">>> ";
	if( error.Offset != -1 ) {
		destStr << "in symbol " << error.Offset;
	}
	SetText( destStr.str() );
}

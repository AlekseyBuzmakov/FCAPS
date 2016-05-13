// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef JSONTOOLS_H_INCLUDED
#define JSONTOOLS_H_INCLUDED

#include <common.h>

#include <fcaps/BasicTypes.h>

#include <Exception.h>
#include <RelativePathes.h>

#include <rapidjson/rapidjson.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>

interface IModule;

////////////////////////////////////////////////////////////////////

#define JSON_VAR(name) static const char json##name[]=#name

////////////////////////////////////////////////////////////////////

struct CJsonError {
	// Reference to the data where the error where found
	std::string Data;
	// Error explanation
	std::string Error;
	// Offcet to the error position
	DWORD Offset;

	CJsonError() :
		Offset( -1 ) {}
	CJsonError( const std::string& data, const std::string& error, DWORD offset = -1 ) :
		Data(data),Error(error),Offset( -1 ) {}
	void Clear()
		{ Data.clear(); Error.clear(); Offset = -1; }
};

////////////////////////////////////////////////////////////////////

// Read JSON document from string to an object
bool ReadJsonString( const std::string& str, rapidjson::Document& doc, CJsonError& errorText );

// Create string from JSON
void CreateStringFromJSON( const rapidjson::Value& json, JSON& result );

// Read JSON document to an object.
bool ReadJsonFile( const std::string& path, rapidjson::Document& doc, CJsonError& errorText );

// Saving the current read file in pathes stack
class CJsonFile {
public:
	CJsonFile( const std::string& _path ) {
		RelativePathes::GetFullPath( _path, fullPath );

		std::string dirPath;
		RelativePathes::BaseName( fullPath, dirPath );
		path.SetPath( dirPath );
	}

	bool Read( rapidjson::Document& doc, CJsonError& errorText ) {
		if( !ReadJsonFile( fullPath, doc, errorText ) ) {
			path.Pull();
			return false;
		}
		return true;
	}
private:
	RelativePathes::CSearchPath path;
	std::string fullPath;
};

////////////////////////////////////////////////////////////////////

// Output std stream wrapper
//		taken from rapidjson
class OStreamWrapper {
public:
	typedef char Ch;
	OStreamWrapper(std::ostream& os) : os_(os) {
	}
	Ch Peek() const { assert(false); return '\0'; }
	Ch Take() { assert(false); return '\0'; }
	size_t Tell() const { return 0; }
	Ch* PutBegin() { assert(false); return 0; }
	void Put(Ch c) { os_.put(c); }                  // 1
	void Flush() { os_.flush(); }                   // 2
	size_t PutEnd(Ch*) { assert(false); return 0; }
private:
	OStreamWrapper(const OStreamWrapper&);
	OStreamWrapper& operator=(const OStreamWrapper&);
	std::ostream& os_;
};

////////////////////////////////////////////////////////////////////

class CJsonException : public CException {
public:
	CJsonException( const std::string& place, const CJsonError& _error );

private:
	const CJsonError error;
};
////////////////////////////////////////////////////////////////////

#endif // JSONTOOLS_H_INCLUDED

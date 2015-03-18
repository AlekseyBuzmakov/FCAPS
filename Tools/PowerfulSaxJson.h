#ifndef CPOWERFULSAXJSON_H
#define CPOWERFULSAXJSON_H

#include <rapidjson/rapidjson.h>
#include <string>
#include <sstream>

enum TPowerfulSaxJsonResults{
	// An array, an object, or a member should be skiped
	PSJR_Skip = 0,
	// Iterate the content of an array/object/member
	PSJR_Iterate,
	// Load all content of array/object/member
	PSJR_Load,
	// An error has happend, should finish
	PSJR_Error,

	PSJR_EnumCount
};

////////////////////////////////////////////////////////////////////

class CStdJsonPartLoader {
public:
	CStdJsonPartLoader() :
		isCommaNeeded( false ) {}
	bool Null() { processComma(true); ss << "null"; return true; }
	bool Bool(bool b) { processComma(true); ss << b; return true; }
	bool Int(int i) { processComma(true); ss << i; return true; }
	bool Uint(unsigned u) { processComma(true); ss << u; return true; }
	bool Int64(int64_t i) { processComma(true); ss << i; return true; }
	bool Uint64(int64_t u) { processComma(true); ss << u; return true; }
	bool Double(double d) { processComma(true); ss << d; return true; }
	bool String(const char* str, size_t /*length*/, bool /*copy*/) {
		processComma(true);
		ss << '"' << str << '"';
		return true;
	}

	bool Key(const char* str, size_t /*length*/, bool /*copy*/) {
		processComma(false);
		ss << '"' << str << '"' << ':';
		return true;
	}

	bool StartObject() {
		processComma(false);
		ss << '{';
		return true;
	}
	bool EndObject(size_t /*memberCount*/) {
		ss << '}';
		isCommaNeeded = true;
		return true;
	}
	bool StartArray() {
		processComma(false);
		ss << '[';
		return true;
	}
	bool EndArray(size_t /*elementCount*/) {
		ss << ']';
		isCommaNeeded=true;
		return true;
	}

	std::string GetValue() {
		std::string result = ss.str();
		ss.str("");
		isCommaNeeded=false;
		return result;
	}

private:
	std::stringstream ss;
	bool isCommaNeeded;

	void processComma( bool after ) {
		if( isCommaNeeded ) {
			ss << ',';
		}
		isCommaNeeded = after;
	}
};


////////////////////////////////////////////////////////////////////
// The members of SaxJsonClass are those rapidjson::Handler
//  but Array, Object and Key returns TPowerfulSaxJsonResults
//  it also should have a method bool Load(const std:string& )
//  for loading a whole entity

#define PassValue(v) \
switch(state) { \
	case PSJR_Iterate: return handler.v; \
	case PSJR_Skip: if(term == PT_Member) {state=PSJR_Iterate;} return true; \
	case PSJR_Load: if(term == PT_Member) {state=PSJR_Iterate; return handler.v; } else { return loader.v; } \
	default: return false; \
}

template<typename SaxJsonClass, typename JsonPartLoader=CStdJsonPartLoader>
class CPowerfulSaxJson {
public:
	CPowerfulSaxJson( SaxJsonClass& _handler ) :
		handler(_handler), term(PT_Object), state( PSJR_Iterate) { }

	bool Null() { PassValue(Null()); }
	bool Bool(bool b) { PassValue(Bool(b)); }
	bool Int(int i) { PassValue(Int(i)); }
	bool Uint(unsigned u) { PassValue(Uint(u)); }
	bool Int64(int64_t i) { PassValue(Int64(i)); }
	bool Uint64(int64_t u) { PassValue(Uint64(u)); }
	bool Double(double d) { PassValue(Double(d)); }
	bool String(const char* str, size_t length, bool copy) {
		PassValue(String(str,length,copy));
	}

	bool Key(const char* str, size_t length, bool copy) {
		switch( state ){
		case PSJR_Skip:
			return true;
		case PSJR_Load:
			return loader.Key(str,length,copy);
		case PSJR_Iterate:
			state = handler.Key(str,length,copy);;
			if( state == PSJR_Load ) {
				term=PT_Member;
			}
			return state != PSJR_Error;
		default:
			return false;
		}
	}

	bool StartObject() {
		switch( state ){
		case PSJR_Skip:
			if( term == PT_Object ) {
				++paranthesisCounter;
			}
			return true;
		case PSJR_Load:
			if( term == PT_Object ) {
				++paranthesisCounter;
			} else if( term == PT_Member ) {
				term = PT_Object;
				paranthesisCounter = 1;
			}
			return loader.StartObject();
		case PSJR_Iterate:
			state = handler.StartObject();
			paranthesisCounter = 1;
			if( state == PSJR_Load ) {
				return loader.StartObject();
			}
			return state != PSJR_Error;
		default:
			return false;
		}
	}
	bool EndObject(size_t memberCount) {
		switch( state ){
		case PSJR_Skip:
			if( term == PT_Object ) {
				--paranthesisCounter;
				if( paranthesisCounter == 0 ) {
					state = PSJR_Iterate;
				}
			}
			return true;
		case PSJR_Load: {
			if( !loader.EndObject(memberCount) ) {
				return false;
			};
			if( term == PT_Object ) {
				--paranthesisCounter;
				if( paranthesisCounter == 0 ) {
					state = PSJR_Iterate;
					return handler.Load(loader.GetValue());
				}
			}
			return true;
		}
		case PSJR_Iterate:
			return handler.EndObject( memberCount );
		default:
			return false;
		}
	}
	bool StartArray() {
		switch( state ){
		case PSJR_Skip:
			if( term == PT_Array ) {
				++paranthesisCounter;
			}
			return true;
		case PSJR_Load:
			if( term == PT_Array ) {
				++paranthesisCounter;
			} else if( term == PT_Member ) {
				term = PT_Array;
				paranthesisCounter = 1;
			}
			return loader.StartArray();
		case PSJR_Iterate:
			state = handler.StartArray();
			paranthesisCounter = 1;
			if( state == PSJR_Load ) {
				return loader.StartArray();
			}
			return state != PSJR_Error;
		default:
			return false;
		}
	}
	bool EndArray(size_t elementCount) {
		switch( state ){
		case PSJR_Skip:
			if( term == PT_Array ) {
				--paranthesisCounter;
				if( paranthesisCounter == 0 ) {
					state = PSJR_Iterate;
				}
			}
			return true;
		case PSJR_Load: {
			if( !loader.EndArray(elementCount) ) {
				return false;
			};
			if( term == PT_Array ) {
				--paranthesisCounter;
				if( paranthesisCounter == 0 ) {
					state = PSJR_Iterate;
					return handler.Load(loader.GetValue());
				}
			}
			return true;
		}
		case PSJR_Iterate:
			return handler.EndArray( elementCount );
		default:
			return false;
		}
	}

private:
	enum TProcessingTerm {
		PT_Array,
		PT_Object,
		// One member of an object.
		PT_Member,

		PT_EnumCount
	};
private:
	SaxJsonClass& handler;
	JsonPartLoader loader;

	TProcessingTerm term;
	TPowerfulSaxJsonResults state;
	int paranthesisCounter;
};

////////////////////////////////////////////////////////////////////

class CSaxJsonDefaultTemplate {
public:
	bool Null() { assert(false); return true; }
	bool Bool(bool /*b*/) { assert(false); return true; }
	bool Int(int /*i*/) { assert(false); return true; }
	bool Uint(unsigned /*u*/) { assert(false); return true; }
	bool Int64(int64_t /*i*/) { assert(false); return true; }
	bool Uint64(int64_t /*u*/) { assert(false); return true; }
	bool Double(double /*d*/) { assert(false); return true; }
	bool String(const char* /*str*/, size_t /*length*/, bool /*copy*/)
		{ assert(false); return true; }

	TPowerfulSaxJsonResults Key(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
		return PSJR_Skip;
	}

	TPowerfulSaxJsonResults StartObject() {
		return PSJR_Skip;
	}
	bool EndObject(size_t /*memberCount*/) {
		return true;
	}
	TPowerfulSaxJsonResults StartArray() {
		return PSJR_Skip;
	}
	bool EndArray(size_t /*elementCount*/) {
		return true;
	}

	bool Load( const std::string& /*json*/ ) {
		return true;
	}
};
#endif // CPOWERFULSAXJSON_H

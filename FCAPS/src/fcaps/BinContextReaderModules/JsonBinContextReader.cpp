// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2011-2015, v0.7

#include <fcaps/BinContextReaderModules/JsonBinContextReader.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>
#include <ListWrapper.h>

#include <rapidjson/document.h>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(...) #__VA_ARGS__

const char description[] =
STR(
	{
	"Name":"Reads Binary context from JSON file",
	"Description":"An object provides a context by means of its attirbutes based on a json file using the SAX approach that is more memory and time efficient",
	"Params": {
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of Json Context Attributes (SAX)",
			"type": "object",
			"properties": {
				"ContextFilePath":{
					"description": "The path to a file containing the context in JSON format",
					"type": "file-path"
				},
					"Order":{
					"description": "The sorting order of attributes in the context: desc(ending), asc(ending), rand(om), n(one).",
					"type": "string"
				}
			}
		}
	}
);

///////////////////////////////////////////////////////////////////////////////
// A class for dealing with the events of SAX JSON Reader
class CSAXAttributeReader{
public:
	enum TMode {
		// Just computing the support of every attribute
		M_Initialize = 0,
		// Reads the context
		M_Read,

		M_Count
	};
public:
	CSAXAttributeReader(CJsonBinContextReader::TAttributes& _attrs) :
		attributes(_attrs), mode(M_Initialize),
		isInsideAttributes(false), dataEntryPoint(-1), curObjectNumber(-1),
		hasAttrNames(false),
		fp(0),
		totalObjectNumber(-1),
		isObjectRead(false)
	{ curIndex.SetKey("ROOT"); }

	~CSAXAttributeReader() {
		clear();
		closeFile();
	}

	void SetFile(const string& filePath) {
		clear();
		closeFile();

		path = filePath;
	    fp = fopen( path.c_str(), "r");
		if( fp == 0 ) {
			error.Data = path;
			error.Error = "Cannot open the file";
			throw new CJsonException( "CJsonBinContextReader::LoadParams", error );
		}
	}

	// The first pass through the data for getting the number of attributes and their usage.
	void FirstPass() {
		assert(attributes.size()==0);

		mode = M_Initialize;
		clear();
		
		rapidjson::FileReadStream is( fp, buffer, sizeof(buffer) );
		if(!reader.Parse(is, *this)) {
			rapidjson::ParseErrorCode e = reader.GetParseErrorCode();
			error.Data = path;
			error.Offset = reader.GetErrorOffset();
			error.Error = rapidjson::GetParseError_En(e);
			throw new CJsonException( "CJsonBinContextReader::LoadParams", error );
		}
	}

	// Reading the objects one by one (the second pass)
	void StartReadingObjects() {
		mode = M_Read;
		assert(attributes.size() != 0);
		attrBuffer.reserve(totalObjectNumber);

		clear();

		fseek(fp,0,SEEK_SET); // moving to the begining of the stream
		is.reset( new rapidjson::FileReadStream( fp, buffer, sizeof(buffer) ));

		reader.IterativeParseInit();
	}

	// Reads the intent of the next object, returns false if no more object is available
	bool ReadNextObject(int& attrCount, const int*& attrs) {
		isObjectRead = false;
		while (!isObjectRead && !reader.IterativeParseComplete()) {
			if( !reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(*is, *this) ) {
				rapidjson::ParseErrorCode e = reader.GetParseErrorCode();
				error.Data = path;
				error.Offset = reader.GetErrorOffset();
				error.Error = rapidjson::GetParseError_En(e);
				throw new CJsonException( "CJsonBinContextReader::LoadParams", error );
			}
		}
		assert(!(isObjectRead && reader.IterativeParseComplete()));
		attrCount = attrBuffer.size();
		attrs = attrBuffer.data();
		return isObjectRead;
	}

	bool Null()
	{
		incIndex();
		return true;
	}
	bool Bool(bool b) 
	{
		assert(!isInsideAttributes);
		assert(curObjectNumber < 0);
		incIndex();
		return true;
	}
    bool Int(int i)
	{
		assert(!isInsideAttributes);
		assert(curObjectNumber < 0);
		incIndex();
		return true;
	}
    bool Uint(unsigned i)
	{
		assert(!isInsideAttributes);

		if(curObjectNumber >= 0) {
			if( hasAttrNames ) {
				// Should check that a valid index is provided
				assert( i < attributes.size());
			}
			addAttributesTill(i);
			assert( i < attributes.size());
			switch(mode) {
			case M_Initialize:
				// Just counting the number of objects for each attribute
				++attributes[i].Support;
				break;
			case M_Read: {
				attrBuffer.push_back(i);
				break;
			}
			default:
				assert(false);
			}
			
		}
		incIndex();
		return true;
	}
    bool Int64(int64_t i)
	{
		assert(!isInsideAttributes);
		assert(curObjectNumber < 0);
		incIndex();
		return true;
	}
    bool Uint64(uint64_t i)
	{
		assert(!isInsideAttributes);
		assert(curObjectNumber < 0);
		incIndex();
		return true;
	}
	bool Double(double d)
	{
		assert(!isInsideAttributes);
		assert(curObjectNumber < 0);
		incIndex();
		return true;
	}
	bool RawNumber(const char* str, size_t length, bool copy) {
		assert(false);
		incIndex();
		return true;
	}
    bool String(const char* str, size_t length, bool copy)
	{
		assert(curObjectNumber < 0);
		if(isInsideAttributes) {
			assert(mode == M_Initialize);

			addAttributeName(string(str,length));	
			assert(curIndex.Index + 1 == attributes.size());
		}
		incIndex();
		return true;
	}
    bool StartObject()
	{
		stack.push_back(CStackElement(curIndex, SET_Obj));
		curIndex.SetKey("");
		return true;
	}
    bool EndObject(size_t memberCount)
	{
		CStackElement& last = stack.back();
		assert(last.Type == SET_Obj);
		curIndex = last.Index;
		incIndex();
		stack.pop_back();
		return true;
	}
	bool Key(const char* str, size_t length, bool /*copy*/) {
		string key = string(str, length);
		curIndex.SetKey(key);
		return true;
	}
	bool StartArray()
	{
		assert(!isInsideAttributes);
		if(mode == M_Initialize && curIndex.Key == "AttrNames" && canEnterAttrNames() ) {
			isInsideAttributes = true;
		}
		if(curIndex.Key == "Data" && canEnterData()) {
			dataEntryPoint = stack.size();
		}
		if(dataEntryPoint >= 0 && curIndex.Key == "Inds") {
			assert( curObjectNumber + 1 == stack.back().Index.Index);
			assert(stack.size() > 0);
			curObjectNumber = stack.back().Index.Index;
			attrBuffer.resize(0);
		}

		stack.push_back(CStackElement(curIndex, SET_Array));
		curIndex.SetIndex(0);
		return true;
	}
    bool EndArray(size_t elementCount)
	{
		assert(stack.size() > 0);
		CStackElement& last = stack.back();
		assert(last.Type == SET_Array);

		isInsideAttributes = false; // Entering on array-open and always exiting on array-close
		if(dataEntryPoint >= 0 && last.Index.Key == "Inds") {
			isObjectRead = true;
		}
		if( dataEntryPoint == stack.size() - 1) {
			assert(last.Index.Key == "Data");
			dataEntryPoint = -1;
			totalObjectNumber = curObjectNumber+1;
			curObjectNumber = -1;
		}

		curIndex = last.Index;
		incIndex();
		stack.pop_back();

		return true;
	}

	int GetObjectNumber() const
		{return totalObjectNumber;}

private:
	struct CIndex{
		string Key;
		int Index;

		CIndex() : Index(-1) {}
		void SetKey( const string& str )
			{ Key = str; Index = -1;}
		void SetIndex( int i )
			{ assert(i >= 0); Key = ""; Index = i;}
	};
	enum CStackElementType {
		SET_Obj = 0,
		SET_Array,

		SET_Count
	};
	struct CStackElement {
		CIndex Index;
		CStackElementType Type;

		CStackElement() : Type(SET_Count) {}
		CStackElement( const CIndex& ind, CStackElementType t) : Index(ind), Type(t) {}
	};

private:

	// Readers and related vars
	string path;
	FILE* fp;
	char buffer[4096];
	rapidjson::Reader reader;
	CJsonError error;
	CPtrOwner<rapidjson::FileReadStream> is;

	// Current working mode
	TMode mode;
	// The current index
	CIndex curIndex;
	// Indicates if we are inside the attribute names
	bool isInsideAttributes;
	// Indicates the index in the stack of "data" entry point
	int dataEntryPoint;
	// The currentObjectNumber
	int curObjectNumber;
	// Stack for storing depth of objects/arrays
	deque<CStackElement> stack;
	// A flag indicating if the attributes names are provided
	bool hasAttrNames;
	// The place where the attributes should be read
	CJsonBinContextReader::TAttributes& attributes;
	int totalObjectNumber;
	// Buffer for the attributes
	vector<int> attrBuffer;
	// A flag indicating if a new object is read
	bool isObjectRead;

	// The function takes care about the correct index
	void incIndex()
	{
		if( curIndex.Index >= 0 ) {
			++curIndex.Index;
		}
	}
	// The function adds new attribute and initialize it with the name
	void addAttributeName(const string& str) {
		assert( mode == M_Initialize);
		attributes.resize(attributes.size() + 1);
		attributes.back().Name = str;
		hasAttrNames = true;
	}
	// Adds attributes such to include the attribute "i" with their numbers as names
	void addAttributesTill(unsigned i) {
		const unsigned oldSize = attributes.size();
		if( oldSize <= i) {
			attributes.resize(i + 1);
		}
		for(unsigned j = oldSize; j <=i; ++j) {
			attributes[j].Name = to_string(j);
		}
	}
	// Validates if the current image of the stack could correspond to attribute names
	bool canEnterAttrNames() const
		{ return true; }
	// Validates if the current image of the stack could correspond to data entry point
	bool canEnterData() const
		{ return true; }

	// Closes the file
	void closeFile()
		{ if(fp != 0) { fclose(fp); fp=0; clear(); }}
	// Clear the object for reading from the begining
	void clear() {
		curIndex.SetKey("ROOT");
		isInsideAttributes = false;
		dataEntryPoint = -1;
		curObjectNumber = -1;
		stack.clear();

		is.reset(0);
	}
};

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CJsonBinContextReader> CJsonBinContextReader::registrar;

const char* const CJsonBinContextReader::Desc()
{
	return description;
}

CJsonBinContextReader::CJsonBinContextReader() :
	objectNum(0),
	order(AO_Desc),
	saxReader(new CSAXAttributeReader(attributes))
{
	
}
void CJsonBinContextReader::Start()
{
	assert(saxReader != 0);
	
	nextObjectAttrCount = 0;
	nextObjectData = 0; 

	saxReader->StartReadingObjects();
}
int CJsonBinContextReader::GetNextObjectIntentSize()
{
	assert(saxReader != 0);

	nextObjectAttrCount = 0;
	nextObjectData = 0; 
	const bool res = saxReader->ReadNextObject(nextObjectAttrCount, nextObjectData);
	if( !res ) {
		return -1;
	}
	assert(nextObjectAttrCount >=0);
	assert(nextObjectAttrCount <= attributes.size() );

	return nextObjectAttrCount;
}
void CJsonBinContextReader::GetNextObject(CObjectIntent& intent)
{
	if( intent.Size < nextObjectAttrCount ) {
		throw new CTextException("CJsonBinContextReader::GetNextObject", "Insuffisient intent memory");
	}
	intent.Size = nextObjectAttrCount;
	assert(sizeof(int) == sizeof(IBinContextReader::TAttributeID));
	for(int i = 0; i < nextObjectAttrCount; ++i) {
		const TAttributeID curAttr = nextObjectData[i];
		assert(0 <= curAttr && curAttr < reverseAttrOrder.size());
		intent.Attributes[i] = reverseAttrOrder[curAttr];
	}
}


void CJsonBinContextReader::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CJsonBinContextReader::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == GetType() );
	assert( string( params["Name"].GetString() ) == GetName() );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for file path to the context";
		throw new CJsonException("CJsonBinContextReader::LoadParams", error);
	}
	// Reading class labels information
	if(!(params["Params"].HasMember("ContextFilePath") && params["Params"]["ContextFilePath"].IsString())) {
		error.Data = json;
		error.Error = "Params.ContextFilePath is not found or is not a file name.";
		throw new CJsonException("CJsonBinContextReader::LoadParams", error);
	}
	if(params["Params"].HasMember("Order") && params["Params"]["Order"].IsString()) {
		string orderStr = params["Params"]["Order"].GetString();
		if(orderStr == "desc") {
			order = AO_Desc;
		} else if(orderStr == "asc") {
			order = AO_Asc;
		} else if(orderStr == "rand") {
			order = AO_Random;
		} else {
			order = AO_None;
		}
	}

	filePath = params["Params"]["ContextFilePath"].GetString();

	loadContext();
}

JSON CJsonBinContextReader::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", rapidjson::StringRef(GetType()), alloc )
		.AddMember( "Name", rapidjson::StringRef(GetName()), alloc )
		.AddMember("Params", "TODO", alloc);

	// TODO 
	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

///////////////////////////////////////////////////////////////////////////////
// Class for sorting the attributes
namespace {
	class CAttrSorter {
	public:
		CAttrSorter( CJsonBinContextReader::TAttrOrderMode _order, const CJsonBinContextReader::TAttributes& _attrs ) :
			order(_order), attrs( _attrs )
		{
			if( order == CJsonBinContextReader::AO_Random ) {
				initRandomOrder();
			}
		}
		bool operator()( DWORD i1, DWORD i2 ) const
		{
			switch( order ) {
			case CJsonBinContextReader::AO_None:
				return i1 < i2;
			case CJsonBinContextReader::AO_Asc:
				assert( i1 < attrs.size() && i2 < attrs.size());
				return attrs[i1].Support < attrs[i2].Support;
			case CJsonBinContextReader::AO_Desc:
				assert( i1 < attrs.size() && i2 < attrs.size());
				return attrs[i1].Support > attrs[i2].Support;
			case CJsonBinContextReader::AO_Random:
				assert( i1 < randomOrder.size() );
				assert( i2 < randomOrder.size() );
				return randomOrder[i1] < randomOrder[i2];
				break;
			default:
				assert( false );
				return true;
			}
		}
	private:
		const CJsonBinContextReader::TAttrOrderMode order;
		const CJsonBinContextReader::TAttributes& attrs;
		vector<DWORD> randomOrder;

		void initRandomOrder();
	};

	void CAttrSorter::initRandomOrder() {
		randomOrder.resize( attrs.size(), -1 );
		for(DWORD i= 0; i < randomOrder.size(); ++i) {
			DWORD position = rand() % (randomOrder.size() - i);
			DWORD j = 0;
			for( ; j < randomOrder.size(); ++j) {
				if(randomOrder[j] != -1) {
					continue;
				}
				if( position == 0 ) {
					randomOrder[j] = i;
					break;
				} else {
					--position;
				}
			}
			assert(position < randomOrder.size());
		}
	}
}

// Load context from a JSON file
void CJsonBinContextReader::loadContext()
{
	assert(saxReader != 0);
	
	CJsonError error;
	string path;
	RelativePathes::GetFullPath( filePath, path);
	saxReader->SetFile(path);
	saxReader->FirstPass();
	objectNum = saxReader->GetObjectNumber();

	// Sorting attributes
	attrOrder.resize(attributes.size());
	for(int i = 0; i < attrOrder.size(); ++i) {
		attrOrder[i] = i;
	}
	if( order != AO_None ) {
		CAttrSorter sorter( order, attributes );
		sort( attrOrder.begin(), attrOrder.end(), sorter );
	}
	reverseAttrOrder.resize(attrOrder.size());
	for(int i = 0; i < attrOrder.size(); ++i) {
		reverseAttrOrder[attrOrder[i]] = i;
	}
}


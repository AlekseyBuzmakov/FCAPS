// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2011-2015, v0.7

#include <fcaps/ContextAttributesModules/SAXJsonContextAttributes.h>

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
	"Name":"Context Attributes from JSON (SAX)",
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
					"description": "The sorting order of attributes in the context: desc(ending), (asc)ending, rand(om), (n)one.",
					"type": "string"
				},
				"WrittingMode": {
					"description": "The mode for writing sets of attributes. It can be 'names', 'indices', or 'both'",
					"type": "string"
				}
			}
		}
	}
);

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CSAXJsonContextAttributes> CSAXJsonContextAttributes::registrar;

const char* const CSAXJsonContextAttributes::Desc()
{
	return description;
}

CSAXJsonContextAttributes::CSAXJsonContextAttributes() :
	objectNum(0),
	order(AO_Desc),
	mode(WM_Names)
{
	
}
CSAXJsonContextAttributes::~CSAXJsonContextAttributes()
{
	for(int i = 0; i < attributes.size(); ++i) {
		if(attributes[i].Image.Objects != 0) {
			delete[] attributes[i].Image.Objects;
		}
	}
}

JSON CSAXJsonContextAttributes::DescribeAttributeSet(int* attrsSet, int attrsCount)
{
	assert(attrsSet != 0);

	std::stringstream rslt;
	rslt << "{" << "\"Count\":" << attrsCount << std::endl;
	if( mode == WM_Names || mode == WM_Both )  {
		rslt << ",\"Names\":[\n\t";
		for( int i = 0; i < attrsCount; ++i) {
			if( i != 0 ) {
				rslt << ",\n\t";
			}
			const int attr = attrsSet[i];
			assert(0 <= attr && attr < attrOrder.size());
			const int internalAttr = attrOrder[attr];
			assert(0 <= internalAttr && internalAttr < attrOrder.size());
			rslt << "\"" << attributes[internalAttr].Name << "\"";
		}
		rslt << "\n]" << std::endl;
	}
	if( mode == WM_Indices || mode == WM_Both )  {
		rslt << ",\"Inds\":[\n\t";
		for( int i = 0; i < attrsCount; ++i) {
			if( i != 0 ) {
				rslt << ",\n\t";
			}
			const int attr = attrsSet[i];
			assert(0 <= attr && attr < attrOrder.size());
			const int internalAttr = attrOrder[attr];
			assert(0 <= internalAttr && internalAttr < attrOrder.size());
			rslt <<  " " << internalAttr;
		}
		rslt << "\n]" << std::endl;
	}
	rslt << "}" << std::endl;
	return rslt.str();
}

void CSAXJsonContextAttributes::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CSAXJsonContextAttributes::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == GetType() );
	assert( string( params["Name"].GetString() ) == GetName() );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for file path to the context";
		throw new CJsonException("CSAXJsonContextAttributes::LoadParams", error);
	}
	// Reading class labels information
	if(!(params["Params"].HasMember("ContextFilePath") && params["Params"]["ContextFilePath"].IsString())) {
		error.Data = json;
		error.Error = "Params.ContextFilePath is not found or is not a file name.";
		throw new CJsonException("CSAXJsonContextAttributes::LoadParams", error);
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
	if(params["Params"].HasMember("WrittingMode") && params["Params"]["WrittingMode"].IsString()) {
		const string modeStr = params["Params"]["WrittingMode"].GetString();
		if( modeStr == "both") {
			mode = WM_Both;
		} else if( modeStr == "indices") {
			mode = WM_Indices;
		} else {
			mode = WM_Names;
		}
	}

	filePath = params["Params"]["ContextFilePath"].GetString();

	loadContext();
}

JSON CSAXJsonContextAttributes::SaveParams() const
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
	CSAXAttributeReader(CSAXJsonContextAttributes::TAttributes& _attrs) :
		attributes(_attrs), mode(M_Initialize),
		isInsideAttributes(false), dataEntryPoint(-1), curObjectNumber(-1),
		hasAttrNames(false)
	{ curIndex.SetKey("ROOT"); }

	void SetMode(TMode m) {
		mode = m;
		curIndex.SetKey("ROOT");
		isInsideAttributes = false;
		dataEntryPoint = -1;
		curObjectNumber = -1;
		stack.clear();

		if( m == M_Read) {
			assert(attrsAddedObjects.size() == 0);
			assert(attributes.size() != 0);
			attrsAddedObjects.resize(attributes.size(), 0);
			for( int i = 0; i < attributes.size(); ++i ) {
				attributes[i].Image.Objects = new int[attributes[i].Image.ImageSize];
			}
		}
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
				++attributes[i].Image.ImageSize;
				break;
			case M_Read: {
				assert(attrsAddedObjects.size() == attributes.size());
				const unsigned objPos = attrsAddedObjects[i];
				assert(objPos < attributes[i].Image.ImageSize);
				const_cast<int*>(attributes[i].Image.Objects)[objPos] = curObjectNumber;
				++attrsAddedObjects[i];
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
		if( dataEntryPoint == stack.size() - 1) {
			assert(last.Index.Key == "Data");
			dataEntryPoint = -1;
		}

		curIndex = last.Index;
		incIndex();
		stack.pop_back();

		return true;
	}

	int GetObjectNumber() const
		{return curObjectNumber+1;}

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
	CSAXJsonContextAttributes::TAttributes& attributes;
	// The number of added objects for every attribute
	vector<unsigned> attrsAddedObjects;

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
	bool canEnterAttrNames() const { return true; }
	// Validates if the current image of the stack could correspond to data entry point
	bool canEnterData() const { return true; }

};

///////////////////////////////////////////////////////////////////////////////
// Class for sorting the attributes
namespace {
	class CAttrSorter {
	public:
		CAttrSorter( CSAXJsonContextAttributes::TAttrOrderMode _order, const CSAXJsonContextAttributes::TAttributes& _attrs ) :
			order(_order), attrs( _attrs )
		{
			if( order == CSAXJsonContextAttributes::AO_Random ) {
				initRandomOrder();
			}
		}
		bool operator()( DWORD i1, DWORD i2 ) const
		{
			switch( order ) {
			case CSAXJsonContextAttributes::AO_None:
				return i1 < i2;
			case CSAXJsonContextAttributes::AO_Asc:
				assert( i1 < attrs.size() && i2 < attrs.size());
				return attrs[i1].Image.ImageSize < attrs[i2].Image.ImageSize;
			case CSAXJsonContextAttributes::AO_Desc:
				assert( i1 < attrs.size() && i2 < attrs.size());
				return attrs[i1].Image.ImageSize > attrs[i2].Image.ImageSize;
			case CSAXJsonContextAttributes::AO_Random:
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
		const CSAXJsonContextAttributes::TAttrOrderMode order;
		const CSAXJsonContextAttributes::TAttributes& attrs;
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
void CSAXJsonContextAttributes::loadContext()
{

	CJsonError error;
	string path;
	RelativePathes::GetFullPath( filePath, path);

	{
		// Buffer for io operations.
		char buffer[4096];

		rapidjson::Reader reader;
		CSAXAttributeReader saxHandler(attributes);

		FILE* fp = fopen( path.c_str(), "r");
		if( fp == 0 ) {
			error.Data = path;
			error.Error = "Cannot open the file";
			throw new CJsonException( "CSAXJsonContextAttributes::LoadParams", error );
		}
		try{
			// First pass, just counting the number of objects
			{
				rapidjson::FileReadStream is( fp, buffer, sizeof(buffer) );
				saxHandler.SetMode(CSAXAttributeReader::M_Initialize);
				if(!reader.Parse(is, saxHandler)) {
					rapidjson::ParseErrorCode e = reader.GetParseErrorCode();
					error.Data = path;
					error.Offset = reader.GetErrorOffset();
					error.Error = rapidjson::GetParseError_En(e);
					throw new CJsonException( "CSAXJsonContextAttributes::LoadParams", error );
				}
			}
			// Second pass, reading the actual attributes
			{
				fseek(fp,0,SEEK_SET); // moving to the begining of the stream
				rapidjson::FileReadStream is( fp, buffer, sizeof(buffer) );

				saxHandler.SetMode(CSAXAttributeReader::M_Read);
				if(!reader.Parse(is, saxHandler)) {
					rapidjson::ParseErrorCode e = reader.GetParseErrorCode();
					error.Data = path;
					error.Offset = reader.GetErrorOffset();
					error.Error = rapidjson::GetParseError_En(e);
					throw new CJsonException( "CSAXJsonContextAttributes::LoadParams", error );
				}
			}
		} catch(CException* e) {
			fclose(fp);
			throw e;
		}

		objectNum = saxHandler.GetObjectNumber();
	}

	// Sorting attributes
	attrOrder.resize(attributes.size());
	for(int i = 0; i < attrOrder.size(); ++i) {
		attrOrder[i] = i;
	}
	if( order != AO_None ) {
		CAttrSorter sorter( order, attributes );
		sort( attrOrder.begin(), attrOrder.end(), sorter );
	}
}


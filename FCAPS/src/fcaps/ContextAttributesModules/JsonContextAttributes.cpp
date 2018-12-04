// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2011-2015, v0.7

#include <fcaps/ContextAttributesModules/JsonContextAttributes.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>
#include <ListWrapper.h>

#include <rapidjson/document.h>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(x...) #x

const char description[] =
STR(
	{
	"Name":"Context Attributes from JSON",
	"Description":"An object provides a context by means of its attirbutes based on a json file",
	"Params": {
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of Json Context Attributes",
			"type": "object",
			"properties": {
				"ContextFilePath":{
					"description": "The path to a file containing the context in JSON format",
					"type": "file-path"
				}
			}
		}
	}
);

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CJsonContextAttributes> CJsonContextAttributes::registrar;

const char* const CJsonContextAttributes::Desc()
{
	return description;
}

CJsonContextAttributes::CJsonContextAttributes() :
	objectNum(0),
	order(AO_Desc)
{
	
}
CJsonContextAttributes::~CJsonContextAttributes()
{
	for(int i = 0; i < attributes.size(); ++i) {
		if(attributes[i].Image.ImageSize > 0) {
			delete[] attributes[i].Image.Objects;
		}
	}
}

JSON CJsonContextAttributes::DescribeAttributeSet(int* attrsSet, int attrsCount)
{
	assert(attrsSet != 0);

	std::stringstream rslt;
	rslt << "{" << "\"Count\":" << attrsCount << ", "
	     << "\"Names\":[";
	for( int i = 0; i < attrsCount; ++i) {
		if( i != 0 ) {
			rslt << ",";
		}
		const int attr = attrsSet[i];
		assert(0 <= attr && attr < attributes.size());
		rslt << "\"" << attributes[attr].Name << "\"";
	}
	rslt << "]}";
	return rslt.str();
}

void CJsonContextAttributes::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CJsonContextAttributes::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == GetType() );
	assert( string( params["Name"].GetString() ) == GetName() );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for file path to the context";
		throw new CJsonException("CJsonContextAttributes::LoadParams", error);
	}
	// Reading class labels information
	if(!(params["Params"].HasMember("ContextFilePath") && params["Params"]["ContextFilePath"].IsString())) {
		error.Data = json;
		error.Error = "Params.ContextFilePath is not found or is not a file name.";
		throw new CJsonException("CJsonContextAttributes::LoadParams", error);
	}

	filePath = params["Params"]["ContextFilePath"].GetString();

	loadContext();


}

JSON CJsonContextAttributes::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", ContextAttributesModuleType, alloc )
		.AddMember( "Name", JsonContextAttributes, alloc )
		.AddMember("Params", "TODO", alloc);

	// TODO 
	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

// Class for sorting the attributes
class CAttrSorter {
public:
	CAttrSorter( CJsonContextAttributes::TAttributeOrder _order, const vector< CList<int> >& _attrs ) :
		order(_order), attrs( _attrs )
	{
		if( order == CJsonContextAttributes::AO_Random ) {
			initRandomOrder();
		}
	}
	bool operator()( DWORD i1, DWORD i2 ) const
	{
		switch( order ) {
		case CJsonContextAttributes::AO_None:
			return true;
		case CJsonContextAttributes::AO_Asc:
			return attrs[i1].Size() < attrs[i2].Size();
		case CJsonContextAttributes::AO_Desc:
			return attrs[i1].Size() > attrs[i2].Size();
		case CJsonContextAttributes::AO_Random:
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
	const CJsonContextAttributes::TAttributeOrder order;
	const vector< CList<int> >& attrs;
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

// Load context from a JSON file
void CJsonContextAttributes::loadContext()
{
	CJsonError error;
	rapidjson::Document jsonContext;
	if( !ReadJsonFile( filePath, jsonContext, error ) ) {
		throw new CJsonException( "CJsonContextAttributes::LoadParams", error );
	}
	if(!jsonContext.IsArray() || jsonContext.Size() < 2
	   || !jsonContext[1].IsObject() || !jsonContext[1].HasMember("Data") || !jsonContext[1]["Data"].IsArray())
	{
		throw new CTextException( "CJsonContextAttributes::LoadParams", "Not a context File: Data is not found" );
	}

	const rapidjson::Value& jsonObjs = jsonContext[1]["Data"];
	objectNum = jsonObjs.Size();

	vector< CList<int> > attrs;
	for(int i = 0; i < objectNum; ++i) {
		if(!jsonObjs[i].IsObject() || !jsonObjs[i].HasMember("Inds") || !jsonObjs[i]["Inds"].IsArray()) {
			cout << "\tWARNING: Incorrect object " << i << ". IGNORED." << endl;
			continue;
		}
		const rapidjson::Value& inds = jsonObjs[i]["Inds"];
		for(int j = 0; j < inds.Size(); ++j) {
			if(!inds[j].IsUint()) {
				cout << "\tWARNING: Incorrect object " << i << ". IGNORED." << endl;
				continue;
			}
			const int a = inds[j].GetInt();
			assert(a >= 0);
			if(a >= attrs.size()) {
				attrs.resize(a+1);
			}
			attrs[a].PushBack(i);
		}
			
	}

	// For names retrieval of the attributes
	const bool hasAttrNames = jsonContext[0].HasMember("Params") && jsonContext[0]["Params"].HasMember("AttrNames")
		&& jsonContext[0]["Params"]["AttrNames"].IsArray();

	// Attributes are collected.
	// Sorting attributes
	vector<DWORD> attrOrder;
	attrOrder.resize(attrs.size());
	for(int i = 0; i < attrOrder.size(); ++i) {
		attrOrder[i] = i;
	}
	if( order != AO_None ) {
		CAttrSorter sorter( order, attrs );
		sort( attrOrder.begin(), attrOrder.end(), sorter );
	}
	// Converting attributes to CPatternImage
	attributes.resize(attrs.size());
	for(int i = 0; i < attrOrder.size(); ++i) {
		const int a = attrOrder[i];
		assert(0 <= a && a < attrs.size());
		attributes[i].Image.PatternId = i;
		attributes[i].Image.ImageSize = attrs[a].Size();
		if(hasAttrNames && a < jsonContext[0]["Params"]["AttrNames"].Size() && jsonContext[0]["Params"]["AttrNames"][a].IsString()) {
			attributes[i].Name = jsonContext[0]["Params"]["AttrNames"][a].GetString();
		} else {
			attributes[i].Name = StdExt::to_string(a);
		}
		int* objects = new int[attributes[i].Image.ImageSize];
		attributes[i].Image.Objects = objects;
		auto itr = attrs[a].Begin();
		auto end = attrs[a].End();
		for(int j = 0; itr != end; ++itr, ++j ) {
			assert( j < attributes[i].Image.ImageSize);
			const int obj = *itr;
			assert( 0 <= obj && obj < objectNum);
			objects[j] = obj;
		}
	}
}


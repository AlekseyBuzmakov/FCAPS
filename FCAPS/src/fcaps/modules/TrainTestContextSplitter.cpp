#include <fcaps/modules/TrainTestContextSplitter.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/writer.h>

#include <time.h>

using namespace std;

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CTrainTestContextSplitter> CTrainTestContextSplitter::registrar(
	FilterModuleType, TrainTestContextSplitter );

CTrainTestContextSplitter::CTrainTestContextSplitter() :
	proportion(0.9),
	seed( time(0) )
{
}

void CTrainTestContextSplitter::Process()
{
	static const char place[] = "CTrainTestContextSplitter::Process";
	CJsonError error(inputFile,"");
	rapidjson::Document doc;
	if( !ReadJsonFile( inputFile, doc, error ) ) {
		throw new CJsonException( place, error );
	}
	if( !doc.IsArray() || doc.Size() < 2 || !doc[1].IsObject()
		|| !doc[1].HasMember("Data") || !doc[1]["Data"].IsArray() )
	{
		error.Error = "DATA[1].Data not found. Not a valid context";
		throw new CJsonException( place, error );
	}

	if( !doc[0].IsObject() ) {
		doc[0].SetObject();
	}
	JSON json = SaveParams();
	rapidjson::Document thisParams;
	if( !ReadJsonString( json, thisParams, error ) ) {
		assert(false);
	}
	if( !doc[0].HasMember("Filters") ) {
		doc[0].AddMember( "Filters", rapidjson::Value().SetArray(), doc.GetAllocator());
	}
	doc[0]["Filters"].PushBack(thisParams, doc.GetAllocator());

	params.SetObject() = doc[0];
	if( params.HasMember("ObjNames") && params["ObjNames"].IsArray() ) {
		objNames = params["ObjNames"];
		params.RemoveMember("ObjNames");
	}

	data = doc[1]["Data"];
	vector<int> indices;
	indices.resize(data.Size());
	for( DWORD i = 0; i < data.Size(); ++i ) {
		indices[i]=i;
	}

	srand(seed);
	std::random_shuffle(indices.begin(), indices.end());
	assert( 0 <= proportion && proportion < indices.size() );
	DWORD testStart = (DWORD)(indices.size() * proportion);
	if( testStart >= indices.size() ) {
		testStart = indices.size() - 1;
	}
	if( testStart <= 0 ) {
		testStart = 1;
	}

	// Writing train file
	writeDataset(indices.begin(), indices.begin() + testStart, results[0]);
	writeDataset(indices.begin()+testStart, indices.end(), results[1]);
}

void CTrainTestContextSplitter::LoadParams( const JSON& json )
{
	static const char place[] = "CTrainTestContextSplitter::LoadParams";
	CJsonError error;
	error.Data = json;
	rapidjson::Document doc;
	if( !ReadJsonString( json, doc, error ) ) {
		throw new CJsonException( place, error );
	}
	assert( string( doc["Type"].GetString() ) == FilterModuleType );
	assert( string( doc["Name"].GetString() ) == TrainTestContextSplitter );
	if( !(doc.HasMember( "Params" ) && doc["Params"].IsObject()) ) {
		error.Error = "Params is not found. Necessary for output names";
		throw new CJsonException(place, error);
	}
	const rapidjson::Value& params=doc["Params"];

	results.clear();
	if( !params.HasMember("TrainPath") || !params["TrainPath"].IsString() ) {
		error.Error = "'TrainPath' not found";
		throw new CJsonException(place, error);
	}
	results.push_back( params["TrainPath"].GetString() );
	if( !params.HasMember("TestPath") || !params["TestPath"].IsString() ) {
		error.Error = "'TestPath' not found";
		throw new CJsonException(place, error);
	}
	results.push_back( params["TestPath"].GetString() );
	if( params.HasMember("TrainTestProportion") && params["TrainTestProportion"].IsDouble() ) {
		proportion = params["TrainTestProportion"].GetDouble();
		proportion = min( proportion, 1.0);
		proportion = max( proportion, 0.0);
	}
	if( params.HasMember("Seed") && params["Seeed"].IsInt() ) {
		seed = params["Seed"].GetInt();
	}
}
JSON CTrainTestContextSplitter::SaveParams() const
{
	assert( results.size() == 2 );
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", FilterModuleType, alloc )
		.AddMember( "Name", TrainTestContextSplitter, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "TrainPath", rapidjson::Value().SetString(
				rapidjson::StringRef(results[0].c_str())), alloc )
			.AddMember( "TestPath", rapidjson::Value().SetString(
				rapidjson::StringRef(results[1].c_str())), alloc )
			.AddMember( "TrainTestProportion", rapidjson::Value().SetDouble( proportion ), alloc )
			.AddMember( "Seed", rapidjson::Value().SetInt( seed ), alloc ),
		alloc );
	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

void CTrainTestContextSplitter::writeDataset(
	std::vector<int>::const_iterator b, std::vector<int>::const_iterator e,
	const std::string& path )
{
	initObjectNames(b,e);

	CDestStream dst( path );
	OStreamWrapper os( dst );
	rapidjson::Writer<OStreamWrapper> writer( os );

	dst << "[";
	params.Accept( writer );
	writer.Reset(os);
	dst << ",\n{\"Count\":" << e-b << ",\"Data\":[\n";
	bool writeComma = false;
	for( ; b != e; ++b ) {
		if( writeComma) {
			dst << ",";
		}
		writeComma = true;

		assert(*b < data.Size());
		data[*b].Accept( writer );
		writer.Reset(os);
		dst << "\n";
	}
	dst << "]}]";
}

void CTrainTestContextSplitter::initObjectNames(std::vector<int>::const_iterator b, std::vector<int>::const_iterator e )
{
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	if( !params.HasMember("ObjNames") ) {
		params.AddMember("ObjNames", rapidjson::Value().SetArray(), alloc );
	} else {
		params["ObjNames"].Clear();
	}
	rapidjson::Value& names = params["ObjNames"];
	for( ; b != e; ++b ) {
		if(objNames.IsArray() && *b < objNames.Size() && objNames[*b].IsString() ) {
			names.PushBack( objNames[*b], alloc );
		} else {
			const string result = StdExt::to_string(*b);
			names.PushBack( rapidjson::Value().SetString(result.c_str(), alloc ), alloc );
		}
	}
}


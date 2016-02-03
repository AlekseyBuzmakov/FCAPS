// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2016, v0.7

// Author: Aleksey Buzmakov
// Description: Implementation of pattern enumeration interface. It is realy on the original code of Gaston from an external library.
//  In order to rely on existing code of Gaston, the most simple thing is to find places in this code where patterns are reported
//  and then report the patterns by a callback. However in this case pattern reported by callback is hard to translate to the IPatternEnumerator interface
//  Accordingly, we run Gaston in parallel thread and are switching between threads when we want to get next pattern by the callback.

#include "GastonGraphPatternEnumerator.h"

#include <JSONTools.h>
#include <RelativePathes.h>

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

//#define DEBUG_OUTPUT

class CThumbCOUT {
public:
    template<typename T>
    CThumbCOUT& operator << ( T )
    { return *this; }
} thumbCOUT;

#ifdef DEBUG_OUTPUT
#define COUT std::cout
#else
#define COUT thumbCOUT
#endif // DEBUG_OUTPUT

using namespace std;
using namespace boost;


////////////////////////////////////////////////////////////////////////

const std::string& CGastonGraphPatternEnumerator::CLabelMap::GetLabel( DWORD id ) const
{
	assert( id < labelToId.size() );
	if( id >= labelToId.size() ) {
        static const std::string emptyString;
		return emptyString;
	} else {
		return idToLabel[id];
	}
}
// Returns id by label
const DWORD CGastonGraphPatternEnumerator::CLabelMap::GetId( const std::string& label )
{
	CStdIterator<CLabelToIdMap::iterator> itr( labelToId.find( label ), labelToId.end() );
	if( itr.IsEnd() ) {
		itr.Reset( labelToId.insert( pair<string,DWORD>(label, idToLabel.size() ) ).first, labelToId.end() );
		idToLabel.push_back(label);
	}
	return (*itr).second;
}

////////////////////////////////////////////////////////////////////////

struct CGastonGraphPatternEnumerator::CSyncData{
	// A mutex to ensure run of only one thread.
	boost::mutex Access;
	// Thread is active when new graph arrives.
	boost::condition_variable HasNewGraph;
	// Thread is active when new graph is requested
	boost::condition_variable NewGraphRequested;

	// DATA
	enum TDataState{
		DS_Ready = 0,
		DS_InPreparation,
		DS_End,

		DS_EnumCount
	} DataState;
	bool IsGraphRequested;

	// The curr graph pattern
	CPatternImage PatternImage;
	// Was the found graph usefull
	TCurrentPatternUsage currPatternUsage;

	CSyncData() :
		DataState( DS_InPreparation ), IsGraphRequested( false ), currPatternUsage( CPU_EnumCount ) {}
};

////////////////////////////////////////////////////////////////////////

CModuleRegistrar<CGastonGraphPatternEnumerator> CGastonGraphPatternEnumerator::registar(PatternEnumeratorModuleType, GastonGraphPatternEnumeratorModule);

CGastonGraphPatternEnumerator::CGastonGraphPatternEnumerator::CGastonGraphPatternEnumerator() :
	libraryPath( "libGaston.so" ),
	inputPath( boost::filesystem::unique_path().native() ),
	removeInput( true ),
	syncData( new CSyncData ),
	isGastonRun( false ),
	gastonMinSupport( 0 ),
	gastonMaxPtrnSize( -1 ),
	gastonMode( GRM_All )
{
    //ctor
}

CGastonGraphPatternEnumerator::~CGastonGraphPatternEnumerator()
{
    if( removeInput ) {
        boost::filesystem::remove(inputPath);
    }
}

void CGastonGraphPatternEnumerator::AddObject( DWORD objectNum, const JSON& intent )
{
    if( !inputStream.is_open() ) {
        inputStream.open( inputPath );
    }
	CJsonError errorText;
	rapidjson::Document graph;
	if( !ReadJsonString( intent, graph, errorText ) ) {
		throw new CJsonException( "CGastonGraphPatternEnumerator::AddObject", errorText );
	}

	if( !graph.IsArray() || graph.Size() < 2 ) {
		errorText.Data = intent;
		errorText.Error = "Graph object should be an array with >= 2 elements.";
		throw new CJsonException( "CGastonGraphPatternEnumerator::AddObject", errorText );
	}
	assert( inputStream.is_open() );

	origObjIDs.push_back( objectNum );

	inputStream << "t # " << objectNum << "\n";

	if( !graph[1].HasMember("Nodes") || !graph[1]["Nodes"].IsArray() ) {
		errorText.Data = intent;
		errorText.Error = "First element of the graph object should have an array of vertices called 'Nodes'";
		throw new CJsonException( "CGastonGraphPatternEnumerator::AddObject", errorText );
	}
	const rapidjson::Value& nodes = graph[1]["Nodes"];
	for( DWORD i = 0; i < nodes.Size(); ++i ) {
		const rapidjson::Value& node = nodes[i];
		if( !node.IsObject() || !node.HasMember("L") || !node["L"].IsString() ) {
			errorText.Data = intent;
			errorText.Error = "Every node should be labeled. The label is stored in property 'L' of the node object.";
			throw new CJsonException( "CGastonGraphPatternEnumerator::AddObject", errorText );
		}
		inputStream << "v " << i << " " << vertexLabelMap.GetId( node["L"].GetString() ) << "\n";
	}

	if( !graph[2].HasMember("Arcs") || !graph[2]["Arcs"].IsArray() ) {
		errorText.Data = intent;
		errorText.Error = "Second element of the graph object should have an array of edges called 'Arcs'";
		throw new CJsonException( "CGastonGraphPatternEnumerator::AddObject", errorText );
	}
	const rapidjson::Value& arcs = graph[2]["Arcs"];
	for( DWORD i = 0; i < arcs.Size(); ++i ) {
		const rapidjson::Value& arc = arcs[i];
		if( !arc.IsObject()
		    || !arc.HasMember("S") || !arc["S"].IsUint()
		    || !arc.HasMember("D") || !arc["D"].IsUint()
		    || !arc.HasMember("L") || !arc["L"].IsString() )
		{
			errorText.Data = intent;
			errorText.Error = "Every arc should have source ('S') and destination ('D') node id and should be labeled ('L').";
			throw new CJsonException( "CGastonGraphPatternEnumerator::AddObject", errorText );
		}
		inputStream << "e " << arc["S"].GetUint() << " " << arc["D"].GetUint() << " " << edgeLabelMap.GetId( arc["L"].GetString() ) << "\n";
	}
}
bool CGastonGraphPatternEnumerator::GetNextPattern( TCurrentPatternUsage usage, CPatternImage& pattern )
{
    if( !isGastonRun ) {
        // Does not allow for modification of the input data anymore
        if( inputStream.is_open() ) {
            inputStream.close();
        }
        createGastonThread();
        isGastonRun = true;
    }

	return getNextPattern( usage, pattern );
}
void CGastonGraphPatternEnumerator::ClearMemory( CPatternImage& pattern )
{
	delete[] pattern.Objects;
}

void CGastonGraphPatternEnumerator::LoadParams( const JSON& json )
{
	CJsonError errorText;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "CGastonGraphPatternEnumerator::LoadParams", errorText );
	}
	assert( string( params["Type"].GetString() ) == GetType() );
	assert( string( params["Name"].GetString() ) == GetName());

	if( params.HasMember( "Params" ) && params["Params"].IsObject() ) {
		const rapidjson::Value& paramsObj = params["Params"];

		if( paramsObj.HasMember("LibPath") && paramsObj["LibPath"].IsString() ) {
			RelativePathes::GetFullPath( paramsObj["LibPath"].GetString(), libraryPath);
		}
		if( paramsObj.HasMember("InputPath") && paramsObj["InputPath"].IsString() ) {
			RelativePathes::GetFullPath( paramsObj["InputPath"].GetString(), inputPath);
		}
		if( paramsObj.HasMember( "RemoveInput" ) && paramsObj["RemoveInput"].IsBool() ) {
			removeInput = paramsObj["RemoveInput"].GetBool();
		}
		if( paramsObj.HasMember("PatternPath") && paramsObj["PatternPath"].IsString() ) {
			RelativePathes::GetFullPath( paramsObj["PatternPath"].GetString(), patternPath);
		}
		if( paramsObj.HasMember("MinSupport") && paramsObj["MinSupport"].IsInt() ) {
			gastonMinSupport = paramsObj["MinSupport"].GetInt();
		}
		if( paramsObj.HasMember("MaxGraphSize") && paramsObj["MaxGraphSize"].IsInt() ) {
			gastonMaxPtrnSize = paramsObj["MaxGraphSize"].GetInt();
		}
		if( paramsObj.HasMember("GastonMode") && paramsObj["GastonMode"].IsString() ) {
			const string mode( paramsObj["GastonMode"].GetString() );
			if( mode == "Trees" ) {
				gastonMode = GRM_Trees;
			} else if ( mode == "Pathes" ) {
				gastonMode = GRM_Pathes;
			} else {
				gastonMode = GRM_All;
			}
		}
	}
	loadLibrary();
}
JSON CGastonGraphPatternEnumerator::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	const char* modeName = 0;
	switch( gastonMode ) {
	case GRM_All:
		modeName = "Full";
		break;
	case GRM_Trees:
		modeName = "Trees";
		break;
	case GRM_Pathes:
		modeName = "Pathes";
		break;
	default:
		assert(false);
		modeName = "Error";
	}
	params.SetObject()
		.AddMember( "Type", PatternEnumeratorModuleType, alloc )
		.AddMember( "Name", GastonGraphPatternEnumeratorModule, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
		            .AddMember( "LibPath", rapidjson::Value().SetString( rapidjson::StringRef(libraryPath.c_str())), alloc )
		            .AddMember( "InputPath", rapidjson::Value().SetString( rapidjson::StringRef(inputPath.c_str())), alloc )
		            .AddMember( "RemoveInput", rapidjson::Value().SetBool(removeInput), alloc )
		            .AddMember( "PatternPath", rapidjson::Value().SetString( rapidjson::StringRef(patternPath.c_str())), alloc )
		            .AddMember( "MinSupport", rapidjson::Value().SetInt(gastonMinSupport), alloc )
		            .AddMember( "MaxGraphSize", rapidjson::Value().SetInt(gastonMaxPtrnSize), alloc )
		            .AddMember( "GastonMode", rapidjson::Value().SetString( rapidjson::StringRef(modeName)), alloc ),
			alloc );

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

void CGastonGraphPatternEnumerator::RunGastonThread()
{
    {
        // It should be unlocked before running Gasotn in order to lock it later in callback when we are going to report a graph
        boost::unique_lock<boost::mutex> lock( syncData->Access );
        COUT << "(!) Gaston: Waiting until first graph requested\n";

        // Waiting until the first graph is requested
        while(!syncData->IsGraphRequested) {
            syncData->NewGraphRequested.wait(lock);
        }
        syncData->IsGraphRequested = false;

        // We are not accessing syncData now
    }

	// Just run Gaston
	COUT << "(!) Gaston: Starting\n";
	const bool res = runGaston( this, inputPath.c_str(), gastonMinSupport, &gastonCallback, gastonMaxPtrnSize, gastonMode );
	assert(res);

    {
        // But now we need a new access to syncData.
        boost::unique_lock<boost::mutex> lock( syncData->Access );

        syncData->DataState = CSyncData::DS_End;
        syncData->HasNewGraph.notify_one();
    }
}

// A function used as a callback for Gaston algo.
bool CGastonGraphPatternEnumerator::gastonCallback( LibGastonDataRef data, const LibGastonGraph* graph )
{
	assert( data != 0 );
	return reinterpret_cast<CGastonGraphPatternEnumerator*>(data)->registerGraph( graph );
}

// Loads libraries and initiate the procedure.
void CGastonGraphPatternEnumerator::loadLibrary()
{
    COUT << "Openning library '" << libraryPath << "' with Gaston algo.\n";
	gastonLib.Open(libraryPath);
	runGaston = reinterpret_cast<RunGastonFunc>(gastonLib.GetFunc( "RunGaston" ));
	assert( runGaston != 0 );

	if( !patternPath.empty() ) {
		patternStream.open( patternPath );
	}
}

// A simple wrapper to avoid copy of the class
class CThreadStarter {
public:
    CThreadStarter( CGastonGraphPatternEnumerator& _e ) : e(_e) {}

    void operator() ()
	{ e.RunGastonThread(); COUT << "(!) Finishing gaston thread\n"; }
private:
    CGastonGraphPatternEnumerator& e;
};

// Run Gaston in a separate thread.
void CGastonGraphPatternEnumerator::createGastonThread()
{
	assert( runGaston != 0 );
	COUT << "(!) Starting gaston thread\n";
	boost::thread( CThreadStarter( *this ) );
}
// Registers a graph found by Gaston.
bool CGastonGraphPatternEnumerator::registerGraph( const LibGastonGraph* graph )
{
	COUT << "(!) Gaston: callback entry for graph " << syncData->PatternImage.PatternId+1 << "\n";

	assert( graph != 0 );
	boost::unique_lock<boost::mutex> lock( syncData->Access );
	//Updating currGraphNumber
	++syncData->PatternImage.PatternId;
	COUT << "(!) Gaston: Registering graph " << syncData->PatternImage.PatternId << "\n";


	// Updating graph info
	CPatternImage& pi = syncData->PatternImage;
	if( pi.ImageSize > 0 ) {
		delete[] pi.Objects;
	}

	pi.ImageSize = graph->Support;
	pi.Objects = new int[pi.ImageSize];
	memcpy(pi.Objects, graph->Objects, pi.ImageSize * sizeof( pi.Objects[0] ));

	syncData->DataState = CSyncData::DS_Ready;
	syncData->HasNewGraph.notify_one();

	// Writing graph info
	writePattern( graph );

	COUT << "(!) Gaston: Registered graph " << syncData->PatternImage.PatternId << "\n";

	// Waiting untill the next graph is requested
	while(!syncData->IsGraphRequested) {
		syncData->NewGraphRequested.wait(lock);
	}
	syncData->IsGraphRequested = false;
	COUT << "(!) Gaston: Return from callback for graph " << syncData->PatternImage.PatternId << "\n";
	return syncData->currPatternUsage == CPU_Expand;
}
// Takes the next pattern. Function is used to put together with registerGraph.
inline bool CGastonGraphPatternEnumerator::getNextPattern( TCurrentPatternUsage usage, CPatternImage& pattern )
{
	boost::unique_lock<boost::mutex> lock( syncData->Access );

	// Requesting next graph
	COUT << "(!) SOFIA: requesting next graph (curr is " << syncData->PatternImage.PatternId << ") \n";
	syncData->currPatternUsage = usage;
	syncData->IsGraphRequested = true;
	syncData->NewGraphRequested.notify_one();

	// Waiting while the next graph is ready
	while( syncData->DataState == CSyncData::DS_InPreparation ) {
		syncData->HasNewGraph.wait(lock);
	}
	if( syncData->DataState == CSyncData::DS_End ) {
		return false;
	}
	assert( syncData->DataState == CSyncData::DS_Ready );

	syncData->DataState = CSyncData::DS_InPreparation;
	COUT << "(!) SOFIA: graph " << syncData->PatternImage.PatternId << " is ready\n";

	ClearMemory( pattern );
	pattern.PatternId = syncData->PatternImage.PatternId;
	pattern.ImageSize = syncData->PatternImage.ImageSize;
	pattern.Objects = syncData->PatternImage.Objects;

	syncData->PatternImage.Objects = 0;
	syncData->PatternImage.ImageSize = 0;

	return true;
}

void CGastonGraphPatternEnumerator::writePattern( const LibGastonGraph* graph )
{
	assert( graph != 0 );
	if( !patternStream.is_open() ) {
		return;
	}

	patternStream << "# " << graph->Support << "\n";
	patternStream << "t # " << syncData->PatternImage.PatternId << "\n";

	for (DWORD i = 0; i < graph->VertexCount; i++) {
		patternStream << "v " << i << " " << graph->Vertices[i] /*<< " # " << vertexLabelMap.GetLabel(graph->Vertices[i])*/ << "\n";
	}
	for (DWORD i = 0; i < graph->EdgeCount; i++) {
		patternStream << "e " << i
		              << " " << graph->Edges[i].From << " " << graph->Edges[i].To
		              /*<< " # " << edgeLabelMap.GetLabel(graph->Edges[i].Label)*/ << "\n";
	}
	patternStream << "x";
	for( DWORD i = 0; i < graph->Support; ++i ) {
        patternStream << " " << graph->Objects[i];
	}
	patternStream << "\n";

	patternStream.flush();
}

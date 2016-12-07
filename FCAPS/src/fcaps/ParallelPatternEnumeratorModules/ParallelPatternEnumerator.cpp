// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2016, v0.7

// Author: Aleksey Buzmakov
// Description: Implementation of pattern enumeration interface. It is realy on the original code of the ALGO from an external library.
//  In order to rely on existing code of ALGO, the most simple thing is to find places in this code where patterns are reported
//  and then report the patterns by a callback. However in this case pattern reported by callback is hard to translate to the IPatternEnumerator interface
//  Accordingly, we run ALGO in parallel thread and are switching between threads when we want to get next pattern by the callback.

#include "ParallelPatternEnumerator.h"

#include <fcaps/PatternEnumeratorByCallback.h>

#include <JSONTools.h>
#include <ModuleJSONTools.h>

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include <sstream>

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

struct CParallelPatternEnumerator::CSyncData{
	// A mutex to ensure run of only one thread.
	boost::mutex Access;
	// Thread is active when new graph arrives.
	boost::condition_variable HasNewPattern;
	// Thread is active when new graph is requested
	boost::condition_variable NewPatternRequested;

	// DATA
	enum TDataState{
		DS_Ready = 0,
		DS_InPreparation,
		DS_End,

		DS_EnumCount
	} DataState;
	bool IsPatternRequested;

	// The curr graph pattern
	CPatternImage PatternImage;
	// Was the found graph usefull
	TCurrentPatternUsage currPatternUsage;

	CSyncData() :
		DataState( DS_InPreparation ), IsPatternRequested( false ), currPatternUsage( CPU_EnumCount ) {}
};

////////////////////////////////////////////////////////////////////////

CModuleRegistrar<CParallelPatternEnumerator> CParallelPatternEnumerator::registar(PatternEnumeratorModuleType, ParallelPatternEnumeratorModule);

CParallelPatternEnumerator::CParallelPatternEnumerator() :
	syncData( new CSyncData ),
	isAlgoRun( false )
{
    //ctor
}

CParallelPatternEnumerator::~CParallelPatternEnumerator()
{
}

void CParallelPatternEnumerator::AddObject( DWORD objectNum, const JSON& intent )
{
	assert( peByCallback != 0 );
	peByCallback->AddObject(objectNum,intent);
}
bool CParallelPatternEnumerator::GetNextPattern( TCurrentPatternUsage usage, CPatternImage& pattern )
{
    if( !isAlgoRun ) {
        createAlgoThread();
        isAlgoRun = true;
    }

	return getNextPattern( usage, pattern );
}
void CParallelPatternEnumerator::ClearMemory( CPatternImage& pattern )
{
	delete[] pattern.Objects;
	pattern.Objects=0;
	pattern.ImageSize=0;
}

void CParallelPatternEnumerator::LoadParams( const JSON& json )
{
	CJsonError errorText;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "CParallelPatternEnumerator::LoadParams", errorText );
	}
	assert( string( params["Type"].GetString() ) == GetType() );
	assert( string( params["Name"].GetString() ) == GetName());

	if( !params.HasMember( "Params" ) || !params["Params"].IsObject() || !params["Params"].HasMember("PatternEnumeratorByCallback") ) {
		errorText.Data = json;
		errorText.Offset = -1;
		errorText.Error = "Module for enumerating patterns by callback is not found";
		throw new CJsonException( "CParallelPatternEnumerator::LoadParams",
		    errorText );
	}
	rapidjson::Value& pe = params["Params"]["PatternEnumeratorByCallback"];

	string error;
	peByCallback.reset(
		dynamic_cast<IPatternEnumeratorByCallback*>( CreateModuleFromJSON( pe, error ) ) );
	if( peByCallback.get() == 0 ) {
		stringstream destStr;
		destStr << "Cannot create the module ";
		if( !error.empty() ) {
			destStr << "with an error <<\n" << error << "\n>>";
		}
		throw new CTextException( "CParallelPatternEnumerator::LoadParams", destStr.str() );
	}
}

JSON CParallelPatternEnumerator::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", PatternEnumeratorModuleType, alloc )
		.AddMember( "Name", ParallelPatternEnumeratorModule, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject(), alloc );

    const IModule& module = dynamic_cast<const IModule&>( *peByCallback );
    rapidjson::Document internalParams;
    internalParams.Parse( module.SaveParams().c_str() );
    params["Params"].AddMember( "PatternEnumeratorByCallback", internalParams.Move(), alloc );

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

void CParallelPatternEnumerator::RunAlgoThread()
{
	assert(syncData != 0);
	assert(peByCallback != 0);
    {
        // It should be unlocked before running the Algo in order to lock it later in callback when we are going to report a graph
        boost::unique_lock<boost::mutex> lock( syncData->Access );
        COUT << "(!) ALGO: Waiting until first pattern requested\n";

        // Waiting until the first pattern is requested
        while(!syncData->IsPatternRequested) {
            syncData->NewPatternRequested.wait(lock);
        }
        syncData->IsPatternRequested = false;

        // We are not accessing syncData now
    }

	// Just run the ALGO
	COUT << "(!) ALGO: Starting\n";
	peByCallback->Run(&callback,this);

    {
        // But now we need a new access to syncData.
        boost::unique_lock<boost::mutex> lock( syncData->Access );

        syncData->DataState = CSyncData::DS_End;
        syncData->HasNewPattern.notify_one();
    }
}

// A function used as a callback for Algo hidden in peByCallback.
bool CParallelPatternEnumerator::callback( PECDataRef data, const CPatternImage& ptrn )
{
	assert( data != 0 );
	return reinterpret_cast<CParallelPatternEnumerator*>(data)->registerPattern( ptrn );
}

// A simple wrapper to avoid copy of the class
class CThreadStarter {
public:
    CThreadStarter( CParallelPatternEnumerator& _e ) : e(_e) {}

    void operator() ()
	{ e.RunAlgoThread(); COUT << "(!) Finishing ALGO thread\n"; }
private:
    CParallelPatternEnumerator& e;
};

// Run the ALGO in a separate thread.
void CParallelPatternEnumerator::createAlgoThread()
{
	COUT << "(!) Starting ALGO thread\n";
	boost::thread( CThreadStarter( *this ) );
}
// Registers a graph found by the ALGO.
bool CParallelPatternEnumerator::registerPattern( const CPatternImage& ptrn )
{
	COUT << "(!) ALGO: callback entry for a pattern " << ptrn.PatternId << "\n";

	boost::unique_lock<boost::mutex> lock( syncData->Access );
	//Updating currPatternNumber
	syncData->PatternImage.PatternId = ptrn.PatternId;
	COUT << "(!) ALGO: Registering a pattern " << syncData->PatternImage.PatternId << "\n";


	// Updating pattern info
	CPatternImage& pi = syncData->PatternImage;
	if( pi.ImageSize > 0 ) {
		delete[] pi.Objects;
	}

	pi.ImageSize = ptrn.ImageSize;
	pi.Objects = new int[pi.ImageSize];
	memcpy(pi.Objects, ptrn.Objects, pi.ImageSize * sizeof( pi.Objects[0] ));

	syncData->DataState = CSyncData::DS_Ready;
	syncData->HasNewPattern.notify_one();

	COUT << "(!) Algo: Registered graph " << syncData->PatternImage.PatternId << "\n";

	// Waiting untill the next graph is requested
	while(!syncData->IsPatternRequested) {
		syncData->NewPatternRequested.wait(lock);
	}
	syncData->IsPatternRequested = false;
	COUT << "(!) ALGO: Return from callback for a pattern " << syncData->PatternImage.PatternId << "\n";
	return syncData->currPatternUsage == CPU_Expand;
}
// Takes the next pattern. Function is used to put together with registerPattern.
inline bool CParallelPatternEnumerator::getNextPattern( TCurrentPatternUsage usage, CPatternImage& pattern )
{
	boost::unique_lock<boost::mutex> lock( syncData->Access );

	// Requesting next graph
	COUT << "(!) SOFIA: requesting next graph (curr is " << syncData->PatternImage.PatternId << ") \n";
	syncData->currPatternUsage = usage;
	syncData->IsPatternRequested = true;
	syncData->NewPatternRequested.notify_one();

	// Waiting while the next graph is ready
	while( syncData->DataState == CSyncData::DS_InPreparation ) {
		syncData->HasNewPattern.wait(lock);
	}
	if( syncData->DataState == CSyncData::DS_End ) {
		return false;
	}
	assert( syncData->DataState == CSyncData::DS_Ready );

	syncData->DataState = CSyncData::DS_InPreparation;
	COUT << "(!) SOFIA: pattern " << syncData->PatternImage.PatternId << " is ready\n";

	ClearMemory( pattern );
	pattern.PatternId = syncData->PatternImage.PatternId;
	pattern.ImageSize = syncData->PatternImage.ImageSize;
	pattern.Objects = syncData->PatternImage.Objects;

	syncData->PatternImage.Objects = 0;
	syncData->PatternImage.ImageSize = 0;

	return true;
}

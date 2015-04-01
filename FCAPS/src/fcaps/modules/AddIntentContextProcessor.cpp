#include "AddIntentContextProcessor.h"

#include <fcaps/modules/details/AddIntentLatticeBuilder.h>
#include <fcaps/modules/details/ExtentImpl.h>
#include <fcaps/storages/VectorIntentStorage.h>
#include <fcaps/PatternManager.h>

using namespace std;

////////////////////////////////////////////////////////////////////

const CModuleRegistrar<CAddIntentContextProcessor> CAddIntentContextProcessor::registrar(
	ContextProcessorModuleType, AddIntentContextProcessorModule );

CAddIntentContextProcessor::CAddIntentContextProcessor() :
	callback( 0 ),
	intStorage( new CVectorIntentStorage ),
	extStorage( new CDequeExtentStorage ),
	builder( new CAddIntentLatticeBuilder ),
	objectCount(0)
{
	//ctor
}

const std::vector<std::string>& CAddIntentContextProcessor::GetObjNames() const
{
	assert(extStorage != 0 );
	return extStorage->GetNames();
}
void CAddIntentContextProcessor::SetObjNames( const std::vector<std::string>& names )
{
	assert(extStorage != 0 );
	extStorage->SetNames( names );
}


void CAddIntentContextProcessor::PassDescriptionParams( const JSON& json )
{
	assert( cmp != 0 );
	IModule& cmpModule = dynamic_cast<IModule&>(*cmp);
	const JSON params = string("{") +
		"\"Type\":\"" + cmpModule.GetType() + "\","
		"\"Name\":\"" + cmpModule.GetName() + "\","
		"\"Params\":" + json +
		"}";
	cmpModule.LoadParams( params );
}

void CAddIntentContextProcessor::AddObject( DWORD objectNum, const JSON& intent )
{
	assert(cmp != 0);
	const DWORD intentID = intStorage->LoadObject( intent );
	assert( intentID != NotFound );
	builder->AddObject( objectNum, intentID );
	if( callback != 0 ) {
		callback->ReportProgress( 1, "Lattice Size is " + StdExt::to_string( lattice.Size() ) );
	}
	++objectCount;
}
void CAddIntentContextProcessor::ProcessAllObjectsAddition()
{
	// Counting edge number
	CStdIterator<CLatticeNodes::const_iterator> node( lattice.GetNodes() );
	DWORD edgeCount = 0;
	for( ; !node.IsEnd(); ++node ) {
		edgeCount += (*node).Parents.size();
	}

	if( callback != 0 ) {
		callback->ReportProgress( 1,
			"Lattice Size = " + StdExt::to_string( lattice.Size() ) +
			"Edges Count = " + StdExt::to_string( edgeCount ) );
	}
}
void CAddIntentContextProcessor::SaveResult( const std::string& path )
{
	const string outFullDataPath( path );

	string outSelectedDataPath( path );
	const size_t ext = outSelectedDataPath.find_last_of( "." );
	if( ext != string::npos ) {
		outSelectedDataPath.substr(0,ext);
	}
	outSelectedDataPath += ".selected.json";

	outputParams.PercentageBase = objectCount;

	CLatticeWriter( lattice, intStorage, extStorage, outputParams )
		.Write( outFullDataPath, outSelectedDataPath );
}

void CAddIntentContextProcessor::LoadParams( const JSON& json )
{
	// TODO
	assert( false );
}
JSON CAddIntentContextProcessor::SaveParams() const
{
	// TODO
	assert( false );
	return "";
}

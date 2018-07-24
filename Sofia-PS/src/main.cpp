// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

//#include <iostream>

#include <ConsoleApplication.h>

#include <Sofia.h>

#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <fcaps/ComputationProcedure.h>
#include <fcaps/Filter.h>

#include <Library.h>
#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/document.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>
#include <boost/filesystem.hpp> // For enumerating modules

#include <iomanip>
#include <time.h>

using namespace std;
using namespace boost;
using namespace boost::filesystem;

////////////////////////////////////////////////////////////////////
TModule SofiaAPI CreateModuleFunc( const char* t, int tLength, const char* n, int nLength, const char* jp, int jLength )
{
	const string type(t,tLength);
	const string name(n,nLength);
	const string jsonParams( jp, jLength );
	
	IModule* modulePtr = CreateModule( type, name, jsonParams );
	return modulePtr;
}

////////////////////////////////////////////////////////////////////

TSofiaFunction SofiaAPI GetSofiaFunction(const char* chName, int nLength)
{
	const string name(chName,nLength);
	if( name == CreateModuleFunction ) {
		return reinterpret_cast<TSofiaFunction>(CreateModuleFunc);
	} else {
		// No Function found
		return 0;
	}
}

////////////////////////////////////////////////////////////////////

class CThisConsoleApplication : public CConsoleApplication, public IComputationCallback {
public:
	CThisConsoleApplication( int _argc, char** _argv ) :
		CConsoleApplication( _argc, _argv ),
		state( S_AddingObjects ),
		writeOutput(false),
		pathToModules( "./modules/" )
	{
	}

	// Methods of CConsoleApplication
	virtual bool ProcessParam( const std::string& param, const std::string& value );
	virtual bool ProcessOption( const std::string& option );
	virtual bool FinalizeParams();
	virtual bool AreParamsCorrect() const;
	virtual std::string GetCmdLineDescription( const std::string& progName ) const;
	virtual int Execute();

	// Methdos of IComputationProcedureCallback
	virtual void ReportProgress( const double& p, const std::string& info ) const;
	virtual void ReportNextStage( const std::string& stageName ) const;
	virtual void Warning(const std::string& warning) const;
	virtual bool IsInterrupted() const
		{return false;}

private:
	enum TState {
		S_Prepare = 0,
		S_AddingObjects,
		S_ProcessingAllAditions,
		S_SavingResults,

		S_EnumCount
	};
private:
	TState state;
	string cbPath;
	string fltrPath;
	string outBaseName;
	bool writeOutput;

	mutable string lastCtxProcessorInfo;

	string pathToModules;
	vector< CSharedPtr<Library> > modules;

	void printException( const CException& e ) const;

	void loadModules();
	void loadModule(const string& name);
	void extractModules( const JSON& description ) const;

	void runComputationProcedure();

	void readComputationProcedureJson( rapidjson::Document& cb ) const;
	IComputationProcedure* createComputationProcedure( rapidjson::Document& cb ) const;

	void runFilters();
	IFilter* createFilter() const;
};

bool CThisConsoleApplication::ProcessParam( const std::string& param, const std::string& value )
{
	if ( param == "-CP" ) {
		cbPath = value;
	} else if( param == "-fltr" ) {
		fltrPath = value;
	} else if ( param == "-out" ) {
		outBaseName = value;
		writeOutput = true;
	} else if (param == "-M") {
		pathToModules = value;
	} else {
		return CConsoleApplication::ProcessParam( param, value );
	}
	return true;
}

bool CThisConsoleApplication::ProcessOption( const std::string& option )
{
	if ( option == "-out" ) {
		writeOutput = true;
	} else {
		return CConsoleApplication::ProcessOption( option );
	}
	return true;
}

bool CThisConsoleApplication::FinalizeParams()
{
	loadModules();
	if( outBaseName.empty() ) {
		outBaseName = string("fcaps-result-") +StdExt::to_string(time(NULL))+ ".json";
	}
	return true;
}

bool CThisConsoleApplication::AreParamsCorrect() const
{
	return (!cbPath.empty() || !fltrPath.empty() )
		&& CConsoleApplication::AreParamsCorrect();
}

std::string CThisConsoleApplication::GetCmdLineDescription( const std::string& progName ) const
{
	return progName + " -data:{Path} [-CP:{Path}][-fltr:{PATH}] [OPTIONS]\n"
	"OPTIONS = \n"
	"\t[-out:{Path} -M:{Path}]\n"
	">>Name of the output is {output file}-patterns\n"

	" -CP -- Path to params of Concept Processor in JSON (see JSON-Specification).\n"
	" -fltr -- Path to params of the filter applied to the lattice\n"
	" -out -- Base path of the result. Suffixes can be added.\n"
	" -M -- the path to the folder with modules\n"
	;
}

int CThisConsoleApplication::Execute()
{
	GetInfoStream() << "Starting computations\n";
	if( !cbPath.empty() ) {
		GetInfoStream() << "  with params from\n\t\"" << cbPath << "\"\n";
	}
	if( !fltrPath.empty() ) {
		GetInfoStream() << "  with filters from\n\t\"" << fltrPath << "\"\n";
	}
	GetInfoStream() << "Output is saved to\n"
	                << "\t" << outBaseName << "\n";
	
	try{
		if( !cbPath.empty() ) {
			runComputationProcedure();
		}
		if( !fltrPath.empty() && !outBaseName.empty() ) {
			runFilters();
		}
		GetStatusStream() << "\n--------------DONE---------------\n";
		return 0;
	} catch( CException* e ) {
		printException( *e );
		delete e;
	}

	return -1;
}

void CThisConsoleApplication::ReportProgress( const double& p, const std::string& info ) const
{
	lastCtxProcessorInfo = info;

	if( 0 <= p && p <= 1 ) {
		GetStatusStream() << std::fixed << std::setprecision(3);
        GetStatusStream() << "   In " << p*100 << "%. ";
	} else {
		GetStatusStream() << std::fixed << std::setprecision(1);
        GetStatusStream() << "   In " << p << ". ";
	}
	GetStatusStream() << info << "   \r";
	GetStatusStream().flush();
}

void CThisConsoleApplication::ReportNextStage( const std::string& stageName ) const
{
	GetStatusStream() << "\n== Stage '" << stageName << "' ==\n"; 
}
void CThisConsoleApplication::Warning(const std::string& warning) const
{
	GetWarningStream() << "\n[!] " << warning << "\n";
}

void CThisConsoleApplication::runComputationProcedure() {

	string cbFullPath;
	RelativePathes::BaseName(cbPath,cbFullPath);
	RelativePathes::CSearchPath cbPathSwitcher( cbFullPath );

	rapidjson::Document cb;
	readComputationProcedureJson( cb );

	CSharedPtr<IComputationProcedure> compProcedure ( createComputationProcedure(cb) );
	compProcedure->SetCallback( this );

	time_t start = time( NULL );
	compProcedure->Run();

	//Iterate objects.
	const time_t end = time( NULL );
	GetInfoStream() << "\nProcessing time is " << end - start << "                                 \n";
	GetInfoStream() << lastCtxProcessorInfo << "\n";

	// Output base path
	if( writeOutput ) {
		state = S_SavingResults;
		const time_t startOutput = time( NULL );
		compProcedure->SaveResult(outBaseName);
		const time_t endOutput = time( NULL );
		GetInfoStream() << "Output produced " << endOutput - startOutput << " seconds\n";
	}
}

void CThisConsoleApplication::printException( const CException& e ) const
{
	GetErrorStream() << "\nError in " << e.GetPlace()
		<< "\n" << e.GetText() << "\n\n";
}

void CThisConsoleApplication::loadModules() 
{
	string fullPathToModules;
	RelativePathes::GetFullPath(pathToModules, fullPathToModules );

	GetStatusStream() << "Searching for Module Dynamic Libraries in '" << fullPathToModules << "'\n";

	path p(fullPathToModules);
	directory_iterator begin(p),end;
	CStdIterator<directory_iterator> itr( begin,end );
	for( ; !itr.IsEnd(); ++itr ) {
		if(!is_regular_file(itr->status())) {
			continue;
		}
		const string name( itr->path().string() );
		if( name.find("Module") == string::npos
			|| (
				name.find(".DLL") == string::npos
				&& name.find(".dll") == string::npos
				&& name.find(".so") == string::npos
				&& name.find(".dylib") == string::npos
				))
		{
			// Not a module
			continue;
		}
		GetStatusStream() << "Loading modules from '" << name << "'\n";
		loadModule(name);
	}
}
void CThisConsoleApplication::loadModule(const string& name) 
{
	try{
		Library moduleLib( name );
		assert( moduleLib.IsOpen() );

		const TInitializeModuleFunc initModuleFunc = reinterpret_cast<TInitializeModuleFunc>( moduleLib.GetFunc( "InitializeModule" ) );
		const TGetModuleDescriptionFunc getModuleDescriptionFunc = reinterpret_cast<TGetModuleDescriptionFunc>( moduleLib.GetFunc( "GetModuleDescription" ) );

		initModuleFunc( &GetSofiaFunction );
		JSON description = getModuleDescriptionFunc();
		#ifdef DEBUG
		GetInfoStream() << description << "\n";
		#endif
		extractModules( description );

		modules.push_back(CSharedPtr<Library>( new Library) );
		moduleLib.MoveTo(*(modules.back()));
	} catch( CException* e ) {
		GetInfoStream() << "Cannot load a module (ignored)\n\t";
		GetInfoStream() << e->GetText() << "\n";
	}
}

void CThisConsoleApplication::extractModules( const JSON& description ) const
{
	CJsonError jsonError;
	rapidjson::Document doc;
	if( !ReadJsonString( description, doc, jsonError ) ) {
		throw new CJsonException( "ExtractModules", jsonError );
	}
	if( !doc.IsArray() ) {
		jsonError.Data=description;
		jsonError.Error="Not an array JSON";
		throw new CJsonException( "ExtractModules", jsonError );
	}
	for( int i = 0; i < doc.Size(); ++i ) {
		if( !doc[i].HasMember("Type") || !doc[i]["Type"].IsString()
		    || !doc[i].HasMember("Name") || !doc[i]["Name"].IsString()
		    || !doc[i].HasMember("Func") || !doc[i]["Func"].IsUint64() )
		{
			jsonError.Data=description;
			jsonError.Error="'Type', 'Name', or 'Func' has an incorrect format";
			throw new CJsonException( "ExtractModules", jsonError );
		}
		RegisterModule( doc[i]["Type"].GetString(), doc[i]["Name"].GetString(), reinterpret_cast<CreateFunc>(doc[i]["Func"].GetUint64()) );
	}
}

void CThisConsoleApplication::readComputationProcedureJson( rapidjson::Document& cb ) const
{
	CJsonError jsonError;
	if( !ReadJsonFile( cbPath, cb, jsonError ) ) {
		throw new CJsonException( "ComputationProcedureJson", jsonError );
	}
	if( !cb.IsObject() ) {
		throw new CTextException( "ComputationProcedureJson", "JSON params of CB are not in an json-object" );
	}
	if( !cb.HasMember( "Params" ) ) {
		cb.AddMember( "Params", rapidjson::Value().SetObject(), cb.GetAllocator() );
	}
}

IComputationProcedure* CThisConsoleApplication::createComputationProcedure( rapidjson::Document& cb ) const
{
	string errorText;
	unique_ptr<IComputationProcedure> cp;
	cp.reset( dynamic_cast<IComputationProcedure*>(
		CreateModuleFromJSON( cb, errorText ) ) );

	if( cp.get() == 0 ) {
		if( errorText.empty() ) {
			throw new CTextException( "execute", "CB given by params is not a Computation Procedure.");
		} else {
			throw new CTextException( "execute",  errorText );
		}
	}
	return cp.release();
}

void CThisConsoleApplication::runFilters()
{
	string filterFullPath;
	RelativePathes::BaseName(fltrPath,filterFullPath);
	RelativePathes::CSearchPath cbPathSwitcher( filterFullPath );

	GetInfoStream() << "\nApplying filters...";
	const time_t start = time( NULL );

	CSharedPtr<IFilter> filter ( createFilter() );
	// data is not available now. Probably filters should extract this information somehow
	// filter->SetDataFile( dataPath.c_str() );
	filter->SetInputFile( outBaseName.c_str() );
	filter->Process();

	const time_t end = time( NULL );
	GetInfoStream() << "\rThe filters applied in " << end - start << " seconds\n";
}

IFilter* CThisConsoleApplication::createFilter() const
{
	CJsonError jsonError;
	rapidjson::Document fltrParams;
	CJsonFile file(fltrPath);
	if( !file.Read(  fltrParams, jsonError ) ) {
		throw new CJsonException( "ComputationProcedureJson", jsonError );
	}

	string errorText;
	unique_ptr<IFilter> filter;
	filter.reset( CreateModuleFromJSON<IFilter>( fltrParams, errorText ) );

	if( filter.get() == 0 ) {
		throw new CTextException( "execute",  errorText );
	}
	return filter.release();
}

////////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )
{
	return CThisConsoleApplication(argc, argv).Run();
}

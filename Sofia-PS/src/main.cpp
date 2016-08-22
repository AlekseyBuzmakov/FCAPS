// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

//#include <iostream>

#include <ConsoleApplication.h>

#include <Sofia.h>

#include <fcaps/Module.h>
#include <ModuleTools.h>
#include <fcaps/ContextProcessor.h>
#include <fcaps/Filter.h>

#include <Library.h>
#include <ModuleJSONTools.h>

#include <JSONTools.h>

#include <ListWrapper.h>

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

class CThisConsoleApplication : public CConsoleApplication, public IContextProcessorCallback {
public:
	CThisConsoleApplication( int _argc, char** _argv ) :
		CConsoleApplication( _argc, _argv ),
		state( S_AddingObjects ),
		writeOutput(false),
		maxObjsCount( -1 ),
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

	// Methdos of IContextProcessorCallback
	virtual void ReportProgress( const double& p, const std::string& info ) const;

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
	string dataPath;
	string cbPath;
	string fltrPath;
	string outBaseName;
	bool writeOutput;

	DWORD maxObjsCount;
	CList<DWORD> indexes;

	vector<string> objNames;
	mutable string lastCtxProcessorInfo;

	string pathToModules;
	vector< CSharedPtr<Library> > modules;

	void printException( const CException& e ) const;

	void loadModules();
	void loadModule(const string& name);
	void extractModules( const JSON& description ) const;

	void runContextProcessor();

	void readContextProcessorJson( rapidjson::Document& cb ) const;
	void readDataJson( rapidjson::Document& data ) const;
	void extractObjectNames( rapidjson::Document& data );
	IContextProcessor* createContextProcessor( rapidjson::Document& cb ) const;

	void runFilters();
	IFilter* createFilter() const;
};

bool CThisConsoleApplication::ProcessParam( const std::string& param, const std::string& value )
{
	if ( param == "-data" ) {
		dataPath = value;
	} else if ( param == "-CP"
		|| param == "-CB"
		|| param == "-PM" )
	{
		cbPath = value;
	} else if( param == "-fltr" ) {
		fltrPath = value;
	} else if ( param == "-out" ) {
		outBaseName = value;
		writeOutput = true;
	} else if ( param == "-n" ) {
		maxObjsCount = lexical_cast<DWORD>( value );
	} else if ( param == "-index" ) {
		indexes.Clear();
		vector<string> indexesStr;
		split( indexesStr, value, is_any_of( ","), token_compress_on );
		for( size_t i = 0; i < indexesStr.size(); ++i ) {
			if( indexesStr[i].empty() ) {
				continue;
			}
			const DWORD nextIndex = lexical_cast<DWORD>( indexesStr[i] );
			if( !indexes.IsEmpty() && indexes.Back() >= nextIndex ) {
				indexes.Clear();
				GetInfoStream() << "Not sorted indexes -- ignored\n";
				return true;
			}
			indexes.PushBack( nextIndex );
		}
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
	return true;
}

bool CThisConsoleApplication::AreParamsCorrect() const
{
	return !dataPath.empty() && (!cbPath.empty() || !fltrPath.empty() )
		&& CConsoleApplication::AreParamsCorrect();
}

std::string CThisConsoleApplication::GetCmdLineDescription( const std::string& progName ) const
{
	return progName + " -data:{Path} [-CP:{Path}][-fltr:{PATH}] [OPTIONS]\n"
	"OPTIONS = \n"
	"\t[-out:{Path} -n:{Num} -index:{Index Set} -M:{Path}]\n"
	">>Name of the output is {output file}-patterns\n"

	" -data -- Path to the data in JSON format (see ./JSON-Examples).\n"
	" -CP -- Path to params of Concept Processor in JSON (see JSON-Specification).\n"
	" \t[aliases: -CB,-PM]\n"
	" -fltr -- Path to params of the filter applied to the lattice\n"
	" -out -- Base path of the result. Suffixes can be added.\n"
	" -n -- the number of objects to process\n"
	" -index -- add only objects with requested indexes (e.g. '-index:1,2,6')\n"
	" -M -- the path to the folder with modules\n"
	;
}

int CThisConsoleApplication::Execute()
{
	GetInfoStream() << "Processing\n\t\"" << dataPath << "\"\n";
	if( !cbPath.empty() ) {
		GetInfoStream() << "with params from\n\t\"" << cbPath << "\"\n";
	}
	if( !fltrPath.empty() ) {
		GetInfoStream() << "with filters from\n\t\"" << fltrPath << "\"\n";
	}
	try{
		if( !cbPath.empty() ) {
			runContextProcessor();
		} else {
			outBaseName = dataPath;
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

	if( state == S_AddingObjects ) {
		return;
	}
    GetStatusStream() << std::fixed << std::setprecision(3);
	if( 0 <= p && p <= 1 ) {
        GetStatusStream() << "Processing " << p*100 << "%. ";
	} else {
        GetStatusStream() << "Processing " << p << ". ";
	}
	GetStatusStream() << info << "   \r";
	GetStatusStream().flush();
}

void CThisConsoleApplication::runContextProcessor() {

	rapidjson::Document cb;
	readContextProcessorJson( cb );

	string cbFullPath;
	RelativePathes::BaseName(cbPath,cbFullPath);
	RelativePathes::CSearchPath cbPathSwitcher( cbFullPath );

	rapidjson::Document data;
	readDataJson( data );

	extractObjectNames( data );

	CSharedPtr<IContextProcessor> processor ( createContextProcessor(cb) );
	processor->SetCallback( this );

	string dataParams;
	if( data[0].HasMember("Params") ) {
		CreateStringFromJSON(data[0]["Params"], dataParams);
		processor->PassDescriptionParams( dataParams );
	}

	// Preparation
	GetStatusStream() << "Preparation...                               \r" << flush;
	state=S_Prepare;
	processor->Prepare();

	//Iterate objects.
	GetStatusStream() << "Adding objects...                               \r" << flush;
	state = S_AddingObjects;
	time_t start = time( NULL );
	string objectJson;
	const rapidjson::Value& dataBody=data[1]["Data"];

	CStdIterator<CList<DWORD>::CConstIterator, false> index( indexes );
	DWORD objNum = 0;
	for( size_t i = 0; i < dataBody.Size(); ++i ) {
		// Select objects with good indices.
		if( !indexes.IsEmpty() ) {
			if( index.IsEnd() ) {
				break;
			}
			i = *index;
			++index;
		}
		// Cut if have processed to much.
		if( objNum >= maxObjsCount ) {
			break;
		}

		CreateStringFromJSON( dataBody[i], objectJson );
		try{
			processor->AddObject( i, objectJson );
			++objNum;
		} catch( CException* e ) {
			GetWarningStream() << "Object " << i << " has a bad description -> IGNORED\n";
			printException( *e );
			delete e;
			continue;
		}

		const time_t end = time( NULL );
		GetStatusStream() << "\rAdded " << objNum << "th object. "
			<< lastCtxProcessorInfo << " "
			<< "Time is " << end - start << flush;
	}
	GetStatusStream() << "\rAdded all objects.                                       \n";

	state = S_ProcessingAllAditions;
	processor->ProcessAllObjectsAddition();

	const time_t end = time( NULL );
	GetInfoStream() << "\rProcessing time is " << end - start << "                                 \n";
	GetInfoStream() << lastCtxProcessorInfo << "\n";

	// Output base path
	if( writeOutput ) {
		state = S_SavingResults;
		if( outBaseName.empty() ) {
			outBaseName = dataPath;
			const size_t ext = outBaseName.find_last_of( "." );
			if( ext != string::npos ) {
				outBaseName = outBaseName.substr(0,ext);
			}
			outBaseName += ".out.json";
		}
		const time_t startOutput = time( NULL );
		GetStatusStream() << "\nProducing output...\r" << flush;

		processor->SaveResult( outBaseName );

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

	path p(fullPathToModules);
	directory_iterator begin(p),end;
	CStdIterator<directory_iterator> itr( begin,end );
	for( ; !itr.IsEnd(); ++itr ) {
		if(!is_regular_file(itr->status())) {
			continue;
		}
		const string name( itr->path().c_str() );
		if( name.find("Module") == string::npos ) {
			// Not a module
			continue;
		}
		GetInfoStream() << "Loading module '" << name << "'\n";
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
		    || !doc[i].HasMember("Func") || !doc[i]["Func"].IsUint() )
		{
			jsonError.Data=description;
			jsonError.Error="'Type', 'Name', or 'Func' has an incorrect format";
			throw new CJsonException( "ExtractModules", jsonError );
		}
		RegisterModule( doc[i]["Type"].GetString(), doc[i]["Name"].GetString(), reinterpret_cast<CreateFunc>(doc[i]["Func"].GetUint()) );
	}
}

void CThisConsoleApplication::readContextProcessorJson( rapidjson::Document& cb ) const
{
	CJsonError jsonError;
	if( !ReadJsonFile( cbPath, cb, jsonError ) ) {
		throw new CJsonException( "ContextProcessorJson", jsonError );
	}
	if( !cb.IsObject() ) {
		throw new CTextException( "ContextProcessorJson", "JSON params of CB are not in an json-object" );
	}
	if( !cb.HasMember( "Params" ) ) {
		cb.AddMember( "Params", rapidjson::Value().SetObject(), cb.GetAllocator() );
	}
}
void CThisConsoleApplication::readDataJson( rapidjson::Document& data ) const
{
	CJsonError jsonError;
	if( !ReadJsonFile( dataPath, data, jsonError ) ) {
		throw new CJsonException( "readDataJson", jsonError );
	}
	if( !data.IsArray() || data.Size() < 2 ) {
		throw new CTextException( "readDataJson", "JSON data are not in an 2-sized json-array" );
	}
	if( !data[1].HasMember( "Data") ) {
		throw new CTextException( "readDataJson", "No DATA[1].Data found" );
	}
	if( !data[1]["Data"].IsArray() ) {
		throw new CTextException( "readDataJson", "DATA[1].Data should be an array" );
	}
}

void CThisConsoleApplication::extractObjectNames( rapidjson::Document& data )
{
	const rapidjson::Value& dataParams = data[0u];
	if( !dataParams.IsObject() ) {
		GetWarningStream() << "DATA[0] is not an object\n";
		return;
	}

	if( !dataParams.HasMember( "ObjNames" ) || !dataParams["ObjNames"].IsArray() ) {
		return;
	}

	const rapidjson::Value& objNamesArray = dataParams["ObjNames"];
	objNames.reserve( objNamesArray.Size() );
	for( int i = 0; i < objNamesArray.Size(); ++i) {
		const rapidjson::Value& name = objNamesArray[i];
		if( !name.IsString() ) {
			GetWarningStream() << "The " << i << "th object name is not a string -- ignored.\n";
			objNames.push_back( StdExt::to_string(i) );
			continue;
		}
		objNames.push_back( name.GetString() );
	}

	if( data[1]["Data"].Size() != objNames.size() ) {
		GetWarningStream() << "The number of objects (" << data[1]["Data"].Size() << ")"
			<< " does not correspond to the number of object names (" << objNames.size() << ").\n";
	}
}

IContextProcessor* CThisConsoleApplication::createContextProcessor( rapidjson::Document& cb ) const
{
	string errorText;
	auto_ptr<IContextProcessor> processor;
	processor.reset( dynamic_cast<IContextProcessor*>(
		CreateModuleFromJSON( cb, errorText ) ) );

	if( processor.get() == 0 ) {
		if( errorText.empty() ) {
			throw new CTextException( "execute", "CB given by params is not a Context Processor.");
		} else {
			throw new CTextException( "execute",  errorText );
		}
	}
	processor->SetObjNames( objNames );
	return processor.release();
}

void CThisConsoleApplication::runFilters()
{
	GetInfoStream() << "\nApplying filters...";
	const time_t start = time( NULL );

	CSharedPtr<IFilter> filter ( createFilter() );
	filter->SetInputFile( outBaseName );
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
		throw new CJsonException( "ContextProcessorJson", jsonError );
	}

	string errorText;
	auto_ptr<IFilter> filter;
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

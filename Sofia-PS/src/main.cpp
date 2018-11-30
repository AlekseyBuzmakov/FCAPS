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
		pathToModules( "./modules/" ),
		interactiveMode(false),
		ipAlloc(interactiveParams.GetAllocator())
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
	bool interactiveMode;

	mutable string lastCtxProcessorInfo;

	string pathToModules;
	vector< CSharedPtr<Library> > modules;

	mutable rapidjson::Document interactiveParams;
	rapidjson::MemoryPoolAllocator<>& ipAlloc;

	void printException( const CException& e ) const;

	void loadModules();
	void loadModule(const string& name);
	void extractModules( const JSON& description ) const;

	void runComputationProcedure();

	void readComputationProcedureJson( rapidjson::Document& cb ) const;
	IComputationProcedure* createComputationProcedure( rapidjson::Document& cb ) const;

	void runFilters();
	IFilter* createFilter() const;

	// Function for interactive mode that creates settings by asking the user
	void startInteractiveMode() const;
	void addUniqueMember(rapidjson::Value& obj, const string& name, rapidjson::Value& val) const;
	void askForModule(const string& name, const string& moduleType, rapidjson::Value& res) const;
	int getOpt(int size) const;
	void extractNameDescription(const string& desc, string& name, string& description) const;
	void askForValue(const string& name, const rapidjson::Value& d, rapidjson::Value& res) const;
	void askForObjectKeys(const string& name, const string& description, const rapidjson::Value& d, rapidjson::Value& res) const;
	void askForModuleParams(const CModuleRegistration& module, rapidjson::Value& res) const;
	void askForArrayElements(const string& name,const string& description,const rapidjson::Value& d, rapidjson::Value& res) const;
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
	} else if ( option == "-I" ) {
		interactiveMode = true;
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
	"\t[-out:{Path} -M:{Path} -I]\n"
	">>Name of the output is {output file}-patterns\n"

	" -CP -- Path to params of Concept Processor in JSON (see JSON-Specification).\n"
	" -fltr -- Path to params of the filter applied to the lattice\n"
	" -out -- Base path of the result. Suffixes can be added.\n"
	" -M -- the path to the folder with modules\n"
    " -I -- interactive mode. The job file is interactively created and saved to the file in the -CP key\n"
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
	if( interactiveMode ) {
		GetInfoStream() << "Params are interactively created\n";
	}

	if( interactiveMode ) {
		try{
			startInteractiveMode();
		} catch( CException* e ) {
			printException( *e );
			delete e;
			return -1;
		}
	}
	
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
		string desc = "{}";
		if(doc[i].HasMember("Desc") && doc[i]["Desc"].IsObject()){
			CreateStringFromJSON( doc[i]["Desc"], desc );
		}
		RegisterModule( doc[i]["Type"].GetString(), doc[i]["Name"].GetString(), reinterpret_cast<CreateFunc>(doc[i]["Func"].GetUint64()), desc );
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

void CThisConsoleApplication::startInteractiveMode() const
{
	askForModule("Computation Procedure", ComputationProcedureModuleType, interactiveParams);
	JSON result;
	CreateStringFromJSON( interactiveParams, result, true );
	CDestStream dst(cbPath);
	dst << result;
}
void CThisConsoleApplication::addUniqueMember(rapidjson::Value& obj, const string& name, rapidjson::Value& val) const
{
	if(!obj.HasMember(name.c_str())) {
		obj.AddMember(rapidjson::Value(name.c_str(), ipAlloc),val.Move(),ipAlloc);
	} else {
		obj[name.c_str()] = val.Move();
	}
}
void CThisConsoleApplication::askForModule(const string& name, const string& moduleType, rapidjson::Value& res) const
{
	res.SetObject();
	cout << "-------------------------------------------" << endl;
	cout << "Please choose a " << name << endl;
	cout << "[?] for quit type [q] for level up type [u]." << endl << endl;

	std::vector<CModuleRegistration> modules;
	EnumerateModuleRegistrations( moduleType, modules );

	for( int i = 0; i < modules.size(); ++i) {
		string name;
		string description;
		extractNameDescription(modules[i].Desc, name, description);
		if( name == "" ) {
			name=modules[i].Name;
		}
		cout << "[" << i << "] " << name << endl;
		cout << "    " << description << endl << endl;
	}
	int opt=getOpt(modules.size());
	if( opt == -1 ) {
		return;
	}
	assert( 0 <= opt && opt < modules.size());
	const CModuleRegistration& module = modules[opt];
	addUniqueMember(res,"Type", rapidjson::Value().SetString(module.Type.c_str(), ipAlloc));
	addUniqueMember(res,"Name", rapidjson::Value().SetString(module.Name.c_str(),  ipAlloc));
	addUniqueMember(res,"Params", rapidjson::Value().SetObject());
	askForModuleParams(module,res["Params"]);
}
int CThisConsoleApplication::getOpt(int size) const
{
	int opt = -1;
	string cmd;
	do{
		cout << "Your choice: " << flush;
		cin >> cmd;
		if(cmd== "q") {
			throw  new CTextException("InteractiveMode", "Interactive mode has been canceled.");
		}
		if(cmd== "u") {
			return -1;
		}
		try{
			opt = stoi(cmd);
			if( opt >= 0 && opt < size ) {
				return opt;
			}
		} catch( ... ) {
		}
		cout << " [!] bad option" << endl;
	} while(true);
}
void CThisConsoleApplication::extractNameDescription(const string& desc, string& name, string& description) const
{
	CJsonError error;
	rapidjson::Document d;
	if( !ReadJsonString( desc, d, error ) ) {
		throw new CJsonException( "extractNameDescription", error );
	}
	if( d.HasMember("Name") && d["Name"].IsString()) {
		name = d["Name"].GetString();
	}
	if( d.HasMember("Description") && d["Description"].IsString()) {
		description = d["Description"].GetString();
	}
}
void CThisConsoleApplication::askForModuleParams(const CModuleRegistration& module, rapidjson::Value& res) const
{
	CJsonError error;
	rapidjson::Document d;
	if( !ReadJsonString( module.Desc, d, error ) ) {
		throw new CJsonException( module.Name, error );
	}
	string name;
	string description;
	if( d.HasMember("Name") && d["Name"].IsString()) {
		name = d["Name"].GetString();
	} else {
		name = module.Name;
	}
	if( d.HasMember("Description") && d["Description"].IsString()) {
		description = d["Description"].GetString();
	}

	if(!d.HasMember("Params") || !d["Params"].IsObject()) {
		cout << "No module description is found :(" << endl;
		return;
	}
	askForObjectKeys(name, description, d["Params"], res);
	
}
void CThisConsoleApplication::askForObjectKeys(const string& name, const string& description, const rapidjson::Value& d, rapidjson::Value& res) const
{
	assert(d.HasMember("type") && d["type"].IsString() && string(d["type"].GetString()) == "object");
	
	res.SetObject();
	do {
		cout << "------------------------------------" << endl;
		cout << "Object '" << name << "'" << endl;
		cout << description << endl;
		cout << "[?] for quit type [q] for level up type [u]." << endl << endl;

		if(!d.HasMember("properties") || !d["properties"].IsObject()
		|| d["properties"].MemberBegin() ==  d["properties"].MemberEnd())
		{
			cout << "... no propertes has been registered ..." << endl;
			return;
		} else {
			const rapidjson::Value& props = d["properties"];
			vector<string> names;
			int i = 0;
			for(auto p = props.MemberBegin(); p != props.MemberEnd(); ++p) {
				const string name = p->name.GetString();
				names.push_back(name);
				string description;

				assert(p->value.IsObject());
				if(p->value.IsObject() && p->value.HasMember("description")
				&& p->value["description"].IsString() )
				{
					description = p->value["description"].GetString();
				}

				cout << "[" << i << "] " << name << endl;
				cout << "    " << description << endl;
				++i;
			}
			const int opt = getOpt(i);
			if( opt == -1) {
				break;
			}
			assert(0 <= opt && opt < names.size());
			const char* name = names[opt].c_str();
			if( !res.HasMember(name) ) {
				res.AddMember(rapidjson::Value(name, ipAlloc).Move(),rapidjson::Value(),ipAlloc);
			}
			askForValue(names[opt], props[names[opt].c_str()],res[names[opt].c_str()] );
		}
	} while(true);
}
enum LimitType{
	// No limit is available
	LT_None = 0,
	// Limit but excluding the actual number
	LT_Exclusive,
	// Limit including the number
	LT_Inclusive,

	LT_EnumCount
};
template<typename T>
struct CNumberLimits {
	T Min;
	T Max;
	LimitType MinType;
	LimitType MaxType;

	CNumberLimits() :
		Min(0),Max(0),MinType(LT_None),MaxType(LT_None) {}
};
template<typename T>
std::string getInterval(const CNumberLimits<T>& l ) {
	string res;
	switch(l.MinType) {
	case LT_None:
		res += "(-inf,";
		break;
	case LT_Exclusive:
		res += "(" + StdExt::to_string(l.Min) + ",";
		break;
	case LT_Inclusive:
		res += "[" + StdExt::to_string(l.Min) + ",";
		break;
	default:
		assert(false);
	}
	switch(l.MaxType) {
	case LT_None:
		res += "+inf)";
		break;
	case LT_Exclusive:
		res += StdExt::to_string(l.Max) + ")";
		break;
	case LT_Inclusive:
		res += StdExt::to_string(l.Max) + "]";
		break;
	default:
		assert(false);
	}
	return res;
}
template<typename T>
T askForNumber(const string& name, const string& desc, const CNumberLimits<T>& l)
{
	cout << "Deciding on '" << name << "'" << endl;
	cout << desc << endl;
	cout << "[?] for quit type [q]." << endl << endl;
	string cmd;
	do{
		cout << "Number in " << getInterval(l) << ":" << flush;
		cin >> cmd;
		if(cmd== "q") {
			throw  new CTextException("InteractiveMode", "Interactive mode has been canceled.");
		}
		T t = 0;
		try{
			t = boost::lexical_cast<T>(cmd);
			if( (l.MinType == LT_None || l.MinType == LT_Exclusive && l.Min < t || l.MinType == LT_Inclusive && l.Min <= t)
				&& (l.MaxType == LT_None || l.MaxType == LT_Exclusive && l.Max < t || l.MaxType == LT_Inclusive && l.Max <= t) )
			{
				return t;
			}
		} catch( ... ) {
		}
		cout << " [!] bad option" << endl;
	} while(true);
}
template<typename T>
void fillLimits(const rapidjson::Value& d, CNumberLimits<T>& limits)
{
	rapidjson::Value::ConstMemberIterator itr = d.FindMember("minimum");
	if( itr != d.MemberEnd() && itr->value.IsInt()) {
		limits.Min = itr->value.GetInt();
		itr = d.FindMember("exclusiveMinimum");
		if( itr != d.MemberEnd() && itr->value.IsBool() && itr->value.GetBool() ) {
			limits.MinType = LT_Exclusive;
		} else {
			limits.MinType = LT_Inclusive;
		}
	}
	itr = d.FindMember("maximum");
	if( itr != d.MemberEnd() && itr->value.IsInt()) {
		limits.Max = itr->value.GetInt();
		itr = d.FindMember("exclusiveMaximum");
		if( itr != d.MemberEnd() && itr->value.IsBool() && itr->value.GetBool() ) {
			limits.MaxType = LT_Exclusive;
		} else {
			limits.MaxType = LT_Inclusive;
		}
	}
}
bool askForBool(const string& name, const string& desc)
{
	cout << "Deciding on '" << name << "'" << endl;
	cout << desc << endl;
	string cmd;
	do{
		cout << "Type (t)rue or (f)alse:" << flush;
		cin >> cmd;
		if(cmd== "q") {
			throw  new CTextException("InteractiveMode", "Interactive mode has been canceled.");
		}
		if( cmd == "t" || cmd == "true") {
			return true;
		}
		if( cmd == "f" || cmd == "false") {
			return false;
		}
		cout << " [!] bad option" << endl;
	} while(true);
}
void askForString(const string& name, const string& desc, string& result)
{
	cout << "Deciding on '" << name << "'" << endl;
	cout << desc << endl;

	cout << "Your string:" << flush;
	cin >> result;
}
void askForFilePath(const string& name, const string& desc, string& result)
{
	cout << "Deciding on '" << name << "'" << endl;
	cout << desc << endl;

	cout << "Your file path:" << flush;
	cin >> result;
}
void CThisConsoleApplication::askForValue(const string& name, const rapidjson::Value& d, rapidjson::Value& res) const
{
	assert(d.HasMember("type") && d["type"].IsString());
	string description;
	if(d.HasMember("description") && d["description"].IsString()) {
		description = d["description"].GetString();
	}
	const string type =d["type"].GetString();
	if( type == "object"){
		askForObjectKeys(name, description, d, res);
	} else if( type == "array") {
		askForArrayElements(name,description,d, res);
	} else if( type[0] == '@' ) {
		askForModule(name, type.substr(1), res);
	} else if( type == "integer"){
		CNumberLimits<int> limits;
		fillLimits(d, limits);
		const int value = askForNumber(name, description,limits);
		res.SetInt(value);
	} else if( type == "number"){
		CNumberLimits<double> limits;
		fillLimits(d, limits);
		const double value = askForNumber(name, description,limits);
		res.SetDouble(value);
	} else if( type == "boolean"){
		const bool value = askForBool(name, description);
		res.SetBool(value);
	} else if( type == "string"){
		string result;
		askForString(name, description, result);
		res.SetString(result.c_str(),ipAlloc);
	} else if( type == "file-path"){
		rapidjson::Value::ConstMemberIterator itr = d.FindMember("example");
		if( itr != d.MemberEnd() ) {
			JSON ex;
			CreateStringFromJSON( itr->value, ex );
			description += "\n    FILE EXAMPLE: " + ex;
		}
		string result;
		askForFilePath(name, description, result);
		res.SetString(result.c_str(),ipAlloc);
	} else {
		assert(false);
	}
}

void CThisConsoleApplication::askForArrayElements(const string& name,const string& description,const rapidjson::Value& d, rapidjson::Value& res) const
{
	assert(d.HasMember("type") && d["type"].IsString() && string(d["type"].GetString()) == "array");

	if(!d.HasMember("items") || !d["items"].IsObject()
	   || !d["items"].HasMember("type") || !d["items"]["type"].IsString() )
	{
		cout << "------------------------------------" << endl;
		cout << "Array '" << name << "'" << endl;
		cout << description << endl;
		cout << "... array elements are not well descrbibed ..." << endl;
		return;
	}
	res.SetArray();
	for( int i = 0; true; ++i) {
		cout << "------------------------------------" << endl;
		cout << "Array '" << name << "'" << endl;
		cout << description << endl;
		if( i > 0 ) {
			cout << "[?] Add next element (" << i << ")" << endl;
			cout << "    type [no] to stop, type [q] to quit, type other keys for continue: " << flush;
			string cmd;
			cin >> cmd;
			if(cmd== "q") {
				throw new  CTextException("InteractiveMode", "Interactive mode has been canceled.");
			}
			if(cmd== "no") {
				break;
			}
		}
		res.PushBack(rapidjson::Value(), ipAlloc);
		askForValue(name + "[" + StdExt::to_string(i) + "]", d["items"], res[res.Size() - 1] );
	}
}

////////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )
{
	return CThisConsoleApplication(argc, argv).Run();
}

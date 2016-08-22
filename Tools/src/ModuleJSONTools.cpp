// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <ModuleJSONTools.h>

#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <JSONTools.h>
#include <StdTools.h>

using namespace std;
using namespace rapidjson;

////////////////////////////////////////////////////////////////////

IModule* CreateModuleFromJSON( const rapidjson::Value& moduleJson, std::string& errorText )
{
	errorText.clear();

	if( !moduleJson.IsObject() ) {
		errorText += "Module JSON should be an object\n";
		return 0;
	}
	if( !moduleJson.HasMember( "Type" ) || !moduleJson["Type"].IsString()
		 || !moduleJson.HasMember( "Name" ) || !moduleJson["Name"].IsString() ) {
		errorText += "In Module JSON fields 'Type' and 'Name' are obligatory and should be strings.";
		return 0;
	}

	std::string paramsJSON = "{}";
	CreateStringFromJSON( moduleJson, paramsJSON );

	auto_ptr<IModule> result( CreateModule( moduleJson["Type"].GetString(), moduleJson["Name"].GetString(), paramsJSON ) );
	if( result.get() == 0 ) {
		errorText += string( "No Module found with type '" ) + moduleJson["Type"].GetString() + "'"
			" and name '" + moduleJson["Name"].GetString() + "'";
	}
	return result.release();
}

JSON EnumerateRegisteredModulesToJSON()
{
	vector<CModuleRegistration> regs;
	EnumerateModuleRegistrations( regs );

	JSON result;
	result += "[";
	for( int i = 0; i < regs.size(); ++i ) {
		if( i > 0 ) {
			result += ",\n";
		}
		result += "{";
		result = result + "\"Type\":\"" + regs[i].Type + "\",";
		result = result + "\"Name\":\"" + regs[i].Name + "\",";
		result = result + "\"Func\":" + StdExt::to_string(reinterpret_cast<size_t>(regs[i].Func));
		result += "}";
	}
	result += "]\n";
	return result;
}

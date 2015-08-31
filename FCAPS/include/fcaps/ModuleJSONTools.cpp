// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/ModuleJSONTools.h>

#include <fcaps/Module.h>
#include <fcaps/ModuleTools.h>

#include <JSONTools.h>

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


#ifndef MODULEJSONTOOLS_H_INCLUDED
#define MODULEJSONTOOLS_H_INCLUDED

#include <common.h>
#include <rapidjson/document.h>

interface IModule;

////////////////////////////////////////////////////////////////////

// Create Module by its JSON description.
IModule* CreateModuleFromJSON( const rapidjson::Value& moduleJson, std::string& errorText );

template<class TResult>
TResult* CreateModuleFromJSON( const rapidjson::Value& moduleJson, std::string& errorText )
{
	TResult* res = dynamic_cast<TResult*>(CreateModuleFromJSON(moduleJson,errorText));
	if( res != 0 ) {
		return res;
	}
	if( errorText.empty() ) {
		errorText="The type of module is incorrect";
	}
	return 0;
}
#endif // MODULEJSONTOOLS_H_INCLUDED

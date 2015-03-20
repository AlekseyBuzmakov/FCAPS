// Author: Aleksey Buzmakov
// Description: Tools for working with Modules.

#ifndef MODULETOOLS_H_INCLUDED
#define MODULETOOLS_H_INCLUDED

#include <Module.h>

////////////////////////////////////////////////////////////////////

// Common Module Types
const char ContextProcessorModuleType[] = "ContextProcessorModules";

////////////////////////////////////////////////////////////////////

// Function for creating a module by type and name
IModule* CreateModule( const std::string& type, const std::string& name, const JSON& params );

template<typename ModuleInterface>
ModuleInterface* CreateModule( const std::string& type, const std::string& name, const JSON& params )
{
	return dynamic_cast< ModuleInterface* >( CreateModule( type, name, params ) );
}

////////////////////////////////////////////////////////////////////

// Type of function creating a new module
typedef IModule* (*CreateFunc)();
// Function to register a new module, normally used from CModuleRegistrar
void RegisterModule(const std::string& type, const std::string& name, CreateFunc func );

// Class for registering a module. Should be used as static member of ModuleClass.
template<typename ModuleClass>
class CModuleRegistrar {
public:
	CModuleRegistrar( const std::string& type, const std::string& name )
		{ RegisterModule( type, name, &createModule ); }

private:
	static IModule* createModule()
		{ return new ModuleClass; }
};

#endif //MODULETOOLS_H_INCLUDED

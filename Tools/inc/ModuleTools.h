// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Tools for working with Modules.

#ifndef MODULETOOLS_H_INCLUDED
#define MODULETOOLS_H_INCLUDED

#include <fcaps/Module.h>
#include <vector>

////////////////////////////////////////////////////////////////////

// Function for creating a module by type and name. The module is either registered by function RegisterModule or is created by an external function.
IModule* CreateModule( const std::string& type, const std::string& name, const JSON& params );

template<typename ModuleInterface>
ModuleInterface* CreateModule( const std::string& type, const std::string& name, const JSON& params )
{
	return dynamic_cast< ModuleInterface* >( CreateModule( type, name, params ) );
}

// Prototype of a function for creating modules by types and names. A pointer to this function is used inside CreateModule
typedef IModule* (*TExternalCreateModuleFunc)( const std::string& type, const std::string& name, const JSON& params);
// Function for switching to an external module creating function
// @return The previous value.
TExternalCreateModuleFunc SwitchToExternalFunction( TExternalCreateModuleFunc  func );

////////////////////////////////////////////////////////////////////

// Type of function creating a new module
typedef IModule* (*CreateFunc)();
// Function to register a new module, normally used from CModuleRegistrar
void RegisterModule(const std::string& type, const std::string& name, CreateFunc func, const std::string& desc = std::string("{}") );
// Get the number of registered modules
int GetModuleNumber();
// Enumerating of registered modules
struct CModuleRegistration{
	std::string Type;
	std::string Name;
	CreateFunc Func;
	std::string Desc;

	CModuleRegistration() :
		Func(0) {}
};
void EnumerateModuleRegistrations( std::vector<CModuleRegistration>& regs );

// Class for registering a module. Should be used as static member of ModuleClass.
template<typename ModuleClass>
class CModuleRegistrar {
public:
	CModuleRegistrar() :
		type( ModuleClass::Type() ), name( ModuleClass::Name() ) { RegisterModule( ModuleClass::Type(), ModuleClass::Name(), &createModule, ModuleClass::Desc() ); }
	CModuleRegistrar( const std::string& t, const std::string& n ) :
		type( t ), name( n ) { RegisterModule( t, n, &createModule, ModuleClass::Desc() ); }

	const char* const GetType() const
		 { return type.c_str(); }
	const char* const GetName() const
		 { return name.c_str(); }
private:
	const std::string type;
	const std::string name;
	static IModule* createModule()
		{ return new ModuleClass; }
};

#endif //MODULETOOLS_H_INCLUDED

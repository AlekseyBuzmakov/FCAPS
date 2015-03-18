// Author: Aleksey Buzmakov
// Description: Module interface for creting modules by the module name.

#ifndef MODULE_H_INCLUDED
#define MODULE_H_INCLUDED

#include <common.h>

#include <fcaps/BasicTypes.h>

////////////////////////////////////////////////////////////////////

// An interface of a module.
interface IModule : public IObject {
	// Load/Save module params.
	virtual void LoadParams( const JSON& ) = 0;
	virtual JSON SaveParams() const = 0;
};

////////////////////////////////////////////////////////////////////

// Common Module Types
const char PatternManagerModuleType[] = "Pattern Manager";
const char LatticeBuilderModuleType[] = "Lattice Builder";
const char ClassifierModuleType[] = "Classifier";
const char ConceptBuilderModuleType[] = "Concept Builder";
const char ProjectionChainType[] = "Projection Chain";

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

////////////////////////////////////////////////////////////////////

#endif // MODULE_H_INCLUDED

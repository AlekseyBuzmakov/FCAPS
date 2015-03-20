// Author: Aleksey Buzmakov
// Description: Module interface for creting modules by the module name.

#include <fcaps/Module.h>
#include <fcaps/ModuleTools.h>

#include <boost/unordered_map.hpp>

using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////

typedef unordered_map<string,CreateFunc> CTypedModuleCollection;
typedef unordered_map<string, CTypedModuleCollection> CModuleCollection;

CModuleCollection& getModuleCollection()
{
	static CModuleCollection collection;
	return collection;
}

////////////////////////////////////////////////////////////////////

IModule* CreateModule( const string& type, const string& name, const JSON& params )
{
	CModuleCollection& collection = getModuleCollection();
	CTypedModuleCollection& typedCollection = collection[type];
	CTypedModuleCollection::const_iterator fnd = typedCollection.find( name );
	if( fnd == typedCollection.end() ) {
		return 0;
	}
	CreateFunc createFunc = fnd->second;
	auto_ptr<IModule> newModule( createFunc() );
	assert( newModule.get() != 0 );
	newModule->LoadParams( params );
	return newModule.release();
}

////////////////////////////////////////////////////////////////////

void RegisterModule(const std::string& type, const std::string& name, CreateFunc func )
{
	CModuleCollection& collection = getModuleCollection();
	CTypedModuleCollection& typedCollection = collection[type];
	assert( typedCollection.find( name ) == typedCollection.end() );
	typedCollection[name]=func;
}

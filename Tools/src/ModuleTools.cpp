// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Module interface for creting modules by the module name.

#include <ModuleTools.h>

#include <fcaps/Module.h>

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

TExternalCreateModuleFunc& getCreateModuleFunc()
{
	static TExternalCreateModuleFunc func = 0;
	return func;
}

IModule* CreateModule( const string& type, const string& name, const JSON& params )
{

	TExternalCreateModuleFunc & extCreateModuleFunc = getCreateModuleFunc();
	if( extCreateModuleFunc != 0 ) {
		return extCreateModuleFunc(type,name,params);
	} else {
		assert( extCreateModuleFunc == 0 );
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
}

TExternalCreateModuleFunc SwitchToExternalFunction( TExternalCreateModuleFunc f )
{
	TExternalCreateModuleFunc& func = getCreateModuleFunc();
	const TExternalCreateModuleFunc oldFunc = func;
	func = f;
	return oldFunc;
}

////////////////////////////////////////////////////////////////////

void RegisterModule(const std::string& type, const std::string& name, CreateFunc func )
{
	CModuleCollection& collection = getModuleCollection();
	CTypedModuleCollection& typedCollection = collection[type];
	assert( typedCollection.find( name ) == typedCollection.end() );
	typedCollection[name]=func;
}
int GetModuleNumber()
{
	CModuleCollection& collection = getModuleCollection();
	int result = 0;
	CStdIterator<CModuleCollection::const_iterator> itr( collection );
	for( ; !itr.IsEnd(); ++itr ) {
		result += itr->second.size();
	}
	return result;
}
void EnumerateModuleRegistrations( std::vector<CModuleRegistration>& regs )
{
	CModuleCollection& collection = getModuleCollection();
	regs.resize(GetModuleNumber());
	CStdIterator<CModuleCollection::const_iterator> itr( collection );
	for(int i = 0; !itr.IsEnd(); ++itr ) {
		const string& type = itr->first;
		CStdIterator<CTypedModuleCollection::const_iterator> reg( itr->second );
		for( ; !reg.IsEnd(); ++reg, ++i ) {
			regs[i].Type = type;
			regs[i].Name = reg->first;
			regs[i].Func = reg->second;
		}
	}
}


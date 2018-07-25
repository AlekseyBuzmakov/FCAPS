// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Module interface for creting modules by the module name.

#include <ModuleTools.h>

#include <fcaps/Module.h>

#include <boost/unordered_map.hpp>

using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////

struct CModuleInfo {
	CreateFunc Func;
	string Desc;

	CModuleInfo() :
		Func(0) {}
	CModuleInfo( CreateFunc f, const string& desc) :
		Func(f), Desc(desc) {}
};
typedef unordered_map<string,CModuleInfo> CTypedModuleCollection;
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
		CreateFunc createFunc = fnd->second.Func;
		unique_ptr<IModule> newModule( createFunc() );
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

void RegisterModule(const std::string& type, const std::string& name, CreateFunc func, const std::string& desc)
{
	CModuleCollection& collection = getModuleCollection();
	CTypedModuleCollection& typedCollection = collection[type];
	assert( typedCollection.find( name ) == typedCollection.end() );
	typedCollection.insert(std::make_pair(name, CModuleInfo(func, desc)));
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
void EnumerateModuleRegistrations( const std::string& type, std::vector<CModuleRegistration>& regs )
{
	CModuleCollection& collection = getModuleCollection();
	CStdIterator<CTypedModuleCollection::const_iterator> reg( collection[type] );
	for( ; !reg.IsEnd(); ++reg ) {
		CModuleRegistration newReg;
		newReg.Type = type;
		newReg.Name = reg->first;
		newReg.Func = reg->second.Func;
		newReg.Desc = reg->second.Desc;
		regs.push_back(newReg);
	}
}
void EnumerateModuleRegistrations( std::vector<CModuleRegistration>& regs )
{
	CModuleCollection& collection = getModuleCollection();
	regs.clear();
	regs.reserve(GetModuleNumber());
	CStdIterator<CModuleCollection::const_iterator> itr( collection );
	for(; !itr.IsEnd(); ++itr ) {
		const string& type = itr->first;
		EnumerateModuleRegistrations( type, regs );
	}
}


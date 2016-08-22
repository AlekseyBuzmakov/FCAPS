// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2016, v0.7

#include <common.h>
#include <Sofia.h>
#include <ModuleTools.h>
#include <ModuleJSONTools.h>

/////////////////////////////////////////////
TGetSofiaFunction& getSofiaApiFunc()
{
	static TGetSofiaFunction func = 0;
	return func;
}
IModule* CreateModuleBySofiaApi( const std::string& type, const std::string& name, const std::string& jsonParams )
{
	const TGetSofiaFunction func = getSofiaApiFunc();
	assert( func != 0 );

	static const TCreateModuleFunc createModuleFunc = reinterpret_cast<TCreateModuleFunc>(
		                func( CreateModuleFunction,sizeof(CreateModuleFunction)/sizeof(CreateModuleFunction[0])-1) );
	assert(createModuleFunc != 0);

	return reinterpret_cast<IModule*>(createModuleFunc(type.c_str(),type.length(),name.c_str(),name.length(),jsonParams.c_str(),jsonParams.length()));
}

/////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

bool SofiaAPI InitializeModule( TGetSofiaFunction func )
{
	getSofiaApiFunc()=func;
	SwitchToExternalFunction( CreateModuleBySofiaApi );
	return true;
}

const char* SofiaAPI GetModuleDescription()
{
	static const std::string description ( EnumerateRegisteredModulesToJSON() );
	std::cout << description << "\n";
	return description.c_str();
}

#ifdef __cplusplus
}
#endif


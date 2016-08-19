// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2016, v0.7

#include <common.h>
#include <Sofia.h>
#include <ModuleJSONTools.h>

#ifdef __cplusplus
extern "C" {
#endif

bool SofiaAPI InitializeModule( TGetSofiaFunction func )
{
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


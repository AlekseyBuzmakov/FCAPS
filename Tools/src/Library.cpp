// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2016, v0.7

// Author: Aleksey Buzmakov
// Description: A wrap-up for dealing with libraries.

#include "Library.h"

#ifndef _WIN32

#include <dlfcn.h>
#include <assert.h>

#else

#include <windows.h>
#include <StdTools.h>

#endif

////////////////////////////////////////////////////////////////////////

Library::Library() :
	handle( 0 )
{
    //ctor
}

Library::Library( const std::string& p ) :
	handle( 0 )
{
	Open( p );
}

Library::~Library()
{
	Close();
}

void Library::Open( const std::string& p )
{
	Close();
	path = p;
#ifndef _WIN32
	handle = dlopen (path.c_str(), RTLD_LAZY);
	if (handle == 0) {
		throw new CLibException("Library::Open()", dlerror());
	}
#else
	handle = LoadLibrary(TEXT(path.c_str()));
	if (handle == 0) {
		throw new CLibException("Library::Open()", "Error code: " + StdExt::to_string(GetLastError()));
	}
#endif
}
void Library::Close()
{
	if( IsOpen() ) {
#ifndef _WIN32
		dlclose(handle);
#else
		FreeLibrary(reinterpret_cast<HMODULE>(handle));
#endif
	}
	handle = 0;
	path.clear();
}
Library::TFuncPtr Library::GetFunc( const std::string& name ) const
{
	assert( handle != 0 );
#ifndef _WIN32
	const TFuncPtr result = dlsym( handle, name.c_str() );
	const char* const error = dlerror();
    if ( error != 0 )  {
		throw new CLibException( "Library::GetFunc('" + name + "')", error );
    }
#else
	const TFuncPtr result = GetProcAddress(reinterpret_cast<HMODULE>(handle), name.c_str());
	if( result == 0 ) {
		throw new CLibException( "Library::GetFunc('" + name + "')", "Error code: " + StdExt::to_string(GetLastError()) );
	}
#endif
    return result;
}

void Library::MoveTo( Library& other ) {
	other.handle = handle;
	other.path = path;
	handle = 0;
	path = "";
}

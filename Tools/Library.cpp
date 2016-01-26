// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2016, v0.7

// Author: Aleksey Buzmakov
// Description: A wrap-up for dealing with libraries.

#include "Library.h"

#include <dlfcn.h>

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
	handle = dlopen (path.c_str(), RTLD_LAZY);
	if( !handle ) {
		throw new CLibException( "Library::Open()", dlerror() );
	}
}
void Library::Close()
{
	if( IsOpen() ) {
		dlclose(handle);
	}
	handle = 0;
	path.clear();
}
Library::TFuncPtr Library::GetFunc( const std::string& name ) const
{
	assert( handle != 0 );
	const TFuncPtr result = dlsym( handle, name.c_str() );
	const char* const error = dlerror();
    if ( error != 0 )  {
		throw new CLibException( "Library::GetFunc('" + name + "')", error );
    }
    return result;
}

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include "RelativePathes.h"

#include <deque>

using namespace std;

namespace RelativePathes {
////////////////////////////////////////////////////////////////////////

void BaseName( const std::string& path, std::string& dirName )
{
	size_t size = path.size();
	size_t pos = path.find_last_of( "/\\");
	while( pos == size - 1 ) {
		size = pos;
		pos = path.find_last_of( "/\\", pos );
	}
	if( pos == std::string::npos ) {
		dirName == "./";
	} else {
		dirName = path.substr( 0, pos + 1 );
	}
}

deque<string>& getSPathes()
{
	static deque<string> pathes;
	return pathes;
}
void PushSearchPath( const std::string& path )
{
	const size_t pos = path.find_last_of( "/\\");
	assert( pos == path.size() - 1 );
	getSPathes().push_back( path );
}
void PullSearchPath()
{
	assert( getSPathes().size() > 0 );
	getSPathes().pop_back();
}

DWORD GetSearchPath( DWORD ID, std::string& path )
{
	const deque<string>& pathes = getSPathes();
	if( pathes.empty()  ) {
		path = "";
		return END_SEARCH_PATH;
	}
	assert( TOP_SEARCH_PATH > pathes.size() );
	if( ID > pathes.size() ) {
		path = pathes.back();
		return pathes.size() - 1;
	} else {
		path = pathes[ID - 1];
		return ID - 1;
	}
}

inline bool fileExists (const std::string& name) {
	if (FILE *file = fopen(name.c_str(), "r")) {
		fclose(file);
		return true;
	} else {
		return false;
	}
}

void GetFullPath( const std::string& path, std::string& fullPath )
{
	DWORD id = TOP_SEARCH_PATH;
	string sPath;
	while( id != END_SEARCH_PATH ) {
		id = GetSearchPath( id, sPath );
		fullPath = sPath + path;
		if( fileExists( fullPath ) ) {
			return;
		}
	}
	if( fileExists( path ) ) {
		fullPath = path;
	} else {
		// We hope that the most general path is working.
	}
}

////////////////////////////////////////////////////////////////////////
} // namespace RelativePathes

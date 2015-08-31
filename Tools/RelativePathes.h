// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef RELATIVEPATHES_H
#define RELATIVEPATHES_H

#include <common.h>

namespace RelativePathes {
////////////////////////////////////////////////////////////////////////

// Returns the folder of the file/directory given by 'path'
void BaseName( const std::string& path, std::string& dirName );

// Push and pull search path to search pathes stack
void PushSearchPath( const std::string& path );
void PullSearchPath();

// Iteration of Search pathes
// @param ID: the id of the search path
// @result: the id of the next search path
const DWORD TOP_SEARCH_PATH = -1;
const DWORD END_SEARCH_PATH = 0;
DWORD GetSearchPath( DWORD ID, std::string& path );

// Automatic switcher of search pathes
class CSearchPath {
public:
	CSearchPath()
		{}
	CSearchPath( const std::string& _path ):
		path(_path) { PushSearchPath( path ); }
	~CSearchPath() {
		Pull();
	}

	const std::string& Path() const
		{return path;}
	void SetPath( const std::string& p )
		{ Pull(); path = p; PushSearchPath(path); }
	void Pull() const {
		std::string topPath;
		GetSearchPath( TOP_SEARCH_PATH, topPath );
		if( !path.empty() && path == topPath ) {
			PullSearchPath();
		}
	}

private:
	std::string path;
};

// Returns the full path of the file
void GetFullPath( const std::string& path, std::string& fullPath );

////////////////////////////////////////////////////////////////////////
} // namespace RelativePathes

#endif // RELATIVEPATHES_H

#ifndef LIBRARY_H
#define LIBRARY_H

#include <Exception.h>

#include <string>

////////////////////////////////////////////////////////////////////////

class CLibException : public CException {
public:
	CLibException ( const std::string& place, const std::string& error ) :
		CException( place ) { SetText( error ); }
};

////////////////////////////////////////////////////////////////////////

class Library {
public:
	typedef void* TFuncPtr;
public:
    Library();
	// If there is a problem, it throws CLibException
	Library( const std::string& path );
    ~Library();

	// Open/close library
	//  (if there is a problem, it throws CLibException)
	void Open( const std::string& path );
	void Close();
	// Checks if open
	bool IsOpen() const
	{ return handle != 0; }
	// Get a pointer to a function
	//  (if there is a problem, it throws CLibException)
	TFuncPtr GetFunc( const std::string& name ) const;
	// Moves handle to another Library
	void MoveTo( Library& other );
private:
	// Path to library
	std::string path;
	// A handle to the loaded library
	void* handle;
	const Library& operator=(const Library& other);
};

#endif // LIBRARY_H

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: common defenitions
#ifndef common_h
#define common_h

// usual includings
#include <iostream>

#include <boost/assert.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>

#include <StdIteratorWrapper.h>

#define CSharedPtr boost::shared_ptr
#define CWeakPtr boost::weak_ptr
#define CPtrOwner boost::scoped_ptr
#define CDetachedPtrOwner std::auto_ptr

typedef boost::iostreams::stream<boost::iostreams::file_source> CSourceStream;
typedef boost::iostreams::stream<boost::iostreams::file_sink> CDestStream;

#define interface struct

const int NotFound = -1;

interface IObject {
	// Virtual destructor. It should be in all virtual classes.
	virtual ~IObject()
		{}
};

#ifdef _DEBUG
#define debug_cast dynamic_cast
#else
#define debug_cast static_cast
#endif

////////////////////////////////////////////////////////////////////

#define DWORD unsigned int
#define FLAG(a) (1 << a)

inline bool HasAllFlags( DWORD flagsSet, DWORD flagsToCheck )
{
	return (flagsSet & flagsToCheck) == flagsToCheck;
}

inline bool HasAnyFlag( DWORD flagsSet, DWORD flagsToCheck )
{
	return (flagsSet & flagsToCheck) != 0;
}

////////////////////////////////////////////////////////////////////

template<typename T>
inline T& assertNotNull( T& value )
{
	assert( value != 0 );
	return value;
}

#endif // common_h

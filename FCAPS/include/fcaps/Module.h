// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Module interface for creting modules by the module name.

#ifndef MODULE_H_INCLUDED
#define MODULE_H_INCLUDED

#include <common.h>

#include <fcaps/BasicTypes.h>

////////////////////////////////////////////////////////////////////

// An interface of a module.
interface IModule : public IObject {
	// Load/Save module params.
	// Works with successors of schemas/module.json
	//  the params that are not in the JSON does not change
	virtual void LoadParams( const JSON& ) = 0;
	virtual JSON SaveParams() const = 0;
	// Get type and name of the module
	virtual const char* const GetType() const = 0;
	virtual const char* const GetName() const = 0;
};

////////////////////////////////////////////////////////////////////

#endif // MODULE_H_INCLUDED

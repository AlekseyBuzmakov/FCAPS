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
	virtual void LoadParams( const JSON& ) = 0;
	virtual JSON SaveParams() const = 0;
};

////////////////////////////////////////////////////////////////////

#endif // MODULE_H_INCLUDED

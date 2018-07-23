// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Interface of algorithms building FCA lattices.

#ifndef CONCEPTBUILDER_H_
#define CONCEPTBUILDER_H_

#include <common.h>

#include <fcaps/BasicTypes.h>

#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////

interface IComputationCallback;

////////////////////////////////////////////////////////////////////

const char ContextProcessorModuleType[] = "ContextProcessorModules";

interface IContextProcessor : public virtual IObject {
	// Callback for progress reporting.
	virtual void SetCallback( const IComputationCallback * cb ) = 0;

	// Get/Set object names
	virtual const std::vector<std::string>& GetObjNames() const = 0;
	virtual void SetObjNames( const std::vector<std::string>& ) = 0;

	// Passes common params of all descriptions.
	virtual void PassDescriptionParams( const JSON& ) = 0;

	// Preparation for object addition
	virtual void Prepare() = 0;
	// Add next object to algorithm.
	//  objectNum -- numero of the object.
	virtual void AddObject( DWORD objectNum, const JSON& intent ) = 0;

	// Post processing after addition of last object
	virtual void ProcessAllObjectsAddition() = 0;

	// Saving a result of the processor
	virtual void SaveResult( const std::string& path ) = 0;
};

////////////////////////////////////////////////////////////////////////

#endif // CONCEPTBUILDER_H_

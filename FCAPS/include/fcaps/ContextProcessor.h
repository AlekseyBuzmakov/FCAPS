// Author: Aleksey Buzmakov
// Description: Interface of algorithms building FCA lattices.

#ifndef CONCEPTBUILDER_H_
#define CONCEPTBUILDER_H_

#include <common.h>

#include <fcaps/BasicTypes.h>

#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////

interface IContextProcessorCallback;

////////////////////////////////////////////////////////////////////

const char ContextProcessorModuleType[] = "ConceptProcessorModule";

interface IContextProcessor : public virtual IObject {
	// Callback for progress reporting.
	virtual void SetCallback( const IContextProcessorCallback * cb ) = 0;

	// Get/Set object names
	virtual const std::vector<std::string>& GetObjNames() const = 0;
	virtual void SetObjNames( const std::vector<std::string>& ) = 0;

	// Add next object to algorithm.
	//  objectNum -- numero of the object.
	virtual void AddObject( DWORD objectNum, const JSON& intent ) = 0;

	// Post processing after addition of last object
	virtual void ProcessAllObjectsAddition() = 0;

	// Saving a result of the processor
	virtual void SaveResult( const std::string& path ) = 0;
};

////////////////////////////////////////////////////////////////////////

interface IContextProcessorCallback : public virtual IObject {
	virtual void ReportProgress( const double& p, const std::string& info ) const = 0;
};

////////////////////////////////////////////////////////////////////////

#endif // CONCEPTBUILDER_H_

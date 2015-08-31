// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Interface of algorithms building FCA lattices.

#ifndef LATTICEBUILDER_H_
#define LATTICEBUILDER_H_

#include <common.h>

////////////////////////////////////////////////////////////////////////

struct CLattice;
interface IPatternPrunner;
interface IExtentStorage;
interface IIntentStorage;

interface ILatticeBuilder : public virtual IObject {
	// extentManager - operations with extents;
	// comparator - operations with patterns.
	virtual void Initialize(
		const CSharedPtr<IExtentStorage>& extentManager,
		const CSharedPtr<IIntentStorage>& intentStorage ) = 0;
	// Set the lattice where results shoud be saved
	//  result - lattice to save result.
	virtual void SetResultLattice( CLattice& result ) = 0;
	// Set prunner to throw out unneeded concepts.
	//  prunner - if possible returns whether concept with parents (children) is needed, could be 0;
	virtual void SetPrunner( const CSharedPtr<IPatternPrunner>& prunner ) = 0;

	// Add next object to algorithm.
	//  objectNum -- numero of the object.
	virtual void AddObject( DWORD objectNum, DWORD patternID ) = 0;

	// Post processing after addition of last object
	virtual void ProcessAllObjectsAddition() = 0;
};

////////////////////////////////////////////////////////////////////////
#endif // LATTICEBUILDER_H_

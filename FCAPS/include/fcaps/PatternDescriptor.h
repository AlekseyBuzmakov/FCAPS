// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Interface of abstruct pattern

#ifndef PATTERNDESCRIPTOR_H
#define PATTERNDESCRIPTOR_H
#include <common.h>
#include <fcaps/BasicTypes.h>

////////////////////////////////////////////////////////////////////

// Object-data for pattern.
interface IPatternDescriptor : public virtual IObject {
	// Is the descriptor most general, i.e. describes all the objects.
	virtual bool IsMostGeneral() const = 0;
	//Get hash of the pattern.
	virtual size_t Hash() const = 0;
	// Get TopoValue of the pattern, i.e., given paterns p and q, if p<q then p.TopoValue() < q.TopoValue(); if p == q, then p.TopoValue() = q.TopoValue(); if p<>q, then any TopoValues are correct
	// virtual double TopoValue() const = 0;
};

////////////////////////////////////////////////////////////////////

#endif // PATTERNDESCRIPTOR_H

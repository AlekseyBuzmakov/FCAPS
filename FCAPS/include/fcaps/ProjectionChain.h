// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef PROJECTIONCHAIN_H_INCLUDED
#define PROJECTIONCHAIN_H_INCLUDED

#include <common.h>
#include <fcaps/PatternManager.h>

#include <ListWrapper.h>
#include <vector>

////////////////////////////////////////////////////////////////////

const char ProjectionChainModuleType[] = "ProjectionChainModules";

////////////////////////////////////////////////////////////////////

interface IProjectionChain : public virtual IObject {
	// Memory inside CPattern is not controlled. The user is responsable for it.
	typedef CList<const IPatternDescriptor*> CPatternList;

	// Get/Set object names, since the patterns could contain both extent and intent.
	virtual const std::vector<std::string>& GetObjNames() const = 0;
	virtual void SetObjNames( const std::vector<std::string>& ) = 0;

	// Add context to the projection chain iteratively.
	//  objectNum -- numero of the object.
	virtual void AddObject( DWORD objectNum, const JSON& intent ) = 0;

	// Updates the interest threshold
	virtual void UpdateInterestThreshold( const double& thld ) = 0;
	// Get interest of pattern 'p'
	virtual double GetPatternInterest( const IPatternDescriptor* p ) = 0;

	// Compare patterns.
	virtual bool AreEqual(const IPatternDescriptor* p, const IPatternDescriptor* q) const = 0;
	virtual bool IsSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const = 0;
	//  a linear order of patterns including IsSmaller order
	virtual bool IsTopoSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const = 0;

	// Free memory of a pattern
	virtual void FreePattern(const IPatternDescriptor* p ) const = 0;

	// Compute the ZERO projection, i.e. the starting one.
	virtual void ComputeZeroProjection( CPatternList& ptrns ) = 0;
	// Go to the next projection.
	//  returns false when all projections are processed.
	virtual bool NextProjection() = 0;
	// Returns the percentage of accompliching
	virtual double GetProgress() const = 0;
	// Compute the preimages of a pattern.
	//  Pattern 'd' is not included in the set.
	//  Probably updates the measure of 'd'.
	//  Useless preimages (e.g. unstable) can be omitted from 'preimages'
	//  NOTE: the preimages are added at the end(!); preimages. Nothing is removed.
	virtual void Preimages( const IPatternDescriptor* d, CPatternList& preimages ) = 0;

	// Some attributes of patterns
	virtual int GetExtentSize( const IPatternDescriptor* d ) const = 0;

	// Saves extent and intent of a pattern
	virtual JSON SaveExtent( const IPatternDescriptor* d ) const = 0;
	virtual JSON SaveIntent( const IPatternDescriptor* d ) const = 0;
};

////////////////////////////////////////////////////////////////////

#endif // PROJECTIONCHAIN_H_INCLUDED

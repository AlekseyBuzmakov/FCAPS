// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8

#ifndef LOCALPROJECTIONCHAIN_H_INCLUDED
#define LOCALPROJECTIONCHAIN_H_INCLUDED

#include <common.h>
#include <fcaps/PatternDescriptor.h>

#include <ListWrapper.h>
#include <vector>

////////////////////////////////////////////////////////////////////

const char LocalProjectionChainModuleType[] = "LocalProjectionChainModules";

////////////////////////////////////////////////////////////////////

interface ILocalProjectionChain : public virtual IObject {
	// Memory inside CPattern is not controlled. The user is responsable for it.
	typedef CList<const IPatternDescriptor*> CPatternList;

	// Get number of objects in the dataset
	virtual int GetObjectNumber() const = 0;

	// Gets/Updates the interest threshold
	virtual double GetInterestThreshold() const = 0;
	virtual void UpdateInterestThreshold( const double& thld ) = 0;
	// Get interest of pattern 'p'
	virtual double GetPatternInterest( const IPatternDescriptor* p ) = 0;

	// Compare patterns.

	// TODO: probably will be used later
	// virtual bool AreEqual(const IPatternDescriptor* p, const IPatternDescriptor* q) const = 0;
	// virtual bool IsSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const = 0;
	//  a linear order of patterns including IsSmaller order
	virtual bool IsTopoSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const = 0;

	// Free memory of a pattern
	virtual void FreePattern(const IPatternDescriptor* p ) const = 0;

	// Compute the ZERO projection, i.e. the starting one.
	virtual void ComputeZeroProjection( CPatternList& ptrns ) = 0;
	// Compute the preimages of a pattern.
	//  Pattern 'd' is not included in the set.
	//  Probably updates the measure of 'd'.
	//  Useless preimages (e.g. unstable) can be omitted from 'preimages'
	//  NOTE: the preimages are added at the end(!); preimages. Nothing is removed.
	// @return A flag indicating if more projections are possible
	virtual bool Preimages( const IPatternDescriptor* d, CPatternList& preimages ) = 0;

	// Some attributes of patterns
	virtual int GetExtentSize( const IPatternDescriptor* d ) const = 0;

	// Saves extent and intent of a pattern
	virtual JSON SaveExtent( const IPatternDescriptor* d ) const = 0;
	virtual JSON SaveIntent( const IPatternDescriptor* d ) const = 0;
};

////////////////////////////////////////////////////////////////////

#endif // LOCALPROJECTIONCHAIN_H_INCLUDED


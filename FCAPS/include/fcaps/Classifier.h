// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CLASSIFIER_H_INCLUDED
#define CLASSIFIER_H_INCLUDED

#include <common.h>
#include <fcaps/BasicTypes.h>

//////////////////////////////////////////////////////////////////////////

const char ClassifierModuleType[] = "ClassifierModules";

//////////////////////////////////////////////////////////////////////////

// A classes corresponding to no dicision
const char CL_NotEnoughData[] = "###NotEnoughData###";
const char CL_ContradictoryData[] = "###ContradictoryData###";

interface IClassifier : public IObject {
	// Passes common params of all descriptions.
	virtual void PassDescriptionParams( const JSON& ) = 0;
	// Prepare for classification. Should be called when all data are given.
	virtual void Prepare() = 0;
	// Return the class of the object. Can be called only after Prepare().
	virtual std::string Classify( const JSON& ptrn ) const = 0;
};

//////////////////////////////////////////////////////////////////////////

#endif // CLASSIFIER_H_INCLUDED

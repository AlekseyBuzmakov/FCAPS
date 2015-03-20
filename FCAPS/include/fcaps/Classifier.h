#ifndef CLASSIFIER_H_INCLUDED
#define CLASSIFIER_H_INCLUDED

#include <common.h>

#include <string>
#include <boost/unordered_map.hpp>

interface IIntentStorage;

//////////////////////////////////////////////////////////////////////////

const char ClassifierModuleType[] = "ClassifierModules";

//////////////////////////////////////////////////////////////////////////

// A classes corresponding to no dicision
const char CL_NotEnoughData[] = "###NotEnoughData###";
const char CL_ContradictoryData[] = "###ContradictoryData###";

interface IClassifier : public IObject {
	typedef boost::unordered_map<std::string,std::string> CObjToClassesMap;
	// Set pattern manager to with with certain patterns.
	virtual void SetPatternManager( const CSharedPtr<IIntentStorage>& cmp ) = 0;
	// Set path to the training data.
	virtual void SetTrainData( const std::string& path ) = 0;
	// Set mapping from object names to classes.
	virtual void SetClasses( const CObjToClassesMap& classes ) = 0;
	// Prepare for classification. Should be called when all data are given.
	virtual void Prepare() = 0;
	// Return the class of the object. Can be called only after Prepare().
	virtual std::string Classify( int intID ) const = 0;
};

//////////////////////////////////////////////////////////////////////////

#endif // CLASSIFIER_H_INCLUDED

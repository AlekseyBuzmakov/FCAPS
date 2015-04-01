#ifndef CCAEPBYDONGCLASSIFIER_H
#define CCAEPBYDONGCLASSIFIER_H

#include <fcaps/Classifier.h>
#include <fcaps/Module.h>
#include <fcaps/ModuleTools.h>

const char CAEPByDongClassifier[] = "CAEPByDongClassifierModule";

class CCAEPByDongClassifier : public IClassifier, public IModule {
public:
	CCAEPByDongClassifier();

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return ClassifierModuleType; };
	virtual const char* const GetName() const
		{ return CAEPByDongClassifier; };

	// Methods of IClassifier
	virtual void SetPatternManager( const CSharedPtr<IIntentStorage>& cmp );
	virtual void SetTrainData( const std::string& path );
	virtual void SetClasses( const CObjToClassesMap& classes );
	virtual void Prepare();
	virtual std::string Classify( int intID ) const;

	// SelfMethods
	void AddPattern( const std::vector<std::string>& extent, const JSON& intent );

private:
	struct CEmergingPattern{
		DWORD Id;
		std::string Class;
		double Accuracy;

		CEmergingPattern() :
			Id(-1), Accuracy(0) {}
		CEmergingPattern(DWORD id,std::string cl,double acc) :
			Id(id), Class(cl), Accuracy(acc) {}
	};
private:
	static const CModuleRegistrar<CCAEPByDongClassifier> registrar;

	CSharedPtr<IIntentStorage> cmp;
	std::string trainPath;
	const CObjToClassesMap* classes;
	// The emergency threshold. If there are n_1, n_2, n_k objects of k classes, then emergency wrt class 1 is n_1 / (n_2+...+n_k).
	double emThld;
	// The coverage of an object
	boost::unordered_map<std::string,int> objToPtrnCoverage;
	// The set of emerging patterns
	std::vector<CEmergingPattern> eps;
	// The base scores for every class
	boost::unordered_map<std::string,int> baseScores;

	void computeAgregateScores();

};

#endif // CCAEPBYDONGCLASSIFIER_H

#ifndef CCLASSIFIERCONTEXTPROCESSOR_H
#define CCLASSIFIERCONTEXTPROCESSOR_H

#include <fcaps/ContextProcessor.h>
#include <fcaps/Module.h>
#include <fcaps/ModuleTools.h>

#include <fcaps/modules/details/JsonClassifierClasses.h>

interface IClassifier;

////////////////////////////////////////////////////////////////////

const char ClassifierContextProcessorModule[] = "ClassifierContextProcessorModule";

class CClassifierContextProcessor : public IContextProcessor, public IModule {
public:
	CClassifierContextProcessor();

	// Methods of IContextProcessor
	virtual void SetCallback( const IContextProcessorCallback * cb )
		 {callback = cb;}
	virtual const std::vector<std::string>& GetObjNames() const;
	virtual void SetObjNames( const std::vector<std::string>& );
	virtual void PassDescriptionParams( const JSON& );
	virtual void Prepare();
	virtual void AddObject( DWORD objectNum, const JSON& intent );
	virtual void ProcessAllObjectsAddition();
	virtual void SaveResult( const std::string& path );

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return registrar.GetType(); }
	virtual const char* const GetName() const
		{ return registrar.GetName(); }

private:
	struct COutputParams {
		// Should the overall accuracy be printed
		bool OutAccuracy;
		// Should be confusion matrix be printed
		bool OutConfusion;
		// Should the separate object prediction be printed
		bool OutObjectClassification;

		COutputParams() :
			OutAccuracy( true ), OutConfusion( true ), OutObjectClassification( false ) {}
	};

private:
	static const CModuleRegistrar<CClassifierContextProcessor> registrar;
	CSharedPtr<IClassifier> classifier;
	COutputParams outParams;
	const IContextProcessorCallback * callback;
	std::vector<std::string> objectNames;

	std::string classesPath;
	JsonClassifierClasses::CObjToClassesMap classes;
	JsonClassifierClasses::CObjToClassesMap predictions;

	void computeConfusion( boost::unordered_map<std::pair<std::string,std::string>, int>& conf, int& truePredictionNum , int& predictionNum ) const;
};

////////////////////////////////////////////////////////////////////

#endif // CCLASSIFIERCONTEXTPROCESSOR_H

#ifndef CSTABILITYESTIMATORCONTEXTPROCESSOR_H
#define CSTABILITYESTIMATORCONTEXTPROCESSOR_H

#include <fcaps/ContextProcessor.h>
#include <fcaps/Module.h>

#include <fcaps/tools/StabilityCalculation.h>
#include <fcaps/tools/StabilityChildrenApproximation.h>

#include <fcaps/ModuleTools.h>

class CVectorBinarySetJoinComparator;

////////////////////////////////////////////////////////////////////

const char StabilityEstimatorContextProcessorModule[] = "StabilityEstimatorContextProcessorModule";

class CStabilityEstimatorContextProcessor : public IContextProcessor, public IModule {
public:
	CStabilityEstimatorContextProcessor();

	// Methods of IContextProcessor
	virtual void SetCallback( const IContextProcessorCallback * cb )
		 {callback = cb;}
	virtual const std::vector<std::string>& GetObjNames() const
		{return objectNames;}
	virtual void SetObjNames( const std::vector<std::string>& names )
		{ objectNames = names; }
	virtual void PassDescriptionParams( const JSON& )
		{ /*DO nothing for the moment*/ }
	virtual void Prepare()
		{ /*DO nothing for the moment*/ }
	virtual void AddObject( DWORD objectNum, const JSON& intent );
	virtual void ProcessAllObjectsAddition()
		{ /*DO nothing for the moment*/ }
	virtual void SaveResult( const std::string& path );

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return registrar.GetType(); }
	virtual const char* const GetName() const
		{ return registrar.GetName(); }

private:
	struct CResult {
		DWORD Id;
		double LBound;
		double UBound;

		CResult():
			Id(-1), LBound(0), UBound( -1 ) {}
	};
private:
	static const CModuleRegistrar<CStabilityEstimatorContextProcessor> registrar;
	const IContextProcessorCallback * callback;
	std::vector<std::string> objectNames;

	std::string contextPath;
	double alpha;

	CSharedPtr<CVectorBinarySetJoinComparator> cmp;
	CBinarySetCollection attrToTidsetMap;

	CStabilityChildrenApproximation stabApprox;

	boost::container::deque<CResult> results;

	void loadContext();

};

#endif // CSTABILITYESTIMATORCONTEXTPROCESSOR_H

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CINTERVALPATTERNMANAGER_H
#define CINTERVALPATTERNMANAGER_H

#include <fcaps/PatternManager.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <fcaps/SharedModulesLib/details/JsonIntervalPattern.h>

////////////////////////////////////////////////////////////////////

const char IntervalPatternManagerModule[] = "IntervalPatternManagerModule";

////////////////////////////////////////////////////////////////////

class CIntervalPatternDescriptor : public IPatternDescriptor {
public:
	// Methods of IPatternDescriptor
	virtual bool IsMostGeneral() const
		{return false;}
	virtual size_t Hash() const;

	JsonIntervalPattern::CPattern& Intervals()
		 {return ints;}
	const JsonIntervalPattern::CPattern& Intervals() const
		 {return ints;}
private:
	JsonIntervalPattern::CPattern ints;
};

////////////////////////////////////////////////////////////////////

class CIntervalPatternManager : public IPatternManager, public IModule {
public:
	CIntervalPatternManager();

	virtual const CIntervalPatternDescriptor* LoadObject( const JSON& );
	virtual JSON SavePattern( const IPatternDescriptor* ) const;
	virtual const CIntervalPatternDescriptor* LoadPattern( const JSON& );

	virtual const CIntervalPatternDescriptor* CalculateSimilarity(
		const IPatternDescriptor* first, const IPatternDescriptor* second );
	virtual TCompareResult Compare(
		const IPatternDescriptor* first, const IPatternDescriptor* second,
		DWORD interestingResults = CR_AllResults, DWORD possibleResults = CR_AllResults | CR_Incomparable );

	virtual void FreePattern( const IPatternDescriptor * );

	virtual void Write( const IPatternDescriptor* pattern, std::ostream& dst ) const;

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); };
	virtual const char* const GetName() const
		{ return Name(); };
	// For CModuleRegistrar
	static const char* const Type()
		{ return PatternManagerModuleType;}
	static const char* const Name()
		{ return IntervalPatternManagerModule; }
	static const char* const Desc()
		{ return "{}"; }

protected:
	CIntervalPatternDescriptor* NewPattern();
	static const CIntervalPatternDescriptor& Pattern( const IPatternDescriptor* p );

private:
	static const CModuleRegistrar<CIntervalPatternManager> registrar;

	// Number of intervals in patterns;
	DWORD intsCount;
};

#endif // CINTERVALPATTERNMANAGER_H

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CCOMPOSITPATTERNMANAGER_H
#define CCOMPOSITPATTERNMANAGER_H

#include <common.h>

#include <fcaps/PatternManager.h>
#include <fcaps/Module.h>
#include <ModuleTools.h>

#include<boost/ptr_container/ptr_vector.hpp>

////////////////////////////////////////////////////////////////////

const char CompositPatternManager[] = "CompositPatternManagerModule";

////////////////////////////////////////////////////////////////////

class CCompositPatternManager;

////////////////////////////////////////////////////////////////////

class CCompositePatternDescriptor : public IPatternDescriptor {
	friend class CCompositPatternManager;
public:
	CCompositePatternDescriptor()
		{}
	~CCompositePatternDescriptor();

	// Methods of IPatternDescriptor
	virtual bool IsMostGeneral() const;
	virtual size_t Hash() const;

private:
	std::vector<const IPatternDescriptor*> ptrns;
};

////////////////////////////////////////////////////////////////////

class CCompositPatternManager : public IPatternManager, public IModule {
public:
	CCompositPatternManager();

	// Methods of IPatternManagerr
	virtual const CCompositePatternDescriptor* LoadObject( const JSON& );
	virtual JSON SavePattern( const IPatternDescriptor* ) const;
	virtual const CCompositePatternDescriptor* LoadPattern( const JSON& );

	virtual const CCompositePatternDescriptor* CalculateSimilarity(
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
		{ return CompositPatternManager; }
	static const char* const Desc();

private:
	static const CModuleRegistrar<CCompositPatternManager> registrar;

	boost::ptr_vector<IPatternManager> cmps;

	static const CCompositePatternDescriptor& getCompositPattern( const IPatternDescriptor* ptrn );
	JSON savePattern( const IPatternDescriptor* ) const;
	const CCompositePatternDescriptor* loadPattern( const JSON& );
};

#endif // CCOMPOSITPATTERNMANAGER_H

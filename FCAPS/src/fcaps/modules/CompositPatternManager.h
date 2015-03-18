#ifndef CCOMPOSITPATTERNMANAGER_H
#define CCOMPOSITPATTERNMANAGER_H

#include <common.h>

#include <fcaps/PatternDescriptor.h>
#include <fcaps/Module.h>

#include<boost/ptr_container/ptr_vector.hpp>

////////////////////////////////////////////////////////////////////

const char CompositPatternManager[] = "CompositPatternManager";

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
	virtual TPatternType GetType() const
		{ return PT_Composit; }
	virtual bool IsMostGeneral() const;
	virtual size_t Hash() const;

private:
	std::vector<const IPatternDescriptor*> ptrns;
};

////////////////////////////////////////////////////////////////////

class CCompositPatternManager : public IPatternDescriptorComparator, public IModule {
public:
	CCompositPatternManager();

	// Methods of IPatternDescriptorComparatorr
	virtual TPatternType GetPatternsType() const
		{ return PT_Composit; }

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

private:
	static const CModuleRegistrar<CCompositPatternManager> registrar;

	boost::ptr_vector<IPatternDescriptorComparator> cmps;

	static const CCompositePatternDescriptor& getCompositPattern( const IPatternDescriptor* ptrn );
	JSON savePattern( const IPatternDescriptor* ) const;
	const CCompositePatternDescriptor* loadPattern( const JSON& );
};

#endif // CCOMPOSITPATTERNMANAGER_H

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef GENERALSTRINGSPARTIALORDER_H
#define GENERALSTRINGSPARTIALORDER_H

#include <fcaps/PS-Modules/SimpleStringsPartialOrder.h>
#include <fcaps/PatternManager.h>

////////////////////////////////////////////////////////////////////

const char GeneralStringPartialOrderComparator[] = "GeneralStringPartialOrderComparatorModule";

////////////////////////////////////////////////////////////////////

class CGeneralStringsPartialOrderElement : public CStringPartialOrderElement< CSharedPtr<const IPatternDescriptor> > {
public:
  	// Methods of IPartialOrderElement
	virtual TPartialOrderType GetType() const
		{ return "GeneralStrings"; }
};

////////////////////////////////////////////////////////////////////

class CGeneralStringsPartialOrderComparator : public CStringsPartialOrderComparator< CSharedPtr<const IPatternDescriptor> >, public IModule {
public:
	typedef CSharedPtr<const IPatternDescriptor> TSymb;
public:
	CGeneralStringsPartialOrderComparator()
		{}

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return PartialOrderElementsComparatorModuleType; };
	virtual const char* const GetName() const
		{ return GeneralStringPartialOrderComparator; }

	// Set comparator of elements in strings.
	void Initialize( const CSharedPtr<IPatternManager>& _elemsCmp )
		{ elemsCmp = _elemsCmp; assert(elemsCmp != 0); }

protected:
	// Methods of CStringsPartialOrderComparator
	virtual void SaveSymb( const TSymb&, rapidjson::Value& str, rapidjson::MemoryPoolAllocator<>& ) const;
	virtual const TSymb LoadSymb( const rapidjson::Value& val );
    virtual TCompareResult CompareSymbs( DWORD cmpMask, const TSymb& first, const TSymb& last ) const;
	virtual TSymb CalculateSymbSimilarity( const TSymb& first, const TSymb& last ) const;
	virtual bool IsMostGeneralSymb( const TSymb& el ) const;

private:
	static const CModuleRegistrar<CGeneralStringsPartialOrderComparator> registrar;

	CSharedPtr<IPatternManager> elemsCmp;
};

#endif // GENERALSTRINGSPARTIALORDER_H

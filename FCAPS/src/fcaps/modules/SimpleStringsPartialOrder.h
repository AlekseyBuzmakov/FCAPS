// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef SIMPLESTRINGPARTIALORDER_H
#define SIMPLESTRINGPARTIALORDER_H

#include <common.h>

#include <fcaps/PartialOrderElementsComparator.h>

#include <fcaps/Module.h>
#include <ModuleTools.h>

#include <vector>

// For the implementation

#include <JSONTools.h>
#include <StdTools.h>
#include <ListWrapper.h>

#include <rapidjson/document.h>

////////////////////////////////////////////////////////////////////

const char DwordStringPartialOrderComparator[] = "DwordStringPartialOrderComparatorModule";
const char CharStringPartialOrderComparator[] = "CharStringPartialOrderComparatorModule";

////////////////////////////////////////////////////////////////////

template <typename TSymb>
class CStringPartialOrderElement : public IPartialOrderElement {
public:
	typedef typename std::vector<TSymb> CString;

public:
	CStringPartialOrderElement()
		{}
	// Methods of IPartialOrderElement
	virtual TPartialOrderType GetType() const
		{ return "SimpleStrings"; }
	// Is the descriptor most general, i.e. describes all the objects.
	virtual bool IsMostGeneral() const
		{ return str.size() <= 0; }

	// Methods of the class
	CString& String()
		{ return str; }
	const CString& String() const
		{ return str; }

private:
	CString str;
};

////////////////////////////////////////////////////////////////////

template <typename TSymb>
class CStringsPartialOrderComparator : public IPartialOrderElementsComparator {
public:
	CStringsPartialOrderComparator() :
		minStrLength( 1 ), cutOnEmptyElems( true ) {}

	// Methods of IPartialOrderElementsComparator
	virtual TPartialOrderType GetElementType() const
		{ return "SimpleStrings"; }
	virtual JSON SaveElement( const IPartialOrderElement* ) const;
	virtual void LoadElements( const JSON&, CElementSet& );
	virtual void FindAllParents( const IPartialOrderElement* first, const IPartialOrderElement* second,
		CElementSet& parents ) const;
	virtual TCompareResult Compare( DWORD interestingResults,
		const IPartialOrderElement* first, const IPartialOrderElement* second ) const;
	virtual void FreeElement( const IPartialOrderElement* el)
        { delete el; };
	virtual void FreeElementSet( const CElementSet& set )
        { delete[] set.Elements; };

	// Get/Set minimal lenght of string in the result.
	DWORD GetMinStrLength() const
		{ return minStrLength; }
	void SetMinStrLength( DWORD minLenght )
		{ minStrLength = 1 < minLenght ? minLenght : 1; }
	// Set wether is it necessary to cut substrings on empty (IsMoreGeneral) elements
	bool GetCutOnEmptyElems() const
		{ return cutOnEmptyElems; }
	void SetCutOnEmptyElems( bool value )
		{ cutOnEmptyElems = value; }

protected:
    // Save/Load symbols to/from JSON
	virtual void SaveSymb( const TSymb&, rapidjson::Value& str, rapidjson::MemoryPoolAllocator<>& ) const = 0;
	virtual const TSymb LoadSymb( const rapidjson::Value& val ) = 0;
	// Compare elements of strings, cmpMask -- the flag-set of interesting results.
	virtual TCompareResult CompareSymbs( DWORD cmpMask, const TSymb& first, const TSymb& last ) const = 0;
	// Calculate similarity between elements in strings
	virtual TSymb CalculateSymbSimilarity( const TSymb& first, const TSymb& last ) const = 0;
	// Check if the element is the most general one;
	virtual bool IsMostGeneralSymb( const TSymb& el ) const = 0;

private:
	typedef typename CStringPartialOrderElement<TSymb>::CString CString;


private:
	DWORD minStrLength;
	bool cutOnEmptyElems;

	void intersectStrings( const CString& str1, const CString& str2, int str2Shift, CString& result ) const;
	void splitString( const CString& str, CList<CStringPartialOrderElement<TSymb>*>& parents ) const;
};

////////////////////////////////////////////////////////////////////

template <typename TSymb>
class CSimpleStringsPartialOrderComparator : public CStringsPartialOrderComparator<TSymb> {
public:
	CSimpleStringsPartialOrderComparator( const TSymb& _mostGeneralElem ) :
		mostGeneralElem( _mostGeneralElem ) {}

protected:
	// Methods of CStringsPartialOrderComparator
	virtual TCompareResult CompareSymbs( DWORD cmpMask, const TSymb& first, const TSymb& last ) const
		{ return first == last && (cmpMask & CR_Equal) != 0 ? CR_Equal : CR_Incomparable; }
	virtual TSymb CalculateSymbSimilarity( const TSymb& first, const TSymb& last ) const
		{ if( first == last ) { return first; } else { return mostGeneralElem; } }
	virtual bool IsMostGeneralSymb( const TSymb& el ) const
		{ return el == mostGeneralElem; }

    TSymb& MostGeneralElem()
        { return mostGeneralElem; }
    const TSymb& MostGeneralElem() const
        { return mostGeneralElem; }

private:
	TSymb mostGeneralElem;
};

////////////////////////////////////////////////////////////////////////////////////////////////

class CDwordStringPartialOrderComparator : public CSimpleStringsPartialOrderComparator<DWORD>, public IModule {
public:
    CDwordStringPartialOrderComparator() :
        CSimpleStringsPartialOrderComparator<DWORD>( NotFound ) {};

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return PartialOrderElementsComparatorModuleType; };
	virtual const char* const GetName() const
		{ return DwordStringPartialOrderComparator; };

protected:
    // Methods of CStringsPartialOrderComparator<DWORD>
	virtual void SaveSymb( const DWORD&, rapidjson::Value& str, rapidjson::MemoryPoolAllocator<>& ) const;
	virtual const DWORD LoadSymb( const rapidjson::Value& val );

private:
	static const CModuleRegistrar<CDwordStringPartialOrderComparator> registrar;
};

////////////////////////////////////////////////////////////////////

class CCharStringPartialOrderComparator : public CSimpleStringsPartialOrderComparator<char>, public IModule {
public:
    CCharStringPartialOrderComparator() :
        CSimpleStringsPartialOrderComparator<char>( 0 ) {};

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return PartialOrderElementsComparatorModuleType; };
	virtual const char* const GetName() const
		{ return CharStringPartialOrderComparator; };

protected:
    // Methods of CStringsPartialOrderComparator<DWORD>
	virtual void SaveSymb( const char&, rapidjson::Value& str, rapidjson::MemoryPoolAllocator<>& ) const;
	virtual const char LoadSymb( const rapidjson::Value& val );

private:
	static const CModuleRegistrar<CCharStringPartialOrderComparator> registrar;
};

////////////////////////////////////////////////////////////////////

#include "SimpleStringsPartialOrder.inl"

#endif // SIMPLESTRINGPARTIALORDER_H

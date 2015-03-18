#ifndef SIMPLESTRINGPARTIALORDER_H
#define SIMPLESTRINGPARTIALORDER_H

#include <PartialOrderPatternDescriptor.h>

////////////////////////////////////////////////////////////////////

template <typename TElem>
class CStringsPartialOrderElement : public IPartialOrderElement {
public:
	typedef typename std::vector<TElem> CString;

public:
	CStringsPartialOrderElement()
		{}
	// Methods of IPartialOrderElement
	virtual TPartialOrderType GetType() const
		{ return POT_SimpleStrings; }
	// Is the descriptor most general, i.e. describes all the objects.
	virtual bool IsMostGeneral() const
		{ return str.size() <= 0; }

	// Methods of the class
	CString& GetString()
		{ return str; }
	const CString& GetString() const
		{ return str; }

private:
	CString str;
};

////////////////////////////////////////////////////////////////////

template <typename TElem>
class CStringsPartialOrderComparator : public IPartialOrderElementsComparator {
public:
	CStringsPartialOrderComparator() :
		minStrLength( 1 ), cutOnEmptyElems( true ) {}

	// Get/Set minimal lenght of string in the result.
	DWORD GetMinStrLength() const
		{ return minStrLength; }
	void SetMinStrLength( DWORD minLenght )
		{ minStrLength = 1 < minLenght ? minLenght : 1; }
	// Set wether is it necessary to cut substrings on empty (IsMoreGeneral) elements
	DWORD GetCutOnEmptyElems() const
		{ return cutOnEmptyElems; }
	void SetCutOnEmptyElems( bool value )
		{ cutOnEmptyElems = value; }

	// Methods of IPartialOrderElementsComparator
	virtual TPartialOrderType GetType() const
		{ return POT_SimpleStrings; }
	virtual void PreprocessElement( const CSharedPtr<IPartialOrderElement>& el, CElementSet& parts ) const;
	virtual void FindAllParents(
		const CSharedPtr<IPartialOrderElement>& first, const CSharedPtr<IPartialOrderElement>& second,
		CElementSet& parents ) const;
	virtual TCompareResult Compare( DWORD interestingResults,
		const CSharedPtr<IPartialOrderElement>& first, const CSharedPtr<IPartialOrderElement>& second ) const;
	virtual void Write( const CSharedPtr<IPartialOrderElement>& element, std::ostream& dst ) const;

protected:
	// Preprocess element of a string
	virtual void PreprocessStrElem( const TElem& el ) const
		{}
	// Compare elements of strings, cmpMask -- the flag-set of interesting results.
	virtual TCompareResult CompareElems( DWORD cmpMask, const TElem& first, const TElem& last ) const = 0;
	// Calculate similarity between elements in strings
	virtual TElem CalculateElemsSimilarity( const TElem& first, const TElem& last ) const = 0;
	// Check if the element is the most general one;
	virtual bool IsMostGeneralElem( const TElem& el ) const = 0;
	// Write an element to a stream.
	virtual void WriteElem( const TElem& el, std::ostream& dst ) const = 0;

private:
	typedef typename CStringsPartialOrderElement<TElem>::CString CString;

private:
	DWORD minStrLength;
	bool cutOnEmptyElems;

	void intersectStrings( const CString& str1, const CString& str2, int str2Shift, CString& result ) const;
	void splitString( const CString& str, CElementSet& parents ) const;
	void write( const CString& element, std::ostream& dst ) const;
};

////////////////////////////////////////////////////////////////////

template <typename TElem>
class CSimpleStringsPartialOrderComparator : public CStringsPartialOrderComparator<TElem> {
public:
	CSimpleStringsPartialOrderComparator( const TElem& _mostGeneralElem ) :
		mostGeneralElem( _mostGeneralElem ) {}

protected:
	// Methods of CStringsPartialOrderComparator
	virtual TCompareResult CompareElems( DWORD cmpMask, const TElem& first, const TElem& last ) const
		{ return first == last && (cmpMask & CR_Equal) != 0 ? CR_Equal : CR_Incomparable; }
	virtual TElem CalculateElemsSimilarity( const TElem& first, const TElem& last ) const
		{ if( first == last ) { return first; } else { return mostGeneralElem; } }
	virtual bool IsMostGeneralElem( const TElem& el ) const
		{ return el == mostGeneralElem; }
	virtual void WriteElem( const TElem& el, std::ostream& dst ) const
		{ dst << el; }

private:
	const TElem mostGeneralElem;
};

////////////////////////////////////////////////////////////////////

#include <SimpleStringsPartialOrder.inl>

#endif // SIMPLESTRINGPARTIALORDER_H

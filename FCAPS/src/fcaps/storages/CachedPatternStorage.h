// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CCACHEDPATTERNSTORAGE_H
#define CCACHEDPATTERNSTORAGE_H

#include <common.h>

#include <fcaps/PatternManager.h>

#include <StdIteratorWrapper.h>

#include <boost/unordered_set.hpp>
#include <boost/foreach.hpp>

template<typename THasher>
class CCachedPatternStorage {
public:
	typedef boost::unordered_set<const IPatternDescriptor*, THasher, THasher> CPatternSet;
	typedef typename CPatternSet::iterator CIterator;
	typedef typename CPatternSet::const_iterator CConstIterator;
	typedef CStdIterator<CIterator> CIteratorWrapper;
	typedef CStdIterator<CConstIterator> CConstIteratorWrapper;
public:
	CCachedPatternStorage();
	~CCachedPatternStorage()
		{Clear();}

	void Initialize( const THasher& h );

	void Reserve( size_t size );

	// Add pattern to the storage
	const IPatternDescriptor* AddPattern( const IPatternDescriptor* p );
	// Check if pattern is in the storage
	bool HasPattern( const IPatternDescriptor* p ) const;
	// Removes pattern from the storage
	void RemovePattern(const IPatternDescriptor* p);
	void RemovePattern(const CIterator& itr);

	// Number of patterns in the set
	DWORD Size() const
		{ return patterns->size(); }

	// Iteration of elements
	CIteratorWrapper Iterator();
	CConstIteratorWrapper Iterator() const;

	// Clear the storage
	void Clear();

private:
	THasher hasher;

	CPtrOwner<CPatternSet> patterns;
};

////////////////////////////////////////////////////////////////////

class CPatternComparatorHasher {
public:
	CPatternComparatorHasher()
		{}
	CPatternComparatorHasher( const CSharedPtr<IPatternManager>& _cmp ) :
		cmp(_cmp) {}
	void Init( const CSharedPtr<IPatternManager>& _cmp )
		{ cmp = _cmp; }

	// boost Hasher
	bool operator()(
		const IPatternDescriptor* x, const IPatternDescriptor* y) const;
	size_t operator()( const IPatternDescriptor* x) const;
	// Removing of a pattern
	void Free( const IPatternDescriptor* x);


private:
	CSharedPtr<IPatternManager> cmp;
};

#include "CachedPatternStorage.inl"

#endif // CCACHEDPATTERNSTORAGE_H

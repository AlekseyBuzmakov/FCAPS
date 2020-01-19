// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Finds partial order of concepts given by template class
// Author: Aleksey Buzmakov
#ifndef FINDCONCEPTORDER_H_INCLUDED
#define FINDCONCEPTORDER_H_INCLUDED

#include <fcaps/CompareResults.h>
#include <ListWrapper.h>

template<class TConcepts>
class CFindConceptOrder {
public:
	struct CConcept {
		CList<DWORD> Parents;

		CConcept() {}
	};
public:
	CFindConceptOrder( const TConcepts& _inConcepts );

	// Compute order relation.
	void Compute();

	// Get parents for the given concept
	const CList<DWORD>& GetParents( DWORD conceptNum ) const
	{
		assert( conceptNum < concepts.size() );
		return concepts[conceptNum].Parents;
	}

	// Get arcs number
	DWORD GetArcsCount() const
		{ return arcsCount; }
	// Get all top nodes, i.e. the nodes that have no parent
	const CList<DWORD>& GetTops() const
		{ return tops; }
	// Get all bottom nodes, i.e. the nodes that have no child
	const CList<DWORD>&GetBottoms() const
		{ return bottoms; }

	// Function for topological sort
	bool operator()( DWORD c1, DWORD c2 )
	{
		assert(c1 < concepts.size() );
		assert(c2 < concepts.size() );
		return inConcepts.IsTopologicallyLess(c1, c2);
	}

private:
	const TConcepts& inConcepts;
	std::vector<CConcept> concepts;
	std::vector<DWORD> order;
	std::vector<bool> flags;
	DWORD arcsCount;
	CList<DWORD> tops;
	CList<DWORD> bottoms;

	void markFlags( DWORD ind );
};

/*
// Methods expected from TConcepts
// TConcept is any type of concept you want.
interface TConcepts {
	// Number of concepts
	DWORD Size() const;
	// Access to the concept
	TConcept operator[]( int DWORD ) const;
	// Topological sort of concepts Compare(c1,c2) == CR_MoreGeneral <=> operator<(c1,c2) = true
	bool IsTopologicallyLess( DWORD, DWORD ) const;
	// Comparison of two concepts
	TCompareResult IsLess( DWORD, DWORD ) const;
};
*/

template<class TConcepts>
CFindConceptOrder<TConcepts>::CFindConceptOrder( const TConcepts& _inConcepts ):
	inConcepts( _inConcepts ),
	arcsCount(0)
{
	flags.resize( inConcepts.Size(), false );
	concepts.resize( inConcepts.Size() );
	order.resize( concepts.size() );
	for( DWORD i = 0; i < order.size(); ++i ) {
		order[i]=i;
	}
}

template<class TConcepts>
void CFindConceptOrder<TConcepts>::Compute()
{
	std::sort(order.begin(), order.end(), *this );

	for( int i = 0; i < order.size(); ++i ) {
		std::fill(flags.begin(), flags.end(), false);
		for( int j = i-1; j >= 0; --j ) {
			if( flags[order[j]] ) {
				continue;
			}
			if(!inConcepts.IsLess( order[j], order[i] )) {
				continue;
			}
			concepts[order[i]].Parents.PushBack( order[j] );
			markFlags( order[j] );
		}
	}

	// Computing tops and bottoms
	std::fill(flags.begin(), flags.end(), false);
	arcsCount = 0;
	for( DWORD c = 0; c < concepts.size(); ++c ) {
		if( concepts[c].Parents.Size() == 0 ) {
			tops.PushBack( c );
		}
		arcsCount += concepts[c].Parents.Size();
		CStdIterator<CList<DWORD>::CConstIterator, false> p( concepts[c].Parents );
		for( ; !p.IsEnd(); ++p ) {
			flags[*p]=true;
		}
	}
	for(DWORD c = 0; c < flags.size(); ++c ) {
		if(!flags[c]){
			bottoms.PushBack(c);
		}
	}
}

template<class TConcepts>
void CFindConceptOrder<TConcepts>::markFlags( DWORD ind )
{
	assert( ind < flags.size() );
	flags[ind] = true;
	CStdIterator<CList<DWORD>::CConstIterator, false> itr( concepts[ind].Parents );
	for( ; !itr.IsEnd(); ++itr ) {
		markFlags( *itr );
	}
}


#endif // FINDCONCEPTORDER_H_INCLUDED

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Set of structures describing lattice.
//			  Bottom - near empty set of objects or empty extent.
//			  Top - full set of objects.
//			  Parents are more close to the top, children - to bottom.

#ifndef LATTICE_H_INCLUDED
#define LATTICE_H_INCLUDED

#include <common.h>

#include <fcaps/storages/IntentStorage.h>

#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/unordered_set.hpp>

typedef DWORD TLatticeNodeId;
typedef boost::unordered_set<TLatticeNodeId> CEdges;
struct CLatticeConcept {
	// Extent of concept.
	TIntentId Extent;
	// Similarity of such objects.
	TIntentId Intent;

	CLatticeConcept()
		: Extent( -1 ), Intent( -1 ) {}
};
struct CLatticeNode {
	CEdges Parents;
//	CEdges Children;

	// Description
	CLatticeConcept Data;

	//User data attached to a concept
	void* UserData;

	CLatticeNode() :
		UserData( 0 ) {}
};
typedef boost::ptr_deque<CLatticeNode> CLatticeNodes;

// Bottom part of the lattice.
class CLattice {
public:
	CLattice()
		{}

	int Size() const
		{ return nodesOwner.size(); }
	TLatticeNodeId AddNewNode();

	void AddLink( TLatticeNodeId parent, TLatticeNodeId child );
	void RemoveLink( TLatticeNodeId parent, TLatticeNodeId child );

	CLatticeNode& GetNode( TLatticeNodeId id )
		{ return nodesOwner[id]; }
	const CLatticeNode& GetNode( TLatticeNodeId id ) const
		{ return nodesOwner[id]; }

	CLatticeNodes& GetNodes()
		{ return nodesOwner; }
	const CLatticeNodes& GetNodes() const
		{ return nodesOwner; }

private:
	CLatticeNodes nodesOwner;
};

inline TLatticeNodeId CLattice::AddNewNode()
{
	nodesOwner.push_back( new CLatticeNode );
	return nodesOwner.size() - 1;
}

inline void CLattice::AddLink( TLatticeNodeId parent, TLatticeNodeId child )
{
	nodesOwner[child].Parents.insert( parent );
}

inline void CLattice::RemoveLink( TLatticeNodeId parent, TLatticeNodeId child )
{
	nodesOwner[child].Parents.erase( parent );
}


#endif // LATTICE_H_INCLUDED

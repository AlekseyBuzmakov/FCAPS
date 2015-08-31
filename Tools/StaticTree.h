// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef TREE_H_INCLUDED
#define TREE_H_INCLUDED

#include <common.h>

#include <ListWrapper.h>
#include <vector>

////////////////////////////////////////////////////////////////////

template<typename T>
class CStaticTreeNode;

template<typename T>
class CStaticTree {
public:
	typedef T Type;
	typedef DWORD TNodeIndex;
	typedef std::vector<TNodeIndex> CChildrenSet;
	typedef CStdIterator<CList< TNodeIndex >::CConstIterator, true> CIterator;
	typedef CStaticTreeNode<T> CNode;

public:
	CStaticTree()
		{}

	TNodeIndex GetRoot() const
		{ return 0; }
	CNode& GetNode( TNodeIndex node )
		{ return nodes[node]; }
	const CNode& GetNode( TNodeIndex node ) const
		{ return nodes[node]; }
	void Reserve( DWORD size )
		{ nodes.reserve(); }
	void Clear()
		{ nodes.clear(); }
	size_t GetSize() const
		{ return nodes.size(); }

	TNodeIndex AddNode( TNodeIndex parent )
	{
		assert( parent == (DWORD) -1 || parent < nodes.size() );
		nodes.push_back( CNode( parent ) );
		if( parent < nodes.size() ) {
			nodes[parent].children.push_back( nodes.size() - 1 );
		}
		return nodes.size() - 1;
	}

private:
	std::vector<CNode> nodes;
};

template<typename T>
class CStaticTreeNode {
	friend class CStaticTree<T>;
public:
	typedef typename CStaticTree<T>::CChildrenSet CChildrenSet;
	typedef typename CStaticTree<T>::TNodeIndex TNodeIndex;
	typedef typename CStaticTree<T>::CIterator CIterator;

public:
	// Data contained in the node.
	T Data;

	CStaticTreeNode( TNodeIndex _parent ) :
		parent( _parent ) {}

	TNodeIndex GetParent() const
		{ return parent;}

	DWORD ChildrenCount() const
		{ return children.size(); }
	DWORD operator[] ( DWORD child ) const
		{ return children[child]; }

	CIterator ChildrenIterator() const
		{ return CIterator( children ); }

private:
	TNodeIndex parent;
	CChildrenSet children;
};

template<typename T>
class CDeepFirstStaticTreeIterator {
public:
	typedef CDeepFirstStaticTreeIterator<T> _Self;
	typedef CStaticTree<T> CTree;
	typedef typename CTree::CNode CNode;
	typedef typename CTree::TNodeIndex TNodeIndex;

	typedef T* pointer;
	typedef T& reference;

public:
	CDeepFirstStaticTreeIterator() :
		tree( 0 ), node( 0 ), isForward( true ), isEnd( true) {}
	CDeepFirstStaticTreeIterator( TNodeIndex root, const CTree& _tree ) :
		tree( &_tree ), node( root ), childrenItr( 0 ), isForward( true ), isEnd( false ) {}

	void Reset( TNodeIndex root, const CTree& _tree )
		{ tree = &_tree; node = root; childrenItr = 0; isForward = true; isEnd = false; stack.Clear(); }

	TNodeIndex operator*() const
		{ return node; }

	_Self& operator++()
		{ next(); return *this; }

	_Self operator++(int)
		{ _Self tmp = *this; next(); return tmp; }

	bool operator==( const _Self& other ) const
		{ return node == other.node && childrenItr == other.childrenItr && isForward == other.isForward; }
	bool operator!=( const _Self& other ) const
		{ return !operator==( other ); }

	bool IsForward() const
		{ return isForward; }
	bool IsEnd() const
		{ return isEnd; }

private:
	struct CState {
		TNodeIndex Vertex;
		DWORD Child;

		CState() :
			Vertex(-1), Child(0) {}
		CState( TNodeIndex v, DWORD c ) :
			Vertex(v), Child(c) {}
	};
private:
	const CTree* tree;
	TNodeIndex node;
	TNodeIndex childrenItr;
	bool isForward;
	bool isEnd;

	CList<CState> stack;

	void next();
};

template<typename T>
inline void CDeepFirstStaticTreeIterator<T>::next()
{
	if( childrenItr < tree->GetNode(node).ChildrenCount() ) {
		stack.PushBack( CState( node, childrenItr ) );
		node = tree->GetNode(node)[childrenItr];
		childrenItr = 0;
		isForward = true;
		return;
	}

	if( stack.IsEmpty() ) {
		isEnd = true;
		return;
	}

	const CState& back = stack.Back();
	node = back.Vertex;
	childrenItr = back.Child;
	++childrenItr;
	stack.PopBack();
	isForward = false;
}

////////////////////////////////////////////////////////////////////

#endif // TREE_H_INCLUDED

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef PATRITIA_TREE_H_INCLUDED
#define PATRITIA_TREE_H_INCLUDED

#include <common.h>

#include <ListWrapper.h>
#include <vector>
#include <deque>
#include <set>

////////////////////////////////////////////////////////////////////

class CPatritiaTreeNode;

class CPatritiaTree {
public:
	typedef int TNodeIndex;
	typedef int TAttribute;
	typedef int TObject;
	typedef std::set<TNodeIndex, CPatritiaTree> CChildrenSet;
	typedef CPatritiaTreeNode CNode;

public:
	CPatritiaTree() : tmpNode(0),
		{}

	TNodeIndex GetRoot() const {
		if( nodes.size() == 0) {
			AddNode(-1, -1);
		}
		return 0;
	}
	CNode& GetNode( TNodeIndex node )
		{ return nodes[node]; }
	const CNode& GetNode( TNodeIndex node ) const
		{ return nodes[node]; }
	void Clear()
		{ nodes.clear(); }
	size_t GetSize() const
		{ return nodes.size(); }

	TNodeIndex AddNode( TNodeIndex parent, TAttribute genAttr )
	{
		assert( parent == -1 || 0 <= parent && parent < nodes.size() );
		nodes.push_back( CNode( parent, genAttr ) );
		if( parent >= 0 ) {
			auto res = nodes[parent].children.insert( nodes.size() - 1 );
			assert(res.second); // Insertion should have happend here
		}
		return nodes.size() - 1;
	}
	TNodeIndex GetAttributeNode(TNodeIndex id, TAttribute a) {
		assert(0 <= id && id : nodes.size());
		CNode& nd = nodes[id];

		tmpNode.genAttr = a;
		auto res = nd.children.find(tmpNode);
		if(res != nd.children.end()) {
			return *res;
		} else {
			return AddNode(id, a);
		}
	}
	// Comparator
	bool operator()( const TNodeIndex& lhs, const TNodeIndex& rhs ) const
	{
		assert( 0 <= lhs && lhs < nodes.size());
		assert( 0 <= rhs && rhs < nodes.size());

		return nodes[lhs].genAttr < nodes[rhs].genAttr;
	}

private:
	std::deque<CNode> nodes;
	std::vector<TAttribute> closureAttrs;
	std::vector<TObject> objects;

	CNode tmpNode; // for search purposes
};

class CPatritiaTreeNode {
	friend class CPatritiaTree<T>;
public:
	typedef typename CPatritiaTree<T>::TAttribute TAttribute;

	typedef typename CPatritiaTree<T>::CChildrenSet CChildrenSet;
	typedef typename CPatritiaTree<T>::TNodeIndex TNodeIndex;
	typedef typename CPatritiaTree<T>::CIterator CIterator;

public:
	CPatritiaTreeNode( TNodeIndex _parent, TAttribute a ) :
		parent( _parent ), genAttr(a),
		closureAttrStart(-1), closureAttrEnd(-1),
		beforeAttrsCount(0), afterAttrsCount(0)
	{}

	TNodeIndex GetParent() const
		{ return parent;}

	// Returns the child, corresponding to generator attribute attr.
	//   -1 is for no-generator child
	// Returns negative value if the node is not found
	TNodeIndex GetChild( TAttribute attr ) const {
		auto res = children.find(attr);
		if( res  == children.end()) {
			return -1;
		} else {
			return *res;
		}
	}

	// The set of children
	CChildrenSet Children;

	// Generator Attribute
	TAttribute GenAttr;

	// The interval [] of sorted set of closure attributes
	int ClosureAttrStart;
	int ClosureAttrEnd;

	// The interval [] specifying the objects belonging to the node
	int ObjStart;
	int ObjEnd;

	// The number of common attributes after the genAttr
	int AfterAttrsCount;
	// The number of common attributes before the genAttr (it is always more ore equal the depth of the node)
	int BeforeAttrsCount;

private:
	// Index of the parent node
	TNodeIndex parent;
};

class CDeepFirstPatritiaTreeIterator {
public:
	typedef CDeepFirstPatritiaTreeIterator _Self;
	typedef CPatritiaTree CTree;
	typedef typename CTree::CNode CNode;
	typedef typename CTree::TNodeIndex TNodeIndex;

	enum TStatus {
	              // The first time a node is visited
	              S_Forward = 0,
	              // The internal visit of the node (going from one child to another one)
	              S_Return,
	              // The last visit of the node
	              S_Exit
	};


public:
	CDeepFirstPatritiaTreeIterator() :
		tree( 0 ), node( 0 ), status( S_Forward ), isEnd( true) {}
	CDeepFirstPatritiaTreeIterator( const CTree& _tree ) :
		tree( &_tree ), node( _tree.GetRoot() ), childrenItr( 0 ), status( S_Forward ), isEnd( false ) {}

	void Reset( const CTree& _tree )
		{ tree = &_tree; node = tree->GetRoot(); childrenItr = 0; status = S_Forward; isEnd = false; stack.Clear(); }

	TNodeIndex operator*() const
		{ return node; }

	_Self& operator++()
		{ next(); return *this; }

	_Self operator++(int)
		{ _Self tmp = *this; next(); return tmp; }

	bool operator==( const _Self& other ) const
		{ return tree == other.tree && node == other.node && childrenItr == other.childrenItr && isForward == other.isForward; }
	bool operator!=( const _Self& other ) const
		{ return !operator==( other ); }

	T_Status Status() const
		{ return status;}
	bool IsEnd() const
		{ return isEnd; }

private:
	struct CState {
		TNodeIndex Node;
		CNode::CChildrenSet::const_iterator Child;

		CState() :
			Node(-1) {}
		CState( TNodeIndex v, CNode::CChildrenSet::const_iterator c ) :
			Vertex(v), Child(c) {}
	};
private:
	const CTree* tree;
	TNodeIndex node;
	CNode::CChildrenSet::const_iterator child;
	TStatus status;
	bool isEnd;

	CList<CState> stack;

	void next();
};

inline void CDeepFirstPatritiaTreeIterator<T>::next()
{
	if( isEnd ) {
		return;
	}

	assert(tree != 0);
	switch(status) {
	case S_Exit:
		assert(child == tree->GetNode(node).Children().end());

		if( stack.IsEmpty() ) {
			isEnd = true;
			return;
		}

		const CState& back = stack.Back();
		node = back.Node;
		child = back.Child;
		stack.PopBack();
		status = S_Return;
		return;
	case S_Forward:
		child = tree->GetNode(node).Children().begin();
		break;
	case S_Return:
		++child;
		break;
	default:
		assert(false);
		return;
	}

	if( child == tree->GetNode(node).Children().end()) {
		status = S_Exit;
		return;
	}

	stack.PushBack( CState( node, child ) );
	node = *child;
	child = tree->GetNode(node).Children().end();
	status == S_Forward;
}

////////////////////////////////////////////////////////////////////

#endif // PATRITIA_TREE_H_INCLUDED

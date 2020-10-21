// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef PATRITIA_TREE_H_INCLUDED
#define PATRITIA_TREE_H_INCLUDED

#include <common.h>

#include <ListWrapper.h>
#include <vector>
#include <deque>
#include <set>
#include <map>

////////////////////////////////////////////////////////////////////

class CPatritiaTreeNode;
class CPatritiaTree;

class CPatritiaTreeNodeCmp {
public:
	typedef int TNodeIndex;
public:
	CPatritiaTreeNodeCmp(const CPatritiaTree& _tree):
		tree(_tree) {}

	bool operator()( const TNodeIndex& lhs, const TNodeIndex& rhs ) const;
private:
	const CPatritiaTree& tree;
};

class CPatritiaTree {
public:
	typedef CPatritiaTreeNodeCmp::TNodeIndex TNodeIndex;
	typedef int TAttribute;
	typedef std::set<TNodeIndex, CPatritiaTreeNodeCmp> CChildrenSet;

	typedef int TObject;
	typedef CPatritiaTreeNode CNode;

public:
	CPatritiaTree();

	TNodeIndex GetRoot() const {
		assert( nodes.size() != 0);
		return 0;
	}
	CNode& GetNode( TNodeIndex node )
		{ return nodes[node]; }
	const CNode& GetNode( TNodeIndex node ) const
		{ return nodes[node]; }
	TNodeIndex GetNodeIndex( const CNode& nd) const;
	void Clear()
		{ nodes.clear(); AddNode(-1,-1); }
	size_t GetSize() const
		{ return nodes.size(); }

	TNodeIndex AddNode( TNodeIndex parent, TAttribute genAttr );
	void MoveChild( TNodeIndex child, TNodeIndex newParent);

	TNodeIndex GetAttributeNode(TNodeIndex id, TAttribute a) const;
	TNodeIndex GetAttributeNode(const CNode& node, TAttribute a) const;
	TNodeIndex GetOrCreateAttributeNode(TNodeIndex id, TAttribute a);
	void ChangeGenAttr(TNodeIndex id, TAttribute a);

	// Adds objects to tree
	int AddObject(TObject obj) {
		objects.push_back(obj);
		return objects.size() - 1;
	}
	TObject GetObject(int index) const
		{ assert( 0 <= index && index < objects.size()); return objects[index];}
	// Add attributes in the closure
	int AddAttribute(TAttribute a) {
		closureAttrs.push_back(a);
		return closureAttrs.size() - 1;
	}
	TAttribute GetClsAttribute(int index) const
		{ assert( 0 <= index && index < closureAttrs.size()); return closureAttrs[index];}
	// Get object number and attribute number
	int GetObjNumber() const
		{ return objects.size(); }
	int GetAttrNumber() const;

	// Comparator
	bool operator()( const TNodeIndex& lhs, const TNodeIndex& rhs ) const;

	// Prints the structure of the tree
	void Print();

private:
	std::deque<CNode> nodes;
	std::vector<TAttribute> closureAttrs;
	std::vector<TObject> objects;

	mutable TAttribute genAttributeToSearch;
};

class CPatritiaTreeNode {
	friend CPatritiaTree;
public:
	typedef typename CPatritiaTree::TAttribute TAttribute;
	typedef typename CPatritiaTree::CChildrenSet CChildrenSet;
	typedef typename CPatritiaTree::TNodeIndex TNodeIndex;
	typedef std::set<TAttribute> TAttributeSet;
	typedef std::multimap<TAttribute, const CPatritiaTreeNode*> TNextAttributeIntersections;

public:
	CPatritiaTreeNode( TNodeIndex _parent, TAttribute a, CPatritiaTree& tree ) :
		Children(CPatritiaTreeNodeCmp(tree)),
		parent( _parent ), GenAttr(a),
		ClosureAttrStart(-1), ClosureAttrEnd(-1),
		ObjStart(-1), ObjEnd(-1)
	{}

	TNodeIndex GetParent() const
		{ return parent;}

	// Returns the child, corresponding to generator attribute attr.
	//   -1 is for no-generator child
	// Returns negative value if the node is not found
	TNodeIndex GetChild( TAttribute attr ) const {
		auto res = Children.find(attr);
		if( res  == Children.end()) {
			return -1;
		} else {
			return *res;
		}
	}

	// Cleans the data minimizin the memory consumption
	void Clear() {
		ClosureAttrStart =-1;
		ClosureAttrEnd = -1;
		ObjStart = -1;
		ObjEnd = -1;
		Children.clear();
		CommonAttributes.clear();
	}

	// The set of children
	// Children.find should not be used! Use CPatritiaTree::GetAttributeNode instead
	CChildrenSet Children;

	// The set of all common attributes to the node
	TAttributeSet CommonAttributes;
	// The references to the nodes that contains intersection of the current node with attributes larger than genAttr.
	TNextAttributeIntersections NextAttributeIntersections;
	// Generator Attribute the attributes that splits the object in the parrent vertex in two disjoint sets.
	TAttribute GenAttr;

	// The interval [] of sorted set of closure attributes in the CPatritiaTree.closureAttrs
	int ClosureAttrStart;
	int ClosureAttrEnd;

	// The interval [] specifying the objects belonging to the node in CPatritiaTree.objects
	int ObjStart;
	int ObjEnd;

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
	CDeepFirstPatritiaTreeIterator( CTree& _tree ) :
		tree( &_tree ), node( &_tree.GetNode(_tree.GetRoot()) ), status( S_Forward ), isEnd( false ) {}
	CDeepFirstPatritiaTreeIterator( CTree& _tree, TNodeIndex i ) :
		tree( &_tree ), node( &_tree.GetNode(i) ), status( S_Forward ), isEnd( false ) {}
	CDeepFirstPatritiaTreeIterator( CTree& _tree, CNode& nd ) :
		tree( &_tree ), node( &nd ), status( S_Forward ), isEnd( false ) {}


	void Reset( CTree& _tree ) {
		tree = &_tree;
		node =&tree->GetNode(tree->GetRoot());
		child = node->Children.end();
		status = S_Forward;
		isEnd = false;
		stack.Clear();
	}

	CNode& operator*()
		{ return *node; }
	CNode* operator->()
	{ return node; }

	_Self& operator++()
		{ next(); return *this; }

	_Self operator++(int)
		{ _Self tmp = *this; next(); return tmp; }

	bool operator==( const _Self& other ) const
		{ return tree == other.tree && node == other.node && child == other.child && status == other.status; }
	bool operator!=( const _Self& other ) const
		{ return !operator==( other ); }

	TStatus Status() const
		{ return status;}
	bool IsEnd() const
		{ return isEnd; }

private:
	struct CState {
		CNode* Node;
		CNode::CChildrenSet::const_iterator Child;

		CState() :
			Node(0) {}
		CState( CNode* v, CNode::CChildrenSet::const_iterator c ) :
			Node(v), Child(c) {}
	};
private:
	CTree* tree;
	CNode* node;
	CNode::CChildrenSet::const_iterator child;
	TStatus status;
	bool isEnd;

	CList<CState> stack;

	void next();
};

inline void CDeepFirstPatritiaTreeIterator::next()
{
	if( isEnd ) {
		return;
	}

	assert(tree != 0);
	assert(node != 0);
	switch(status) {
	case S_Exit: {
		assert(child == node->Children.end());

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
	}
	case S_Forward:
		child = node->Children.begin();
		break;
	case S_Return:
		assert(child != node->Children.end());
		++child;
		break;
	default:
		assert(false);
		return;
	}

	if( child == node->Children.end()) {
		status = S_Exit;
		return;
	}

	stack.PushBack( CState( node, child ) );
	node = &tree->GetNode(*child);
	child = node->Children.end();
	status = S_Forward;
}

#include "PatritiaTree.inl"

////////////////////////////////////////////////////////////////////

#endif // PATRITIA_TREE_H_INCLUDED

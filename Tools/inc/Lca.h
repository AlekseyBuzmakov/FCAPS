// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

// Least Common Ancestor in the tree algorithm
// Author: Aleksey Buzmakov

#ifndef LCA_H_INCLUDED
#define LCA_H_INCLUDED

#include <StaticTree.h>
#include <Rmq.h>

#include <vector>

// LCA, works with
// TIndex is an unsigned integer type
template<typename TTreeData, typename TIndex = unsigned short>
class CLcaAlgorithm {
public:
	typedef CStaticTree<TTreeData> CTree;
public:
	// Does not copy the tree.
	CLcaAlgorithm( const CTree& _tree ) :
		tree( _tree ), rmq( depthArray ), indecisArray( 0 ), dataArray( 0 ) {}

	// Initialization
	//  The tree should not be changed after this function
	void Initialize();

	// Get parent of two nodes
	const TIndex GetParent( const TIndex& first, const TIndex& ) const;

	// Get initializing tree
	const CTree& GetTree() const
		{ return tree; }

private:
	typedef unsigned short TDepth;
	typedef unsigned short TRMQIndex;

	const CTree& tree;
	// The array for RMQ
	std::vector<TDepth> depthArray;
	// RMQ
	CRmqAlgorithm<TDepth, TIndex> rmq;
	// The map form index to RMQ-index
	std::vector<TRMQIndex> indecisArray;
	// The map from RMQ-index to index
	std::vector<TIndex> dataArray;

	size_t calcTreeSize() const;
};

template<typename TTreeData, typename TIndex>
void CLcaAlgorithm<TTreeData, TIndex>::Initialize()
{
	const size_t size = calcTreeSize();
	depthArray.resize( size * 2 - 1 );
	dataArray.resize( depthArray.size() );

	CDeepFirstStaticTreeIterator<TTreeData> node( tree.GetRoot(), tree );
	TDepth depth = -1;
	size_t index = 0;
	for( ; !node.IsEnd(); ++node, ++index ) {
		assert( index <= depthArray.size() );

		if( node.IsForward() ) {
			depth++;
		} else {
			depth--;
		}

		const TIndex nodeIndex = *node;
		assert( 0 <= nodeIndex );
		dataArray[index] = nodeIndex;

		if( nodeIndex >= indecisArray.size() ) {
			indecisArray.resize( nodeIndex + 1, 0 );
		}
		indecisArray[nodeIndex] = index;

		depthArray[index] = depth;
	}
	rmq.Initialize();
}

template<typename TTreeData, typename TIndex>
inline size_t CLcaAlgorithm<TTreeData, TIndex>::calcTreeSize() const
{
	return tree.GetSize();
}

template<typename TTreeData, typename TIndex>
inline const TIndex CLcaAlgorithm<TTreeData, TIndex>::GetParent( const TIndex& first, const TIndex& second ) const
{
	assert( 0 <= first && first < indecisArray.size() );
	assert( 0 <=second && second < indecisArray.size() );
	const TRMQIndex& firstIndex = indecisArray[first];
	const TRMQIndex& secondIndex = indecisArray[second];

	const TRMQIndex resultIndex = rmq.GetMinIndexOnRange( std::min( firstIndex, secondIndex), std::max( firstIndex, secondIndex ) );
	assert( 0 <= resultIndex && resultIndex < dataArray.size() );

	return dataArray[resultIndex];
}



#endif // LCA_H_INCLUDED

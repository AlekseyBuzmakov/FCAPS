#include <iomanip>

bool CPatritiaTreeNodeCmp::operator()( const TNodeIndex& lhs, const TNodeIndex& rhs ) const
{
	return tree(lhs,rhs);
}

///////////////////////////////////////////////

CPatritiaTree::CPatritiaTree() : genAttributeToSearch(-1)
{
	AddNode(-1, -1);
}

CPatritiaTree::TNodeIndex CPatritiaTree::AddNode( TNodeIndex parent, TAttribute genAttr )
{
	assert( parent == -1 || 0 <= parent && parent < nodes.size() );
	nodes.push_back( CNode( parent, genAttr, *this ) );
	if( parent >= 0 ) {
		auto res = nodes[parent].Children.insert( nodes.size() - 1 );
		assert(res.second); // Insertion should have happend here
	}
	return nodes.size() - 1;
}
void CPatritiaTree::MoveChild( TNodeIndex child, TNodeIndex newParent)
{
	assert( 0 <= child && child < nodes.size());
	assert( 0 <= newParent && newParent < nodes.size());
	CNode& p = nodes[newParent];
	CNode& ch = nodes[child];

	assert(p.Children.find(child) == p.Children.end()); 
	p.Children.insert(child);
	ch.parent = newParent;
}
CPatritiaTree::TNodeIndex CPatritiaTree::GetAttributeNode(const CNode& nd, TAttribute a)
{
	genAttributeToSearch = a;
	auto res = nd.Children.find(-1); // A special flag indicating that genAttributeToSearch should be used instead
	genAttributeToSearch = -1;

	if(res != nd.Children.end()) {
		return *res;
	} else {
		return -1;
	}
}
CPatritiaTree::TNodeIndex CPatritiaTree::GetAttributeNode(TNodeIndex id, TAttribute a)
{
	assert(0 <= id && id < nodes.size());
	CNode& nd = nodes[id];

	return GetAttributeNode(nd, a);
}
bool CPatritiaTree::operator()( const TNodeIndex& lhs, const TNodeIndex& rhs ) const
{
	assert( lhs == -1 || 0 <= lhs && lhs < nodes.size());
	assert( rhs == -1 || 0 <= rhs && rhs < nodes.size());

	const TAttribute l = (lhs == -1) ? genAttributeToSearch : nodes[lhs].GenAttr;
	const TAttribute r = (rhs == -1) ? genAttributeToSearch : nodes[rhs].GenAttr;
	assert( 0 <= l &&  0 <= r);

	return l < r;
}
void CPatritiaTree::Print()
{
	CDeepFirstPatritiaTreeIterator treeItr(*this);
	int depth = 0;
	bool isIndented = false;
	for(; !treeItr.IsEnd(); ++treeItr) {
		  switch(treeItr.Status()) {
		  case CDeepFirstPatritiaTreeIterator::S_Exit:
			   break;
		  case CDeepFirstPatritiaTreeIterator::S_Forward: {
		  	   if( !isIndented) {
			   	   std::cout << std::endl;
			   	   for( int i = 0; i < depth; ++i) {
				   		std::cout << "      ";
				   }
				   // isIndented = true;
			   }
		  	   std::cout << "->" << std::setw(4) << treeItr->GenAttr << " (";
			   assert(0 <= treeItr->ClosureAttrStart && treeItr->ClosureAttrEnd <= closureAttrs.size());
			   for( int i = treeItr->ClosureAttrStart; i < treeItr->ClosureAttrEnd; ++i) {
			   		if( i != treeItr->ClosureAttrStart) {
						std::cout << ",";
					}
					std::cout << closureAttrs[i];
			   }
				std::cout << ") Ext(" << treeItr->ObjEnd - treeItr->ObjStart << ")";
			   ++depth;
			   break;
		  }
		  case CDeepFirstPatritiaTreeIterator::S_Return:
		  	   isIndented = false;
			   assert( depth > 0);
			   --depth;
			   break;
		  default:
			assert(false);
	      }
	}
	std::cout << std::endl;
}

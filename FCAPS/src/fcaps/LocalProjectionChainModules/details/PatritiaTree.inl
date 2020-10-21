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

CPatritiaTree::TNodeIndex CPatritiaTree::GetNodeIndex( const CNode& nd) const
{
	return nd.GetParent() == -1 ? GetRoot() : GetAttributeNode(nd.GetParent(), nd.GenAttr);
}

CPatritiaTree::TNodeIndex CPatritiaTree::AddNode( TNodeIndex parent, TAttribute genAttr )
{
	assert( parent == -1 || 0 <= parent && parent < nodes.size() );
	
	assert( parent == -1 || GetAttributeNode(parent, genAttr) == -1);
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
	assert( ch.GetParent() == -1 || 0 <= ch.GetParent() && ch.GetParent() < nodes.size());
	if( ch.GetParent() >= 0 ) {
		CNode& oldParent = nodes[ch.GetParent()];
		auto itr = oldParent.Children.find(child);
		assert( itr != oldParent.Children.end());
		oldParent.Children.erase(itr);
	}

	assert(p.Children.find(child) == p.Children.end()); 
	p.Children.insert(child);
	ch.parent = newParent;
}
CPatritiaTree::TNodeIndex CPatritiaTree::GetAttributeNode(const CNode& nd, TAttribute a) const
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
CPatritiaTree::TNodeIndex CPatritiaTree::GetAttributeNode(TNodeIndex id, TAttribute a) const
{
	assert(0 <= id && id < nodes.size());
	const CNode& nd = nodes[id];

	return GetAttributeNode(nd, a);
}

CPatritiaTree::TNodeIndex CPatritiaTree::GetOrCreateAttributeNode(TNodeIndex id, TAttribute a)
{
	const TNodeIndex res = GetAttributeNode(id, a);
	return res == -1 ? AddNode(id,a) : res;
}

void CPatritiaTree::ChangeGenAttr(TNodeIndex id, TAttribute a)
{
	assert(0 <= id && id < nodes.size());
	CNode& node = nodes[id];
	assert(0 <= node.parent && node.parent < nodes.size());
	CNode& p = nodes[node.parent];
	p.Children.erase(id);
	assert(GetAttributeNode(node.parent, a) == -1);
	node.GenAttr = a;
	p.Children.insert(id);
	assert(GetAttributeNode(node.parent, a) != -1);
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

int CPatritiaTree::GetAttrNumber() const
{
	const CNode& root = nodes[GetRoot()];
	int attrNum = root.NextAttributeIntersections.rend()->first + 1;
	if( !root.CommonAttributes.empty() ) {
		attrNum = std::max(attrNum, *root.CommonAttributes.rend() + 1);
	}
	return attrNum;
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
			   auto attr = treeItr->CommonAttributes.upper_bound(treeItr->GenAttr);
			   bool isFirst = true;
			   for(; attr != treeItr->CommonAttributes.end(); ++attr) {
			   		 if(!isFirst) {
						std::cout << ",";
					 } 
					 std::cout << *attr;
					 isFirst = false;
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

CPatritiaTree::CPatritiaTree() : genAttributeToSearch(-1)
	{AddNode(-1, -1);}

CPatritiaTree::TNodeIndex CPatritiaTree::AddNode( TNodeIndex parent, TAttribute genAttr )
{
	assert( parent == -1 || 0 <= parent && parent < nodes.size() );
	nodes.push_back( CNode( parent, genAttr ) );
	if( parent >= 0 ) {
		auto res = nodes[parent].Children.insert( nodes.size() - 1 );
		assert(res.second); // Insertion should have happend here
	}
	return nodes.size() - 1;
}
CPatritiaTree::TNodeIndex CPatritiaTree::GetAttributeNode(TNodeIndex id, TAttribute a) {
	assert(0 <= id && id < nodes.size());
	CNode& nd = nodes[id];

	genAttributeToSearch = a;
	auto res = nd.Children.find(-1); // A special flag indicating that genAttributeToSearch should be used instead
	genAttributeToSearch = -1;

	if(res != nd.Children.end()) {
		return *res;
	} else {
		return AddNode(id, a);
	}
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

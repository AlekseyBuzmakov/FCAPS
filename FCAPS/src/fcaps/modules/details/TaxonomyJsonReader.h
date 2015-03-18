#ifndef CTAXONOMYJSONREADER_H
#define CTAXONOMYJSONREADER_H

#include <common.h>

#include <StaticTree.h>

#include <rapidjson/document.h>

#include <boost/unordered_map.hpp>

class CTaxonomyJsonReader {
public:
	struct CNodeInfo{
		// String id of the node
		std::string ID;
		// Weight of the node
		DWORD Weight;

		CNodeInfo() : Weight( 0 )
			{}

	};
	typedef CStaticTree<CNodeInfo> CTree;
	typedef boost::unordered_map<std::string, DWORD> CNameToIndexMap;
public:
	CTaxonomyJsonReader( CTree& _tree, CNameToIndexMap& _map ) :
		tree( _tree), map( _map ), weight(0) {}

	void ReadTree( const std::string& pathToTree );

private:
	typedef boost::unordered_multimap<DWORD, DWORD> CJSONChildren;
private:
	CTree& tree;
	CNameToIndexMap& map;
	// current weight of the node
	DWORD weight;

	void addNode( CTree::TNodeIndex parentID, DWORD node, const rapidjson::Value& nodes, const CJSONChildren& children );
};

////////////////////////////////////////////////////////////////////

#endif // CTAXONOMYJSONREADER_H

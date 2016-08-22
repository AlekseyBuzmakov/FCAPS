// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/PS-Modules/details/TaxonomyJsonReader.h>

#include <JSONTools.h>

////////////////////////////////////////////////////////////////////

using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////

void CTaxonomyJsonReader::ReadTree( const string& pathToTree )
{
	static char errorPlace[]="CTaxonomyJsonReader::ReadTree";
	// Clear Tree
	tree.Clear();
	map.clear();

	CJsonError errorText;
	rapidjson::Document treeJson;
	CJsonFile file(pathToTree);
	if( !file.Read(  treeJson, errorText ) ) {
		throw new CJsonException( errorPlace, errorText );
	}
	if( !( treeJson.HasMember( "Root" ) && treeJson["Root"].IsUint() ) ) {
		throw new CTextException( errorPlace, "No 'Root' found in <<\n" + pathToTree + "\n>>");
	}
	const DWORD root = treeJson["Root"].GetUint();
	if(!( treeJson.IsObject() && treeJson.HasMember( "Nodes" ) && treeJson["Nodes"].IsArray() ) ) {
		throw new CTextException( errorPlace, "No 'Nodes' found in <<\n" + pathToTree + "\n>>");
	}
	const rapidjson::Value& nodes = treeJson["Nodes"];
	if( root >= nodes.Size() || nodes[root].HasMember( "Parent" ) && !nodes[root]["Parent"].IsNull() ) {
		throw new CTextException( errorPlace, "Root of the tree is incorrect <<\n" + pathToTree + "\n>>");
	}
	CJSONChildren children;
	for( DWORD i = 0; i < nodes.Size(); ++i ) {
		if( nodes[i].HasMember( "Parent" ) && !nodes[i]["Parent"].IsNull() ) {
			children.emplace( nodes[i]["Parent"].GetUint(), i );
		}
	}

	addNode( -1, root, nodes, children );
}

void CTaxonomyJsonReader::addNode( CTree::TNodeIndex parentID, DWORD node, const rapidjson::Value& nodes, const CJSONChildren& children )
{
	const CTree::TNodeIndex newNodeIndex = tree.AddNode( parentID );

	tree.GetNode( newNodeIndex ).Data.ID = nodes[node]["ID"].GetString();
	DWORD localWeight = 0;
	if( nodes[node].HasMember( "Weight" ) && nodes[node]["Weight"].IsUint() ) {
		localWeight += nodes[node]["Weight"].GetUint();
	}
	weight += localWeight;
	tree.GetNode( newNodeIndex ).Data.Weight = weight;

	const string& nodeID = tree.GetNode( newNodeIndex ).Data.ID;
	map.emplace( nodeID, newNodeIndex );

	CStdIterator<CJSONChildren::const_iterator> itr( children.equal_range( node ) );
	for( ; !itr.IsEnd(); ++itr ) {
		//assert( newNodeIndex == (*itr).first );
		addNode( newNodeIndex, (*itr).second, nodes, children );
	}
	weight -= localWeight;
}

////////////////////////////////////////////////////////////////////

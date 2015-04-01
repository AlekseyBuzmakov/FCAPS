#include <common.h>

#include <fcaps/modules/details/FCAUtils.h>
#include <fcaps/modules/details/Lattice.h>
#include <fcaps/modules/details/Extent.h>
#include <fcaps/storages/IntentStorage.h>
#include <fcaps/tools/FindConceptOrder.h>

#include <JSONTools.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <vector>
#include <iostream>
#include <math.h>

using namespace std;
using namespace boost;
using namespace rapidjson;

////////////////////////////////////////////////////////////////////

class CInterestingConcepts {
public:
	CInterestingConcepts(
		const vector<TLatticeNodeId>& _nodes,
		const CLattice& _lattice,
		const IIntentStorage& _cmp,
		const IExtentStorage& _extCmp
	) : nodes(_nodes),lattice(_lattice),cmp(_cmp),extCmp(_extCmp) {}
	// Index of a concept
	typedef DWORD TConcept;
	// Number of concepts
	DWORD Size() const
		{ return nodes.size(); }
//	// Access to the concept
//	TConcept operator[]( int DWORD ) const;
	// Topological sort of concepts Compare(c1,c2) == CR_MoreGeneral => operator<(c1,c2) = true
	bool IsTopologicallyLess( DWORD c1, DWORD c2 ) const
	{
		 return extCmp.Size(lattice.GetNode(nodes[c1]).Data.Extent)
			> extCmp.Size(lattice.GetNode(nodes[c2]).Data.Extent);
	}
	// Comparison of two concepts
	bool IsLess( DWORD c1, DWORD c2 ) const
	{
		return cmp.Compare(
			lattice.GetNode(nodes[c1]).Data.Intent,
			lattice.GetNode(nodes[c2]).Data.Intent,
			CR_MoreGeneral
		) != CR_Incomparable;
	}

private:
	const vector<TLatticeNodeId>& nodes;
	const CLattice& lattice;
	const IIntentStorage& cmp;
	const IExtentStorage& extCmp;
};

////////////////////////////////////////////////////////////////////

void CLatticeWriter::Write( const std::string& latticePath, const std::string& stabLatticePath )
{
	assert( cmp != 0 );
	assert( extStorage != 0 );

	// Computing other concept indices.
	computeConceptData();

	// Computing stability if needed.
	if( params.OutStability ) {
		computeStabilityData();
	}

	outLattice( latticePath );

	vector<TLatticeNodeId> interestingNodes;
	findInterestingConcepts( interestingNodes );

	outInterestingConcepts( interestingNodes, stabLatticePath );
}

// Compute concept data for all concepts
void CLatticeWriter::computeConceptData()
{
	cData.clear();
	cData.resize( lattice.GetNodes().size() );

	fillChildren();

	// fill log stability range
	const CLatticeNodes& nodes = lattice.GetNodes();
	for( TLatticeNodeId nodeId = 0; nodeId < nodes.size(); ++nodeId ) {
		fillConceptStabilityLogRange( nodeId );
	}
}
// Computes children of concepts.
void CLatticeWriter::fillChildren()
{
	arcsCount = 0;
	const CLatticeNodes& nodes = lattice.GetNodes();
	for( TLatticeNodeId nodeId = 0; nodeId < nodes.size(); ++nodeId ) {
		const CLatticeNode& node = nodes[nodeId];
		CStdIterator<CEdges::const_iterator> parent( node.Parents );
		for( ; !parent.IsEnd(); ++parent ) {
			TLatticeNodeId parentNodeId = *parent;
			assert( 0 <= parentNodeId && parentNodeId < cData.size() );
			cData[parentNodeId].Children.push_back( nodeId );
			++arcsCount;
		}
	}
}

// estimates stability for a given concept
void CLatticeWriter::fillConceptStabilityLogRange( TLatticeNodeId concept )
{
	assert( 0 <= concept && concept < lattice.GetNodes().size() );
	assert( cData.size() == lattice.GetNodes().size() );

	DWORD& maxStab = cData[concept].MaxStab;
	double& minStab = cData[concept].MinStab;

	const int extSize = extStorage->Size( lattice.GetNode(concept).Data.Extent );
	maxStab = extSize;
	vector<int> diffs;
	CStdIterator<vector<TLatticeNodeId>::const_iterator> child( cData[concept].Children );
	for( ; !child.IsEnd(); ++child ) {
		const int childSize = extStorage->Size( lattice.GetNode(*child).Data.Extent );
		maxStab = std::min( (int)maxStab, extSize - childSize );
		diffs.push_back( extSize - childSize );
	}

	minStab = 0;
	for( int i = 0; i < diffs.size(); ++i ) {
		minStab += pow((double)2.0, (int)(maxStab - diffs[i]) );
	}
	minStab = maxStab - log( minStab ) / log( 2 );
}

// computes stability for all concepts
void CLatticeWriter::computeStabilityData()
{
	std::cout << "Stability Calculation BEGIN\n";
	sData.resize( lattice.Size() );
	std::cout << ">>StabData size done.\n";
	vector<TLatticeNodeId> indecis;
	indecis.resize( sData.size() );
	std::cout << ">>Indices size done.\n";
	vector<bool> flags;
	flags.resize( sData.size(), false );
	std::cout << ">>Flags size done.\n";

	CStdIterator<CLatticeNodes::const_iterator> node( lattice.GetNodes() );
	TLatticeNodeId nodeId = 0;
	for( ; !node.IsEnd(); ++node, ++nodeId ) {
		indecis[nodeId] = nodeId;
		sData[nodeId].ExtentSize = extStorage->Size( (*node).Data.Extent );
	}

	CStabilityIndecisCmp cmp( sData );
	std::sort( indecis.begin(), indecis.end(), cmp );

	int lastNodeExtentSize = 0;
	for( size_t i = 0; i < indecis.size(); ++i ) {
		const TLatticeNodeId conceptNum = indecis[i];
		const CStabilityData& currStabilityData = sData[conceptNum];
		assert( lastNodeExtentSize <= currStabilityData.ExtentSize );
		lastNodeExtentSize = currStabilityData.ExtentSize;

		// Correct all parents
		assert( !flags[conceptNum] );
		CStabilityContext context( currStabilityData, flags );
		correctParents( indecis[i], context );
		unmarkFlags( indecis[i], context );

		std::cout << "\r>>Processed: " << i+1 << "(" << indecis.size() << ")";
	}
	std::cout << "\nStability Calculation END                                      \n";
}

void CLatticeWriter::unmarkFlags( TLatticeNodeId node, CStabilityContext& context )
{
	CStdIterator<CEdges::const_iterator> itr( lattice.GetNode( node ).Parents );
	for( ; !itr.IsEnd(); ++itr ) {
		const TLatticeNodeId nodeId = *itr;
		if( !context.Flags[nodeId] ) {
			continue;
		}
		context.Flags[nodeId] = false;
		unmarkFlags( *itr, context );
	}
}

void CLatticeWriter::correctParents( TLatticeNodeId node, CStabilityContext& context )
{
	CStdIterator<CEdges::const_iterator> itr( lattice.GetNode( node ).Parents );
	for( ; !itr.IsEnd(); ++itr ) {
		const TLatticeNodeId nodeId = *itr;
		if( context.Flags[nodeId] ) {
			continue;
		}
		context.Flags[nodeId] = true;
		CStabilityData& parentData = sData[nodeId];
		parentData.Stability -= context.Node.Stability * pow(
			(long double)2.0, context.Node.ExtentSize - parentData.ExtentSize );
		correctParents( *itr, context );
	}
}

// Out full lattice in the file.
void CLatticeWriter::outLattice( const std::string& latticeOutPath )
{
	CDestStream dst( latticeOutPath );
	dst << "[";

	OStreamWrapper os( dst );
	Writer<OStreamWrapper> writer( os );

	const CLatticeNodes& nodes = lattice.GetNodes();

	Document latParams;
	MemoryPoolAllocator<>& alloc = latParams.GetAllocator();

	latParams.SetObject()
		.AddMember( "NodesCount", Value().SetUint( nodes.size() ), alloc )
		.AddMember( "ArcsCount", Value().SetUint( arcsCount ), alloc )
		.AddMember( "Bottom", Value().SetArray()
			.PushBack( Value().SetUint( 0 ), alloc ), alloc )
		.AddMember( "Top", Value().SetArray(), alloc );
	Value& topJson = latParams["Top"];
	for( TLatticeNodeId i = 0; i < nodes.size(); ++i ) {
		if( nodes[i].Parents.empty() ) {
			topJson.PushBack( Value().SetUint( i), alloc );
		}
	}
	latParams.Accept( writer );
	writer.Reset(os);

	dst << ",{ \"Nodes\":[\n";

	TLatticeNodeId nodeId = 0;
	for( ; nodeId < nodes.size(); ++nodeId ) {
		Document conceptJson;
		conceptJson.SetObject();
		fillConceptJson( nodeId, conceptJson.GetAllocator(), conceptJson );

		conceptJson.Accept( writer );
		writer.Reset(os);

		if( nodeId < nodes.size() -1 ) {
			dst << ",";
		}
		dst << "\n";
	}

	dst << "]}";

	if( params.OutOrder ) {
		dst << ",{ \"Arcs\":[\n";
		nodeId = 0;
		bool isFirst = true;
		for( ; nodeId < nodes.size(); ++nodeId ) {
			CStdIterator<std::vector<TLatticeNodeId>::const_iterator> ch( cData[nodeId].Children );
			for( ; !ch.IsEnd(); ) {
				if( isFirst ) {
					isFirst = false;
				} else {
					dst << ",\n";
				}
				dst << "{\"S\":" << nodeId << ",\"D\":" << *ch << "}";
				++ch;
				if( ch.IsEnd() ) {
					break;
				}
			}
		}
		dst << "]}";
	}

	dst << "]";
}

// Add info about the concept to the json object
void CLatticeWriter::fillConceptJson( TLatticeNodeId nodeId, MemoryPoolAllocator<>& alloc, Value& result )
{
	CJsonError errorText;
	const CLatticeNode& node = lattice.GetNode( nodeId );
	if( params.OutExtent ) {
		Document extent( &alloc );
		const bool rslt = ReadJsonString( extStorage->Save( node.Data.Extent ), extent, errorText );
		assert( rslt );
		result.AddMember( "Ext", extent.Move(), alloc );
	}
	if( params.OutSupport ) {
		result.AddMember( "Supp", extStorage->Size( node.Data.Extent ), alloc );
	}
	if( params.OutStabEstimation ) {
		assert( cData.size() == lattice.GetNodes().size() );
		if( isfinite( cData[nodeId].MinStab ) && isfinite( cData[nodeId].MaxStab ) ) {
			if( params.IsStabilityInLog ) {
				result
					.AddMember( "LStab", Value().SetDouble( cData[nodeId].MinStab ), alloc )
					.AddMember( "UStab", Value().SetUint( cData[nodeId].MaxStab ), alloc );
			} else {
				result
					.AddMember( "LStab", Value().SetDouble( 1 - pow(2.0l, -(int)cData[nodeId].MinStab) ), alloc )
					.AddMember( "UStab", Value().SetDouble( 1 - pow(2.0l, (int)cData[nodeId].MaxStab ) ), alloc );
			}
		}
	}
	if( params.OutStability ) {
		assert( sData.size() == lattice.GetNodes().size() );
		if( params.IsStabilityInLog ) {
			const double stab = -log( 1 - sData[nodeId].Stability ) / log( 2 );
			if( isfinite( stab ) ) {
				result.AddMember( "Stab", Value().SetDouble( stab ), alloc );
			} else {
				result.AddMember( "Stab", Value().SetString( StringRef( "inf" ) ), alloc );
			}
		} else {
			result.AddMember( "Stab", Value().SetDouble( sData[nodeId].Stability ), alloc );
		}
	}

	Document intent( &alloc );
	const bool rslt = ReadJsonString( cmp->SavePattern( node.Data.Intent ), intent, errorText );
	assert( rslt );
	result.AddMember( "Int", intent.Move(), alloc );
}

// Finds nodes respecting creteria
void CLatticeWriter::findInterestingConcepts( vector<TLatticeNodeId>& nodes )
{
	nodes.clear();
	for( TLatticeNodeId i = 0; i < lattice.Size(); ++i ) {
		if( isNodeInteresting( i ) ) {
			nodes.push_back(i);
		}
	}
}

// Checks if the node is interesting
bool CLatticeWriter::isNodeInteresting( TLatticeNodeId node )
{
	assert( cData.size() > node );
	assert( !params.OutStability || sData.size() > node );

	if( extStorage->Size( lattice.GetNode( node ).Data.Extent ) < params.MinExtentSize ) {
		return false;
	}
	if( cData[node].MaxStab < params.MinLift ) {
		return false;
	}

	if( params.OutStability ) {
		const double stab = -log( 1 - sData[node].Stability ) / log( 2 );
		if (params.IsStabilityInLog
				? isfinite(stab) && stab < params.MinStab: sData[node].Stability < params.MinStab)
		{
			return false;
		}
	}
	return true;
}

// Prints only given set of concepts
void CLatticeWriter::outInterestingConcepts(
	const std::vector<TLatticeNodeId>& nodes, const string& path )
{
	CInterestingConcepts concepts( nodes, lattice, *cmp, *extStorage );
	CFindConceptOrder<CInterestingConcepts> order( concepts );
	if( params.OutOrder ) {
		std::cout << "Interesting concepts order BEGIN\n";
		order.Compute();
		std::cout << "Interesting concepts order END\n";
	}

	CDestStream dst( path );
	dst << "[";

	OStreamWrapper os( dst );
	Writer<OStreamWrapper> writer( os );

	Document latParams;
	MemoryPoolAllocator<>& alloc = latParams.GetAllocator();

	latParams.SetObject()
		.AddMember( "NodesCount", Value().SetUint( nodes.size() ), alloc )
		.AddMember( "ArcsCount", Value().SetUint( order.GetArcsCount() ), alloc )
		.AddMember( "Bottom", Value().SetArray(), alloc )
		.AddMember( "Top", Value().SetArray(), alloc );

	Value& bottomJson = latParams["Bottom"];
	CStdIterator<CList<DWORD>::CConstIterator,false> itr( order.GetBottoms() );
	for( ; !itr.IsEnd(); ++itr ) {
		bottomJson.PushBack( Value().SetUint( *itr ), alloc );
	}

	Value& topJson = latParams["Top"];
	itr.Reset( order.GetTops() );
	for( ; !itr.IsEnd(); ++itr ) {
		topJson.PushBack( Value().SetUint( *itr ), alloc );
	}

	latParams.Accept( writer );
	writer.Reset(os);

	dst << ",{ \"Nodes\":[\n";
	for( DWORD node = 0; node < nodes.size(); ++node ) {
		Document conceptJson;
		conceptJson.SetObject();
		fillConceptJson( nodes[node], conceptJson.GetAllocator(), conceptJson );
		conceptJson.Accept( writer );
		writer.Reset(os);
		if( node < nodes.size() -1 ) {
			dst << ",";
		}
		dst << "\n";
	}
	dst << "]}";

	if( params.OutOrder ) {
		dst << ",{ \"Arcs\":[\n";
		bool isFirst = true;
		for( DWORD node = 0; node < nodes.size(); ++node ) {
			CStdIterator<CList<DWORD>::CConstIterator, false> p( order.GetParents( node ) );
			for( ; !p.IsEnd(); ++p ) {
				if( isFirst ) {
					isFirst = false;
				} else {
					dst << ",\n";
				}
				dst << "{\"S\":" << *p << ",\"D\":" << node << "}";
			}
		}
		dst << "]}";
	}

	dst << "]";
}

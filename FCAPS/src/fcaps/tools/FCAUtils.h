// Author: Aleksey Buzmakov
// Different small FCA utils

#include <common.h>
#include <fcaps/premodules/Lattice.h>

#include <boost/unordered_map.hpp>
#include <rapidjson/document.h>

#include <vector>

// Params to filter out a lattice
struct CLatticeFilterParams {
	// The minimal interesting size of extent
	DWORD MinExtentSize;
	// The minimal interesting
	DWORD MinLift;
	// Minimal Stability to output
	double MinStab;

	// Should output full extent
	bool OutExtent;
	// Should support be outed
	bool OutSupport;
	// Out order of concepts
	bool OutOrder;
	// Out estimation for stability
	bool OutStabEstimation;
	// Compute and out stability
	bool OutStability;
	// Use log stability
	bool IsStabilityInLog;

	// Base number to count percentage for the extent,
	//  if 0 no percentage counting
	DWORD PercentageBase;

	CLatticeFilterParams() :
		MinExtentSize( 0 ), MinLift( 1 ), MinStab( 0.95 ),
		OutExtent( false ), OutSupport( true ), OutOrder( true ),
		OutStabEstimation( true ), OutStability( false ), IsStabilityInLog( true ),
		PercentageBase( 0 ) {}
};

struct CLattice;
interface IIntentStorage;
interface IExtentStorage;

// Out full lattice and filtered lattice.
// Returns the number of "interesting" concepts.
DWORD OutputFilteredLattice( const CLattice& lattice,
	const CSharedPtr<IIntentStorage>& cmp,
	const CSharedPtr<IExtentStorage>& extManager,
	const CLatticeFilterParams& params,
	const std::string& latticeOutPath,  const std::string& filteredLatticeOutPath );

////////////////////////////////////////////////////////////////////

class CLatticeWriter {
public:
	CLatticeWriter(
			const CLattice& _lattice,
			const CSharedPtr<IIntentStorage>& _cmp,
			const CSharedPtr<IExtentStorage>& _extStorage,
			const CLatticeFilterParams& _params ) :
		lattice( _lattice ),
		cmp( _cmp ),
		extStorage( _extStorage ),
		params( _params ),
		arcsCount( 0 ) {}

	void Write( const std::string& latticePath, const std::string& stabLatticePath );

private:
	// All additional data for a concept.
	struct CConceptData {
		DWORD MaxStab;
		double MinStab;
		std::vector<TLatticeNodeId> Children;

		CConceptData() :
			MaxStab(-1), MinStab(0) {}
	};
	// Data about stability.
	struct CStabilityData {
		// Stability measure
		long double Stability;
		// Extent size of the correponding concept
		int ExtentSize;

		CStabilityData() :
			Stability( 1 ), ExtentSize( 0 ) {}
	};
	// Comparator of stability
	class CStabilityIndecisCmp{
	public:
		CStabilityIndecisCmp( const std::vector<CStabilityData>& _stabDataArray ):
			stabDataArray( _stabDataArray ) {}

		bool operator() ( TLatticeNodeId i, TLatticeNodeId j ) const
		{
			assert( i <= stabDataArray.size() && j <= stabDataArray.size() );
			return stabDataArray[i].ExtentSize < stabDataArray[j].ExtentSize;
		}

	private:
		const std::vector<CStabilityData>& stabDataArray;
	};
	struct CStabilityContext {
		const CStabilityData& Node;
		std::vector<bool>& Flags;

		CStabilityContext( const CStabilityData& node, std::vector<bool>& flags ) :
			Node( node ), Flags( flags ) {}
	};

private:
	const CLattice& lattice;
	const CSharedPtr<IIntentStorage> cmp;
	const CSharedPtr<IExtentStorage> extStorage;
	const CLatticeFilterParams& params;

	std::vector<CStabilityData> sData;
	std::vector<CConceptData> cData;
	DWORD arcsCount;

	void computeConceptData();
	void fillChildren();
	void fillConceptStabilityLogRange( TLatticeNodeId concept );
	void computeStabilityData();
	void unmarkFlags( TLatticeNodeId node, CStabilityContext& context );
	void correctParents( TLatticeNodeId node, CStabilityContext& context );
	void outLattice( const std::string& latticeOutPath );
	void fillConceptJson( TLatticeNodeId node, rapidjson::MemoryPoolAllocator<>& alloc, rapidjson::Value& result );

	void findInterestingConcepts( std::vector<TLatticeNodeId>& nodes );
	bool isNodeInteresting( TLatticeNodeId node );
	void outInterestingConcepts(
		const std::vector<TLatticeNodeId>& nodes, const std::string& path );
};

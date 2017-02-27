// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/SharedModulesLib/StabilityMonteCarloApproximation.h>

#include <fcaps/SharedModulesLib/VectorBinarySetDescriptor.h>

#include <boost/random/mersenne_twister.hpp>

#include <vector>

using namespace std;
using namespace boost;

CStabilityMonteCarloApproximation::CStabilityMonteCarloApproximation(
		CVectorBinarySetJoinComparator& _cmp,
		const CVectorBinarySetDescriptor& _extent,
		const CVectorBinarySetDescriptor& _intent,
		const CBinarySetCollection& _objToDescrMap,
		bool _isLog ) :
	cmp( _cmp ),
	cmpDeleter( _cmp ),
	extent( _extent ),
	intent( _intent ),
	objToDescrMap( _objToDescrMap ),
	threshold( 0 ),
	e(0.01),
	d(0.01),
	trialsNum( CalculateTrialsCount( e, d ) ),
	isLog( _isLog ),
	leftLimit( 0.0 ),
	rightLimit( 1.0 )
{
	//ctor
}

DWORD CStabilityMonteCarloApproximation::CalculateTrialsCount(
	const double& e, const double& d )
{
	return static_cast<DWORD>( ceil( log( 2.0 / d ) / 2 / (e*e) ) );
}

void CStabilityMonteCarloApproximation::SetStableThreshold( double value )
{
	if( isLog ) {
		threshold = 1 - pow( 2.0, -std::max( 0.0, value ) );
	} else {
		threshold = std::min( 1.0, std::max( 0.0, value ) );
	}
}

void CStabilityMonteCarloApproximation::SetPrecision( const double& epsilon, const double& delta )
{
	e = std::max( 0.0, std::min( 1.0, epsilon ) );
	d = std::max( 0.0, std::min( 1.0, delta ) );
	trialsNum = CalculateTrialsCount( e, d );
}

void CStabilityMonteCarloApproximation::Compute()
{
	leftLimit = 0.0;
	rightLimit = 0.0;

	boost::random::mt19937 rnd( 1234 );

	const char rndDigets = 31;
	const char residue = extent.Size() % rndDigets;
	const DWORD residueMask = (1 << residue) - 1;

	vector<DWORD> bitMembership;
	bitMembership.resize( extent.Size() / rndDigets + 1, 0 );

	CList<DWORD> attrs;
	cmp.EnumValues( extent, attrs );

	// The number of iterations when the descriptions of a subset was equal to intent.
	DWORD successNum = 0;
	for( DWORD i = 0; i < trialsNum; ++i ) {
		// Generate random subset
		for( DWORD j = 0; j < bitMembership.size() - 1; ++j ) {
			bitMembership[j] = rnd();
		}
		bitMembership.back() = (rnd() & residueMask);

		// Intersect description of all objects in the subset
		CSharedPtr<const CVectorBinarySetDescriptor> result;
		CStdIterator<CList<DWORD>::CConstIterator, false> obj( attrs );
		for( DWORD objIndex = 0; !obj.IsEnd(); ++obj, ++objIndex ) {
			const DWORD index = objIndex / rndDigets;
			assert( index < bitMembership.size() );
			const char objResidue = objIndex % rndDigets;

			if( (bitMembership[index] & (1 << objResidue)) == 0 ) {
				// Object excluded from in this trial
				continue;
			}

			assert( *obj < objToDescrMap.size() );
			if( result == 0 ) {
				result.reset( new CVectorBinarySetDescriptor( *objToDescrMap[*obj] ), cmpDeleter );
			} else {
				result.reset( cmp.CalculateSimilarity( result.get(), objToDescrMap[*obj].get() ), cmpDeleter );
			}
		}

		// Check if it is equal to concept intent
		const TCompareResult rslt = result.get() == 0 ? CR_Incomparable : cmp.Compare(
			result.get(), &intent, CR_Equal, CR_AllResults | CR_Incomparable );
		if( rslt == CR_Equal ) {
			++successNum;
		}
		if( i < 100 ) {
			// In order to be in the range of big numbers
			continue;
		}
		const double currMean = 1.0 * successNum / i;
		const double currEpsilon = sqrt( log( 2.0 / d ) / 2 / i );
		if( currMean + currEpsilon < threshold ) {
			// Likely to be unstable.
			leftLimit = std::max( 0.0, currMean - currEpsilon );
			rightLimit = std::min( 1.0, currMean + currEpsilon );
			if( isLog ) {
				leftLimit = -log2(1-leftLimit);
				rightLimit = rightLimit >= 1.0 ? -1 : -log2( 1 - rightLimit );
			}
			return;
		}
	}

	const double currMean = 1.0 * successNum / trialsNum;
	leftLimit = std::max( 0.0, currMean - e );
	rightLimit = std::min( 1.0, currMean + e );
	if( isLog ) {
		leftLimit = -log2(1-leftLimit);
		rightLimit = rightLimit >= 1.0 ? -1 : -log2( 1 - rightLimit );
	}
}


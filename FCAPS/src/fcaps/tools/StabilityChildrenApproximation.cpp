#include <fcaps/tools/StabilityChildrenApproximation.h>

#include <fcaps/modules/VectorBinarySetDescriptor.h>

#include <cmath>

using namespace std;

CStabilityChildrenApproximation::CStabilityChildrenApproximation(
		CVectorBinarySetJoinComparator& _cmp,
		bool _isLog,
		bool _isDebug ) :
	cmp( _cmp ),
	deleter( cmp ),
	attrToTidsetMap( 0 ),
	attrOrder( 0 ),
	threshold( 0 ),
	extentSize( -1 ),
	minDiff( 0 ),
	minDiffAttr(NotFound),
	attrNumLimit( -1 ),
	leftLimit( 0 ),
	rightLimit( 1 ),
	isUnstable( false ),
	isLog( _isLog ),
	isDebug( _isDebug )
{
}

CStabilityChildrenApproximation::~CStabilityChildrenApproximation()
{
	clear();
}

void CStabilityChildrenApproximation::SetStableThreshold( double value )
{
	if( isLog ) {
		threshold = value;
	} else {
		threshold = std::min( 1.0, std::max( 0.0, value ) );
	}
}

void CStabilityChildrenApproximation::InitComputation(
	const CVectorBinarySetDescriptor& extent, const CList<DWORD>& intent )
{
	clear();

	if( isDebug ) {
		bool breakFunction = false;
		CSharedPtr<const CVectorBinarySetDescriptor> rslt( computeExtent( intent ) );
		if( rslt == 0 ) {
			std::cout << "\nWarning: Empty intent \n";
		} else {
			const TCompareResult extCmpRslt = cmp.Compare( *rslt, extent, CR_AllResults, CR_AllResults | CR_Incomparable );

			switch( extCmpRslt) {
				case CR_LessGeneral:
					std::cout << "\nNot-closed extent";
					breakFunction = true;
					break;
				case CR_Incomparable:
				case CR_MoreGeneral:
					std::cout << "\nNon-coherent extent";
					breakFunction = true;
					break;
				default:
					break;
			}
			if( !checkIntent( *rslt, intent ) ) {
				std::cout << "\nNot-closed intent";
				breakFunction = true;
			}
			if( breakFunction ) {
				std::cout << "\n";
				return;
			}
		}
	}

	extentSize = extent.Size();
	DWORD diffThld = 0;
	if( isLog ) {
		// In the logariphimc scale threshold is equal to diffThld
		diffThld = threshold;
	} else if( threshold >= 0.5 ) {
		// Can be computed directly
		diffThld = floor( -log2(1.0 - threshold) );
	} else {
		// Too low threshold
		diffThld = 0;
	}
	minDiff = extent.Size();
	minDiffAttr = NotFound;
	CStdIterator<CList<DWORD>::CConstIterator, false> intAttr( intent );
	DWORD currAttr = intAttr.IsEnd() ? -1 : *intAttr;
	size_t i = 0;
	for( ; i < getAttrNum(); ++i ) {
		while( currAttr < i ) {
			++intAttr;
			currAttr = intAttr.IsEnd() ? -1 : *intAttr;
		}
		if( currAttr == i ) {
			// The attribute is in the intent, so should not be considered
			continue;
		}
		CSharedPtr<const CVectorBinarySetDescriptor> meetHolder( computeIntersection( i, extent ), deleter );
		assert( meetHolder.get() != 0 );
		const DWORD newDiff = extentSize - meetHolder->Size();
		if( newDiff == 0 ) {
			// Not closed intent but we can leav with it
			continue;
		}
		children.push_back( meetHolder );
		if( minDiff > newDiff ) {
			minDiff = newDiff;
			minDiffAttr = i;
			if( minDiff < diffThld ) {
				isUnstable = true;
				break;
			}
		}
	}
	if( isLog ) {
		rightLimit = minDiff;
	} else {
		rightLimit = minDiff < 32 ? 1.0 - 1.0 / ( 1 << minDiff ) : 1.0;
		rightLimit = min( 1.0, rightLimit );
	}
	if( rightLimit < threshold ) {
		// Stability is too low, should be ignored
		return;
	}
	assert( i >= getAttrNum() ); // processed up to the end
}

void CStabilityChildrenApproximation::ComputeLowerBound()
{
	if( isUnstable ) {
		leftLimit = 0;
		return;
	}
	CList<DWORD> childDiffs;
	CStdIterator<deque<CSharedPtr<const CVectorBinarySetDescriptor> >::iterator> itr( children );
	for( ; !itr.IsEnd(); ++itr ) {
		if( *itr == 0 ) {
			continue;
		}

		CStdIterator<deque<CSharedPtr<const CVectorBinarySetDescriptor> >::iterator> itr2 = itr;
		for( ++itr2; !itr2.IsEnd(); ++itr2 ) {
			if( *itr2 == 0 ) {
				continue;
			}
			const TCompareResult rslt = cmp.Compare(
				**itr, **itr2, CR_AllResults, CR_AllResults | CR_Incomparable );
			switch( rslt ) {
				case CR_Incomparable:
					break;
				case CR_Equal:
				case CR_LessGeneral:
					// For a descriptor less general means less objects or more attributes,
					//  i.e. itr is a superset of itr2.
					itr2->reset();
					break;
				case CR_MoreGeneral:
					// remove itr1;
					itr->reset();
					break;
				default:
					assert( false );
					break;
			}
			if( *itr == 0 ) {
				break;
			}
		}
		if( *itr == 0 ) {
			continue;
		}
		// Should add this children to approximation
		const DWORD diff = extentSize - (*itr)->Size();
		childDiffs.PushBack( diff );
	}
	leftLimit = 0.0;
	if( isLog ) {
		CStdIterator<CList<DWORD>::CConstIterator,false> itr( childDiffs );
		// Lets take the biggest member out of brackets: 2^-mindiff * (1+2^-(x-mindiff))
		double coef = 0;
		for( ; !itr.IsEnd(); ++itr ) {
			coef += pow( 2.0l, -1.0l * (*itr - minDiff) );
		}
		assert( coef >= 0.99999999 || minDiff == -1 );
		leftLimit = minDiff - log2( coef );
	} else {
		leftLimit = 1.0;
		CStdIterator<CList<DWORD>::CConstIterator,false> itr( childDiffs );
		for( ; !itr.IsEnd(); ++itr ) {
			leftLimit -= pow( 2.0l, -1.0l * *itr );
		}
	}
	leftLimit = max( 0.0, leftLimit );
	assert( leftLimit - rightLimit < 0.00000001 || minDiff == -1 );
}

void CStabilityChildrenApproximation::ComputeUpperBound()
{
	// Everything is done in InitComputation
}

void CStabilityChildrenApproximation::clear()
{
	leftLimit = 0.0;
	rightLimit = 1.0;
	isUnstable = false;
	extentSize = -1;
	minDiff = -1;

	children.clear();
}

inline DWORD CStabilityChildrenApproximation::getAttrCount() const
{
	assert( attrToTidsetMap != 0 );
	assert( attrOrder == 0 || attrOrder->size() == attrToTidsetMap->size() );
	return attrToTidsetMap->size();
}
inline const CVectorBinarySetDescriptor& CStabilityChildrenApproximation::getAttrByNum( DWORD num ) const
{
	assert( attrToTidsetMap != 0 );
	if( attrOrder == 0 ) {
		return *(*attrToTidsetMap)[num];
	} else {
		assert( attrOrder->size() == attrToTidsetMap->size() );
		assert( num < attrOrder->size() );
		assert( (*attrOrder)[num] < attrToTidsetMap->size() );
		return *(*attrToTidsetMap)[(*attrOrder)[num]];
	}
}

inline const CVectorBinarySetDescriptor* CStabilityChildrenApproximation::computeIntersection(
	DWORD colNum, const CVectorBinarySetDescriptor& extent ) const
{
	return cmp.CalculateSimilarity( getAttrByNum(colNum), extent );
}


CSharedPtr<const CVectorBinarySetDescriptor> CStabilityChildrenApproximation::computeExtent(
	const CList<DWORD>& intent ) const
{
	CSharedPtr<const CVectorBinarySetDescriptor> rslt;
	CStdIterator<CList<DWORD>::CConstIterator, false> intAttr( intent );
	for( ; !intAttr.IsEnd(); ++intAttr ) {
		assert( 0 <= *intAttr && *intAttr < getAttrCount() );
		if( rslt.get() == 0 ) {
			rslt.reset(
				cmp.CalculateSimilarity( getAttrByNum(*intAttr), getAttrByNum(*intAttr) ),
				deleter );
			continue;
		}
		rslt.reset( cmp.CalculateSimilarity( *rslt, getAttrByNum(*intAttr) ), deleter );
	}
	return rslt;
}

bool CStabilityChildrenApproximation::checkIntent(
	const CVectorBinarySetDescriptor& extent, const CList<DWORD>& intent ) const
{
	CStdIterator<CList<DWORD>::CConstIterator, false> intAttr( intent );
	DWORD currAttr = intAttr.IsEnd() ? -1 : *intAttr;
	for( DWORD i = 0; i < getAttrCount(); ++i ) {
		while( currAttr < i ) {
			++intAttr;
			currAttr = intAttr.IsEnd() ? -1 : *intAttr;
		}
		if( currAttr == i ) {
			// The attribute is in the intent, so should not be considered
			continue;
		}

		const TCompareResult intCmpRslt = cmp.Compare( extent, getAttrByNum(i),
				CR_Equal | CR_MoreGeneral, CR_AllResults | CR_Incomparable );
		if( intCmpRslt != CR_Incomparable ) {
			return false;
		}
	}
	return true;
}

DWORD CStabilityChildrenApproximation::getAttrNum() const
{
	return std::min<DWORD>( attrNumLimit, getAttrCount() );
}

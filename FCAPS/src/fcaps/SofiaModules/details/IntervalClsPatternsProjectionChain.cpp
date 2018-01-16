// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/SofiaModules/details/IntervalClsPatternsProjectionChain.h>

#include <Exception.h>

using namespace std;

////////////////////////////////////////////////////////////////////

CIntervalClsPatternsProjectionChain::CIntervalClsPatternsProjectionChain() :
	extCmp(new CVectorBinarySetJoinComparator),
	extDeleter(extCmp),
	thld( 0 ),
	precision( 1e-8 ),
	propPrecisionThld(-1)
{

}

const std::vector<std::string>& CIntervalClsPatternsProjectionChain::GetObjNames() const
{
	return extCmp->GetNames();
}
void CIntervalClsPatternsProjectionChain::SetObjNames( const std::vector<std::string>& names )
{
	extCmp->SetNames( names );
}

void CIntervalClsPatternsProjectionChain::UpdateInterestThreshold( const double& t )
{
	thld = t;
}

void CIntervalClsPatternsProjectionChain::AddObject( DWORD objectNum, const JSON& intent )
{
	if( tmpContext.size() <= objectNum ) {
		tmpContext.resize( objectNum + 1 );
	}
	JsonIntervalPattern::LoadPattern( intent, tmpContext[objectNum] );
	if( values.size() == 0 ) {
		values.resize( tmpContext[objectNum].size() );
		if( precisions.size() == 0 ) {
			precisions.resize(values.size(), precision);
		}
	} else if( values.size() != tmpContext[objectNum].size() ) {
		throw new CTextException("CIntervalClsPatternsProjectionChain::AddObject","Patterns have different number of intervals");
	}
}

bool CIntervalClsPatternsProjectionChain::AreEqual(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return extCmp->Compare(Pattern(p).Extent(),Pattern(q).Extent(), CR_Equal) != CR_Incomparable;
}
bool CIntervalClsPatternsProjectionChain::IsSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return extCmp->Compare(Pattern(p).Extent(),Pattern(q).Extent(), CR_LessGeneral) != CR_Incomparable;
}
bool CIntervalClsPatternsProjectionChain::IsTopoSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return Pattern(p).Extent().Size() > Pattern(q).Extent().Size();
}
void CIntervalClsPatternsProjectionChain::FreePattern(const IPatternDescriptor* p ) const
{
	delete &Pattern(p);
}


void CIntervalClsPatternsProjectionChain::ComputeZeroProjection( CPatternList& ptrns )
{
	convertContext();
	computeAttrOrder();

	state.State = CCurrState::S_Left;
	state.AttrNum = -1;
	state.ValueNum = 0;

	extCmp->SetMaxAttrNumber(context.size());
	extCmp->SetWriteNames(true);

	// TODO init order of run.

	CSharedPtr<CVectorBinarySetDescriptor> ptrn(extCmp->NewPattern(), extDeleter);
	auto_ptr<CPatternDescription> result(NewPattern( ptrn ));
	// Extent
	for( DWORD i = 0; i < context.size(); ++i ) {
		extCmp->AddValue(i,*ptrn);
	}
	// Intent
	CIntent& intent = result->Intent();
	intent.resize(values.size());
	for( DWORD i = 0; i < values.size(); ++i ) {
		intent[i].first = 0;
		intent[i].second = values[i].size()-1;
	}

	ptrns.PushBack( result.release() );

}
bool CIntervalClsPatternsProjectionChain::NextProjection()
{
	if( state.State == CCurrState::S_End ) {
		return false;
	}
	stateObjs.reset();

	++state.AttrNum;
	if( state.AttrNum < values.size() ) {
		if( state.ValueNum < values[attrOrder[state.AttrNum]].size() - 1 ) {
			return true;
		}
		// Attributes are ordered w.r.t. decreasing size of values. So we go to the next value.
	}
	state.AttrNum = 0;

	if(state.State == CCurrState::S_Left) {
		state.State = CCurrState::S_Right;
		return true;
	}
	state.State = CCurrState::S_Left;

	++state.ValueNum;
	if( state.ValueNum < values[attrOrder[0]].size() - 1 ) {
		return true;
	}
	state.State = CCurrState::S_End;
	return false;
}

double CIntervalClsPatternsProjectionChain::GetProgress() const
{
	const double total = values[attrOrder.front()].size() * ((double)((DWORD)CCurrState::S_End)) * attrOrder.size();
	const double curr =
		state.ValueNum * ((double)((DWORD)CCurrState::S_End)) * attrOrder.size()
		+ ((double)((DWORD)state.State)) * attrOrder.size()
		+ state.AttrNum;
	assert( state.ValueNum < values[attrOrder.front()].size() );
	return curr / total;
}

void CIntervalClsPatternsProjectionChain::Preimages( const IPatternDescriptor* d, CPatternList& ptrns )
{
	const DWORD oldSize = ptrns.Size();
	JustPreimages(Pattern(d),ptrns);
	if( ptrns.Size() - oldSize == 0 ) {
		return;
	}
	assert(ptrns.Size() - oldSize == 1);
	// If res->Intent() is not closed, than it have been created by something before also,
	//  so it will be destroyed by a hashtable above.
  	const IPatternDescriptor* ptrn = ptrns.Back();

	if( GetPatternInterest( ptrn ) < thld ) {
		ptrns.PopBack();
		FreePattern(ptrn);
	}
}

int CIntervalClsPatternsProjectionChain::GetExtentSize( const IPatternDescriptor* d ) const
{
	return Pattern(d).Extent().Size();
}

JSON CIntervalClsPatternsProjectionChain::SaveExtent( const IPatternDescriptor* d ) const
{
	return extCmp->SavePattern( &Pattern(d).Extent() );
}
JSON CIntervalClsPatternsProjectionChain::SaveIntent( const IPatternDescriptor* d ) const
{
	JsonIntervalPattern::CPattern ptrn;
	const CIntent& intent = Pattern(d).Intent();
	ptrn.resize(intent.size());
	for( DWORD i = 0; i < intent.size(); ++i ) {
		const DWORD ind1 = intent[i].first;
		const DWORD ind2 = intent[i].second;
		assert( ind1 <= ind2 );
		assert( ind2 < values[i].size() );

		ptrn[i].first = values[i][ind1];
		ptrn[i].second = values[i][ind2];
	}
	return JsonIntervalPattern::SavePattern( ptrn );
}

void CIntervalClsPatternsProjectionChain::JustPreimages( const CPatternDescription& p, CPatternList& preimages )
{
	switch (state.State) {
	case CCurrState::S_Left:
		leftPreimages( p, preimages );
		break;
	case CCurrState::S_Right:
		rightPreimages( p, preimages );
		break;
	default:
		assert(false);
	}
}

////////////////////////////////////////////////////////////////////
// CIntervalClsPatternsProjectionChain::convertContext stuff
typedef pair< double, pair<DWORD,DWORD> > CAttrValToIntValue;
typedef vector<CAttrValToIntValue> CAttrValToInt;
bool valToIntValueCmp( const CAttrValToIntValue& a, const CAttrValToIntValue& b )
{
	return a.first < b.first;
}
const pair<DWORD,DWORD>& findInterval( const double& val, const CAttrValToInt& valToInt, const double& eps )
{
	for( DWORD i = 0; i < valToInt.size(); ++i ) {
		if( abs(val - valToInt[i].first) > eps ) {
			continue;
		}
		return valToInt[i].second;
	}
	assert(false);
	return valToInt[0].second;
}

// Convert context to the format when it is given by indexes of values rather than by the values.
void CIntervalClsPatternsProjectionChain::convertContext()
{
	// Initializing the new context
	context.resize( tmpContext.size() );
	for( DWORD i = 0; i < context.size(); ++i ) {
		context[i].resize(values.size());
	}

	// Collecting values
	for( DWORD attr = 0; attr < values.size(); ++attr ) {
		// A correspondance for the attribute 'attr' from values to numerical intervals.
		vector< pair< double, pair<DWORD,DWORD> > > valToInt;
		valToInt.resize( tmpContext.size() * 2 );
		// Collectiong values
		for( DWORD obj = 0; obj < tmpContext.size(); ++obj ) {
			const std::pair<double,double>& val = tmpContext[obj][attr];
			valToInt[obj*2].first = val.first;
			valToInt[obj*2 + 1].first = val.second;
		}
		sort(valToInt.begin(), valToInt.end(), valToIntValueCmp);
		if( 0 < propPrecisionThld && propPrecisionThld < 1 ) {
			double maxDiff = 0;
			for( DWORD v = 1; v <= valToInt.size(); ++v ) {
				maxDiff = max(maxDiff, valToInt[v].first - valToInt[v-1].first );
			}
			precisions[attr]= maxDiff * propPrecisionThld;
		}
		// Joining similar values
		DWORD lastSimilar = 0;
		for( DWORD v = 0; v <= valToInt.size(); ++v ) {
			if(v < valToInt.size() // if v == valToInt.size() we should add the last interval
				&& valToInt[v].first - valToInt[lastSimilar].first < precisions[attr] )
			{
				continue;
			}
			// Should split here.
			assert( values[attr].size() == 0 || values[attr].back() < valToInt[lastSimilar].first );
			const DWORD from = values[attr].size();
			values[attr].push_back( valToInt[lastSimilar].first );
			DWORD to = from;
			if( v > lastSimilar + 1
				&& abs(valToInt[lastSimilar].first - valToInt[v-1].first) > 1e-10 )
			{
				to = values[attr].size();
				values[attr].push_back( valToInt[v-1].first );
			}
			// Updating correspondance to 'values'
			for( DWORD vv = lastSimilar; vv < v; ++vv ) {
				valToInt[vv].second.first=from;
				valToInt[vv].second.second=to;
			}
			lastSimilar = v;
		}

		// Updating new context
		for( DWORD obj = 0; obj < tmpContext.size(); ++obj ) {
			const std::pair<double,double>& val = tmpContext[obj][attr];
			const std::pair<DWORD,DWORD>& int1 =
				 findInterval( val.first, valToInt, precisions[attr] );
			const std::pair<DWORD,DWORD>& int2 =
				 findInterval( val.second, valToInt, precisions[attr] );
			context[obj][attr].first = int1.first;
			context[obj][attr].second = int2.second;
		}
	}

	tmpContext.clear();
}

////////////////////////////////////////////////////////////////////
// CIntervalClsPatternsProjectionChain::computeAttrOrder stuff

class AttrCompare {
public:
	AttrCompare(const std::vector< std::vector<double> >& v ) : values(v) {}

	bool operator()(DWORD a, DWORD b ) const
		{ assert( a < values.size() && b < values.size() ); return values[a].size() > values[b].size(); }
private:
	const std::vector< std::vector<double> >& values;
};
// Computes the decreasing order, wrt the number of values, of attributes.
void CIntervalClsPatternsProjectionChain::computeAttrOrder()
{
	attrOrder.clear();
	attrOrder.resize(values.size());
	for( DWORD i = 0; i < attrOrder.size(); ++i ) {
		attrOrder[i]=i;
	}
	AttrCompare cmp(values);
	sort( attrOrder.begin(), attrOrder.end(), cmp);
}

// Find preimages of a pattern in the case of state.State = S_Left.
void CIntervalClsPatternsProjectionChain::leftPreimages(
	const CPatternDescription& p, CPatternList& preimages )
{
	CIntent& intent = p.Intent();
	if( intent[attrOrder[state.AttrNum]].first != state.ValueNum // There is another pattern which is more close
		|| intent[attrOrder[state.AttrNum]].second == state.ValueNum ) // The pattern of the form [x,x] gives it self for projection x.
	{
		return;
	}
	const CVectorBinarySetDescriptor& descr = getStateObjs();
	CSharedPtr<const CVectorBinarySetDescriptor> sim(
		extCmp->CalculateSimilarity( p.Extent(), descr ), extDeleter );
	const DWORD diff = p.Extent().Size() - sim->Size();
	if( diff == 0 ) {
		// Not closed pattern
		intent[attrOrder[state.AttrNum]].first = state.ValueNum + 1;
		return;
	}

    auto_ptr<CPatternDescription> res( NewPattern(sim) );
    res->Intent() = intent;
    res->Intent()[attrOrder[state.AttrNum]].first = state.ValueNum + 1;
	preimages.PushBack(res.release());
}

void CIntervalClsPatternsProjectionChain::rightPreimages(
	const CPatternDescription& p, CPatternList& preimages )
{
	const DWORD currValue = values[attrOrder[state.AttrNum]].size() - 1 - state.ValueNum;
	assert( 0 < currValue && currValue < values[attrOrder[state.AttrNum]].size() ); // Type overfilling should not happen.
	CIntent& intent = p.Intent();
	if( intent[attrOrder[state.AttrNum]].second != currValue // There is another pattern which is more close
		|| intent[attrOrder[state.AttrNum]].first == currValue ) // The pattern of the form [x,x] gives it self for projection x.
	{
		return;
	}

	const CVectorBinarySetDescriptor& descr = getStateObjs();
	CSharedPtr<const CVectorBinarySetDescriptor> sim(
		extCmp->CalculateSimilarity( p.Extent(), descr ), extDeleter );
	const DWORD diff = p.Extent().Size() - sim->Size();
	if( diff == 0 ) {
		// Not closed pattern
		intent[attrOrder[state.AttrNum]].second = currValue - 1;
		return;
	}

    auto_ptr<CPatternDescription> res( NewPattern(sim) );
    res->Intent() = intent;
    res->Intent()[attrOrder[state.AttrNum]].second = currValue - 1;
	preimages.PushBack(res.release());
}

const CVectorBinarySetDescriptor& CIntervalClsPatternsProjectionChain::getStateObjs()
{
	if( stateObjs != 0 ){
		return *stateObjs;
	}

	CList<DWORD> objs;
	switch( state.State ) {
	case CCurrState::S_Left:
		for( DWORD i = 0; i < context.size(); ++i ) {
			if( state.ValueNum + 1 <= context[i][attrOrder[state.AttrNum]].first ) {
				objs.PushBack(i);
			}
		}
		break;
	case CCurrState::S_Right:
		for( DWORD i = 0; i < context.size(); ++i ) {
			if( context[i][attrOrder[state.AttrNum]].second <= values[attrOrder[state.AttrNum]].size() - 2 - state.ValueNum ) {
				objs.PushBack(i);
			}
		}
		break;
	default:
		assert(false);
	}
	stateObjs.reset(extCmp->NewPattern(), extDeleter );
	extCmp->AddList(objs, *stateObjs );
	return *stateObjs;
}

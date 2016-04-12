// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/modules/StabIntervalClsPatternsProjectionChain.h>

#include <JSONTools.h>

#include <rapidjson/document.h>

using namespace std;

////////////////////////////////////////////////////////////////////

CModuleRegistrar<CStabIntervalClsPatternsProjectionChain> CStabIntervalClsPatternsProjectionChain::registar(
	ProjectionChainModuleType, StabIntervalClsPatternsProjectionChain );

CStabIntervalClsPatternsProjectionChain::CStabIntervalClsPatternsProjectionChain() :
	currStateNum(0)
{
	Thld() = 5;
}

double CStabIntervalClsPatternsProjectionChain::GetPatternInterest( const IPatternDescriptor* p )
{
	const CStabPatternDescription& d = Pattern(p);
	if( d.StabState() == currStateNum ) {
		return d.Stability();
	} else {
		return computeStability(d);
	}
}
void CStabIntervalClsPatternsProjectionChain::ComputeZeroProjection( CPatternList& ptrns )
{
	CIntervalClsPatternsProjectionChain::ComputeZeroProjection( ptrns );

	assert( ptrns.Size() == 1 );
	const CStabPatternDescription& p = Pattern(ptrns.Front());
	p.MinAttr()= 0;
	p.Stability() = Context().size();
	p.StabState() = currStateNum;
}
bool CStabIntervalClsPatternsProjectionChain::NextProjection()
{
	const bool res = CIntervalClsPatternsProjectionChain::NextProjection();
	if( res ) {
		++currStateNum;
	}
	return res;
}
void CStabIntervalClsPatternsProjectionChain::Preimages( const IPatternDescriptor* d, CPatternList& preimages )
{
	assert( State().State != CCurrState::S_End );
	const DWORD oldSize = preimages.Size();
	const CStabPatternDescription& p = Pattern(d);

	JustPreimages(p, preimages );
	if( preimages.Size() - oldSize == 0 ) {
		p.StabState() = currStateNum;
		return;
	}

	assert( preimages.Size() - oldSize == 1 );
	const CStabPatternDescription& newP = Pattern(preimages.Back());

	const DWORD diff = p.Extent().Size() - newP.Extent().Size();
	assert( diff != 0 );

	// Computing stability of 'newP'
	if( !isStable( newP, p.MinAttr() ) ) {
		preimages.PopBack();
		FreePattern( &newP );
	}

	// Updating stability of 'p'
	p.StabState() = currStateNum;
	if( p.Stability() > diff ) {
		p.Stability() = diff;
		p.MinAttr() = State().State == CCurrState::S_Left ? -(int)(State().AttrNum + 1) : State().AttrNum + 1;
	}
}

void CStabIntervalClsPatternsProjectionChain::LoadParams( const JSON& json )
{
	CJsonError errorText;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "CStabIntervalClsPatternsProjectionChain::LoadParams", errorText );
	}
	assert( string( params["Type"].GetString() ) == ProjectionChainModuleType );
	assert( string( params["Name"].GetString() ) == StabIntervalClsPatternsProjectionChain );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		return;
	}
	const rapidjson::Value& paramsObj = params["Params"];
	if( paramsObj.HasMember("Precisions") ) {
		if(paramsObj["Precisions"].IsNumber() ) {
			Precision()=paramsObj["Precisions"].GetDouble();
		} else if( paramsObj["Precisions"].IsArray() ) {
			Precisions().resize( paramsObj["Precisions"].Size() );
			for( DWORD i = 0; i < Precisions().size(); ++i ) {
				if( paramsObj["Precisions"][i].IsNumber() ) {
					Precisions()[i]=paramsObj["Precisions"][i].GetDouble();
				} else {
					Precisions()[i]=Precision();
				}
			}
		}
	} else if( paramsObj.HasMember( "ProportionalPrecisionThreshold" ) && paramsObj["ProportionalPrecisionThreshold"].IsNumber() ) {
		PropPrecisionThld() = paramsObj["ProportionalPrecisionThreshold"].GetDouble();
	}
}
JSON CStabIntervalClsPatternsProjectionChain::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", ProjectionChainModuleType, alloc )
		.AddMember( "Name", StabIntervalClsPatternsProjectionChain, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject(), alloc );
	rapidjson::Value& p = params["Params"];
	if( 0 < PropPrecisionThld() && PropPrecisionThld() < 1 ) {
		p.AddMember( "ProportionalPrecisionThreshold", rapidjson::Value().SetDouble(PropPrecisionThld()),alloc);
	}
	p.AddMember( "Precisions", rapidjson::Value().SetArray(), alloc );
	p["Precisions"].Reserve( Precisions().size(), alloc );
	for( DWORD i = 0; i < Precisions().size(); ++i ) {
		p["Precisions"].PushBack( rapidjson::Value().SetDouble(Precisions()[i]), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}


const double& CStabIntervalClsPatternsProjectionChain::computeStability( const CStabPatternDescription& p ) const
{
	if( !isStable(p,0)) {
		p.Stability() = min(p.Stability(), Thld() - 1 );
		p.StabState() = currStateNum;
	}
	return p.Stability();
}
// Check if a pattern is stable and updates the stability value.
//  @param p is a pattern
//  @param attr is an 1-based index of an attribute that should be checked first, 0 if unknown.
bool CStabIntervalClsPatternsProjectionChain::isStable(const CStabPatternDescription& p, int attr ) const
{
	bool res = true;
	CList<DWORD> extent;
	ExtCmp().EnumValues( p.Extent(), extent );
	p.Stability() = extent.Size();
	res = p.Stability() >= Thld();

	if( attr < 0 ) {
		res = res && isLeftStable(extent, p, -attr - 1 ) && isRightStable(extent, p, 0);
	} else {
		res = res && isRightStable(extent, p, attr - 1 ) && isLeftStable(extent, p, 0);
	}

	p.StabState() = currStateNum;
	return res;
}

// As isStable but check only left part of the interval
//  @param p is a pattern
//  @param attr is a 0-based index of attribute to start
bool CStabIntervalClsPatternsProjectionChain::isLeftStable(
	const CList<DWORD>& ext, const CStabPatternDescription& p, int attr ) const
{
	for( int i = attr; i >=0; --i ) {
		if(!isLeftStableForAttr(ext,p,i) ) {
			return false;
		}
	}
	for( int i = attr+1; i < AttrOrder().size(); ++i ) {
		if(!isLeftStableForAttr(ext,p,i) ) {
			return false;

		}
	}

	return true;
}
// As isStable but check only right part of the interval
//  @param p is a pattern
//  @param attr is a 0-based index of attribute to start
bool CStabIntervalClsPatternsProjectionChain::isRightStable(
	const CList<DWORD>& ext, const CStabPatternDescription& p, int attr ) const
{
	for( int i = attr; i >=0; --i ) {
		if(!isRightStableForAttr(ext,p,i) ) {
			return false;
		}
	}
	for( int i = attr+1; i < AttrOrder().size(); ++i ) {
		if(!isRightStableForAttr(ext,p,i) ) {
			return false;
		}
	}

	return true;
}
// Checks if a child in the direction of left @param attr forbids for a pattern to be stable.
bool CStabIntervalClsPatternsProjectionChain::isLeftStableForAttr(
	const CList<DWORD>& ext, const CStabPatternDescription& p, int attr ) const
{
	if( !canChangeLeft(p,attr)) {
		return true;
	}
	const DWORD childSize = computeLeftIntersectionSize(
		ext,attr, p.Intent()[AttrOrder()[attr]].first + 1);
	const DWORD diff = p.Extent().Size() - childSize;
	if( diff >= p.Stability() ) {
		return true;
	} else {
		p.Stability() = diff;
		p.MinAttr() = -attr - 1;
		return p.Stability() >= Thld();
	}
}
// Checks if a child in the direction of right @param attr forbids for a pattern to be stable.
bool CStabIntervalClsPatternsProjectionChain::isRightStableForAttr(
	const CList<DWORD>& ext, const CStabPatternDescription& p, int attr ) const
{
	if( !canChangeRight(p,attr)) {
		return true;
	}
	const DWORD childSize = computeRightIntersectionSize(
		ext,attr,p.Intent()[AttrOrder()[attr]].second - 1);
	const DWORD diff = p.Extent().Size() - childSize;
	if( diff >= p.Stability() ) {
		return true;
	} else {
		p.Stability() = diff;
		p.MinAttr() = attr + 1;
		return p.Stability() >= Thld();
	}
}

// Check if the attribute @param attr can be changed w.r.t. to current projection
bool CStabIntervalClsPatternsProjectionChain::canChangeLeft(const CStabPatternDescription& p, int attr ) const
{
	return attr <= State().AttrNum
			&& State().ValueNum >= p.Intent()[AttrOrder()[attr]].first
		|| attr > State().AttrNum
			&& State().ValueNum >= p.Intent()[AttrOrder()[attr]].first + 1;
}
// Check if the attribute @param attr can be changed w.r.t. to current projection
bool CStabIntervalClsPatternsProjectionChain::canChangeRight(const CStabPatternDescription& p, int attr ) const
{
	const int stateValueNum = (int)ValuesNumber(AttrOrder()[attr]) - 1 - (int)State().ValueNum;
	return stateValueNum < (int)p.Intent()[AttrOrder()[attr]].second
		|| stateValueNum == (int)p.Intent()[AttrOrder()[attr]].second
			&& State().State == CCurrState::S_Right
			&& attr <= State().AttrNum;
}

DWORD CStabIntervalClsPatternsProjectionChain::computeLeftIntersectionSize(
	const CList<DWORD>& ext, int attr, DWORD valueNum ) const
{
	const DWORD currSize = ext.Size();
	DWORD resultSize = currSize;
	CStdIterator<CList<DWORD>::CConstIterator, false> itr(ext);
	for( ; !itr.IsEnd(); ++itr ) {
		if( Context()[*itr][AttrOrder()[attr]].first < valueNum ) {
			--resultSize;
		}
	}
	return resultSize;
}
DWORD CStabIntervalClsPatternsProjectionChain::computeRightIntersectionSize(
	const CList<DWORD>& ext, int attr, DWORD valueNum ) const
{
	DWORD resultSize = ext.Size();
	CStdIterator<CList<DWORD>::CConstIterator, false> itr(ext);
	for( ; !itr.IsEnd(); ++itr ) {
		if( Context()[*itr][AttrOrder()[attr]].second > valueNum ) {
			--resultSize;
		}
	}
	return resultSize;
}

// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: Implementation of a projection chain that takes patterns (its images) from an algorithm iterating patterns, e.g., frequent graphs,
//   and based on these patterns find stable sets of these patterns.

#include "StabClsPatternProjectionChain.h"

#include <common.h>

#include <fcaps/modules/VectorBinarySetDescriptor.h>
#include <fcaps/ModuleJSONTools.h>

#include <JSONTools.h>

using namespace std;

////////////////////////////////////////////////////////////////////////

class CStabClsPatternProjectionChain::CStabClsPatternDescription : public IPatternDescriptor {
public:
	typedef list< CSharedPtr<const CVectorBinarySetDescriptor> > TChildren;
public:
	CStabClsPatternDescription(const CSharedPtr<const CVectorBinarySetDescriptor>& ext):
		extent( ext ), dMeasure(0) {};
	virtual TPatternType GetType() const
	{return PT_Other;};
	virtual bool IsMostGeneral() const
	{return false /*TODO*/;}
	virtual size_t Hash() const
	{return extent->Hash();}

	const CVectorBinarySetDescriptor& Extent() const
	{ return *extent;}
	DWORD& DMeasure() const
	{ return dMeasure; }
	TChildren& Children() const
	{ return children; }
private:
	// Extent of the pattern.
	CSharedPtr<const CVectorBinarySetDescriptor> extent;
	// Children of the pattern in the concept lattice.
	mutable TChildren children;
	// Delta-measure of the pattern.
	mutable DWORD dMeasure;
};

////////////////////////////////////////////////////////////////////////

CModuleRegistrar<CStabClsPatternProjectionChain> CStabClsPatternProjectionChain::registar(ProjectionChainModuleType, StabClsPatternProjectionChainModule);

CStabClsPatternProjectionChain::CStabClsPatternProjectionChain() :
	objectCount(0),
	patternCount(0),
	extCmp( new CVectorBinarySetJoinComparator() ),
	extDeleter( extCmp ),
	thld( 0 ),
	isStablePtrnFound( true ),
	requestedReserve( NotFound )
{
}
const std::vector<std::string>& CStabClsPatternProjectionChain::GetObjNames() const
{
	return extCmp->GetNames();
}
void CStabClsPatternProjectionChain::SetObjNames( const std::vector<std::string>& names )
{
	extCmp->SetNames( names );
}
void CStabClsPatternProjectionChain::AddObject( DWORD objectNum, const JSON& intent )
{
	assert( enumerator != 0 );
	enumerator->AddObject( objectNum, intent );
	++objectCount;
}
void CStabClsPatternProjectionChain::UpdateInterestThreshold( const double& t )
{
	thld = static_cast<DWORD>( t + 0.5 );
}
double CStabClsPatternProjectionChain::GetPatternInterest( const IPatternDescriptor* p )
{
	// TODO (probably we should check that stability is up to date)
	return Ptrn(p).DMeasure();

	/*  The old code
	const CStabPatternDescription& ptrn = StabPattern(Pattern(p));
	if( ptrn.StabAttrNum() == CurrAttr()
		|| ptrn.StabAttrNum() == Order().size() - 1 )
	{
		return ptrn.Stability();
	} else if( CurrAttr() >= ptrn.GlobMinAttr() ) {
		ptrn.Stability() = min( ptrn.Stability(), (double)ptrn.GlobMinValue() );
		ptrn.StabAttrNum() = Order().size() - 1;
		return ptrn.Stability();
	} else {
		stabApprox.InitComputation( ptrn.Extent(), ptrn.Intent() );
		stabApprox.ComputeUpperBound();

		// Not more than concept.Extent->Size() ?
		ptrn.Stability()=stabApprox.GetStabilityRightLimit(),
		ptrn.StabAttrNum()=CurrAttr();
		ptrn.MinAttr() = stabApprox.GetMinDiffAttr();
		return ptrn.Stability();
	}*/
}
bool CStabClsPatternProjectionChain::AreEqual(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return extCmp->Compare( Ptrn(p).Extent(), Ptrn(q).Extent(), CR_Equal, CR_AllResults | CR_Incomparable ) == CR_Equal;
}
bool CStabClsPatternProjectionChain::IsSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return extCmp->Compare( Ptrn(p).Extent(), Ptrn(q).Extent(), CR_LessGeneral, CR_AllResults | CR_Incomparable ) == CR_LessGeneral;
}
bool CStabClsPatternProjectionChain::IsTopoSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return Ptrn(p).Extent().Size() > Ptrn(q).Extent().Size();
}
void CStabClsPatternProjectionChain::FreePattern(const IPatternDescriptor* p ) const
{
	delete &Ptrn(p);
}
void CStabClsPatternProjectionChain::ComputeZeroProjection( CPatternList& ptrns )
{
    // TODO
    objectCount = 189771;

	extCmp->SetMaxAttrNumber( objectCount );
    if( requestedReserve != (DWORD)-1 ) {
        extCmp->Reserve( requestedReserve );
    }

	CSharedPtr<CVectorBinarySetDescriptor> ext(extCmp->NewPattern(), extDeleter );
	for( DWORD i = 0; i < objectCount; i++ ) {
		extCmp->AddValue( i, *ext.get() );
	}
	CStabClsPatternDescription* ptrn = new CStabClsPatternDescription( ext );
	ptrn->DMeasure() = objectCount;
	ptrns.PushBack(ptrn);
}
bool CStabClsPatternProjectionChain::NextProjection()
{
    assert( enumerator != 0 );

	CPatternImage img;
	CSharedPtr<CVectorBinarySetDescriptor> nextImageCandidate;
	while( true ) {
        const bool res = enumerator->GetNextPattern( isStablePtrnFound ? CPU_Expand : CPU_Reject, img );
        if( !res ) {
            return false;
        }
        /* Cannot ignore unstable results, since it could make unstable existing patterns */
//        if( img.ImageSize < thld ) {
//            // Cannot be stable.
//            // Neither its children
//            isStablePtrnFound=false;
//            enumerator->ClearMemory( img );
//            continue;
//        }
        isStablePtrnFound = true; // while cycling, we should expand.

        ++patternCount;

        nextImageCandidate.reset( extCmp->NewPattern(), extDeleter );
        assert( img.ImageSize > 0 );
        for (DWORD i = 0; i < img.ImageSize; i++) {
            extCmp->AddValue( static_cast<DWORD>( img.Objects[i]), *nextImageCandidate );
        }

        if( nextImage == 0 || extCmp->Compare( *nextImageCandidate, *nextImage, CR_Equal, CR_AllResults | CR_Incomparable ) == CR_Incomparable ) {
            // found a new pattern;
            nextImage = nextImageCandidate;
            break;
        }

        enumerator->ClearMemory( img );
	}

    enumerator->ClearMemory( img );

    isStablePtrnFound = false;
 	return true;
}
double CStabClsPatternProjectionChain::GetProgress() const
{
	return patternCount;
}
void CStabClsPatternProjectionChain::Preimages( const IPatternDescriptor* d, CPatternList& preimages )
{
    if( enumerator == 0 ) {
        throw new CTextException( "CStabClsPatternProjectionChain::Preimages", "Params object not found. Necessary for pattern enumerator." );
    }

    const CStabClsPatternDescription& ptrn = Ptrn(d);
	// Computing the only possible preimage
	CSharedPtr<const CVectorBinarySetDescriptor> res(
		extCmp->CalculateSimilarity( ptrn.Extent(), *nextImage ), extDeleter );
    const DWORD ptrnExtSize = ptrn.Extent().Size();
    const DWORD resExtSize = res->Size();
	const DWORD extDiff = ptrnExtSize - resExtSize;
	if(extDiff == 0 ) {
		isStablePtrnFound = true;
		/*  TODO: if we want to save intent */
		// ptrn.Intent().PushBack( currAttr );

		// Stability of 'ptrn' has not been changed.
		//  (probably we should update the fact that stability is up to date)
		return;
	}
	auto_ptr<CStabClsPatternDescription> newPtrn( new CStabClsPatternDescription( res ) );
	if( initializeNewPattern( ptrn, *newPtrn ) ) {
		// The new pattern is stable.
		//  We should add it to preimages.
		isStablePtrnFound = true;
		preimages.PushBack( newPtrn.release() );
	}

	// Child is added only here because initialiasation rely on the old set of children.
	addChild( res, ptrn );

	// Updating the measure of current pattern.
	ptrn.DMeasure() = min( ptrn.DMeasure(), extDiff );
}
JSON CStabClsPatternProjectionChain::SaveExtent( const IPatternDescriptor* d ) const
{
	return extCmp->SavePattern( &Ptrn(d).Extent() );
}
JSON CStabClsPatternProjectionChain::SaveIntent( const IPatternDescriptor* d ) const
{
	// TODO
	return "{\"Comment\":\"TODO\"}";
}

void CStabClsPatternProjectionChain::LoadParams( const JSON& json )
{
	CJsonError errorText;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "CStabClsPatternProjectionChain::LoadParams", errorText );
	}
	assert( string( params["Type"].GetString() ) == ProjectionChainModuleType );
	assert( string( params["Name"].GetString() ) == StabClsPatternProjectionChainModule );

	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()
		  && params["Params"].HasMember("Enumerator") && params["Params"]["Enumerator"].IsObject() ) )
	{
        // Do not throw an exception since this method can be called to pass some params from a context
        return;
	}

	const rapidjson::Value& paramsObj = params["Params"];
	const rapidjson::Value& pe = paramsObj["Enumerator"];
	string errorTextStr;
	enumerator.reset( CreateModuleFromJSON<IPatternEnumerator>(pe,errorTextStr) );
	if( enumerator == 0 ) {
		throw new CJsonException( "CStabClsPatternProjectionChain::LoadParams", CJsonError( json, errorTextStr ) );
	}
	if( paramsObj.HasMember( "ImageReserve" ) && paramsObj["ImageReserve"].IsUint() ) {
        requestedReserve = paramsObj["ImageReserve"].GetUint();
	}
}
JSON CStabClsPatternProjectionChain::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", ProjectionChainModuleType, alloc )
		.AddMember( "Name", StabClsPatternProjectionChainModule, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
            .AddMember("ImageReserve", rapidjson::Value().SetUint( extCmp->GetAvailableBlockCount() ), alloc )
        , alloc );

	IModule* m = dynamic_cast<IModule*>(enumerator.get());

	JSON enParamsStr;
	rapidjson::Document enParamsDoc;
    assert( m!=0);
	if( m != 0 ) {
		enParamsStr = m->SaveParams();
		CJsonError error;
		const bool rslt = ReadJsonString( enParamsStr, enParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("Enumerator", enParamsDoc.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

// Casts a pointer to an arbitrary pattern to the internal pattern type.
inline const CStabClsPatternProjectionChain::CStabClsPatternDescription&
CStabClsPatternProjectionChain::Ptrn( const IPatternDescriptor* p ) const
{
	assert( p != 0 );
	return *debug_cast<const CStabClsPatternDescription*>(p);
}

/**
   Initializes the new pattern.
   @param parent is the parent pattern that is used to generate new pattern.
   @param newPtrn is the newly generated pattern.
   @return Returns true if new patern is stable.
 */
bool CStabClsPatternProjectionChain::initializeNewPattern(
	const CStabClsPatternDescription& parent, CStabClsPatternDescription& newPtrn)
{
	const DWORD newPtrnExtSize = newPtrn.Extent().Size();
	if( newPtrnExtSize < thld ) {
        // Cannot be stable.
        return false;
	}
    newPtrn.DMeasure() = newPtrnExtSize;

	// Computing intersection of parent's children with the new pattern and verifying that it is still stable.
	CStdIterator<CStabClsPatternDescription::TChildren::const_iterator> ch( parent.Children() );
	for( ; !ch.IsEnd(); ++ch ) {
		CSharedPtr<const CVectorBinarySetDescriptor> res(
			extCmp->CalculateSimilarity( **ch, *nextImage ), extDeleter );
		const DWORD extDiff = newPtrnExtSize - res->Size();
		if(extDiff < thld ) {
			// If extDiff == 0 the result is the same as the intersection with a child.
			//  If child is not stable, than the result is neither stable, if child is stable we will recompute newPtrn another time.
			// If extDiff 0 < extDiff < thld then the newPtrn is not stable.
			// ==> IGNORING
			return false;
		}
		addChild( res, newPtrn );

		newPtrn.DMeasure()=min(newPtrn.DMeasure(), extDiff );
	}
    return true;
}
/**
   The set of immideate ancestors are cached within the pattern in order to compute DMeasure faster.
   @param child is the extent of a possible child of the pattern.
   @param ptrn is the pattern that could have @param child as a child in the concept lattice.
 */
void CStabClsPatternProjectionChain::addChild( const CSharedPtr<const CVectorBinarySetDescriptor>& child, const CStabClsPatternDescription& ptrn )
{
	CStdIterator<CStabClsPatternDescription::TChildren::iterator>
		ch( ptrn.Children() ), currCh, placeToAdd( ptrn.Children().end(),ptrn.Children().end() );
	for( ; !ch.IsEnd(); ) {
		currCh = ch;
		++ch;
		if( child->Size() <= (*currCh)->Size() ) {
			// 'child' could be a child of currCh
			if( extCmp->Compare(*child, **currCh, CR_MoreOrEqual, CR_MoreOrEqual | CR_Incomparable ) != CR_Incomparable ) {
				// no need to add currCh
				break;
			}
			placeToAdd = currCh;
		}
		if( child->Size() > (*currCh)->Size() ) {
			// 'child' could be a parent of currCh
			if( extCmp->Compare(*child, **currCh, CR_LessGeneral, CR_LessGeneral | CR_Incomparable ) != CR_Incomparable ) {
				// ch should be removed
				ptrn.Children().erase( currCh );
			}
		}
	}
	if( placeToAdd.IsEnd() ) {
		ptrn.Children().push_front( child );
	} else {
		++placeToAdd;
		ptrn.Children().insert( placeToAdd, child );
	}
}

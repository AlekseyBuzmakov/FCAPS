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
	CStabClsPatternDescription(const CSharedPtr<const CVectorBinarySetDescriptor>& ext):
		extent( ext ) {};
	virtual TPatternType GetType() const
	{return PT_Other;};
	virtual bool IsMostGeneral() const
	{return false /*TODO*/;}
	virtual size_t Hash() const
	{return extent->Hash();}

	const CVectorBinarySetDescriptor& Extent() const
	{ return *extent;}
private:
	CSharedPtr<const CVectorBinarySetDescriptor> extent;
};

////////////////////////////////////////////////////////////////////////

CModuleRegistrar<CStabClsPatternProjectionChain> CStabClsPatternProjectionChain::registar(ProjectionChainModuleType, StabClsPatternProjectionChainModule);

CStabClsPatternProjectionChain::CStabClsPatternProjectionChain() :
	extCmp( new CVectorBinarySetJoinComparator() ),
	extDeleter( extCmp )
{
}
void CStabClsPatternProjectionChain::AddObject( DWORD objectNum, const JSON& intent )
{
	assert( enumerator != 0 );
	enumerator->AddObject( objectNum, intent );
}
void CStabClsPatternProjectionChain::UpdateInterestThreshold( const double& thld )
{
	// TODO
}
double CStabClsPatternProjectionChain::GetPatternInterest( const IPatternDescriptor* p )
{
	return 2; // TODO
}
bool CStabClsPatternProjectionChain::AreEqual(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return extCmp->Compare( p, q, CR_Equal, CR_AllResults | CR_Incomparable ) == CR_Equal;
}
bool CStabClsPatternProjectionChain::IsSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return extCmp->Compare( p, q, CR_LessGeneral, CR_AllResults | CR_Incomparable ) == CR_LessGeneral;
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
	CSharedPtr<CVectorBinarySetDescriptor> ext(extCmp->NewPattern(), extDeleter );
	for( DWORD i = 0; i < extCmp->GetMaxAttrNumber(); i++ ) {
		extCmp->AddValue( i, *ext.get() );
	}
	CStabClsPatternDescription* ptrn = new CStabClsPatternDescription( ext );
	ptrns.PushBack(ptrn);
}
bool CStabClsPatternProjectionChain::NextProjection()
{
    assert( enumerator != 0 );

	CPatternImage img;
	enumerator->GetNextPattern( CPU_Expand, img );
	enumerator->ClearMemory( img );
	return false;
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

	// TODO
	return;
}
JSON CStabClsPatternProjectionChain::SaveExtent( const IPatternDescriptor* d ) const
{
	// TODO
	return "{}";
}
JSON CStabClsPatternProjectionChain::SaveIntent( const IPatternDescriptor* d ) const
{
	// TODO
	return "{}";
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
}
JSON CStabClsPatternProjectionChain::SaveParams() const
{
	// TODO
	return "{}";
}

// Casts a pointer to an arbitrary pattern to the internal pattern type.
inline const CStabClsPatternProjectionChain::CStabClsPatternDescription&
CStabClsPatternProjectionChain::Ptrn( const IPatternDescriptor* p ) const
{
	assert( p != 0 );
	return *debug_cast<const CStabClsPatternDescription*>(p);
}

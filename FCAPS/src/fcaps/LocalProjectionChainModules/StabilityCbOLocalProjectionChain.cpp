// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8

#include <fcaps/LocalProjectionChainModules/StabilityCbOLocalProjectionChain.h>

#include <fcaps/PatternDescriptor.h>
#include <fcaps/ContextAttributes.h>

#include <fcaps/SharedModulesLib/VectorBinarySetDescriptor.h>
#include <fcaps/SharedModulesLib/BinarySetPatternManager.h>

#include <JSONTools.h>
#include <ModuleJSONTools.h>
#include <StdTools.h>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(x...) #x

const char description[] =
STR(
	{
	"Name":"Stability Close-by-One Local Projection Chain",
	"Description":"Local Projection Chain that is based on Close-by-One (also known as LCM) canonical order and incorporate computation of DELTA-stability.",
	"Params": {
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of Stability Close-by-One Local Projection Chain",
			"type": "object",
			"properties": {
				"ContextAttributes":{
					"description": "A module for storing and accesing formal context",
					"type": "@ContextAttributesModules"
				}			}
		}
	}
);


////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CStabilityCbOLocalProjectionChain> CStabilityCbOLocalProjectionChain::registrar;

const char* const CStabilityCbOLocalProjectionChain::Desc()
{
	return description;
}

////////////////////////////////////////////////////////////////////

class CPattern : public IExtent, public IPatternDescriptor {
	
public:
	CPattern( const CSharedPtr<CVectorBinarySetJoinComparator>& cmp, const CSharedPtr<const CVectorBinarySetDescriptor>& e, const CSharedPtr<CBinarySetPatternDescriptor>& i,
	          int nextAttr, DWORD d, int closestAttribute ) :
		extent(e), intent(i), nextAttribute(nextAttr), delta(d), closestChildAttribute(closestAttribute) {initPatternImage(cmp);}
	~CPattern()
		{ delete img.Objects; }

	// Methods of IExtent
	virtual DWORD Size() const
		{ return Extent().Size();}
    virtual void GetExtent( CPatternImage& extent ) const
		{extent = img;}
	virtual void ClearMemory( CPatternImage& ) const
		{ /*NOTHING memory is not allocated*/}

	// Methos of IPatternDescriptor
	virtual bool IsMostGeneral() const
		{return Intent().GetAttribs().Size() == 0;}
	virtual size_t Hash() const
		{ return Extent().Hash(); }

	// Methods of the class
	const CVectorBinarySetDescriptor& Extent() const
		{return *extent;}
	const CBinarySetPatternDescriptor& Intent() const
		{return *intent;}
	CBinarySetPatternDescriptor& IntentRW() const
		{return *intent;}
	int NextAttribute() const
		{return nextAttribute;}
	void SetNextAttribute(int a) const
		{nextAttribute = a;}
	DWORD Delta() const
		{return delta;}
	void SetDelta(DWORD d) const
		{assert(d <= delta); delta = d;}
	int ClosestChild() const
		{return closestChildAttribute;}
	void SetClosestChild( int a ) const
		{closestChildAttribute = a;}


private:
	const CSharedPtr<const CVectorBinarySetDescriptor> extent;
	CPatternImage img;
	
	mutable CSharedPtr<CBinarySetPatternDescriptor> intent;
	// The minimal number of the attribute that can be added to the pattern.
	// This number also gives the reference to the "core" of the pattern in the CbO canonical order
	mutable int nextAttribute;
	// The delta measure of the pattern w.r.t. the projection not including the @var nextAttribute attribute
	mutable DWORD delta;
	// The attribute of a clossest child. In intersection of the pattern and the attribute the result is the clossest child
	// Used as an optimization for earlier detection of new unstable patterns
	mutable int closestChildAttribute;

	void initPatternImage(const CSharedPtr<CVectorBinarySetJoinComparator>& cmp) {
		assert(extent != 0);
		img.PatternId = Hash();
		img.ImageSize = extent->Size();
		unique_ptr<int[]> objects (new int[img.ImageSize]);
		img.Objects = objects.get(); 
		CList<DWORD> values;
		cmp->EnumValues(*extent, values);
		auto itr=values.Begin();
		DWORD i = 0;
		for(; itr != values.End(); ++itr, ++i) {
			assert(i < img.ImageSize);
			objects[i]=*itr;
		}
		objects.release(); 
		assert( i == img.ImageSize);
	}
};

////////////////////////////////////////////////////////////////////

CStabilityCbOLocalProjectionChain::CStabilityCbOLocalProjectionChain() :
	thld(0),
	extCmp(new CVectorBinarySetJoinComparator),
	extDeleter(extCmp),
	intCmp(new CBinarySetDescriptorsComparator),
	intDeleter(intCmp)
{
}

void CStabilityCbOLocalProjectionChain::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CStabilityCbOLocalProjectionChain::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == Type() );
	assert( string( params["Name"].GetString() ) == Name() );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for Context Attributes object";
		throw new CJsonException("CStabilityCbOLocalProjectionChain::LoadParams", error);
	}

	const rapidjson::Value& p = params["Params"];

	if(!(p.HasMember("ContextAttributes") && (p["ContextAttributes"].IsObject()))) {
		error.Data = json;
		error.Error = "Params.ContextAttributes is not found or is not an object.";
		throw new CJsonException("CStabilityCbOLocalProjectionChain::LoadParams", error);
	}
	const rapidjson::Value& pe = params["Params"]["ContextAttributes"];
	string errorText;
	attrs.reset( CreateModuleFromJSON<IContextAttributes>(pe,errorText) );
	if( pe == 0 ) {
		throw new CJsonException( "CStabilityCbOLocalProjectionChain::LoadParams", CJsonError( json, errorText ) );
	}
	extCmp->SetMaxAttrNumber(attrs->GetObjectNumber());
}

JSON CStabilityCbOLocalProjectionChain::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", rapidjson::StringRef(Type()), alloc )
		.AddMember( "Name", rapidjson::StringRef(Name()), alloc )
		.AddMember( "Params", rapidjson::Value().SetObject(), alloc );

	IModule* m = dynamic_cast<IModule*>(attrs.get());

	JSON peParams;
	rapidjson::Document peParamsDoc;
    assert( m!=0);
	if( m != 0 ) {
		peParams = m->SaveParams();
		CJsonError error;
		const bool rslt = ReadJsonString( peParams, peParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("ContextAttributes", peParamsDoc.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

int CStabilityCbOLocalProjectionChain::GetObjectNumber() const
{
	assert(attrs != 0);
	return attrs->GetObjectNumber();
}
double CStabilityCbOLocalProjectionChain::GetInterestThreshold() const
{
	return thld;
}
void CStabilityCbOLocalProjectionChain::UpdateInterestThreshold( const double& _thld )
{
	thld = thld;
}
double CStabilityCbOLocalProjectionChain::GetPatternInterest( const IPatternDescriptor* p )
{
	return to_pattern(p).Delta();
}
bool CStabilityCbOLocalProjectionChain::IsTopoSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return to_pattern(p).Extent().Size() >= to_pattern(q).Extent().Size();
}
void CStabilityCbOLocalProjectionChain::FreePattern(const IPatternDescriptor* p ) const
{
	delete &to_pattern(p);
}
void CStabilityCbOLocalProjectionChain::ComputeZeroProjection( CPatternList& ptrns )
{
	CSharedPtr<CVectorBinarySetDescriptor> ptrn(extCmp->NewPattern(), extDeleter);
	for( DWORD i = 0; i < GetObjectNumber(); ++i ) {
		extCmp->AddValue(i,*ptrn);
	}
	CSharedPtr<CBinarySetPatternDescriptor> intent(intCmp->NewPattern(), intDeleter);
	ptrns.PushBack( newPattern( ptrn, intent, 0, GetObjectNumber(), 0) );
}
bool CStabilityCbOLocalProjectionChain::Preimages( const IPatternDescriptor* d, CPatternList& preimages )
{
	assert( attrs != 0);
	
	const CPattern& p = to_pattern(d);
    assert(p.Delta() >= thld);

	int a = p.NextAttribute();
	CPatternImage img;
	CSharedPtr<const CVectorBinarySetDescriptor> nextImage;
	while(attrs->HasAttribute(a)){
		// Getting next attribute
		getAttributeImg(p.NextAttribute(),nextImage);

		// Computing the only possible preimage
		CSharedPtr<const CVectorBinarySetDescriptor> res(
			extCmp->CalculateSimilarity( p.Extent(), *nextImage ), extDeleter );
		const DWORD ptrnExtSize = p.Extent().Size();
		const DWORD resExtSize = res->Size();
		const DWORD extDiff = ptrnExtSize - resExtSize;
		if(extDiff == 0 ) {
			// Attribute is in the closure
			p.IntentRW().AddNextAttribNumber(static_cast<DWORD>(a));
			a = attrs->GetNextAttribute(a);
			continue;
		}
		unique_ptr<const CPattern> newPtrn( initializeNewPattern( p, a, res ) );
		if( newPtrn == 0 ) {
			// Pattern 'res' is not stable
			// Any more specific attribute can be ignored
			a = attrs->GetNextNonChildAttribute(a);
			// Should search funther for a better pattern
			continue;
		}
		
		// A new stable pattern is generated
		//  We should add it to preimages.
		assert(newPtrn->Delta() >= thld);
		preimages.PushBack( newPtrn.release() );

		// Updating the measure of the current pattern.
		if( p.Delta() > extDiff ) {
			p.SetDelta(extDiff);
			p.SetClosestChild(a);
		}

		a = attrs->GetNextAttribute(a);
		break;
	}
	p.SetNextAttribute(a);
	return attrs->HasAttribute(a);
}
int CStabilityCbOLocalProjectionChain::GetExtentSize( const IPatternDescriptor* d ) const
{
	return to_pattern(d).Extent().Size();
}
JSON CStabilityCbOLocalProjectionChain::SaveExtent( const IPatternDescriptor* d ) const
{
	return extCmp->SavePattern( &to_pattern(d).Extent() );
}
JSON CStabilityCbOLocalProjectionChain::SaveIntent( const IPatternDescriptor* d ) const
{
	return intCmp->SavePattern( &to_pattern(d).Intent() );
}

const CPattern& CStabilityCbOLocalProjectionChain::to_pattern(const IPatternDescriptor* d) const
{
	assert(d != 0);
	return debug_cast<const CPattern&>(*d);
}

const CPattern* CStabilityCbOLocalProjectionChain::newPattern(
	const CSharedPtr<const CVectorBinarySetDescriptor>& ext,
	const CSharedPtr<CBinarySetPatternDescriptor>& intent,
	int nextAttr, DWORD delta, int clossestAttr)
{
	return new CPattern(extCmp, ext,intent,nextAttr,delta,clossestAttr);
}
void CStabilityCbOLocalProjectionChain::getAttributeImg(int a, CSharedPtr<const CVectorBinarySetDescriptor>& rslt)
{
	CPatternImage img;
	attrs->GetAttribute(a, img);

	CVectorBinarySetDescriptor& ext = *extCmp->NewPattern();
	rslt.reset( &ext, extDeleter );
	assert( img.ImageSize > 0 );
	for (DWORD i = 0; i < img.ImageSize; i++) {
		extCmp->AddValue( static_cast<DWORD>( img.Objects[i]), ext );
	}
	attrs->ClearMemory(img);
}
const CPattern* CStabilityCbOLocalProjectionChain::initializeNewPattern(
	  const CPattern& parent,
	  int genAttr,
	  const CSharedPtr<const CVectorBinarySetDescriptor>& ext)
{
	// If delta is zero, then the attribute is in the intent.
	// However, according to canonical order it should not be there. Thus, such a pattern should also be ignored.
	DWORD delta = getAttributeDelta(parent.ClosestChild(), ext);
	int minAttr = parent.ClosestChild();

	auto intentItr = parent.Intent().GetAttribs().Begin();
	auto intentEnd = parent.Intent().GetAttribs().End();

	for(int a = 0;delta >= thld && a < genAttr && attrs->HasAttribute(a);a = attrs->GetNextNonChildAttribute(a)) {
		while(intentItr != intentEnd && a > *intentItr) {
			const DWORD ia = *intentItr;
			++intentItr;
			assert(ia < *intentItr);
		}
		if(intentItr != intentEnd && a == *intentItr) {
			continue;
		}
		const DWORD aDelta = getAttributeDelta(parent.ClosestChild(), ext);
		if(aDelta < delta) {
			delta = aDelta;
			minAttr = a;
		}
	}
	if( delta < thld ) {
		return 0;
	}
	CSharedPtr<CBinarySetPatternDescriptor> intent(intCmp->NewPattern(),intDeleter);
	intent->AddList(parent.Intent().GetAttribs());
	intent->AddNextAttribNumber(genAttr);
	return(newPattern(ext,intent,attrs->GetNextAttribute(genAttr), delta, minAttr));
}

DWORD CStabilityCbOLocalProjectionChain::getAttributeDelta(int a, const CSharedPtr<const CVectorBinarySetDescriptor>& ext)
{
	CSharedPtr<const CVectorBinarySetDescriptor> attr;
	getAttributeImg(a,attr);

	// Computing the only possible preimage
	CSharedPtr<const CVectorBinarySetDescriptor> res(
		extCmp->CalculateSimilarity( *ext, *attr ), extDeleter );
	const DWORD extDiff = ext->Size() - res->Size();
	return extDiff;
}

// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8

#include <fcaps/LocalProjectionChainModules/StabilityCbOLocalProjectionChain.h>

#include <fcaps/PatternDescriptor.h>
#include <fcaps/ContextAttributes.h>
#include <fcaps/Swappable.h>

#include <fcaps/SharedModulesLib/VectorBinarySetDescriptor.h>

#include <JSONTools.h>
#include <ModuleJSONTools.h>
#include <StdTools.h>

#include <stdint.h>

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
				},
				"ReserveMemory": {
					"description": "A number of patterns to be reserved",
					"type": "integer",
					"minimum":1
				}
			}
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

const CIntentsTree::TAttribute CIntentsTree::InvalidAttribute = static_cast<CIntentsTree::TAttribute>(-1);

CIntentsTree::TAttribute CIntentsTree::GetNextAttribute(TIntentItr& itr) const
{
	assert(0 <= itr && itr < memory.size());
	assert(memory[itr].Attribute != InvalidAttribute);
	const TAttribute res = memory[itr].Attribute;
	itr = memory[itr].Next;
	return res;
}

CIntentsTree::TIntent CIntentsTree::AddAttribute(TIntent intent, TAttribute newAttr)
{
	TIntentItr nextNode = newNode();
	assert(0 <= nextNode && nextNode < memory.size());

	const TIntentItr currItr = GetIterator(intent);
	assert(currItr == -1 || 0 <= currItr && currItr < memory.size());

	memory[nextNode].Next = currItr;
	memory[nextNode].Attribute = newAttr;
	memory[nextNode].BranchesCount = 0;

	if( currItr != -1 ) {
		assert( memory[currItr].Attribute < newAttr);
		assert( 0 <= memory[currItr].Attribute);
		++memory[currItr].BranchesCount;
	}
	return nextNode;
}

void CIntentsTree::Delete(TIntent intent)
{
	TIntentItr itr = GetIterator(intent);
	TIntentItr prevItr = -1;

	while(itr != -1) {
		assert(0 <= itr && itr < memory.size());
		if(memory[itr].BranchesCount > 0) {
			--memory[itr].BranchesCount;
			if( prevItr != -1 ) {
				assert(0 <= prevItr && prevItr < memory.size());
				memory[prevItr].Next = freeNode;
				freeNode = GetIterator(intent);
			}

			// We have found the branching something else exists, so stop it.
			break;
		} else {
			memory[itr].Attribute = InvalidAttribute;
			itr = memory[itr].Next;
		}
	}
	if( itr == -1 && prevItr != -1) {
		assert(0 <= prevItr && prevItr < memory.size());
		memory[prevItr].Next = freeNode;
		freeNode = GetIterator(intent);
	}
}

// Creates a new node dealing with free memory if any
CIntentsTree::TIntentItr CIntentsTree::newNode()
{
	if( freeNode == -1) {
		memory.emplace_back();
		return memory.size() - 1;
	} else {
		assert(0 <= freeNode < memory.size());
		const TIntentItr res = freeNode;
		freeNode = memory[freeNode].Next;
		return res;
	}
}

////////////////////////////////////////////////////////////////////

class CPattern : public IExtent, public IPatternDescriptor, public ISwappable {
	
public:
	// Pattern controls memor for the extent
	CPattern( CVectorBinarySetJoinComparator& _cmp, const CVectorBinarySetDescriptor* e, CPatternDeleter dlt,
	          CIntentsTree& iTree, CIntentsTree::TIntent i,
	          int nextAttr, DWORD d, int closestAttribute ) :
		cmp(_cmp), extent(e, dlt), swappedExtent(-1), intentsTree(iTree), intent(i), nextAttribute(nextAttr), delta(d), closestChildAttribute(closestAttribute)
		{ assert( extent != 0 ); }
	~CPattern()
	{
		intentsTree.Delete(intent);
		if( IsSwapped() ) {
			assert(swappedExtent != static_cast<CVectorBinarySetJoinComparator::TSwappedPattern>(-1));
			cmp.SwapRemove(swappedExtent);
			swappedExtent = -1;
		}
	}

	// Methods of IExtent
	virtual DWORD Size() const
		{ return Extent().Size();}
    virtual void GetExtent( CPatternImage& extent ) const
		{initPatternImage(extent);}
	virtual void ClearMemory( CPatternImage& e) const
		{ delete[] e.Objects;}

	// Methos of IPatternDescriptor
	virtual bool IsMostGeneral() const
		{return intent == -1;}
	virtual size_t Hash() const
		{ return Extent().Hash(); }

	// Methods of ISwappable
	virtual bool IsSwapped() const
		{ extent == 0;}
	virtual void Swap() const
	{
		if( IsSwapped() ) {
			return;
		}
		swappedExtent = cmp.SwapPattern(extent.release());
		assert(extent == 0);
	}

	// Methods of the class
	const CVectorBinarySetDescriptor& Extent() const
		{ restore(); assert(extent != 0); return *extent;}
	CIntentsTree::TIntent Intent() const
		{return intent;}
	void AddAttributeToIntent(CIntentsTree::TAttribute a) const
		{ intent = intentsTree.AddAttribute(intent,a);}

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

	// TOKILL
	size_t GetPatternMemorySize() const
		{ return sizeof(CPattern); }


private:
	CVectorBinarySetJoinComparator& cmp;
	mutable unique_ptr<const CVectorBinarySetDescriptor, CPatternDeleter> extent;
	mutable CVectorBinarySetJoinComparator::TSwappedPattern swappedExtent;

	CIntentsTree& intentsTree;
	mutable CIntentsTree::TIntent intent;

	// The minimal number of the attribute that can be added to the pattern.
	// This number also gives the reference to the "core" of the pattern in the CbO canonical order
	mutable int nextAttribute;
	// The delta measure of the pattern w.r.t. the projection not including the @var nextAttribute attribute
	mutable DWORD delta;
	// The attribute of a clossest child. In intersection of the pattern and the attribute the result is the clossest child
	// Used as an optimization for earlier detection of new unstable patterns
	mutable int closestChildAttribute;

	void initPatternImage(CPatternImage& img) const {
		img.PatternId = Hash();
		img.ImageSize = Extent().Size();
		img.Objects = 0;

		unique_ptr<int[]> objects (new int[img.ImageSize]);
		img.Objects = objects.get(); 
		cmp.EnumValues(Extent(), objects.get(), img.ImageSize);
		objects.release(); 
	}
	void restore() const {
		if( extent != 0 ) {
			return;
		}
		assert(swappedExtent != static_cast<CVectorBinarySetJoinComparator::TSwappedPattern>(-1));
		extent.reset(cmp.SwapRestore(swappedExtent));
		swappedExtent = -1;
		assert(extent != 0);
	}
};

////////////////////////////////////////////////////////////////////

CStabilityCbOLocalProjectionChain::CStabilityCbOLocalProjectionChain() :
	thld(1),
	extCmp(new CVectorBinarySetJoinComparator),
	extDeleter(extCmp),
	totalAllocatedPatterns(0),
	totalAllocatedPatternSize(0)
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

	if(p.HasMember("ReserveMemory") && p["ReserveMemory"].IsUint()) {
		extCmp->Reserve(p["ReserveMemory"].GetInt());
	}
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
	thld = max(1.0,_thld);
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

	--totalAllocatedPatterns;
	totalAllocatedPatternSize -= to_pattern(p).GetPatternMemorySize();

	delete &to_pattern(p);
}
void CStabilityCbOLocalProjectionChain::ComputeZeroProjection( CPatternList& ptrns )
{
	unique_ptr<CVectorBinarySetDescriptor,CPatternDeleter> ptrn(extCmp->NewPattern(), extDeleter);
	for( DWORD i = 0; i < GetObjectNumber(); ++i ) {
		extCmp->AddValue(i,*ptrn);
	}
	ptrns.PushBack( newPattern( ptrn.release(), intentsTree.Create(), 0, GetObjectNumber(), 0) );
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
		getAttributeImg(a,nextImage);

		// Computing the only possible preimage
		unique_ptr<const CVectorBinarySetDescriptor, CPatternDeleter> res(
			extCmp->CalculateSimilarity( p.Extent(), *nextImage ), extDeleter );
		const DWORD ptrnExtSize = p.Extent().Size();
		const DWORD resExtSize = res->Size();
		const DWORD extDiff = ptrnExtSize - resExtSize;
		if(extDiff == 0 ) {
			// Attribute is in the closure
			p.AddAttributeToIntent(a);
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
	return p.Delta() >= thld && attrs->HasAttribute(a);
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
	const CPattern& p = to_pattern(d);
	CIntentsTree::TIntentItr itr = intentsTree.GetIterator(p.Intent());
	DWORD intentSize = 0;
	while(itr != -1) {
		++intentSize;
		intentsTree.GetNextAttribute(itr);
	}
	int* attributes = new int[intentSize];
	itr = intentsTree.GetIterator(p.Intent());
	int i = intentSize - 1;
	while(itr != -1) {
		assert(i >= 0);
		attributes[i] = intentsTree.GetNextAttribute(itr);
		--i;
	}
	JSON rslt = this->attrs->DescribeAttributeSet(attributes,intentSize);
	delete[] attributes;

	return rslt;
}
size_t CStabilityCbOLocalProjectionChain::GetTotalAllocatedPatterns() const
{
	return totalAllocatedPatterns;
}
size_t CStabilityCbOLocalProjectionChain::GetTotalConsumedMemory() const
{
	return totalAllocatedPatternSize + intentsTree.MemorySize() + extCmp->GetMemoryConsumption();
}

const CPattern& CStabilityCbOLocalProjectionChain::to_pattern(const IPatternDescriptor* d) const
{
	assert(d != 0);
	return debug_cast<const CPattern&>(*d);
}

const CPattern* CStabilityCbOLocalProjectionChain::newPattern(
	const CVectorBinarySetDescriptor* ext,
	CIntentsTree::TIntent intent,
	int nextAttr, DWORD delta, int clossestAttr)
{
	const CPattern* p = new CPattern(*extCmp, ext, extDeleter,
	                    intentsTree, intent,
	                    nextAttr,delta,clossestAttr);

	++totalAllocatedPatterns;
	totalAllocatedPatternSize += p->GetPatternMemorySize();
	return p;
}
void CStabilityCbOLocalProjectionChain::getAttributeImg(int a, CSharedPtr<const CVectorBinarySetDescriptor>& rslt)
{
	if( attrsHolder.size() <= a ) {
		attrsHolder.resize(a+1);
	}
	if( attrsHolder[a]!=0) {
		rslt = attrsHolder[a];
		return;
	}
	CPatternImage img;
	attrs->GetAttribute(a, img);

	CVectorBinarySetDescriptor& ext = *extCmp->NewPattern();
	rslt.reset( &ext, extDeleter );

	for (DWORD i = 0; i < img.ImageSize; i++) {
		extCmp->AddValue( static_cast<DWORD>( img.Objects[i]), ext );
	}
	attrs->ClearMemory(img);
	attrsHolder[a] = rslt;
}
const CPattern* CStabilityCbOLocalProjectionChain::initializeNewPattern(
	  const CPattern& parent,
	  int genAttr,
	  unique_ptr<const CVectorBinarySetDescriptor, CPatternDeleter>& ext)
{
	assert( ext != 0 );
	// If delta is zero, then the attribute is in the intent.
	// However, according to canonical order it should not be there. Thus, such a pattern should also be ignored.
	assert(thld >= 1);
	DWORD delta = ext->Size();
	int minAttr = genAttr + 1;
	if( parent.ClosestChild() < genAttr ) {
		getAttributeDelta(parent.ClosestChild(), *ext);
		minAttr = parent.ClosestChild();
	}

	intentStorage.clear();
	CIntentsTree::TIntentItr itr = intentsTree.GetIterator(parent.Intent());
	while(itr != -1) {
		intentStorage.push_back(intentsTree.GetNextAttribute(itr));
	}
	auto intentItr = intentStorage.rbegin();
	auto intentEnd = intentStorage.rend();

	for(int a = 0;delta >= thld && a < genAttr && attrs->HasAttribute(a);a = attrs->GetNextNonChildAttribute(a)) {
		while(intentItr != intentEnd && a > *intentItr) {
			const DWORD ia = *intentItr;
			++intentItr;
			assert(intentItr == intentEnd || ia < *intentItr);
		}
		if(intentItr != intentEnd && a == *intentItr) {
			continue;
		}
		const DWORD aDelta = getAttributeDelta(a, *ext);
		if(aDelta < delta) {
			delta = aDelta;
			minAttr = a;
		}
	}
	if( delta < thld ) {
		return 0;
	}
	CIntentsTree::TIntent intent = intentsTree.Copy(parent.Intent());
	intent = intentsTree.AddAttribute(intent, genAttr);
	return(newPattern(ext.release(),intent,attrs->GetNextAttribute(genAttr), delta, minAttr));
}

DWORD CStabilityCbOLocalProjectionChain::getAttributeDelta(int a, const CVectorBinarySetDescriptor& ext)
{
	CSharedPtr<const CVectorBinarySetDescriptor> attr;
	getAttributeImg(a,attr);

	// Computing the only possible preimage
	CSharedPtr<const CVectorBinarySetDescriptor> res(
		extCmp->CalculateSimilarity( ext, *attr ), extDeleter );
	const DWORD extDiff = ext.Size() - res->Size();
	return extDiff;
}

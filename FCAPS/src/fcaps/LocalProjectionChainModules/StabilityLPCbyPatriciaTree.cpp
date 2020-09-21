// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8

#include <fcaps/LocalProjectionChainModules/StabilityLPCbyPatriciaTree.h>

#include <fcaps/PatternDescriptor.h>
#include <fcaps/BinContextReader.h>
#include <fcaps/Swappable.h>

#include <fcaps/SharedModulesLib/VectorBinarySetDescriptor.h>

#include <JSONTools.h>
#include <ModuleJSONTools.h>
#include <StdTools.h>

#include <set>
#include <map>
#include <stdint.h>


using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(...) #__VA_ARGS__

const char description[] =
STR(
	{
	"Name":"Stability Local Projection Chain by means of Patricia Tree",
	"Description":"Local Projection Chain that is based on Close-by-One (also known as LCM) canonical order and Patricia Tree and incorporates computation of DELTA-stability.",
	"Params": {
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of Stability Patricia Tree LPC",
			"type": "object",
			"properties": {
				"ContextReader":{
					"description": "A module for reading object intents from binary JSON context",
					"type": "@BinContextReaderModules"
				}
			}
		}
	}
);


////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CStabilityLPCbyPatriciaTree> CStabilityLPCbyPatriciaTree::registrar;

const char* const CStabilityLPCbyPatriciaTree::Desc()
{
	return description;
}

////////////////////////////////////////////////////////////////////

// class CIgnoredAttrs {
// public:
// 	void Ignore(TAttribute a) 
// 	{
// 		assert( a >= 0); 
// 		if( a >= ignoredAttrs.size()) {
// 			ignoredAttrs.resize(a+1);
// 		} 
// 		ignoredAttrs[a] = true; 
// 	}
// 	bool IsIgnored(CIntentsTree::TAttribute a) const
// 	{ return 0 <=a && a < ignoredAttrs.size() && ignoredAttrs[a]; }
// 	void Swap(CIgnoredAttrs& other) 
// 		{ ignoredAttrs.swap(other.ignoredAttrs); }
// private:
// 	std::deque<bool> ignoredAttrs;
// };

////////////////////////////////////////////////////////////////////

class CPattern : public IExtent, public IPatternDescriptor, public ISwappable {
public:
// 	// Pattern controls memory for the extent
// 	CPattern( CVectorBinarySetJoinComparator& _cmp, 
// 			const CVectorBinarySetDescriptor* e, 
// 			CPatternDeleter dlt,
// 	          CIntentsTree& iTree, TIntent i,
// 		  CIgnoredAttrs& ignored,
// 	          int nextAttr, DWORD d, int closestAttribute ) :
// 		cmp(_cmp), extent(e, dlt), swappedExtent(-1), extentSize(e->Size()), extentHash(e->Hash()),
// 		intentsTree(iTree), intent(i), nextAttribute(nextAttr), delta(d), closestChildAttribute(closestAttribute)
// 	{
// 		assert( extent != 0 );
// 		ignoredAttrs.Swap(ignored);
// 	}
// 	~CPattern()
// 	{
// 		intentsTree.Delete(intent);
// 		if( IsSwapped() ) {
// 			assert(swappedExtent != static_cast<CVectorBinarySetJoinComparator::TSwappedPattern>(-1));
// 			cmp.SwapRemove(swappedExtent);
// 			swappedExtent = -1;
// 		}
// 	}

// 	// Methods of IExtent
// 	virtual DWORD Size() const
// 		{ return extentSize;}
//     virtual void GetExtent( CPatternImage& extent ) const
// 		{initPatternImage(extent);}
// 	virtual void ClearMemory( CPatternImage& e) const
// 		{ delete[] e.Objects;}

// 	// Methos of IPatternDescriptor
// 	virtual bool IsMostGeneral() const
// 		{return intent == -1;}
// 	virtual size_t Hash() const
// 		{ return extentHash; }

// 	// Methods of ISwappable
// 	virtual bool IsSwapped() const
// 		{ return extent == 0;}
// 	virtual void Swap() const
// 	{
// 		if( extent == 0 ) {
// 			return;
// 		}
// 		swappedExtent = cmp.SwapPattern(extent.release());
// 		assert(extent == 0);
// 	}

// 	// Methods of the class
// 	const CVectorBinarySetDescriptor& Extent() const
// 		{ restore(); assert(extent != 0); return *extent;}
// 	TIntent Intent() const
// 		{return intent;}
// 	// void AddAttributeToIntent(CIntentsTree::TAttribute a) const
// 	// 	{ intent = intentsTree.AddAttribute(intent,a); ignoredAttrs.Ignore(a); }

// 	void IgnoreAttribute(CIntentsTree::TAttribute a) const
// 		{ ignoredAttrs.Ignore(a);}
// 	bool IsIgnored(CIntentsTree::TAttribute a) const
// 		{ return ignoredAttrs.IsIgnored(a); }
// 	const CIgnoredAttrs& IgnoredAttrs() const
// 		{ return ignoredAttrs;}

// 	int NextAttribute() const
// 		{return nextAttribute;}
// 	void SetNextAttribute(int a) const
// 		{nextAttribute = a;}
	DWORD Delta() const
		{return delta;}
// 	void SetDelta(DWORD d) const
// 		{assert(d <= delta); delta = d;}
// 	int ClosestChild() const
// 		{return closestChildAttribute;}
// 	void SetClosestChild( int a ) const
// 		{closestChildAttribute = a;}

// 	// TOKILL
	size_t GetPatternMemorySize() const
		{ return sizeof(CPattern); }


// private:
// 	CVectorBinarySetJoinComparator& cmp;
// 	mutable unique_ptr<const CVectorBinarySetDescriptor, CPatternDeleter> extent;
// 	mutable CVectorBinarySetJoinComparator::TSwappedPattern swappedExtent;
// 	const DWORD extentSize;
// 	const DWORD extentHash;

// 	CIntentsTree& intentsTree;
// 	mutable TIntent intent;
// 	mutable CIgnoredAttrs ignoredAttrs;

// 	// The minimal number of the attribute that can be added to the pattern.
// 	// This number also gives the reference to the "core" of the pattern in the CbO canonical order
// 	mutable int nextAttribute;
// 	// The delta measure of the pattern w.r.t. the projection not including the @var nextAttribute attribute
	mutable DWORD delta;
// 	// The attribute of a clossest child. In intersection of the pattern and the attribute the result is the clossest child
// 	// Used as an optimization for earlier detection of new unstable patterns
// 	mutable int closestChildAttribute;

// 	void initPatternImage(CPatternImage& img) const {
// 		img.PatternId = Hash();
// 		img.ImageSize = Extent().Size();
// 		img.Objects = 0;

// 		unique_ptr<int[]> objects (new int[img.ImageSize]);
// 		img.Objects = objects.get(); 
// 		cmp.EnumValues(Extent(), objects.get(), img.ImageSize);
// 		objects.release(); 
// 	}
// 	void restore() const {
// 		if( extent != 0 ) {
// 			return;
// 		}
// 		assert(swappedExtent != static_cast<CVectorBinarySetJoinComparator::TSwappedPattern>(-1));
// 		extent.reset(cmp.SwapRestore(swappedExtent));
// 		swappedExtent = -1;
// 		assert(extent != 0);
// 	}
};

////////////////////////////////////////////////////////////////////

CStabilityLPCbyPatriciaTree::CStabilityLPCbyPatriciaTree() :
	thld(1),
	extCmp(new CVectorBinarySetJoinComparator),
	extDeleter(extCmp),
	totalAllocatedPatterns(0),
	totalAllocatedPatternSize(0)
{
}
CStabilityLPCbyPatriciaTree::~CStabilityLPCbyPatriciaTree()
{
	for(auto i = attrsHolder.begin(); i != attrsHolder.end(); ++i) {
		extDeleter(*i);
		*i = 0;
	}
}

void CStabilityLPCbyPatriciaTree::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CStabilityLPCbyPatriciaTree::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == Type() );
	assert( string( params["Name"].GetString() ) == Name() );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for Context Attributes object";
		throw new CJsonException("CStabilityLPCbyPatriciaTree::LoadParams", error);
	}

	const rapidjson::Value& p = params["Params"];

	if(!(p.HasMember("ContextReader") && (p["ContextReader"].IsObject()))) {
		error.Data = json;
		error.Error = "Params.ContextReader is not found or is not an object.";
		throw new CJsonException("CStabilityLPCbyPatriciaTree::LoadParams", error);
	}
	const rapidjson::Value& pe = params["Params"]["ContextReader"];
	string errorText;
	attrs.reset( CreateModuleFromJSON<IBinContextReader>(pe,errorText) );
	if( pe == 0 ) {
		throw new CJsonException( "CStabilityLPCbyPatriciaTree::LoadParams", CJsonError( json, errorText ) );
	}
	extCmp->SetMaxAttrNumber(attrs->GetObjectNumber());

	buildPatritiaTree();

	// Debuging
	// cout << "Object Number " << attrs->GetObjectNumber() << endl;
	// int a = 0;
	// while(attrs->HasAttribute(a)) {
	// 	cout << a << " " << attrs->GetAttributeName(a) << " (" << attrs->GetSupport(a) << ")" << endl;
	// 	++a;
	// }
	// cout << endl;


	// // Starting reading the context object by object
	// attrs->Start();

	// IBinContextReader::CObjectIntent intent;
	// vector<int> buffer;

	// while( true ) {
	// 	intent.Size = attrs->GetNextObjectIntentSize();
	// 	if(intent.Size < 0) {
	// 		break;
	// 	}
	// 	buffer.resize(intent.Size);
	// 	intent.Attributes = buffer.data();

	// 	attrs->GetNextObject(intent);

	// 	assert(intent.Size <= buffer.size());

	// 	cout << "Intent: ";
	// 	for(int i = 0; i < intent.Size; ++i) {
	// 		assert(attrs->HasAttribute(intent.Attributes[i]));
	// 		cout << attrs->GetAttributeName(intent.Attributes[i]) << " ";
	// 	}
	// 	cout << endl;
	// }


	
	throw new CTextException("STOP","STOP");
}

JSON CStabilityLPCbyPatriciaTree::SaveParams() const
{
	// rapidjson::Document params;
	// rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	// params.SetObject()
	// 	.AddMember( "Type", rapidjson::StringRef(Type()), alloc )
	// 	.AddMember( "Name", rapidjson::StringRef(Name()), alloc )
	// 	.AddMember( "Params", rapidjson::Value().SetObject()
	// 	            .AddMember("AllAttributesInOnce",rapidjson::Value(areAllInOnce),alloc), alloc );

	// IModule* m = dynamic_cast<IModule*>(attrs.get());

	// JSON peParams;
	// rapidjson::Document peParamsDoc;
    // assert( m!=0);
	// if( m != 0 ) {
	// 	peParams = m->SaveParams();
	// 	CJsonError error;
	// 	const bool rslt = ReadJsonString( peParams, peParamsDoc, error );
	// 	assert(rslt);
	// 	params["Params"].AddMember("ContextAttributes", peParamsDoc.Move(), alloc );
	// }

	JSON result;
	// CreateStringFromJSON( params, result );
	return result;
}

int CStabilityLPCbyPatriciaTree::GetObjectNumber() const
{
	assert(attrs != 0);
	return attrs->GetObjectNumber();
}
double CStabilityLPCbyPatriciaTree::GetInterestThreshold() const
{
	return thld;
}
void CStabilityLPCbyPatriciaTree::UpdateInterestThreshold( const double& _thld )
{
	thld = max(1.0,_thld);
}
double CStabilityLPCbyPatriciaTree::GetPatternInterest( const IPatternDescriptor* p )
{
	return to_pattern(p).Delta();
}
bool CStabilityLPCbyPatriciaTree::IsTopoSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{
	return to_pattern(p).Size() > to_pattern(q).Size() || to_pattern(p).Size() == to_pattern(q).Size() && to_pattern(p).Hash() > to_pattern(q).Hash();
}
void CStabilityLPCbyPatriciaTree::FreePattern(const IPatternDescriptor* p ) const
{

	--totalAllocatedPatterns;
	totalAllocatedPatternSize -= to_pattern(p).GetPatternMemorySize();

	delete &to_pattern(p);
}
void CStabilityLPCbyPatriciaTree::ComputeZeroProjection( CPatternList& ptrns )
{
	// unique_ptr<CVectorBinarySetDescriptor,CPatternDeleter> ptrn(extCmp->NewPattern(), extDeleter);
	// for( DWORD i = 0; i < GetObjectNumber(); ++i ) {
	// 	extCmp->AddValue(i,*ptrn);
	// }
	// CIgnoredAttrs dummyIgnored;
	// ptrns.PushBack( newPattern( ptrn.release(), intentsTree.Create(),
	// 			dummyIgnored,
	// 			0, GetObjectNumber(), -1) );
}

ILocalProjectionChain::TPreimageResult CStabilityLPCbyPatriciaTree::Preimages( const IPatternDescriptor* d, CPatternList& preimages )
{
	// assert( attrs != 0);

	// const CPattern& p = to_pattern(d);
    // assert(p.Delta() >= thld);

	// int a = p.NextAttribute();
	// CPatternImage img;
	// while(attrs->HasAttribute(a)){
	// 	if( p.Delta() < thld) {
	// 		// Unstable concep cannot probuce stable concepts
	// 		break;
	// 	}

	// 	if(p.IsIgnored(a)){
	// 		a = attrs->GetNextAttribute(a);
	// 		continue;
	// 	}
	// 	// Getting next attribute
	// 	const CVectorBinarySetDescriptor& nextImage = *getAttributeImg(a);

	// 	// Computing the only possible preimage
	// 	unique_ptr<const CVectorBinarySetDescriptor, CPatternDeleter> res(
	// 		extCmp->CalculateSimilarity( p.Extent(), nextImage ), extDeleter );
	// 	const DWORD ptrnExtSize = p.Extent().Size();
	// 	const DWORD resExtSize = res->Size();
	// 	const DWORD extDiff = ptrnExtSize - resExtSize;
	// 	if(extDiff == 0 ) {
	// 		// Attribute is in the closure
	// 		p.AddAttributeToIntent(a);
	// 		a = attrs->GetNextAttribute(a);
	// 		continue;
	// 	}

	// 	unique_ptr<const CPattern> newPtrn( initializeNewPattern( p, a, res ) );

	// 	// Updating the measure of the current pattern.
	// 	//    It is here, because initializeNewPattern relies on the p.GetClossestChild()
	// 	if( p.Delta() > extDiff ) {
	// 		p.SetDelta(extDiff);
	// 		p.SetClosestChild(a);
	// 	}

	// 	if( newPtrn == 0 ) {
	// 		// Pattern 'res' is not stable
	// 		// Any more specific attribute can be ignored
	// 		a = attrs->GetNextNonChildAttribute(a);
	// 		// TODO: normally all this attributes will be in the closure of any found children and thus there is no needs to intersect them  with this attribute
	// 		// Should search funther for a better pattern
	// 		continue;
	// 	}
		
	// 	// A new stable pattern is generated
	// 	//  We should add it to preimages.
	// 	assert(newPtrn->Delta() >= thld);
	// 	preimages.PushBack( newPtrn.release() );

	// 	a = attrs->GetNextAttribute(a);
	// 	if(!areAllInOnce) {
	// 		break;
	// 	}
	// }

	// p.SetNextAttribute(a);
	// const bool isStable = p.Delta() >= thld;
	// if(!attrs->HasAttribute(a)){
	// 	return isStable ? PR_Finished : PR_Uninteresting;
	// } else {
	// 	return isStable ? PR_Expandable : PR_Uninteresting;
	// }

	return PR_Uninteresting;
}
bool CStabilityLPCbyPatriciaTree::IsExpandable( const IPatternDescriptor* d ) const
{
	// assert(attrs != 0);

	// const CPattern& p = to_pattern(d);
    // assert(p.Delta() >= thld);
    // return attrs->HasAttribute(p.NextAttribute());
	return false;
}
int CStabilityLPCbyPatriciaTree::GetExtentSize( const IPatternDescriptor* d ) const
{
	return 0;//to_pattern(d).Extent().Size();
}
JSON CStabilityLPCbyPatriciaTree::SaveExtent( const IPatternDescriptor* d ) const
{
	return "";//extCmp->SavePattern( &to_pattern(d).Extent() );
}
JSON CStabilityLPCbyPatriciaTree::SaveIntent( const IPatternDescriptor* d ) const
{
	// const CPattern& p = to_pattern(d);
	// CIntentsTree::TIntentItr itr = intentsTree.GetIterator(p.Intent());
	// DWORD intentSize = 0;
	// while(itr != -1) {
	// 	++intentSize;
	// 	intentsTree.GetNextAttribute(itr);
	// }
	// int* attributes = new int[intentSize];
	// itr = intentsTree.GetIterator(p.Intent());
	// int i = intentSize - 1;
	// while(itr != -1) {
	// 	assert(i >= 0);
	// 	attributes[i] = intentsTree.GetNextAttribute(itr);
	// 	--i;
	// }
	JSON rslt;// = this->attrs->DescribeAttributeSet(attributes,intentSize);
	// delete[] attributes;

	return rslt;
}
size_t CStabilityLPCbyPatriciaTree::GetTotalAllocatedPatterns() const
{
	return totalAllocatedPatterns;
}
size_t CStabilityLPCbyPatriciaTree::GetTotalConsumedMemory() const
{
	return totalAllocatedPatternSize + 0 + extCmp->GetMemoryConsumption();
}

void CStabilityLPCbyPatriciaTree::buildPatritiaTree()
{
	// Starting reading the context object by object
	attrs->Start();

	IBinContextReader::CObjectIntent intent;
	vector<int> buffer;

	// Insertion of all objects to partitia tree
	// The correspondance between nodes in the patritia tree and the objectID
	std::multimap<CPatritiaTree::TNodeIndex, CPatritiaTree::TObject> nodeToObjectMap;
	for(int objectId = 0; true; ++objectId) {
		intent.Size = attrs->GetNextObjectIntentSize();
		if(intent.Size < 0) {
			// end of object iteration
			break;
		}
		buffer.resize(intent.Size);
		intent.Attributes = buffer.data();

		attrs->GetNextObject(intent);

		assert(intent.Size == buffer.size());

		sort(buffer.begin(), buffer.end());

		CPatritiaTree::TNodeIndex nodeId =  pTree.GetRoot(); // Root node with no generator attribute

		for( auto i = buffer.begin(); i!= buffer.end(); ++i) {
			nodeId = pTree.GetAttributeNode(nodeId, *i);
		}
		nodeToObjectMap.insert(std::pair<CPatritiaTree::TNodeIndex, CPatritiaTree::TObject>( nodeId, objectId ));
	}

	// Now we will add objects to the nodes
	
	CDeepFirstPatritiaTreeIterator treeItr(pTree);
	for(; !treeItr.IsEnd(); ++treeItr) {
		if( treeItr.Status() != CDeepFirstPatritiaTreeIterator::S_Exit) {
			// The objects are collected on exit from the node
			continue;
		}
		auto objItrs = nodeToObjectMap.equal_range(*treeItr);
		std::pair<int,int> inserted(-1,-2);
		for(auto obj = objItrs.first; obj != objItrs.second; ++obj) {
			const int objId = pTree.AddObject( obj->second );
			inserted.first = inserted.first < 0 ? objId : inserted.first;
			inserted.second = objId + 1;
		}

		if(!treeItr->Children.empty()) {
			const CPatritiaTree::TNodeIndex firstChildId = *treeItr->Children.begin();
			const CPatritiaTree::CNode& firstChild =  pTree.GetNode(firstChildId);
			assert(inserted.first == -1 || inserted.first >= firstChild.ObjStart);
			treeItr->ObjStart = firstChild.ObjStart;
			auto lastChItr = treeItr->Children.end();
			--lastChItr;
			treeItr->ObjEnd = max(inserted.second, pTree.GetNode(*lastChItr).ObjEnd);
		} else {
			treeItr->ObjStart = inserted.first;
			treeItr->ObjEnd = inserted.second;
		}
	}

	pTree.Print();
	
	// The set of previously processed attibutes
	std::deque<CPatritiaTree::TAttribute> childrenCAttrs;
	std::list<CPatritiaTree::TAttribute> currentNodeAttrs;
	// Compressing of the Tree
    //  Now all attributes that are in the closure should be added to the node and the corresponding tree should be removed
	treeItr.Reset(pTree);
	for(; !treeItr.IsEnd(); ++treeItr) {
		if( treeItr.Status() != CDeepFirstPatritiaTreeIterator::S_Exit) {
			continue;
		}
		// Status is exit and thus all children are compressed
		auto& children = treeItr->Children;
		auto ch = children.rbegin();
		if( ch == children.rend() ) {
			// No children, i.e., all attributes are described
			continue;
		}

		// Now if there are objects associated with the node, then these objects do not have other attributes and thus there is no attributes in the closure.
		if( nodeToObjectMap.find(*treeItr) != nodeToObjectMap.end() ) {
			continue;
		}
		// Only the attributes after the last one can be in the closure,
		// since the previous children are generated with the attribute which definetly upson in the last children
		CPatritiaTree::TNodeIndex lastNodeId = *ch;
		auto& lastNode = pTree.GetNode(lastNodeId);
		currentNodeAttrs.clear();
		// Adding the generated attribute for the last children.
		// It is the only possible generated attribute among the children that can be common to all of children
		currentNodeAttrs.push_back(lastNode.GenAttr);
		// Adding all attributes for the last node as candidates.
		for(int i = lastNode.ClosureAttrStart; i < lastNode.ClosureAttrEnd; ++i) {
			assert(currentNodeAttrs.empty() || 0 <= i && childrenCAttrs.size() && currentNodeAttrs.back() < childrenCAttrs[i]);
			currentNodeAttrs.push_back(childrenCAttrs[i]);
		}
		// A flag indicating if all attributes from the last node are presented in prevous children
		bool areAllLastChildAttributesCommon = true;
		// Now we itearate over the previous children and compute the common attributes
		++ch; // Now it is the last but one child
		for(; ch != children.rend(); ++ch) {
			auto attrItr = currentNodeAttrs.begin();
			const CPatritiaTree::TNodeIndex chId = *ch;
			const CPatritiaTree::CNode& chNode = pTree.GetNode(chId);
			int i = chNode.ClosureAttrStart;
			for(; attrItr != currentNodeAttrs.end(); ) {
				auto a = attrItr;
				++attrItr;

				// Skiping all attributes that are missed in the current child
				while( i < chNode.ClosureAttrEnd && childrenCAttrs[i] < *a) {
					++i;
				}

				if( i >= chNode.ClosureAttrEnd || childrenCAttrs[i] != *a ) {
					// The attribute a is missed from the current child
					currentNodeAttrs.erase(a);
					areAllLastChildAttributesCommon = false;
					continue;
				}

				assert(childrenCAttrs[i] == *a);
				// The attribute can  be potentailly common just continue
			}
		}

		// Now currentNodeAttrs has all common attributes
		assert(treeItr->ClosureAttrStart >= treeItr->ClosureAttrEnd); // No attributes should be in the closure for this node.
		const int commonAttributeNum = currentNodeAttrs.size();

		// Saving attributes in the closure.
		treeItr->ClosureAttrStart = childrenCAttrs.size();
		for( auto aa = currentNodeAttrs.begin(); aa != currentNodeAttrs.end(); ++aa) {
			childrenCAttrs.push_back(*aa);
		}
		treeItr->ClosureAttrEnd = childrenCAttrs.size();

		ch = children.rbegin();
		assert( ch != children.rend());
		if(areAllLastChildAttributesCommon) {
			++ch; // The last node will be removed later
		}

		// Now for all children (sometimes but the last one) we collect the attributes in the closure and register them in the tree
		for( ; ch != children.rend(); ++ch) {
			const CPatritiaTree::TNodeIndex chId = *ch;
			CPatritiaTree::CNode& chNode = pTree.GetNode(chId);
			int clsAttrInds = chNode.ClosureAttrStart;
			const int clsAttrEnd = chNode.ClosureAttrEnd;
			if( clsAttrInds >= clsAttrEnd ) {
				continue;
			}
			chNode.ClosureAttrStart = pTree.AddAttribute(childrenCAttrs.at(clsAttrInds));
			chNode.ClosureAttrEnd = chNode.ClosureAttrStart;
			++clsAttrInds;
			for(; clsAttrInds < clsAttrEnd; ++clsAttrInds) {
				chNode.ClosureAttrEnd = pTree.AddAttribute(childrenCAttrs.at(clsAttrInds));
			}
			++chNode.ClosureAttrEnd;
			assert(chNode.ClosureAttrStart < chNode.ClosureAttrEnd);
		}

		if( areAllLastChildAttributesCommon ) {
			// Then the last child should be merged with the current node
			auto tmpItr = treeItr->Children.end();
			--tmpItr;
			treeItr->Children.insert(lastNode.Children.begin(),lastNode.Children.end());

			lastNode.Clear();
			treeItr->Children.erase(tmpItr);
		}
	}

	pTree.Print();
	
}

const CPattern& CStabilityLPCbyPatriciaTree::to_pattern(const IPatternDescriptor* d) const
{
	assert(d != 0);
	return debug_cast<const CPattern&>(*d);
}
const CVectorBinarySetDescriptor* CStabilityLPCbyPatriciaTree::getAttributeImg(int a)
{
	if( attrsHolder.size() <= a ) {
		attrsHolder.resize(a+1,0);
	}
	if( attrsHolder[a]!=0) {
		return attrsHolder[a];
	}

	// CVectorBinarySetDescriptor* ext = extCmp->NewPattern();
	// assert(attrsHolder[a]==0);
	// attrsHolder[a] = ext;

	// CPatternImage img;
	// attrs->GetAttribute(a, img);
	// for (DWORD i = 0; i < img.ImageSize; i++) {
	// 	extCmp->AddValue( static_cast<DWORD>( img.Objects[i]), *ext );
	// }
	// attrs->ClearMemory(img);

	// return ext;
	return 0;
}
const CPattern* CStabilityLPCbyPatriciaTree::initializeNewPattern(
	  const CPattern& parent,
	  int genAttr,
	  std::unique_ptr<const CVectorBinarySetDescriptor,CPatternDeleter>& ext)
{
	// assert( ext != 0 );

	// // The intent of the new concept
	// CIntentsTree::TIntent intent = intentsTree.Copy(parent.Intent());
	// CIgnoredAttrs extIgnoredAttrs(parent.IgnoredAttrs());

	// intent = intentsTree.AddAttribute(intent, genAttr);
	// extIgnoredAttrs.Ignore(genAttr);
	// // The ignored attributes for the new concept

	// // First we should close the description of ext.
	// bool canBeUnclosed=false;
	// while(canBeUnclosed){
	// 	canBeUnclosed=false;
	// 	int a = attrs->GetNextAttribute(genAttr);
	// 	for(; attrs->HasAttribute(a); a = attrs->GetNextAttribute(a)) {
	// 		if(extIgnoredAttrs.IsIgnored(a)) {
	// 			continue;
	// 		}
	// 		const CVectorBinarySetDescriptor& attr = *getAttributeImg(a);
	// 		unique_ptr<const CVectorBinarySetDescriptor,CPatternDeleter> res(
	// 				extCmp->CalculateSimilarity( *ext, attr ), extDeleter);
	// 		const DWORD extDiff = ext->Size() - res->Size();
	// 		// TODO
	// 		// if( res->Size() < thld ) {
	// 		// ??? could we ignore it somehow
	// 		// }
	// 		if( res->Size() == 0 ) {
	// 			// can be ignored
	// 			extIgnoredAttrs.Ignore(a);
	// 		}
	// 		if(extDiff > thld ) {
	// 			// not in the closure wrt Delta
	// 			continue;
	// 		}
	// 		canBeUnclosed = true;

	// 		if( extDiff > 0 ) {
	// 			ext.reset(res.release());
	// 		}
	// 		extIgnoredAttrs.Ignore(a);
	// 		intent = intentsTree.AddAttribute(intent, a);
	// 	}
	// }


	// // If delta is zero, then the attribute is in the intent.
	// // However, according to canonical order it should not be there. Thus, such a pattern should also be ignored.
	// assert(thld >= 1);
	// DWORD delta = ext->Size();
	// int minAttr = -1;
	// if( parent.ClosestChild() >= 0 ) {
	// 	assert(!parent.IsIgnored(parent.ClosestChild()));
	// 	delta = getAttributeDelta(parent.ClosestChild(), *ext);
	// 	minAttr = parent.ClosestChild();
	// }

	// // intentStorage.clear();
	// // CIntentsTree::TIntentItr itr = intentsTree.GetIterator(parent.Intent());
	// // while(itr != -1) {
	// 	// intentStorage.push_back(intentsTree.GetNextAttribute(itr));
	// // }
	// // auto intentItr = intentStorage.rbegin();
	// // auto intentEnd = intentStorage.rend();

	// for(int a = 0; delta >= thld && a < genAttr && attrs->HasAttribute(a); a = attrs->GetNextNonChildAttribute(a)) {
	// 	if(extIgnoredAttrs.IsIgnored(a)) {
	// 		continue;
	// 	}

	// 	const DWORD aDelta = getAttributeDelta(a, *ext);
	// 	if(aDelta < delta) {
	// 		delta = aDelta;
	// 		minAttr = a;
	// 	}
	// 	if( delta < thld ) {
	// 		intentsTree.Delete(intent);
	// 		return 0;
	// 	}
	// }
	// if( delta < thld ) {
	// 	intentsTree.Delete(intent);
	// 	return 0;
	// }
	// return(newPattern(ext.release(), intent, extIgnoredAttrs,
	// 			attrs->GetNextAttribute(genAttr), delta, minAttr));

	return 0;
}

DWORD CStabilityLPCbyPatriciaTree::getAttributeDelta(int a, const CVectorBinarySetDescriptor& ext)
{
	const CVectorBinarySetDescriptor& attr = *getAttributeImg(a);

	// Computing the only possible preimage
	CSharedPtr<const CVectorBinarySetDescriptor> res(
		extCmp->CalculateSimilarity( ext, attr ), extDeleter );
	const DWORD extDiff = ext.Size() - res->Size();
	return extDiff;
}

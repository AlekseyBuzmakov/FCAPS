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
#include <deque>
#include <algorithm>
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
								   },
								   "AllAttributesInOnce": {
														   "description": "When called to compute next projection should it be generated only one 'next' pattern with one attribute, or all of them",
														   "type": "boolean"
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

struct CAttrIntersectionIntent {
	// The attribute to intersect with
	CPatritiaTree::TAttribute Attr;
	// The extent
	DWORD ExtentSize;
	// The flag indicating that this attribute is in the kernel
	bool IsInKernel;

	CAttrIntersectionIntent	():
		Attr(-1), ExtentSize(0), IsInKernel(false) {}
	CAttrIntersectionIntent	( CPatritiaTree::TAttribute a, DWORD size, bool isK):
		Attr(a), ExtentSize(size), IsInKernel(isK) {}

	// Comparison operator for multimap
	bool operator < (const CAttrIntersectionIntent& other) const
	{ return IsInKernel > other.IsInKernel || IsInKernel == other.IsInKernel && ExtentSize > other.ExtentSize; }
};
////////////////////////////////////////////////////////////////////

class CPTPattern : public IExtent, public IPatternDescriptor, public ISwappable {
public:
	typedef list<const CPatritiaTreeNode*, CountingAllocator<const CPatritiaTreeNode*> > CExtentHolder;
	struct CIntentAttribute{
		friend CPTPattern; // For accessing the counter

		// The link to the previous attribute
		CIntentAttribute* Prev;
		// The attribute value
		CPatritiaTree::TAttribute Attr;

		CIntentAttribute(): Prev(0), Attr(-1), counter(1) {}
		CIntentAttribute(CIntentAttribute* prev, CPatritiaTree::TAttribute a): Prev(prev), Attr(a), counter(1) {}

	private:
		mutable int counter;
	};

public:
	CPTPattern(CPatritiaTree& _pTree, CMemoryCounter& _cnt) :
		pTree(_pTree),
		extent( CountingAllocator<const CPatritiaTreeNode*>(_cnt) ),
		extentSize(0),
		extentHash(0),
		kernelAttr(0),
		clossestAttr( - 1),
		delta(-1),
		intent(0)
	{}
	~CPTPattern()
	{
		while(intent != 0) {
			assert(intent->counter > 0);
			--intent->counter;
			if(intent->counter > 0) {
				break;
			}
			CIntentAttribute* prev = intent->Prev;
			delete intent;
			intent = prev;
		}
	}

	// Methods of IExtent
	virtual DWORD Size() const
	{ return extentSize;}
	virtual void GetExtent( CPatternImage& extent ) const
	{initPatternImage(extent);}
	virtual void ClearMemory( CPatternImage& e) const
	{ delete[] e.Objects;}

	// Methos of IPatternDescriptor
	virtual bool IsMostGeneral() const
	{ return extent.empty() || extent.front()->GetParent() == -1; }
	virtual size_t Hash() const
	{ return extentHash; }

	// Methods of ISwappable
	virtual bool IsSwapped() const
	{ return false;}
	virtual void Swap() const
	{
		// Nothing for the moment
	}

	// Methods of the class
	// Adding new Patritia tree node to the extent
	void AddPTNode(const CPatritiaTreeNode& node)
	{
		assert(extent.empty() || extent.back()->ObjEnd <= node.ObjStart && node.ObjStart < node.ObjEnd);
		extent.push_back(&node);
		extentSize += node.ObjEnd - node.ObjStart;
		extentHash ^= node.ObjStart ^ (node.ObjEnd << 16);
	}
	CExtentHolder::const_iterator Begin() const { return extent.begin(); }
	CExtentHolder::const_iterator End() const { return extent.end(); }

	// Get/Set the kernel attribute
	CPatritiaTree::TAttribute GetKernelAttribute() const
	{ return attrIntersection.begin()->Attr; }
	bool HasKernelAttribute() const
	{ return !attrIntersection.begin()->IsInKernel; }
	void MoveAttributeToKernel(CPatritiaTree::TAttribute a) const
	{
		assert(!attrIntersection.empty());
		auto itr = attrIntersection.begin();
		assert(a == itr->Attr);
		if(a != itr->Attr) {
			for(; itr != attrIntersection.end(); ++itr) {
				if( itr->Attr == a) {
					break;
				}
			}
			assert(itr != attrIntersection.end());
		}
		CAttrIntersectionIntent newAttr = *itr;
		assert(!newAttr.IsInKernel);
		newAttr.IsInKernel = true;

		attrIntersection.erase(itr);
		attrIntersection.insert(newAttr);
	}
	void SetKernelAttribute( CPatritiaTree::TAttribute a ) const
	{
		// Now we should skip all attributes that are in the closure and all atributes that colapse the pattern to zero
		const CPatritiaTreeNode& root = pTree.GetNode(pTree.GetRoot());
		int attrNum = root.NextAttributeIntersections.rend()->first + 1;
		if(!root.CommonAttributes.empty()) {
			attrNum = max(attrNum, *root.CommonAttributes.rend());
		}
		CPatritiaTree::TAttribute minAllRemovedAttr = attrNum;
		for( auto itr = extent.begin(); itr != extent.end(); ++itr ) {
			auto firstCommonAttr = (*itr)->CommonAttributes.upper_bound(a-1);
			if( firstCommonAttr != (*itr)->CommonAttributes.end()) {
				minAllRemovedAttr = min( minAllRemovedAttr, *firstCommonAttr - 1);
			}
			auto firstIntersection = (*itr)->NextAttributeIntersections.upper_bound(a-1);
			if( firstIntersection != (*itr)->NextAttributeIntersections.end() ) {
				minAllRemovedAttr = min( minAllRemovedAttr, firstIntersection->first - 1);
			}
			if(minAllRemovedAttr < a) {
				assert(minAllRemovedAttr == a - 1);
				break;
			}
		}
		kernelAttr = max(a, minAllRemovedAttr + 1);
	}

	// Get/Set the closest child attribute
	CPatritiaTree::TAttribute GetClosestAttribute() const
	{ return clossestAttr; }
	void SetClosestAttribute( CPatritiaTree::TAttribute a ) const
	{ clossestAttr = a; }

	// Get/Set the delta value of the pattern
	DWORD Delta() const
	{return delta;}
	void SetDelta(DWORD d) const
	{assert(d <= delta); delta = d;}

	// Checks if an attribute cannot extent the pattern
	bool IsIgnored( CPatritiaTree::TAttribute a ) const
	{ return false; }
	// Copies intent from another node
	void CopyIntent( const CPTPattern& other ) const {
		intent = other.intent;
		if(intent != 0) {
			++intent->counter;
		}
	}
	// Adding new attribute to the intent
	void AddNewAttributeToIntent(CPatritiaTree::TAttribute a) const
	{
		// assert(intent==0 || intent->Attr < a); 
		intent = new CIntentAttribute(intent, a);
	}
	const CIntentAttribute* GetLastIntentAttribute() const
	{ return intent; }

	// Computes the intersection of the pattern given in extent with every attribute
	//  The parent pattern can be passed as a parameter
	//  For the Root node
	bool ComputeAttributeIntersections()
	{
		vector<int> attributeExtents;
		computeAttrIntersection(attributeExtents);
		for(int i = 0; i < attributeExtents.size(); ++i) {
			if( !registerAttrExtentSize(i, attributeExtents[i], false) ) {
				assert(false);
				return false;
			}
		}
		return true;
	}
	//  For the specified node
	bool ComputeAttributeIntersections( const CPTPattern& other )
	{
		vector<int> attributeExtents;
		computeAttrIntersection(attributeExtents);
		for(auto itr = other.attrIntersection.begin(); itr != other.attrIntersection.end(); ++itr) {
			assert(attributeExtents[itr->Attr] <= itr->ExtentSize);
			if( !registerAttrExtentSize(itr->Attr, attributeExtents[itr->Attr], itr->IsInKernel ) ) {
				return false;
			}
		}
		return true;
	}
private:
	CPatritiaTree& pTree;

	DWORD extentSize;
	DWORD extentHash;

	CExtentHolder extent;
	// The minimal attribute number that can be added to the pattern
	mutable CPatritiaTree::TAttribute kernelAttr;

	// The delta measure of the pattern w.r.t. the projection not including the @var nextAttribute attribute
	mutable DWORD delta;
	// The attribute of a clossest child. In intersection of the pattern and the attribute the result is the clossest child
	// Used as an optimization for earlier detection of new unstable patterns
	mutable int clossestAttr;

	// The reference to the last attribute in the intent
	mutable CIntentAttribute* intent;

	// The intersection of the pattern with its attributes
	mutable multiset<CAttrIntersectionIntent> attrIntersection;

	void initPatternImage(CPatternImage& img) const {
		img.PatternId = Hash();
		img.ImageSize = Size();
		img.Objects = 0;

		unique_ptr<int[]> objects (new int[img.ImageSize]);
		img.Objects = objects.get(); 
		int lastObj = -1;
		int currentImgPos = 0;
		auto itr = extent.begin();
		for( ; itr != extent.end(); ++itr) {
			assert(lastObj <= (*itr)->ObjStart);
			assert((*itr)->ObjStart < (*itr)->ObjEnd);
			for( lastObj = (*itr)->ObjStart; lastObj < (*itr)->ObjEnd; ++lastObj ) {
				assert(currentImgPos < img.ImageSize);
				CPatritiaTree::TObject obj = pTree.GetObject(lastObj);
				objects[currentImgPos] = obj;
				++currentImgPos;
			} 
		}
		assert(currentImgPos == img.ImageSize);

		objects.release(); 
	}
	// computes the extent size after intersection with any attribute
	void computeAttrIntersection(vector<int>& attributeExtents)
	{
		delta = extentSize;

		attributeExtents.clear();
		attributeExtents.resize(pTree.GetAttrNumber(), 0);
		for( auto ptNode = extent.begin(); ptNode != extent.end(); ++ptNode) {
			CDeepFirstPatritiaTreeIterator treeItr(pTree, const_cast<CPatritiaTreeNode&>(**ptNode) ); // CONST CAST is bad here should be another iterator for const trees
			// Processing the starting node
			for(auto a = treeItr->CommonAttributes.begin(); a != treeItr->CommonAttributes.end(); ++a) {
				attributeExtents[*a] += treeItr->ObjEnd - treeItr->ObjStart;
				assert(attributeExtents[*a] <= this->extentSize);
			}
			++treeItr;

			// Processing all other nodes
			for(; !treeItr.IsEnd(); ++treeItr) {
				if( treeItr.Status() != CDeepFirstPatritiaTreeIterator::S_Forward ) {
					// Process only forward pass
					continue;
				}

				const int objectCount = treeItr->ObjEnd - treeItr->ObjStart;
				assert(objectCount > 0);
				attributeExtents[treeItr->GenAttr] += objectCount;
				assert(attributeExtents[treeItr->GenAttr] <= this->extentSize);
				for(int i = treeItr->ClosureAttrStart; i < treeItr->ClosureAttrEnd; ++i) {
					attributeExtents[pTree.GetClsAttribute(i)] += objectCount;
					assert(attributeExtents[pTree.GetClsAttribute(i)] <= this->extentSize);
				}
			}

		}
	}
	// Add attributes to attrIntersection
	bool registerAttrExtentSize(CPatritiaTree::TAttribute a, DWORD extentSize, bool isInKernel )
	{
		assert(extentSize <= this->extentSize );
		if( extentSize == this->extentSize) {
			// The attribute is in the closure, so should not add this to possible extensions

			// kernel attributes are not allowed to be in the closure
			return !isInKernel;
		}
		if(extentSize == 0) {
			return true;
		}

		if( isInKernel ) {
			delta = min(delta, this->extentSize - extentSize);
		}
		attrIntersection.insert(CAttrIntersectionIntent(a, extentSize, isInKernel));
		return true;
	}

};

////////////////////////////////////////////////////////////////////

CStabilityLPCbyPatriciaTree::CStabilityLPCbyPatriciaTree() :
	maxAttribute(0),
	thld(1),
	areAllInOnce( false ),
	totalAllocatedPatterns(0)
{
}
CStabilityLPCbyPatriciaTree::~CStabilityLPCbyPatriciaTree()
{
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

	buildPatritiaTree2();

	// pTree.Print();

	assert( checkPTValidity() );

	assert(pTree.GetNode(pTree.GetRoot()).ObjStart == 0);
	assert(pTree.GetNode(pTree.GetRoot()).ObjEnd == attrs->GetObjectNumber());

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
}

JSON CStabilityLPCbyPatriciaTree::SaveParams() const
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
		params["Params"].AddMember("ContextReader", peParamsDoc.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
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

	delete &to_pattern(p);
}
void CStabilityLPCbyPatriciaTree::ComputeZeroProjection( CPatternList& ptrns )
{
	unique_ptr<CPTPattern> ptrnHolder( new CPTPattern(pTree, memoryCounter));
	ptrnHolder->AddPTNode(pTree.GetNode(pTree.GetRoot()));
	ptrnHolder->SetDelta(ptrnHolder->Size());
	const bool res = ptrnHolder->ComputeAttributeIntersections();
	assert(res);
	ptrns.PushBack( ptrnHolder.release() );
	++totalAllocatedPatterns;
}

ILocalProjectionChain::TPreimageResult CStabilityLPCbyPatriciaTree::Preimages( const IPatternDescriptor* d, CPatternList& preimages )
{
	assert( attrs != 0);

	const CPTPattern& p = to_pattern(d);
	assert(p.Delta() >= thld);

	int a = p.GetKernelAttribute();
	for(; p.HasKernelAttribute(); a = p.GetKernelAttribute()){
		if( p.Delta() < thld) {
			// Unstable concept cannot probuce stable concepts
			break;
		}

		if(p.IsIgnored(a)){
			continue;
		}
		// Computing the only possible preimage
		unique_ptr<CPTPattern> res(computePreimage(p, a));
		const DWORD extDiff = p.Size() - res->Size();
		assert( extDiff == getAttributeDelta(p, a, -1));
		if(extDiff == 0 ) {
			// Attribute is in the closure
			p.AddNewAttributeToIntent(a);
			continue;
		}

		const bool isPreimageStable = initializePreimage(p, a, *res);

		// Updating the measure of the current pattern.
		//    It is here, because initializeNewPattern relies on the p.GetClossestChild()
		if( p.Delta() > extDiff ) {
			p.SetDelta(extDiff);
			p.SetClosestAttribute(a);
		}

		p.MoveAttributeToKernel(a);

		if( !isPreimageStable ) {
			continue;
		}

		// A new stable pattern is generated
		//  We should add it to preimages.
		assert(res->Delta() >= thld);
		preimages.PushBack( res.release() );
		++totalAllocatedPatterns;

		if(!areAllInOnce) {
			++a;
			break;
		}
	}

	// p.SetKernelAttribute(a);
	const bool isStable = p.Delta() >= thld;
	if(!p.HasKernelAttribute()){
		return isStable ? PR_Finished : PR_Uninteresting;
	} else {
		return isStable ? PR_Expandable : PR_Uninteresting;
	}

	return PR_Uninteresting;
}
bool CStabilityLPCbyPatriciaTree::IsExpandable( const IPatternDescriptor* d ) const
{
	assert(attrs != 0);

	const CPTPattern& p = to_pattern(d);
	assert(p.Delta() >= thld);
	return p.GetKernelAttribute() <= maxAttribute;
}
int CStabilityLPCbyPatriciaTree::GetExtentSize( const IPatternDescriptor* d ) const
{
	return to_pattern(d).Size();
}
JSON CStabilityLPCbyPatriciaTree::SaveExtent( const IPatternDescriptor* d ) const
{
	const CPTPattern& p = to_pattern(d);
	CPatternImage img;
	p.GetExtent(img);

	rapidjson::Document extentJson;
	rapidjson::MemoryPoolAllocator<>& alloc = extentJson.GetAllocator();
	extentJson.SetObject()
		.AddMember( "Count", rapidjson::Value().SetUint(img.ImageSize), alloc )
		.AddMember( "Inds", rapidjson::Value().SetArray(), alloc );

	rapidjson::Value& inds = extentJson["Inds"];
	inds.Reserve( img.ImageSize, alloc );

	for( int i = 0; i < img.ImageSize; ++i) {
		inds.PushBack(rapidjson::Value().SetUint(img.Objects[i]), alloc);
	}

	JSON result;
	CreateStringFromJSON( extentJson, result );
	return result;
}
JSON CStabilityLPCbyPatriciaTree::SaveIntent( const IPatternDescriptor* d ) const
{
	const CPTPattern& p = to_pattern(d);

	set<int> intent;
	auto itr = p.Begin();
	int nodeCount = 0;
	for(; itr != p.End(); ++itr, ++nodeCount) {
		const CPatritiaTreeNode* node = *itr;
		assert(node != 0);

		if( nodeCount == 0) {
			intent = node->CommonAttributes;
			continue;
		}

		set<int> res;
		set_intersection(node->CommonAttributes.begin(), node->CommonAttributes.end(),
						 intent.begin(), intent.end(), inserter(res, res.end()));

		intent.swap(res);
	}

	rapidjson::Document intentJson;
	rapidjson::MemoryPoolAllocator<>& alloc = intentJson.GetAllocator();
	intentJson.SetObject()
		.AddMember( "Names", rapidjson::Value().SetArray(), alloc );

	rapidjson::Value& names = intentJson["Names"];
	names.Reserve( intent.size(), alloc );

	for( auto attr = intent.begin(); attr != intent.end(); ++attr) {
		const CPatritiaTree::TAttribute a = *attr;
		const JSON name = attrs->GetAttributeName(a);
		names.PushBack( rapidjson::Value().SetString(name.c_str(), name.length(), alloc), alloc);
	}

	intentJson.AddMember( "Count", rapidjson::Value().SetUint(names.Size()), alloc );

	JSON result;
	CreateStringFromJSON( intentJson, result );
	return result;
}

size_t CStabilityLPCbyPatriciaTree::GetTotalAllocatedPatterns() const
{
	return totalAllocatedPatterns;
}

size_t CStabilityLPCbyPatriciaTree::GetTotalConsumedMemory() const
{
	return totalAllocatedPatterns * sizeof(CPTPattern) + memoryCounter.GetMemoryConsumption();
}

// Builds patritia tree from the context
void CStabilityLPCbyPatriciaTree::buildPatritiaTree()
{
	// Starting reading the context object by object
	attrs->Start();

	IBinContextReader::CObjectIntent intent;
	vector<int> buffer;

	// Insertion of all objects to partitia tree
	// The correspondance between nodes in the patritia tree and the objectID
	std::multimap<const CPatritiaTreeNode*, CPatritiaTree::TObject> nodeToObjectMap;
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
		maxAttribute = max( maxAttribute, buffer.back());

		CPatritiaTree::TNodeIndex nodeId =  pTree.GetRoot(); // Root node with no generator attribute

		for( auto i = buffer.begin(); i!= buffer.end(); ++i) {
			nodeId = pTree.GetOrCreateAttributeNode(nodeId, *i);
		}
		nodeToObjectMap.insert(std::pair<const CPatritiaTreeNode*, CPatritiaTree::TObject>(&pTree.GetNode(nodeId), objectId ));
	}

	// Now we will add objects to the nodes
	addObjectsToPTNodes(nodeToObjectMap);

	// // DEBUG: For printing the result
	// pTree.Print();

	// The set of previously processed attibutes
	std::deque<CPatritiaTree::TAttribute> childrenCAttrs;
	std::list<CPatritiaTree::TAttribute> currentNodeAttrs;
	// Compressing of the Tree
	//  Now all attributes that are in the closure should be added to the node and the corresponding tree should be removed
	CDeepFirstPatritiaTreeIterator treeItr(pTree);
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

		currentNodeAttrs.clear();

		// Only the attributes after the last one can be in the closure,
		// since the previous children are generated with the attribute which definetly upson in the last children
		CPatritiaTree::TNodeIndex lastNodeId = *ch;
		auto& lastNode = pTree.GetNode(lastNodeId);

		// A flag indicating if all attributes from the last node are presented in the previous children
		bool areAllLastChildAttributesCommon = false;

		// Now if there are objects associated with the node, then these objects do not have other attributes and thus there is no attributes in the closure.
		if( nodeToObjectMap.find(&*treeItr) == nodeToObjectMap.end() ) {
			// Adding the generated attribute for the last children.
			// It is the only possible generated attribute among the children that can be common to all of children
			currentNodeAttrs.push_back(lastNode.GenAttr);
			// Adding all attributes for the last node as candidates.
			for(int i = lastNode.ClosureAttrStart; i < lastNode.ClosureAttrEnd; ++i) {
				assert(currentNodeAttrs.empty() || 0 <= i && childrenCAttrs.size() && currentNodeAttrs.back() < childrenCAttrs[i]);
				currentNodeAttrs.push_back(childrenCAttrs[i]);
			}
			// A flag indicating if all attributes from the last node are presented in the previous children
			areAllLastChildAttributesCommon = true;
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
		if( !currentNodeAttrs.empty() && currentNodeAttrs.front() == lastNode.GenAttr && !areAllLastChildAttributesCommon ) {
			auto curNodeAttrsItr = currentNodeAttrs.begin();
			++curNodeAttrsItr;
			// lastNode.GenAttr is common so should be changed
			for( int clsAttrInd = lastNode.ClosureAttrStart; clsAttrInd  < lastNode.ClosureAttrEnd; ++clsAttrInd ) {
				const CPatritiaTree::TAttribute a = childrenCAttrs.at(clsAttrInd);

				assert(curNodeAttrsItr != currentNodeAttrs.end());

				if( a != *curNodeAttrsItr ) {
					// The attribute can be used the generator for the node
					lastNode.GenAttr = a;
					lastNode.ClosureAttrStart = clsAttrInd + 1;
					break;
				}
				++curNodeAttrsItr;
			}
			assert(curNodeAttrsItr != currentNodeAttrs.end());
		}
		if(areAllLastChildAttributesCommon) {
			++ch; // The last node will be removed later
		}

		// Now for all children (sometimes but the last one) we collect the attributes in the closure and register them in the tree
		for( ; ch != children.rend(); ++ch) {
			const CPatritiaTree::TNodeIndex chId = *ch;
			CPatritiaTree::CNode& chNode = pTree.GetNode(chId);
			int clsAttrInds = chNode.ClosureAttrStart;
			const int clsAttrEnd = chNode.ClosureAttrEnd;
			auto curNodeAttrsItr = currentNodeAttrs.begin();
			bool isStartSet = false;
			for(; clsAttrInds < clsAttrEnd; ++clsAttrInds) {
				const CPatritiaTree::TAttribute a  = childrenCAttrs.at(clsAttrInds);
				while( curNodeAttrsItr != currentNodeAttrs.end() && *curNodeAttrsItr < a ) {
					++curNodeAttrsItr;
				}
				if( curNodeAttrsItr != currentNodeAttrs.end() && *curNodeAttrsItr == a ) {
					// Attribute is passed to the parent node
					continue;
				}
				const int newAttrInd = pTree.AddAttribute( a );
				if( !isStartSet ) {
					chNode.ClosureAttrStart = newAttrInd;
					isStartSet = true;
				}

				chNode.ClosureAttrEnd = newAttrInd + 1;
			}
			if(!isStartSet) {
				chNode.ClosureAttrStart = 0;
				chNode.ClosureAttrEnd = 0;
			}

			assert(chNode.ClosureAttrStart <= chNode.ClosureAttrEnd);
		}

		if( areAllLastChildAttributesCommon ) {
			// Then the last child should be merged with the current node
			auto tmpItr = treeItr->Children.end();
			--tmpItr;
			for(auto chItr = lastNode.Children.begin(); chItr != lastNode.Children.end(); ++chItr) {
				pTree.MoveChild(*chItr, pTree.GetNodeIndex(*treeItr));
			}
			lastNode.Clear();
			treeItr->Children.erase(tmpItr);
		}
	}

	// Copying the attributes of the root closure to the tree
	CPatritiaTree::CNode& root = pTree.GetNode(pTree.GetRoot());
	int clsAttrInds = root.ClosureAttrStart;
	const int clsAttrEnd = root.ClosureAttrEnd;
	bool isStartSet = false;
	for(; clsAttrInds < clsAttrEnd; ++clsAttrInds) {
		const CPatritiaTree::TAttribute a  = childrenCAttrs.at(clsAttrInds);
		const int newAttrInd = pTree.AddAttribute( a );
		if( !isStartSet ) {
			root.ClosureAttrStart = newAttrInd;
			isStartSet = true;
		}

		root.ClosureAttrEnd = newAttrInd + 1;
	}
	if(!isStartSet) {
		root.ClosureAttrStart = 0;
		root.ClosureAttrEnd = 0;
	}

	assert(root.ClosureAttrStart <= root.ClosureAttrEnd);

	// // DEBUG: For printing the result
	// pTree.Print();

	computeCommonAttributesinPT();
	computeNextAttributeIntersectionsinPT();
}

// Based on patritia tree extracts the description of every attribute
// void CStabilityLPCbyPatriciaTree::extractAttrsFromPatritiaTree()
// {
// 	CDeepFirstPatritiaTreeIterator treeItr(pTree);
// 	for(; !treeItr.IsEnd(); ++treeItr) {
// 		if( treeItr.Status() != CDeepFirstPatritiaTreeIterator::S_Forward) {
// 			// Entering the node we will register this node as the node for the attribute
// 			continue;
// 		}
// 		CPatritiaTree::TAttribute attr = treeItr->GenAttr;
// 		if( attr >= 0 ) {
// 			addAttributeNode(attr, pTree.GetNode(*treeItr));
// 		}
// 		int clsAttr = treeItr->ClosureAttrStart;
// 		for(; clsAttr != treeItr->ClosureAttrEnd; ++clsAttr) {
// 			attr = pTree.GetClsAttribute(clsAttr);
// 			addAttributeNode(attr, pTree.GetNode(*treeItr));
// 		}
// 	}
// }

// Adds new node in the description of the attribute
// void CStabilityLPCbyPatriciaTree::addAttributeNode(CPatritiaTree::TAttribute attr, const CPatritiaTreeNode& node)
// {
// 	if( attributes.size() <= attr ) {
// 		attributes.resize(attr+1);
// 		assert(attr < attributes.size());
// 	}
// 	list<const CPatritiaTreeNode*>& attrList = attributes[attr];
// 	assert( attrList.empty()
// 	        || attrList.back()->ObjEnd <= node.ObjStart); // The nodes cannot intersect, they should form an antichain

// 	attrList.push_back(&node);
// }

// An alternavite approach to computaition of patritia tree
void CStabilityLPCbyPatriciaTree::buildPatritiaTree2()
{
	// Starting reading the context object by object
	attrs->Start();

	IBinContextReader::CObjectIntent intent;
	vector<int> buffer;
	set<int> intentSet;

	// Insertion of all objects to partitia tree

	// The correspondance between nodes in the patritia tree and the objectID
	std::multimap<const CPatritiaTreeNode*, CPatritiaTree::TObject> nodeToObjectMap;

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

		intentSet.clear();
		intentSet.insert(buffer.begin(), buffer.end());
		maxAttribute = max( maxAttribute, buffer.back());

		insertObjectToPTNode(pTree.GetRoot(), intentSet, nodeToObjectMap, objectId);
	}

	addObjectsToPTNodes(nodeToObjectMap);
	computeCommonAttributesinPT();	
	computeNextAttributeIntersectionsinPT();
}

// Inserts an intent to the node, the information in the intent can be updated for further processing
void CStabilityLPCbyPatriciaTree::insertObjectToPTNode(CPatritiaTree::TNodeIndex nodeId, set<int>& intent,
													   std::multimap<const CPatritiaTreeNode*, CPatritiaTree::TObject>& nodeToObjectMap, CPatritiaTree::TObject objectId)
{
	CPatritiaTreeNode& node = pTree.GetNode(nodeId);
	assert(checkNodeValidity(node));

	if(node.Children.empty() && nodeToObjectMap.find(&node) == nodeToObjectMap.end()) {
		// No objects are associated with the node or its children,
		//   Should just add the node
		node.CommonAttributes = intent;
		nodeToObjectMap.insert(pair<const CPatritiaTreeNode*, CPatritiaTree::TObject>(&node, objectId));
		return;
	}

	set<int> commonAttrs;

	set_intersection(node.CommonAttributes.begin(), node.CommonAttributes.end(),
					 intent.begin(), intent.end(),
					 std::inserter(commonAttrs, commonAttrs.end()));

	set<int> nextAttrs;
	set_difference(node.CommonAttributes.begin(), node.CommonAttributes.end(),
				   commonAttrs.begin(), commonAttrs.end(),
				   std::inserter(nextAttrs, nextAttrs.end()));
	node.CommonAttributes.swap(commonAttrs);
	commonAttrs.clear();
	// Now in node.CommonAttributes is the intersection.
	set_difference( intent.begin(), intent.end(),
					node.CommonAttributes.begin(), node.CommonAttributes.end(),
					std::inserter(commonAttrs, commonAttrs.end()));
	intent.swap(commonAttrs);

	// Now in node.CommonAttributes the common attributes between the node and the new intent
	//   in nextAttrs the rest of previous CommonAttributes
	//   in intent the rest of intent
	if(!nextAttrs.empty()) {
		// The attributes in nextAttrs are common for all children and the objects associated to the node.
		const CPatritiaTree::TAttribute a = *nextAttrs.begin();
		// First we iterate over all possible children of the node
		auto ch = node.Children.begin();
		for(; ch != node.Children.end(); ++ch) {
			const CPatritiaTree::TNodeIndex index = *ch;
			CPatritiaTreeNode& child = pTree.GetNode(index);
			assert(child.GenAttr != a);
			if( child.GenAttr > a) {
				break;
			}
			child.CommonAttributes.insert(nextAttrs.begin(), nextAttrs.end());
			assert(checkNodeValidity(child));
		}

		auto range = nodeToObjectMap.equal_range(&node);
		if(range.first != range.second || ch != node.Children.end() ) {
			// Either some objects are associated with the node or some childs are generated with late attributes
			assert( pTree.GetAttributeNode(nodeId, *nextAttrs.begin()) == -1 );

			CPatritiaTree::TNodeIndex newNodeId	= -1;
			if( range.first == range.second && distance(ch, node.Children.end()) == 1) {
				const CPatritiaTree::TNodeIndex index = *ch;
				CPatritiaTreeNode& child = pTree.GetNode(index);
				const CPatritiaTree::TAttribute oldAttr = child.GenAttr;
				pTree.ChangeGenAttr(*ch, *nextAttrs.begin());
				assert(child.GenAttr == *nextAttrs.begin());
				child.CommonAttributes.insert(oldAttr);
				auto attr = nextAttrs.begin();
				++attr;
				child.CommonAttributes.insert(attr, nextAttrs.end());
				assert(checkNodeValidity(child));
			} else {
				newNodeId = pTree.AddNode(nodeId, *nextAttrs.begin());
				CPatritiaTreeNode& newNode = pTree.GetNode(newNodeId);

				for(; ch != node.Children.end(); ) {
					auto chItr = ch;
					++ch;
					const CPatritiaTree::TNodeIndex index = *chItr;
					CPatritiaTreeNode& child = pTree.GetNode(index);
					assert( pTree.GetAttributeNode(newNodeId, child.GenAttr) == -1 );
					assert( pTree.GetAttributeNode(nodeId, child.GenAttr) == *chItr);
					pTree.MoveChild( *chItr, newNodeId );
					assert( pTree.GetAttributeNode(newNodeId, child.GenAttr) == *chItr );
					assert( pTree.GetAttributeNode(nodeId, child.GenAttr) == -1);
				}

				// Inserting attributes
				auto attr = nextAttrs.begin();
				++attr; // The first one is already a generator
				newNode.CommonAttributes.insert(attr, nextAttrs.end());
				assert(checkNodeValidity(newNode));

				if( range.first != range.second) {
					deque< pair<const CPatritiaTreeNode*, CPatritiaTree::TObject> > toMoveObjs;
					toMoveObjs.insert(toMoveObjs.end(), range.first, range.second);
					nodeToObjectMap.erase(range.first, range.second);

					//moving objects
					for(auto obj = toMoveObjs.begin(); obj != toMoveObjs.end(); ++obj) {
						nodeToObjectMap.insert(pair<const CPatritiaTreeNode*, CPatritiaTree::TObject>(&newNode, obj->second));
					}
				}
			}
		}
	}

	if(intent.empty()) {
		// We can add the object to this node
		nodeToObjectMap.insert(pair<const CPatritiaTreeNode*, CPatritiaTree::TObject>(&node, objectId));
		return;
	}

	CPatritiaTree::TNodeIndex newNode = pTree.GetOrCreateAttributeNode( nodeId, *intent.begin() );

	assert(checkNodeValidity(node));
	
	intent.erase(intent.begin());
	insertObjectToPTNode(newNode, intent, nodeToObjectMap, objectId);
}
bool CStabilityLPCbyPatriciaTree::checkNodeValidity(const CPatritiaTreeNode& node)
{
	auto ch =  node.Children.begin();
	for(; ch != node.Children.end(); ++ch) {
		const CPatritiaTreeNode& child = pTree.GetNode(*ch);
		assert( node.CommonAttributes.find(child.GenAttr) == node.CommonAttributes.end());
	}
	return true;
}


void CStabilityLPCbyPatriciaTree::addObjectsToPTNodes(std::multimap<const CPatritiaTreeNode*, CPatritiaTree::TObject>& nodeToObjectMap)
{
	CDeepFirstPatritiaTreeIterator treeItr(pTree);
	for(; !treeItr.IsEnd(); ++treeItr) {
		if( treeItr.Status() != CDeepFirstPatritiaTreeIterator::S_Exit) {
			// The objects are collected on exit from the node
			continue;
		}
		auto objItrs = nodeToObjectMap.equal_range(&*treeItr);
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
	
}

// For every node in Patritia Tree computes the related attributes
void CStabilityLPCbyPatriciaTree::computeCommonAttributesinPT()
{
	CDeepFirstPatritiaTreeIterator treeItr(pTree);
	CPatritiaTreeNode::TAttributeSet attributes;
	for(; !treeItr.IsEnd(); ++treeItr) {
		switch(treeItr.Status()) {
		case CDeepFirstPatritiaTreeIterator::S_Return:
			attributes = treeItr->CommonAttributes;
			continue;
		case CDeepFirstPatritiaTreeIterator::S_Forward: {
			treeItr->ClosureAttrStart = -1;
			treeItr->ClosureAttrEnd = -1;
			// Saving the closure
			for( auto a = treeItr->CommonAttributes.begin(); a != treeItr->CommonAttributes.end(); ++a) {
				const int attrIndex = pTree.AddAttribute(*a);
				if( treeItr->ClosureAttrStart == -1) {
					treeItr->ClosureAttrStart = attrIndex;
				}
				treeItr->ClosureAttrEnd = attrIndex + 1;
			}
			if(treeItr->GenAttr != -1) {
				attributes.insert(treeItr->GenAttr);
			}
			attributes.insert(treeItr->CommonAttributes.begin(), treeItr->CommonAttributes.end());
			treeItr->CommonAttributes = attributes;
			break;
		}
		case CDeepFirstPatritiaTreeIterator::S_Exit: {
			break;
		}
		default:
			assert(false);
		}
	}
}

// Computhe the intersection of the PT node with an attribute
void CStabilityLPCbyPatriciaTree::computeNextAttributeIntersectionsinPT()
{
	CDeepFirstPatritiaTreeIterator treeItr(pTree);
	for(; !treeItr.IsEnd(); ++treeItr) {
		if( treeItr.Status() != CDeepFirstPatritiaTreeIterator::S_Exit) {
			// On exit all previous nodes are processed
			continue;
		}

		// Adding intersection information from children
		auto chItr = treeItr->Children.begin();
		for( ;chItr != treeItr->Children.end(); ++chItr) {
			const CPatritiaTreeNode* ch = &pTree.GetNode(*chItr);

			// Intersections known in the child
			treeItr->NextAttributeIntersections.insert(ch->NextAttributeIntersections.begin(), ch->NextAttributeIntersections.end());

			// Intersections with the attributes in the closure of the child
			for( auto attr = ch->CommonAttributes.upper_bound(treeItr->GenAttr); attr != ch->CommonAttributes.end(); ++attr) {
				if(treeItr->CommonAttributes.find(*attr) != treeItr->CommonAttributes.end()) {
					// The result is already in the closure of the current node
					continue;
				}
				treeItr->NextAttributeIntersections.insert(CPatritiaTreeNode::TNextAttributeIntersections::value_type(*attr, ch));
			}
		}
	}
	// // DEBUG: For printing the result
	// auto tmp = pTree.GetNode(pTree.GetRoot()).NextAttributeIntersections;
	// for(auto itr = tmp.begin(); itr != tmp.end(); ++itr){
	// 	cout << itr->first << " " << itr->second->GenAttr << endl;
	// }
}

// Checks that the Patritia Tree has a valid structure and rise asserts in the error place
bool CStabilityLPCbyPatriciaTree::checkPTValidity()
{
	// Internal validity of the structure
	CDeepFirstPatritiaTreeIterator treeItr(pTree);
	CPatritiaTree::TObject maxObject = 0;
	for(; !treeItr.IsEnd(); ++treeItr) {
		switch(treeItr.Status()) {
		case CDeepFirstPatritiaTreeIterator::S_Return:
			break;
		case CDeepFirstPatritiaTreeIterator::S_Forward: {
			const CPatritiaTree::TNodeIndex pId = treeItr->GetParent();
			if( pId >= 0 ) {
				const CPatritiaTreeNode& parent = pTree.GetNode(pId);
				const set<CPatritiaTree::TAttribute>& intent = parent.CommonAttributes;

				assert( intent.find(treeItr->GenAttr) == intent.end() );
				assert(treeItr->CommonAttributes.find(treeItr->GenAttr) != treeItr->CommonAttributes.end());
				CPatritiaTree::TAttribute prevAttr = -1;
				for( int i = treeItr->ClosureAttrStart; i < treeItr->ClosureAttrEnd; ++i) {
					const CPatritiaTree::TAttribute a = pTree.GetClsAttribute(i);
					assert( prevAttr < a);
					prevAttr = a;
					assert(intent.find(a) == intent.end());
					assert(a > treeItr->GenAttr);
					assert(treeItr->CommonAttributes.find(a) != treeItr->CommonAttributes.end());
				}

				assert( includes(treeItr->CommonAttributes.begin(), treeItr->CommonAttributes.end(),
								 intent.begin(), intent.end()));
				assert( parent.ObjStart <= treeItr->ObjStart && treeItr->ObjEnd <=parent.ObjEnd);
			}
			auto ch = treeItr->Children.begin();
			auto prevCh = ch;
			if( ch != treeItr->Children.end()) {
				++ch;
			}
			for(; ch != treeItr->Children.end(); prevCh = ch, ++ch) {
				const CPatritiaTreeNode& prev = pTree.GetNode(*prevCh);
				const CPatritiaTreeNode& cur = pTree.GetNode(*ch);
				assert(prev.ObjEnd == cur.ObjStart);
			}

			assert(treeItr->ObjStart <= treeItr->ObjEnd);
			for(int i = treeItr->ObjStart; i < treeItr->ObjEnd; ++i) {
				const CPatritiaTree::TObject o = pTree.GetObject(i);
				assert( 0 <= o );
				maxObject = max( maxObject, o );
			}
			break;
		}
		case CDeepFirstPatritiaTreeIterator::S_Exit:
			break;
		default:
			assert( false );
		}
	}

	// Validating that all objects maps to their own position
	const int attributeNum = maxAttribute + 1;
	assert( attributeNum > 0);

	IBinContextReader::CObjectIntent objectIntent;
	vector<IBinContextReader::TAttributeID> buffer;
	buffer.resize(attributeNum, -1);
	objectIntent.Attributes = buffer.data();

	// Iterating over the objects of the context
	int objectId = 0;
	attrs->Start(); 
	int nextIntentSize = attrs->GetNextObjectIntentSize();
	for(; nextIntentSize >= 0; ++objectId, nextIntentSize = attrs->GetNextObjectIntentSize()) {
		assert(nextIntentSize <= attributeNum);
		objectIntent.Size = nextIntentSize;

		buffer.resize(objectIntent.Size);
		attrs->GetNextObject(objectIntent);
		assert(objectIntent.Size == buffer.size());
		sort(buffer.begin(), buffer.end());

		const CPatritiaTreeNode* node = &pTree.GetNode(pTree.GetRoot());
		int processedAttrs = 0;
		for(int i = 0; i < buffer.size(); ++i) {
			assert( node != 0);
			if( node->CommonAttributes.find(buffer[i]) != node->CommonAttributes.end()) {
				continue;
			}
			
			const CPatritiaTree::TNodeIndex next = pTree.GetAttributeNode(*node, buffer[i]);
			// The next attribute should be in the tree
			assert( next != -1 );
			node = &pTree.GetNode(next);
		}

		// All atributes are somewhere found
		assert(node != 0);
		int i = node->ObjStart;
		for(; i < node->ObjEnd; ++i) {
			if(pTree.GetObject(i) == objectId) {
				break;
			}
		}
		// The object number should be found in the node
		assert(i < node->ObjEnd);
	}
	assert(objectId-1 == maxObject);

	return true;
}

// Converts the general pattern to specific pattern
const CPTPattern& CStabilityLPCbyPatriciaTree::to_pattern(const IPatternDescriptor* d) const
{
	assert(d != 0);
	return debug_cast<const CPTPattern&>(*d);
}

// Computes the preimage of p w.r.t. the attribute a
CPTPattern* CStabilityLPCbyPatriciaTree::computePreimage(const CPTPattern& p, CPatritiaTree::TAttribute a)
{
	assert(p.GetKernelAttribute() <= a);
	unique_ptr<CPTPattern> result( new CPTPattern(pTree, memoryCounter) );
	auto itr = p.Begin();
	for(; itr != p.End(); ++itr) {
		if( (*itr)->CommonAttributes.find(a) != (*itr)->CommonAttributes.end() ){
			result->AddPTNode(**itr);
			continue;
		}
		auto range = (*itr)->NextAttributeIntersections.equal_range(a);
		for( auto resNode = range.first; resNode != range.second; ++resNode) {
			result->AddPTNode(*resNode->second);
		}
	} 
	result->SetKernelAttribute(a+1);
	result->CopyIntent(p);
	result->AddNewAttributeToIntent(a);
	return result.release();
}

bool CStabilityLPCbyPatriciaTree::initializePreimage(const CPTPattern& parent, int genAttr, CPTPattern& res)
{
	if(!res.ComputeAttributeIntersections(parent)) {
		// Not a canonical order
		return false;
	}
	assert( res.Delta() <= parent.Delta());
	return res.Delta() >= thld;

	// If delta is zero, then the attribute is in the intent.
	// However, according to canonical order it should not be there. Thus, such a pattern should also be ignored.
	assert(thld >= 1);
	DWORD delta = min(parent.Delta(), res.Size());
	int minAttr = -1;
	if( parent.GetClosestAttribute() >= 0 ) {
		assert(!parent.IsIgnored(parent.GetClosestAttribute()));
		const DWORD clossestChildDelta = getAttributeDelta(res, parent.GetClosestAttribute(), delta);
		assert(checkAttributeDeltaProblem(parent, res, parent.GetClosestAttribute()));
		assert(clossestChildDelta <= delta);
		delta = clossestChildDelta;
		minAttr = parent.GetClosestAttribute();
	}

	const CPTPattern::CIntentAttribute* lastIntentAttr = res.GetLastIntentAttribute();

	for(int a = genAttr - 1; delta >= thld && a >=0; --a) {
		if(a == parent.GetClosestAttribute()) {
			// Already verified
			continue;
		}
		while(lastIntentAttr != 0 && lastIntentAttr->Attr > a) {
			lastIntentAttr = lastIntentAttr->Prev;
		}
		if( lastIntentAttr != 0 && lastIntentAttr->Attr == a ) {
			// Already in the pattern
			continue;
		}
		const DWORD aDelta = getAttributeDelta(res, a, delta);
		assert(checkAttributeDeltaProblem(parent, res, a));
		if(aDelta < delta) {
			delta = aDelta;
			minAttr = a;
		}
		if( delta < thld ) {
			// The pattern is unstable
			return false;
		}
	}
	if( delta < thld ) {
		// The pattern is unstable
		return false;
	}
	res.SetClosestAttribute(minAttr);
	res.SetDelta(delta);
	return true;
}

// Finds the delta-stability of a pattern p wrt the attribute a.
//   If the delta-stability is larger than maxDelta, then maxDelta+1 is returned
DWORD CStabilityLPCbyPatriciaTree::getAttributeDelta(const CPTPattern& p, CPatritiaTree::TAttribute a, DWORD maxDelta)
{
	DWORD currentDelta = 0;
	auto itr = p.Begin();
	for(; itr != p.End(); ++itr) {
		if( currentDelta > maxDelta ) {
			return maxDelta+1;
		}
		if( (*itr)->GenAttr == a) {
			continue;
		}
		if( (*itr)->CommonAttributes.find(a) != (*itr)->CommonAttributes.end() ) {
			continue;
		}
		if( a < (*itr)->GenAttr ) {
			// The node is removed after intersection
			currentDelta += (*itr)->ObjEnd - (*itr)->ObjStart;
		} else {
			currentDelta += (*itr)->ObjEnd - (*itr)->ObjStart;
			auto range = (*itr)->NextAttributeIntersections.equal_range(a);
			for( auto resNode = range.first; resNode != range.second; ++resNode) {
				currentDelta -= resNode->second->ObjEnd - resNode->second->ObjStart;
			}
		}
	} 
	return currentDelta;
}

// The function verifyies that parent p, a child ch, and the results of intersecting p and ch with the attribute a are correctly placed
// In particular it is verified that p >= ch, p >= pRes = p \cup a, ch >= chRes, pRes >= chRes
// It also verify if an object o is in ch and in pRes than it is necessarily in chRes
bool CStabilityLPCbyPatriciaTree::checkAttributeDeltaProblem(const CPTPattern& p, const CPTPattern& ch, CPatritiaTree::TAttribute a)
{
	return true;

	auto pItr = p.Begin();
	auto chItr = ch.Begin();
	for(; chItr != ch.End(); ++chItr) {
		assert(pItr != p.End()); // Otherwise it is not a child

		const int chObjStart = (*chItr)->ObjStart;
		const int chObjEnd = (*chItr)->ObjEnd;

		while(pItr != p.End() && (*pItr)->ObjEnd <= chObjStart) {
			// The objects are only in the parent.
			// Just skip them
			++pItr;
		}
		assert(pItr != p.End()); // Otherwise it is not a child

		const int pObjStart = (*pItr)->ObjStart;
		const int pObjEnd = (*pItr)->ObjEnd;

		assert( pObjStart <= chObjStart ); // Otherwise it is not a child
		assert( pObjEnd >= chObjEnd ); // The object sets cannot partially overlap


		if( (*pItr)->GenAttr == a
			|| (*pItr)->CommonAttributes.find(a) != (*pItr)->CommonAttributes.end() )
			{
				// The parent node is preserved then the child node should also be preserved
				assert((*chItr)->CommonAttributes.find(a) != (*chItr)->CommonAttributes.end());
				continue;
			}

		if( a < (*pItr)->GenAttr ) {
			// The parent is removed, the child should also be removed
			// Otherwise something from the child is preserved an it is a contradiction
			assert((*chItr)->GenAttr != a);
			assert((*chItr)->CommonAttributes.find(a) == (*chItr)->CommonAttributes.end());
			auto range = (*chItr)->NextAttributeIntersections.equal_range(a);
			assert(range.first == range.second);
			continue;
		}

		auto chRange = (*chItr)->NextAttributeIntersections.equal_range(a);
		auto chResItr = chRange.first;

		int chResObjStart = chObjStart;
		int chResObjEnd = chObjEnd;

		auto pRange = (*pItr)->NextAttributeIntersections.equal_range(a);
		auto pResItr = pRange.first;

		if((*chItr)->GenAttr != a && ((*chItr)->CommonAttributes.find(a) == (*chItr)->CommonAttributes.end())) {
			if(chResItr == chRange.second) {
				// A child is removed with 'a', the result should have no intersections with the child
				for(; pResItr != pRange.second; ++pResItr) {
					const int pResObjStart = pResItr->second->ObjStart;
					const int pResObjEnd = pResItr->second->ObjEnd;
					assert( pResObjStart < pResObjEnd );
					assert(pResObjStart >= chObjEnd || chObjStart >= pResObjEnd);
				}
				continue;
			}
			chResObjStart = chResItr->second->ObjStart;
			chResObjEnd = chResItr->second->ObjEnd;
			assert(chObjStart <= chResObjStart && chResObjEnd <= chObjEnd);
			++chResItr;
		}

		int verifiedLimit = -1;

		assert(pRange.first != pRange.second);
		int pResObjStart = pResItr->second->ObjStart;
		int pResObjEnd = pResItr->second->ObjEnd;

		while( pResItr != pRange.second ) {
			if( pResObjEnd <= chResObjStart ) {
				assert(verifiedLimit == -1 || pResObjEnd <= verifiedLimit);
				++pResItr;
				pResObjStart = pResItr->second->ObjStart;
				pResObjEnd = pResItr->second->ObjEnd;
				assert(pObjStart <= pResObjStart && pResObjEnd <= pObjEnd);
				if( verifiedLimit == -1 && pResObjEnd > chObjStart ) {
					verifiedLimit = max(pResObjStart, chObjStart);
				}
				assert(verifiedLimit == -1 || pResObjEnd <= verifiedLimit || pResObjStart <= verifiedLimit);
				continue;
			}

			assert(pResObjStart <= chResObjStart && chResObjEnd <= pResObjEnd);

			assert( verifiedLimit == -1 || verifiedLimit == chResObjStart );

			verifiedLimit = chResObjEnd;
			if( pResObjEnd == verifiedLimit ) {
				verifiedLimit = -1;
			}

			if( chResItr == chRange.second) {
				break;
			}
			chResObjStart = chResItr->second->ObjStart;
			chResObjEnd = chResItr->second->ObjEnd;
			assert(chObjStart <= chResObjStart && chResObjEnd <= chObjEnd);
			++chResItr;
		}
		assert(verifiedLimit == -1 || verifiedLimit >= chObjEnd);
	} 
	return true;
}

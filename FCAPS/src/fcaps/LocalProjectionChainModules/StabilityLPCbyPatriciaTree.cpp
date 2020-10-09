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
		int counter;
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
		{ return kernelAttr; }
	void SetKernelAttribute( CPatritiaTree::TAttribute a ) const
	{
		// Now we should skip all attributes that are in the closure and all atributes that colapse the pattern to zero
		const int attrNum = pTree.GetNode(pTree.GetRoot()).NextAttributeIntersections.rend()->first + 1;
		CPatritiaTree::TAttribute minAllInClosureAttr = attrNum;
		CPatritiaTree::TAttribute minAllRemovedAttr = attrNum;
		for( auto itr = extent.begin(); itr != extent.end(); ++itr ) {
			minAllRemovedAttr = min( minAllRemovedAttr,*(*itr)->CommonAttributes.upper_bound(a-1) - 1);
			minAllRemovedAttr = min( minAllRemovedAttr,(*itr)->NextAttributeIntersections.upper_bound(a-1)->first - 1);
			if(minAllRemovedAttr <= a) {
				assert(minAllRemovedAttr == a);
				break;
			}
		}
		kernelAttr = max(a, minAllRemovedAttr);
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
	// Adding new attribute to the intent
	void AddNewAttributeToIntent(CPatritiaTree::TAttribute a) const
	{
		assert(intent==0 || intent->Attr < a);
		if(intent != 0) {
			++intent->counter;
		}
		intent = new CIntentAttribute(intent, a);
	}
	const CIntentAttribute* GetLastIntentAttribute() const
		{ return intent; }

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

	buildPatritiaTree();

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
	ptrns.PushBack( ptrnHolder.release() );
	++totalAllocatedPatterns;
}

ILocalProjectionChain::TPreimageResult CStabilityLPCbyPatriciaTree::Preimages( const IPatternDescriptor* d, CPatternList& preimages )
{
	assert( attrs != 0);

	const CPTPattern& p = to_pattern(d);
    assert(p.Delta() >= thld);

	int a = p.GetKernelAttribute();
	for(; a <= maxAttribute; ++a){
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

	p.SetKernelAttribute(a);
	const bool isStable = p.Delta() >= thld;
	if(a > maxAttribute){
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

	vector<int> intent;
	intent.resize(maxAttribute + 1, 0);
	auto itr = p.Begin();
	int nodeCount = 0;
	for(; itr != p.End(); ++itr, ++nodeCount) {
		const CPatritiaTreeNode* node = *itr;
		while( true ) {
			assert( node != 0);
			// Adding all attributes from the closure
			for( int i = node->ClosureAttrStart; i < node->ClosureAttrEnd; ++i ) {
				const int closureAttr = pTree.GetClsAttribute(i);
				assert( 0 <= closureAttr && closureAttr < intent.size());
				assert( intent[closureAttr] <= nodeCount);
				++intent[closureAttr];
			}
			// Adding the generator attribute
			const int closureAttr = node->GenAttr;
			if( closureAttr != -1 ) {
				assert( 0 <= closureAttr && closureAttr < intent.size());
				assert( intent[closureAttr] <= nodeCount);
				++intent[closureAttr];
			}
			// Switching to parent node
			const CPatritiaTree::TNodeIndex parentId = node->GetParent();
			if( parentId == -1) {
				break; // End of Patritia Tree
			}
			node = &pTree.GetNode(parentId);
		}
	}

	rapidjson::Document intentJson;
	rapidjson::MemoryPoolAllocator<>& alloc = intentJson.GetAllocator();
	intentJson.SetObject()
		.AddMember( "Names", rapidjson::Value().SetArray(), alloc );

	rapidjson::Value& names = intentJson["Names"];
	names.Reserve( intent.size(), alloc );

	for( int i = 0; i < intent.size(); ++i) {
		assert( intent[i] <= nodeCount);
		if( intent[i] < nodeCount) {
			// Not in all nodes, should be skipped
			continue;
		}
		const CPatritiaTree::TAttribute a = i;
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
		maxAttribute = max( maxAttribute, buffer.back());

		CPatritiaTree::TNodeIndex nodeId =  pTree.GetRoot(); // Root node with no generator attribute

		for( auto i = buffer.begin(); i!= buffer.end(); ++i) {
			nodeId = pTree.GetOrCreateAttributeNode(nodeId, *i);
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

	// // DEBUG: For printing the result
		// pTree.Print();

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

		currentNodeAttrs.clear();

		// Only the attributes after the last one can be in the closure,
		// since the previous children are generated with the attribute which definetly upson in the last children
		CPatritiaTree::TNodeIndex lastNodeId = *ch;
		auto& lastNode = pTree.GetNode(lastNodeId);

		// A flag indicating if all attributes from the last node are presented in the previous children
		bool areAllLastChildAttributesCommon = false;

		// Now if there are objects associated with the node, then these objects do not have other attributes and thus there is no attributes in the closure.
		if( nodeToObjectMap.find(*treeItr) == nodeToObjectMap.end() ) {
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
				pTree.MoveChild(*chItr, *treeItr);
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

// For every node in Patritia Tree computes the related attributes
void CStabilityLPCbyPatriciaTree::computeCommonAttributesinPT()
{
	CDeepFirstPatritiaTreeIterator treeItr(pTree);
	CPatritiaTreeNode::TAttributeSet attributes;
	for(; !treeItr.IsEnd(); ++treeItr) {
		switch(treeItr.Status()) {
		case CDeepFirstPatritiaTreeIterator::S_Return:
			continue;
		case CDeepFirstPatritiaTreeIterator::S_Forward: {
			if(treeItr->GenAttr != -1) {
				attributes.insert(treeItr->GenAttr);
			}
			for(int a = treeItr->ClosureAttrStart; a < treeItr->ClosureAttrEnd; ++a) {
				attributes.insert(pTree.GetClsAttribute(a));
			}
			treeItr->CommonAttributes = attributes;
			break;
		}
		case CDeepFirstPatritiaTreeIterator::S_Exit: {
			if(treeItr->GenAttr != -1) {
				const size_t n = attributes.erase(treeItr->GenAttr);
				assert(n == 1);
			}
			for(int a = treeItr->ClosureAttrStart; a < treeItr->ClosureAttrEnd; ++a) {
				const size_t n = attributes.erase(pTree.GetClsAttribute(a));
				assert(n == 1);
			}
			break;
		}
		default:
			assert(false);
		}
	}
}

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

		intentSet.clear();
		intentSet.insert(buffer.begin(), buffer.end());

		insertObjectToPTNode(pTree.GetRoot(), intentSet, nodeToObjectMap, objectId);
	}
}

// Inserts an intent to the node, the information in the intent can be updated for further processing
void CStabilityLPCbyPatriciaTree::insertObjectToPTNode(CPatritiaTree::TNodeIndex nodeId, set<int>& intent,
	std::multimap<CPatritiaTree::TNodeIndex, CPatritiaTree::TObject>& nodeToObjectMap, CPatritiaTree::TObject objectId)
{
	CPatritiaTreeNode& node = pTree.GetNode(nodeId);

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

		// First we iterate over all possible children of the node
		for(auto ch = node.Children.begin(); ch != node.Children.end(); ++ch) {
			const CPatritiaTree::TNodeIndex index = *ch;
			CPatritiaTreeNode& child = pTree.GetNode(index);
			child.CommonAttributes.insert(nextAttrs.begin(), nextAttrs.end());
		}

		auto range = nodeToObjectMap.equal_range(nodeId);
		if(range.first != range.second) {
			// Some objects are associated with the node, so they should be migrated to a new node
			const CPatritiaTree::TNodeIndex newNodeId = pTree.AddNode(nodeId, *nextAttrs.begin());
			CPatritiaTreeNode& newNode = pTree.GetNode(newNodeId);
			// Inserting attributes
			auto attr = nextAttrs.begin();
			++attr; // The first one is already a generator
			newNode.CommonAttributes.insert(attr, nextAttrs.end());

			//moving objects
			for(auto obj = range.first; obj != range.second; ++obj) {
				nodeToObjectMap.insert(pair<CPatritiaTree::TNodeIndex, CPatritiaTree::TObject>(newNodeId, obj->second));
			}
			nodeToObjectMap.erase(range.first, range.second);
		}
	}

	if(intent.empty()) {
		// We can add the object to this node
		nodeToObjectMap.insert(pair<CPatritiaTree::TNodeIndex, CPatritiaTree::TObject>(nodeId, objectId));
		return;
	}

	CPatritiaTree::TNodeIndex newNode = pTree.GetOrCreateAttributeNode( nodeId, *intent.begin() );
	intent.erase(intent.begin());
	insertObjectToPTNode(newNode, intent, nodeToObjectMap, objectId);
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

		for( int i = treeItr->ClosureAttrStart; i < treeItr->ClosureAttrEnd; ++i ) {
			const CPatritiaTree::TAttribute a = pTree.GetClsAttribute(i);
			treeItr->NextAttributeIntersections.insert(CPatritiaTreeNode::TNextAttributeIntersections::value_type(a, &pTree.GetNode(*treeItr)));
		}
		// Adding intersection information from children
		auto chItr = treeItr->Children.begin();
		for( ;chItr != treeItr->Children.end(); ++chItr) {
			const CPatritiaTreeNode* ch = &pTree.GetNode(*chItr);
			// Intersection with the generator
			treeItr->NextAttributeIntersections.insert(CPatritiaTreeNode::TNextAttributeIntersections::value_type(ch->GenAttr, ch));
			// Intersections known in the child
			treeItr->NextAttributeIntersections.insert(ch->NextAttributeIntersections.begin(), ch->NextAttributeIntersections.end());
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
	set<CPatritiaTree::TAttribute> intent;
	CDeepFirstPatritiaTreeIterator treeItr(pTree);
	CPatritiaTree::TObject maxObject = 0;
	for(; !treeItr.IsEnd(); ++treeItr) {
		switch(treeItr.Status()) {
		case CDeepFirstPatritiaTreeIterator::S_Return:
			break;
		case CDeepFirstPatritiaTreeIterator::S_Forward: {
			assert( intent.find(treeItr->GenAttr) == intent.end() );
			intent.insert(treeItr->GenAttr);
			for(int i = treeItr->ClosureAttrStart; i < treeItr->ClosureAttrEnd; ++i) {
				const CPatritiaTree::TAttribute a = pTree.GetClsAttribute(i);
				assert( intent.find(a) == intent.end() );
				intent.insert(a);
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
			assert( intent.find(treeItr->GenAttr) != intent.end() );
			intent.erase(treeItr->GenAttr);
			for(int i = treeItr->ClosureAttrStart; i < treeItr->ClosureAttrEnd; ++i) {
				const CPatritiaTree::TAttribute a = pTree.GetClsAttribute(i);
				assert( intent.find(a) != intent.end() );
				intent.erase(a);
			}
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
		maxAttribute = max( maxAttribute, buffer.back());

		const CPatritiaTreeNode* node = &pTree.GetNode(pTree.GetRoot());
		int processedAttrs = 0;
		for(int i = 0; i < buffer.size(); ++i) {
			assert( node != 0);
			// Marking closure attrs
			for(int j = node->ClosureAttrStart; j < node->ClosureAttrEnd; ++j) {
				const CPatritiaTree::TAttribute a = pTree.GetClsAttribute(j);
				auto res = std::find(buffer.begin(), buffer.end(), a);
				assert( res != buffer.end());
				*res = -1;
				++processedAttrs;
			}
			// Moving to the next node in the tree
			while(i < buffer.size() && buffer[i] == -1) {
				++i;
			}
			if( i >= buffer.size()) {
				break;
			}
			assert(buffer[i] != -1);
			const CPatritiaTree::TNodeIndex next = pTree.GetAttributeNode(*node, buffer[i]);
			// The next attribute should be in the tree
			assert( next != -1 );

			++processedAttrs;
			buffer[i] = -1;
			node = &pTree.GetNode(next);
		}

		// All atributes are somewhere found
		assert(processedAttrs == buffer.size());
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
	result->AddNewAttributeToIntent(a);
	return result.release();
}

bool CStabilityLPCbyPatriciaTree::initializePreimage(const CPTPattern& parent, int genAttr, CPTPattern& res)
{
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

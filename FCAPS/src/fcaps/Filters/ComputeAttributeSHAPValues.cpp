// Initial software, Aleksey Buzmakov

#include <fcaps/Filters/ComputeAttributeSHAPValues.h>

#include <fcaps/ContextAttributes.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/writer.h>

#include <vector>
#include <numeric>
#include <algorithm>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(...) #__VA_ARGS__

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CComputeAttributeShapValues> CComputeAttributeShapValues::registrar;

CComputeAttributeShapValues::CComputeAttributeShapValues() :
	rng(std::random_device{}()),
	outSuffix(".SHAP"),
	dlt(cmp),
	deltaThld(-1),
	budgetRnd(10000),
	budgetBF(10000),
	objectNum(0),
	curExtSize(-1)
{
}

void addSHAPValuesToNode(
		const std::deque<double>& shapValues, 
		rapidjson::Value& intent, 
		rapidjson::MemoryPoolAllocator<>& alloc, 
		bool should_reorder)
{
	assert(intent["Inds"].Size() == shapValues.size());

	intent.AddMember("ShapleyValues", rapidjson::Value().SetArray(), alloc );
	rapidjson::Value& shapVals = intent["ShapleyValues"];
	if( !should_reorder ) {
		for( int i = 0; i < shapValues.size(); ++i ) {
			shapVals.PushBack(shapValues[i], alloc);
		}
		return;
	} 

	std::vector<int> idx(shapValues.size());
	std::iota(idx.begin(), idx.end(), 0);

	std::sort(idx.begin(), idx.end(),
		[&shapValues](int i1, int i2) {return shapValues[i1] > shapValues[i2];} );
	for( int i = 0; i < idx.size(); ++i ) {
		const int id = idx[i];
		shapVals.PushBack(shapValues[id],alloc);
	}
	if(intent.HasMember("Inds") && intent["Inds"].IsArray() && intent["Inds"].Size() == idx.size()) {
		rapidjson::Value new_array(rapidjson::Value().SetArray(), alloc);	
		new_array.Reserve(idx.size(), alloc);
		for( int i = 0; i < idx.size(); ++i ) {
			const int id = idx[i];
			new_array.PushBack(intent["Inds"][id], alloc);
		}
		intent["Inds"]=new_array;
	}
	if(intent.HasMember("Names") && intent["Names"].IsArray() && intent["Names"].Size() == idx.size()) {
		assert(intent["Names"].Size() == idx.size());
		rapidjson::Value new_array(rapidjson::Value().SetArray(), alloc);	
		new_array.Reserve(idx.size(), alloc);
		for( int i = 0; i < idx.size(); ++i ) {
			const int id = idx[i];
			new_array.PushBack(intent["Names"][id], alloc);
		}
		intent["Names"]=new_array;
	}
}

void CComputeAttributeShapValues::Process()
{
	results.clear();
	if(outFile.empty()) {
		results.push_back(latticeFile + outSuffix);
	} else {
		results.push_back(outFile);
	}

	static const char place[] = "CComputeAttributeShapValues::Process";
	CJsonError error(latticeFile, "");

	rapidjson::Document doc;

	// Loading lattice
	{
		CJsonFile file(latticeFile);
		if( !file.Read(  doc, error ) ) {
			throw new CJsonException( place, error );
		}
	}
	if( !doc.IsArray() || doc.Size() < 2 || !doc[1].IsObject()
		|| !doc[1].HasMember("Nodes") || !doc[1]["Nodes"].IsArray() )
	{
		error.Error = "InputLattice[1].Nodes not found. Not a valid lattice.";
		throw new CJsonException( place, error );
	}
	if(deltaThld <= 0 ) {	
		if( doc[0].IsObject() && doc[0].HasMember("Params") && doc[0]["Params"].IsObject()
				&& doc[0]["Params"].HasMember("Params") && doc[0]["Params"]["Params"].HasMember("Thld")
				&& doc[0]["Params"]["Params"]["Thld"].IsDouble() )
		{
			deltaThld = std::lround(doc[0]["Params"]["Params"]["Thld"].GetDouble());
		}
	}
	if(deltaThld <= 0 ) {	
		deltaThld = 1;
	}

	rapidjson::MemoryPoolAllocator<>& alloc = doc.GetAllocator();
	rapidjson::Value& inputNodes = doc[1]["Nodes"];

	assert(inputNodes.IsArray());

	for(int i = 0; i < inputNodes.Size(); ++i) {
		rapidjson::Value& cpt = inputNodes[i];
		curExtSize=-1;
		curIntentInds.clear();
		curExtent.reset();
		curSHAPValues.clear();

		if(cpt.HasMember("Ext") && cpt["Ext"].IsInt()) {
			curExtSize = cpt["Ext"].GetInt();
		}
		if(cpt.HasMember("Ext") && cpt["Ext"].IsObject() 
				&& cpt["Ext"].HasMember("Count") && cpt["Ext"]["Count"].IsInt() )
		{
			curExtSize = cpt["Ext"]["Count"].GetInt();
		}
		if(curExtSize <= 0 ) {
			std::cout << "Concept " << i << " has no extent size information. IGNORED." << std::endl;
			continue;
		}

		if(cpt.HasMember("Int") && cpt["Int"].IsObject() 
				&& cpt["Int"].HasMember("Inds") && cpt["Int"]["Inds"].IsArray() )
		{
			rapidjson::Value& inds = cpt["Int"]["Inds"];
			int a = 0;
			for(; a < inds.Size(); ++a) {
				rapidjson::Value& attr = inds[a];
				if(!attr.IsInt()) {
					std::cout << "Concept " << i << " intent has bad indices. The concept is IGNORED." << std::endl;
					break;
				}
				curIntentInds.push_back(attr.GetInt());
				if(curIntentInds.back() < 0 ) {
					std::cout << "Concept " << i << " intent has bad indices. The concept is IGNORED." << std::endl;
					break;
				}
			}
			if(a < inds.Size() ) {
				continue;
			}

		} else if(cpt.HasMember("Int") && cpt["Int"].IsObject()) {
			std::cout << "Concept " << i << " has no intent indices information size information."
				<< std::endl << "Accesing attributes by names is not supported yet." 
				<< std::endl << "The concept is IGNORED." << std::endl;
			continue;
		} else {
			std::cout << "Concept " << i << " has no intent information information. IGNORED." << std::endl;
			continue;
		}
		std::cout << "\rProcessing Concept " << i << " out of " << inputNodes.Size() <<"...";
		std::cout.flush();

		computeSHAPValues();
		addSHAPValuesToNode(curSHAPValues, cpt["Int"], alloc, shouldReorderAttributes);
	}

	// Updating params of the lattice
	JSON json = SaveParams();
	rapidjson::Document thisParams;
	if( !ReadJsonString( json, thisParams, error ) ) {
		assert(false);
	}
	if( !doc[0].HasMember("Filters") ) {
		doc[0].AddMember( "Filters", rapidjson::Value().SetArray(), alloc );
	}
	doc[0]["Filters"].PushBack(thisParams, alloc );

	// Saving new lattice
	JSON outputStr;
	CreateStringFromJSON( doc, outputStr, true );
	CDestStream dst(results[0]);
	dst << outputStr;
}

void CComputeAttributeShapValues::LoadParams( const JSON& json )
{
	static const char place[] = "CComputeAttributeShapValues::LoadParams";
	CJsonError error;
	error.Data = json;
	rapidjson::Document doc;
	if( !ReadJsonString( json, doc, error ) ) {
		throw new CJsonException( place, error );
	}
	assert( string( doc["Type"].GetString() ) == Type() );
	assert( string( doc["Name"].GetString() ) == Name() );
	if( !(doc.HasMember( "Params" ) && doc["Params"].IsObject()) ) {
		error.Error = "Params is not found. Necessary for input files.";
		throw new CJsonException(place, error);
	}
	const rapidjson::Value& params=doc["Params"];

	if( !params.HasMember("Context") || !params["Context"].IsString() ) {
		error.Error = "'Context' not found";
		throw new CJsonException(place, error);
	} else {
		contextFile = params["Context"].GetString();
	}
	// Reading context
	std::string attrsContextParams =
	std::string(STR(
			{
			"Type":"ContextAttributesModules",
			"Name":"JsonContextAttributesModule",
			"Params":{
				"ContextFilePath":)) + '"' + contextFile + '"' + STR(,
				"Order":"None"
			}
			}
	   );
	error.Data = attrsContextParams;
	CSharedPtr<IContextAttributes> attrContext(CreateModule<IContextAttributes>(
			"ContextAttributesModules", "JsonContextAttributesModule",
			attrsContextParams));
	if( attrContext == 0 ) {
		throw new CJsonException( place, error );
	}
	objectNum = attrContext->GetObjectNumber();
	cmp.SetMaxAttrNumber(objectNum);
	CPatternImage image;
	for(int a = 0; attrContext->HasAttribute(a);a = attrContext->GetNextAttribute(a)) {
		attrContext->GetAttribute(a, image);
		std::unique_ptr<CVectorBinarySetDescriptor> desc(cmp.NewPattern()); 
		for(int i = 0; i < image.ImageSize; ++i ) {
			const int objectId = image.Objects[i];
			cmp.AddValue(objectId, *desc);
		}
		context.push_back(CSharedPtr<const CVectorBinarySetDescriptor>(desc.release(), dlt));
		attrContext->ClearMemory(image);
	}

	std::string outSuffix = ".SHAP";
	if( params.HasMember("OutSuffix") && params["OutSuffix"].IsString() ) {
		outSuffix = params["OutSuffix"].GetString();
	}
	if( params.HasMember("OutFile") && params["OutFile"].IsString() ) {
		outFile = params["OutFile"].GetString();
	}
	
	if( params.HasMember("DeltaThld") && params["DeltaThld"].IsInt() ) {
		deltaThld = params["DeltaThld"].GetInt();
	}
	if( params.HasMember("BudgetRnd") && params["BudgetRnd"].IsInt() ) {
		budgetRnd = params["BudgetRnd"].GetInt();
	}
	if( params.HasMember("BudgetBruteForce") && params["BudgetBruteForce"].IsInt() ) {
		budgetBF = params["BudgetBruteForce"].GetInt();
	}
	if( params.HasMember("ShouldReorderAttributes") && params["ShouldReorderAttributes"].IsBool() ) {
		shouldReorderAttributes = params["ShouldReorderAttributes"].GetBool();
	}

	results.clear();
}

JSON CComputeAttributeShapValues::SaveParams() const
{
	assert( results.size() == 1 );
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", LatticeFilterModuleType, alloc )
		.AddMember( "Name", ComputeAttributeShapValuesFilter, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "Lattice", rapidjson::Value().SetString(
				rapidjson::StringRef(latticeFile.c_str())), alloc )
			.AddMember( "Context", rapidjson::Value().SetString(
				rapidjson::StringRef(contextFile.c_str())), alloc )
			.AddMember( "OutFile", rapidjson::Value().SetString(
				rapidjson::StringRef(results[0].c_str())), alloc )
			.AddMember( "DeltaThld", rapidjson::Value().SetInt(deltaThld), alloc )
			.AddMember( "BudgetRnd", rapidjson::Value().SetInt(budgetRnd), alloc )
			.AddMember( "BudgetBruteForce", rapidjson::Value().SetInt(budgetBF), alloc )
			.AddMember( "ShouldReorderAttributes", rapidjson::Value().SetBool(shouldReorderAttributes), alloc ),
		alloc );
	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

inline int choose(int n, int k)
{
	double result = 1;
	for(int i = k; i > 0; --i) {
		result *= n-k+i;
		result /= i;
	}
	return std::lround(result);
}

// Computes SHAP values for every attribute in the intent
void CComputeAttributeShapValues::computeSHAPValues()
{
	for(int i = 0; i < curIntentInds.size(); ++i) {
		const int attr = curIntentInds[i];
		assert(attr >= 0);
		int attrBFBudget = 0;
		double attrBFShap = 0;
		int size = 0;

		// In the bruteforse part we increase the size of the number of attributes in the starting set,
		//  while budget is available and compute the exact SHAP value.
		while(attrBFBudget < budgetBF && size <= (curIntentInds.size()-1) / 2) {
			attrBFShap += computeExactShap(attr, size);
			attrBFBudget += choose(curIntentInds.size()-1, size);
			if(curIntentInds.size() -1 - size != size) {
				attrBFShap += computeExactShap(attr, curIntentInds.size() -1 - size);
				attrBFBudget += choose(curIntentInds.size()-1, size);
			}
			++size;
		}

		int attrRndShap = 0;
		int attrRndN = 0;
		double rndAttrShap = 0;
		if( size <= (curIntentInds.size()-1) / 2) {
			const int totalSizesForRnd = (curIntentInds.size()-1-size - size +1);
			const int attrRndBudgetForSize = budgetRnd / totalSizesForRnd + 1;
			std::uniform_int_distribution<int> size_gen(size,curIntentInds.size()-1 - size);
			for( int s = size; s <= curIntentInds.size()-1 - size; ++s) {
				// There are some sizes of attributes sets there were not considered
				for(int trial = 0; trial < attrRndBudgetForSize; ++trial ) {
					analysedIntent.clear();
					//const int s = size_gen(rng);
					std::sample(
						curIntentInds.begin(), curIntentInds.end(),
						std::back_inserter(analysedIntent), s+1, rng);
					auto curAttrItr = std::find(analysedIntent.begin(), analysedIntent.end(), attr);
					if( curAttrItr != analysedIntent.end() ) {
						*curAttrItr = -1;
					} else {
						analysedIntent.pop_back();
					}
					attrRndShap += isAttrImportant(attr);
					++attrRndN;
				}
			}
			assert(attrRndN > 0);
			rndAttrShap = 1.0 * attrRndShap / attrRndN * totalSizesForRnd;
		}
		curSHAPValues.push_back((attrBFShap+rndAttrShap)/(curIntentInds.size()));
	}
}
double CComputeAttributeShapValues::computeExactShap(int attr, int size)
{
	analysedIntent.clear();
	for(int i = 0; i < curIntentInds.size(); ++i) {
		if(curIntentInds[i] == attr) {
			return 1.0 * computeExactShapGenIntent(i, -1, size) / choose(curIntentInds.size()-1, size);
		}
	}
	assert(false);
	return -1;
}
int CComputeAttributeShapValues::computeExactShapGenIntent(int attrIntentIndex, int prevAttrIndex, int size)
{
	if( size > 0 ) {
		int totalAttrIsImportant = 0;
		for(int a = prevAttrIndex+1; a < curIntentInds.size()-1 - size + 1; ++a) {
			if(a < attrIntentIndex) {
				analysedIntent.push_back(curIntentInds[a]);
			} else {
				analysedIntent.push_back(curIntentInds[a+1]);
			}
			totalAttrIsImportant += computeExactShapGenIntent(attrIntentIndex, a, size-1);
			analysedIntent.pop_back();
		}	
		return totalAttrIsImportant;
	} else {
		return isAttrImportant(curIntentInds[attrIntentIndex]);
	}
}
bool CComputeAttributeShapValues::isAttrImportant(int attr)
{
	curExtent.reset();
	ignoredAttrs.clear();

	computeExtent();
	if(curExtent != 0 && curExtent->Size() == curExtSize) {
		// Closure is without the attribute is equal to the whole closure
		return false;
	} else {
		computeExtent(attr);
		assert(curExtent != 0);
		return curExtent->Size() == curExtSize;
	}
}

// Computes extent by intersecting attributes in analysedIntent and then closing the extent by Delta-closure
void CComputeAttributeShapValues::computeExtent()
{
	for(int i = 0; i < analysedIntent.size(); ++i) {
		if(analysedIntent[i] == -1) {
			continue;
		}
		curExtent =intersectAttrWithConcept(analysedIntent[i]);
		ignoredAttrs.insert(analysedIntent[i]);
	}
	closeExtent();
}
// Computes extent by intersecting curExtent with the attribute attr and then closing the extent by Delta-closure
void CComputeAttributeShapValues::computeExtent(int attr)
{
	curExtent =intersectAttrWithConcept(attr);
	ignoredAttrs.insert(attr);
	closeExtent();
}
// Just adds the attr to concept, i.e., just intersects curExtent with the extent of attr
CSharedPtr<const CVectorBinarySetDescriptor> CComputeAttributeShapValues::intersectAttrWithConcept(int attr)
{
	if(curExtent == 0) {
		return context[attr];
	} else {
		return CSharedPtr<const CVectorBinarySetDescriptor>(cmp.CalculateSimilarity(*context[attr], *curExtent), dlt);
	}
}

// Closes curExtent by DeltaClosure using attributes out of analysedIntent
void CComputeAttributeShapValues::closeExtent()
{
	while(curExtent == 0 || curExtent->Size() != curExtSize) {
		bool hasAddedAttribute = false;
		for(int i = 0; i < curIntentInds.size(); ++i) {
			const int a = curIntentInds[i];
			if(ignoredAttrs.count(a) > 0) {
				continue;
			}
			auto intersection = intersectAttrWithConcept(a);
			if(curExtent == 0 && objectNum - intersection->Size() < deltaThld
				|| curExtent != 0 && curExtent->Size() - intersection->Size() < deltaThld) 
			{
				ignoredAttrs.insert(a);
				curExtent = intersection;
				hasAddedAttribute = true;
				break;
			}
		}
		if(!hasAddedAttribute) {
			break;
		}
	}
}



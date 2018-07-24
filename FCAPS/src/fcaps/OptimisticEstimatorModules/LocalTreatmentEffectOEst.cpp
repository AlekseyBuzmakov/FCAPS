// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/OptimisticEstimatorModules/LocalTreatmentEffectOEst.h>
#include <fcaps/Extent.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/document.h>

#include <boost/sort/spinsort/spinsort.hpp>
#include <boost/math/special_functions/beta.hpp>
#include <sstream>
#include <math.h>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(x...) #x

const char description[] =
STR(
	{
	"Name":"Local Treatment Effect Optimistic Estimator",
	"Description":"An optimistic estimator for computing upper bound of Local Treatment Effect given as a linear function of difference between confidence intervals of Test and Control groups and the total number of objects in the extent of a pattern.",
	"Params": {
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of LTE OEst",
			"type": "object",
			"properties": {
				"ControlLevels":{
					"description": "Array fo strings that gives group names for the objects that correspond to the control set. All others objects are considered to be in a test set",
					"type":"array",
					"items": {
						"type":"string"
					}
				},
				"ObjInfoFile":{
					"description": "The path to a file, containing the description of every object",
					"type": "file-path",
					"example": {
						"description":"File contains information on Values of a response variable (Y) for every object in a dataset, and the group (Trt) the object is pelonging to. Example describes 3 objects.",
						"json":{"Y":[1.0,2.2,-1.5], "Trt": ["Test","Test","Control"]}
					}
				},
				"MinObjNum":{
					"description": "The minimal number of objects in a control a treatment group",
					"type": "integer",
					"minimum": 10
				},
				"Alpha":{
					"description": "The weight of distance between confidence intervals. The distance is normalized to the maximal possible distance",
					"type": "number",
					"minimum": 0
				},
				"Beta":{
					"description": "The weight number of objects in the pattern. The number of objects is normalized to the total number of objects in the dataset",
					"type": "number",
					"minimum": 0
				},
				"SignificanceLevel":{
					"description": "The significance level that is used to compute the confidence interval",
					"type": "number",
					"minimum": 0,
					"exclusiveMinimum": true,
					"maximum": 1,
					"exclusiveMaximum": true
				}
			}
		}
	}
);

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CLocalTreatmentEffectOEst> CLocalTreatmentEffectOEst::registrar(
	                 OptimisticEstimatorModuleType, LocalTreatmentEffectOptimisticEstimator );
const char* const CLocalTreatmentEffectOEst::Desc()
{
	return description;
}

CLocalTreatmentEffectOEst::CLocalTreatmentEffectOEst() :
	signifLevel(0.01),
	minObjNum(100),
	controlSize(0),
	alpha(1),
	beta(1)
{
	
}

double CLocalTreatmentEffectOEst::GetValue(const IExtent* ext) const
{
	assert(ext!=0);
	extractObjValues(ext);
	const double delta = objValues.TestConfLowBound.front()
		- objValues.CntrlConfUpperBound.back();

	if( delta <= 0 ) {
		return 0;
	}
	return getValue(delta,ext->Size()); // Only linear functions are supported
}

double CLocalTreatmentEffectOEst::GetBestSubsetEstimate(const IExtent* ext) const
{
	assert(ext!=0);

	extractObjValues(ext);

	objValues.CntrlToTestPosition.resize(objValues.Cntrl.size());
	objValues.BestTestValueChange.resize(objValues.Test.size());

	std::fill(objValues.CntrlToTestPosition.begin(),objValues.CntrlToTestPosition.end(),0);
	std::fill(objValues.BestTestValueChange.begin(),objValues.BestTestValueChange.end(),0.0);

	// For every position of the cntrl, we find the clossest position in the test set
	int curTestObj=0;
	for(int i = 0; i < objValues.CntrlToTestPosition.size(); ++i) {
		const double cntrlY = objValues.CntrlConfUpperBound[i];
		while(curTestObj < objValues.Test.size()) {
			const double testY = objValues.TestConfLowBound[curTestObj];
			if(testY > cntrlY) {
				break;
			}
			++curTestObj;
		}
		objValues.CntrlToTestPosition[i] = curTestObj <= objValues.Test.size()-minObjNum ? curTestObj : -1;
	}

	// Now for the test set we should know for every object the following.
	//  If the test set contains only last elements starting at [i] what is the optimal size
	const int minNumObjTestPosition = objValues.Test.size()-minObjNum;
	double lastBestQ = 0;
	for(int testStart = minNumObjTestPosition - 1; testStart >= 0; --testStart ) {
		const double diff = getValue(
			objValues.TestConfLowBound[testStart] - objValues.TestConfLowBound[minNumObjTestPosition], minNumObjTestPosition - testStart);
		lastBestQ = max(lastBestQ, diff);
		assert(testStart < objValues.BestTestValueChange.size());
		objValues.BestTestValueChange[testStart]=lastBestQ;
	}

	assert(checkObjValues());

	//  Now just iterate over possible conf interval position in cntrl set and finds the corresponding the best value in the test set
	const double minObjNumValue = getValue(objValues.TestConfLowBound[minNumObjTestPosition] - objValues.CntrlConfUpperBound[minObjNum - 1], 2 * minObjNum); 
	double bestDiff = 0;
	for(int i = minObjNum; i < objValues.CntrlToTestPosition.size(); ++i) {
		const int testStart = objValues.CntrlToTestPosition[i];
		assert( objValues.CntrlConfUpperBound[i] < objValues.TestConfLowBound[testStart] );
		const double currDiff =
			getValue(objValues.CntrlConfUpperBound[minObjNum - 1] - objValues.CntrlConfUpperBound[i],i - minObjNum + 1)
			+ objValues.BestTestValueChange[testStart];
		assert(abs(minObjNumValue + currDiff - getBestValueForSubsets(i,testStart)) < delta0 * 1e-10);
		bestDiff = max(bestDiff, currDiff);
	}

	return bestDiff + minObjNumValue;
}

JSON CLocalTreatmentEffectOEst::GetJsonQuality(const IExtent* ext) const
{
	std::stringstream rslt;
	extractObjValues(ext);
	const double delta = objValues.TestConfLowBound.front()
		- objValues.CntrlConfUpperBound.back();
	rslt << "{"
		<< "\"Delta\":" << delta << ","
		<< "\"Delta0\":" << delta0 << ","
		<< "\"N0\":" << objValues.Cntrl.size() << ","
		<< "\"N1\":" << objValues.Test.size() << ","
		<< "\"TotalSize\":" << objY.size() << ","
		<< "\"Value\":" << GetValue(ext)
		<<"}";
	return rslt.str();
}

void CLocalTreatmentEffectOEst::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CLocalTreatmentEffectOEst::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == OptimisticEstimatorModuleType );
	assert( string( params["Name"].GetString() ) == LocalTreatmentEffectOptimisticEstimator );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for Object Information";
		throw new CJsonException("CLocalTreatmentEffectOEst::LoadParams", error);
	}

	const rapidjson::Value& p = params["Params"];

	// Reading Levels that are considered as positive
	if(!(p.HasMember("ControlLevels") && (p["ControlLevels"].IsArray()))) {
		error.Data = json;
		error.Error = "Params.ControlLevels is not found or is not an 'Array'.";
		throw new CJsonException("CLocalTreatmentEffectOEst::LoadParams", error);
	}
	for( int i = 0 ; i < p["ControlLevels"].Size(); ++i ) {
		if(!p["ControlLevels"][i].IsString()) {
			error.Data = json;
			error.Error = "Params.ControlLevels has nonstring values.";
			throw new CJsonException("CLocalTreatmentEffectOEst::LoadParams", error);
		}
		cntrlLevels.insert(p["ControlLevels"][i].GetString());
	}

	// Reading class labels information
	if(!(p.HasMember("ObjInfoFile") && p["ObjInfoFile"].IsString())) {
		error.Data = json;
		error.Error = "Params.ObjInfoFile is not found or is not a file name.";
		throw new CJsonException("CLocalTreatmentEffectOEst::LoadParams", error);
	}

	rapidjson::Document objInfoDocument;
	objInfoFilePath = p["ObjInfoFile"].GetString();
	if( !ReadJsonFile( objInfoFilePath, objInfoDocument, error ) ) {
		throw new CJsonException( "CLocalTreatmentEffectOEst::LoadParams", error );
	}

	if(!objInfoDocument.IsObject()
	   || !objInfoDocument.HasMember("Y") || !objInfoDocument["Y"].IsArray()
	   || !objInfoDocument.HasMember("Trt") || !objInfoDocument["Trt"].IsArray()
	   || objInfoDocument["Y"].Size() != objInfoDocument["Trt"].Size())
	{
		throw new CTextException( "CLocalTreatmentEffectOEst::LoadParams", "Object Info is not an object; or does not contains 'Y' or 'Trt' arrays; or their sizes are different" );
	}

	const rapidjson::Value& y = objInfoDocument["Y"];
	const rapidjson::Value& trt = objInfoDocument["Trt"];
	assert(y.Size() == trt.Size());

	objY.resize(y.Size());
	objTrt.resize(trt.Size());
	for( int i = 0; i < trt.Size(); ++i ) {
		if(!y[i].IsNumber()) {
			error.Data = json;
			error.Error = "Params.ObjInfoFile.Y has nonnumeric values.";
			throw new CJsonException("CLocalTreatmentEffectOEst::LoadParams", error);
		}
		if(!trt[i].IsString()) {
			error.Data = json;
			error.Error = "Params.ObjInfoFile.Trt has nonstring values.";
			throw new CJsonException("CLocalTreatmentEffectOEst::LoadParams", error);
		}
		objTrt[i]=cntrlLevels.find(trt[i].GetString()) != cntrlLevels.end();
		objY[i]=y[i].GetDouble();
	}
	if(p.HasMember("MinObjNum") && p["MinObjNum"].IsInt()) {
		minObjNum=p["MinObjNum"].GetInt();
	}
	if(p.HasMember("Alpha") && p["Alpha"].IsNumber()) {
		const double a =p["Alpha"].GetDouble();
		alpha=a;
	}
	if(p.HasMember("Beta") && p["Beta"].IsNumber()) {
		const double b =p["Beta"].GetDouble();
		beta=b;
	}
	if(p.HasMember("SignificanceLevel") && p["SignificanceLevel"].IsNumber()) {
		const double sl =p["SignificanceLevel"].GetDouble();
		if( 0 < sl && sl < 1) {
			signifLevel=sl;
		}
	}

	computeSignificantObjectNumbers();
	buildOrder();
	computeDelta0();
}

JSON CLocalTreatmentEffectOEst::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", OptimisticEstimatorModuleType, alloc )
		.AddMember( "Name", LocalTreatmentEffectOptimisticEstimator, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
					.AddMember("ObjInfoFile", rapidjson::StringRef(objInfoFilePath.c_str()), alloc)
		            .AddMember("MinObjNum",rapidjson::Value().SetInt(minObjNum), alloc)
		            .AddMember("Alpha",rapidjson::Value().SetDouble(alpha), alloc)
		            .AddMember("Beta",rapidjson::Value().SetDouble(beta), alloc)
		            .AddMember("SignificanceLevel",rapidjson::Value().SetDouble(signifLevel), alloc)
				, alloc );

	rapidjson::Value& p = params["Params"];

	rapidjson::Value& controlLevels = p.AddMember( "ControlLevels", rapidjson::Value().SetArray(), alloc )["ControlLevels"];
	for(auto it = cntrlLevels.begin(); it != cntrlLevels.end(); ++it) {
		controlLevels.PushBack(rapidjson::StringRef((*it).c_str()), alloc);
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}
bool CLocalTreatmentEffectOEst::operator()(int a, int b) const
{
	assert(objY.size() == objTrt.size());
	assert( 0 <= a && a < objY.size());
	assert( 0 <= b && b < objY.size());
	if(objTrt[a] < objTrt[b]) {
		return true;
	} else if(objTrt[a] > objTrt[b]) {
		return false;
	}
	return objY[a] < objY[b];
}

// For every number of objects compute the object number that corresponds to a boundary of the median with the desired significance level
void CLocalTreatmentEffectOEst::computeSignificantObjectNumbers()
{
	assert(objY.size() == objTrt.size());
	const int n = objY.size();
	signifObjectNum.resize(n);
	signifObjectNum[0]=0;
	signifObjectNum[1]=0;
	for( int i = 2; i < n; ++i) {
		int j = signifObjectNum[i-1]+1;
		for( ; j < i; ++j ) {
			if( incompleteBeta(j, i - j + 1) < 1 -signifLevel) {
				break;
			}
		} 
		assert(j < i);
		signifObjectNum[i]= j - 1;
	}
}

// Computes value for incomplete beta function i and j are positive integers
double CLocalTreatmentEffectOEst::incompleteBeta(int i, int j)
{
	return boost::math::ibeta(i,j,0.5);
}

// sort the objects in increasing order
void CLocalTreatmentEffectOEst::buildOrder()
{
	// order
	order.resize(objY.size());
	for(int i = 0; i < objY.size(); ++i) {
		order[i]=i;
	}
	boost::sort::spinsort(order.begin(), order.end(), *this );
	// inverted order
	posToOrder.resize(order.size());
	for(int i = 0; i < objY.size(); ++i) {
		posToOrder[order[i]]=i;
	}
	// size of the control group
	controlSize=0;
	for(int i = 0; i < objY.size(); ++i){
		if(objTrt[order[i]]) {
			break;
		}
		++controlSize;
	}
}

// Computes delta for the whole dataset
void CLocalTreatmentEffectOEst::computeDelta0()
{
	// TODO change function for dealing with other measures than median confidence interval
	const int sObjNum = signifObjectNum[minObjNum];
	assert(sObjNum < minObjNum);
	assert(sObjNum < controlSize);
	assert(sObjNum < objY.size() - controlSize);
	const double cntrlY = objY[order[minObjNum - 1 - sObjNum + 1]];
	const double testY = objY[order[objY.size() - minObjNum + sObjNum - 1]];
   
	delta0=testY-cntrlY;
}

// Extracts test and control group for the extent
void CLocalTreatmentEffectOEst::extractObjValues(const IExtent* ext) const
{
	assert(ext!=0);
	
	CPatternImage img;
	CPatternImageHolder imgHolder(ext,img);

	currentObjects.resize(objY.size());
	std::fill(currentObjects.begin(),currentObjects.end(),false);

	// Marking currentObjects in the sorting order
	int cntrlSize=0;
	for( int i = 0; i < img.ImageSize; ++i) {
		assert(0 <= img.Objects[i] && img.Objects[i] < posToOrder.size());
		assert(0 <= posToOrder[img.Objects[i]] && posToOrder[img.Objects[i]] < currentObjects.size());
		assert(!currentObjects[posToOrder[img.Objects[i]]]);
		currentObjects[posToOrder[img.Objects[i]]]=true;
		cntrlSize += !objTrt[img.Objects[i]];
		assert((objTrt[img.Objects[i]]) == (posToOrder[img.Objects[i]]>=controlSize));
	}

	// Extracting double values from marked objects
	objValues.Cntrl.resize(cntrlSize);
	objValues.Test.resize(img.ImageSize - cntrlSize);
	int nCntrl=0;
	int nTest=0;
	for( int i = 0; i < currentObjects.size(); ++i) {
		if( !currentObjects[i]) {
			continue;
		}
		if( i < controlSize) {
			assert(nCntrl < objValues.Cntrl.size());
			assert(!objTrt[order[i]]);
			objValues.Cntrl[nCntrl] = objY[order[i]];
			++nCntrl;
		} else {
			assert(nTest < objValues.Test.size());
			assert(objTrt[order[i]]);
			objValues.Test[nTest] = objY[order[i]];
			++nTest;
		}
	}
	assert( nCntrl + nTest == img.ImageSize);

	computeConfidenceIntervalBounds();
}

// Computes the confidence interval bounds for Test and Control sets
void CLocalTreatmentEffectOEst::computeConfidenceIntervalBounds() const
{
	objValues.TestConfLowBound.resize(objValues.Test.size());
	objValues.CntrlConfUpperBound.resize(objValues.Cntrl.size());

	// Computing low bonuds for the Test set
	for( int testStart = objValues.Test.size() - minObjNum; testStart >= 0; --testStart ) {
		const int currConfIntObj = testStart + signifObjectNum[objValues.Test.size() - testStart]-1;
		objValues.TestConfLowBound[testStart] = objValues.Test[currConfIntObj];
	}
	for( int testStart = objValues.Test.size() - minObjNum + 1; testStart < objValues.Test.size(); ++testStart ) {
		objValues.TestConfLowBound[testStart] = max(objValues.Test[testStart],objValues.TestConfLowBound[objValues.Test.size() - minObjNum]);
	}

	// Computing upper bound for Control set
	for( int cntrlSize = minObjNum; cntrlSize <= objValues.Cntrl.size(); ++cntrlSize ) {
		const int currConfIntObj = cntrlSize - signifObjectNum[cntrlSize];
		objValues.CntrlConfUpperBound[cntrlSize - 1] = objValues.Cntrl[currConfIntObj];
	}
	for( int cntrlSize = minObjNum - 1; cntrlSize > 0; --cntrlSize ) {
		objValues.CntrlConfUpperBound[cntrlSize - 1] = min(objValues.Cntrl[cntrlSize - 1], objValues.CntrlConfUpperBound[minObjNum - 1]);
	}
}

// Computes a linear function of delta and size
double CLocalTreatmentEffectOEst::getValue(const double& delta, int size) const
{
	return alpha * delta/delta0  + beta * size/objY.size();
}

// Finds the best possible value if cntrl confidence interval is fixed
double CLocalTreatmentEffectOEst::getBestValueForSubsets(int cntrlLastObject, int testFirstObject) const
{
	const int nCntrl = cntrlLastObject + 1;

	// Finding the best possible test size
	int bestTestStart = testFirstObject;
	double bestValue = 0;
	for( int testStart = testFirstObject; testStart <= objValues.Test.size() - minObjNum; ++testStart) {
		const double delta = objValues.TestConfLowBound[testStart] - objValues.CntrlConfUpperBound[cntrlLastObject];
		const int nTest = objValues.Test.size() - testStart;
		const double value = getValue(delta, nTest+nCntrl);
		if( value > bestValue) {
			bestValue = value;
			bestTestStart = testStart;
		}
	}
	return bestValue;
}


// Checks the validity of objValues object.
bool CLocalTreatmentEffectOEst::checkObjValues() const
{
	// objValues.Test
	double prevValue = objValues.Test[0];
	for(int i = 1; i < objValues.Test.size(); ++i) {
		if( objValues.Test[i] < prevValue ) {
			// Array should be sorted
			return false;
		}
		prevValue = objValues.Test[i];
	}
	// objValues.Cntrl
	prevValue = objValues.Cntrl[0];
	for(int i = 1; i < objValues.Cntrl.size(); ++i) {
		if(objValues.Cntrl[i] < prevValue) {
			// should be sorted
			return false;
		}
		prevValue = objValues.Cntrl[i];
	}
	// objValues.CntrlConfUpperBound
	prevValue = objValues.CntrlConfUpperBound[0];
	for(int i = 1; i < objValues.CntrlConfUpperBound.size(); ++i) {
		if(objValues.CntrlConfUpperBound[i] < prevValue) {
			// should be sorted
			return false;
		}
		prevValue = objValues.CntrlConfUpperBound[i];
	}
	// objValues.TestConfLowBound
	prevValue = objValues.TestConfLowBound[0];
	for(int i = 1; i < objValues.TestConfLowBound.size(); ++i) {
		if(objValues.TestConfLowBound[i] < prevValue) {
			// should be sorted
			return false;
		}
		prevValue = objValues.TestConfLowBound[i];
	}
	// objValues.CntrlToTestPosition
	for( int i = minObjNum-1; i < objValues.CntrlToTestPosition.size(); ++i ) {
		const int testPos = objValues.CntrlToTestPosition[i];
		if( testPos == -1 && objValues.CntrlConfUpperBound[i] < objValues.TestConfLowBound[objValues.Test.size() - minObjNum] ) {
			// -1 pos means that it is not possible to find any larger value in the Test set respecting the minimal size of the tidset
			return false;
		}
		assert( i < objValues.Cntrl.size() );
		assert( testPos < objValues.Test.size() );
		if( objValues.CntrlConfUpperBound[i] >= objValues.TestConfLowBound[testPos]) {
			// The returned value should be lager
			return false;
		}
		if( testPos > 0 && objValues.CntrlConfUpperBound[i] < objValues.TestConfLowBound[testPos-1]) {
			// The testPos should be the minimal available number
			return false;
		}
	}

	// objValues.BestTestValueChange
	// ??

	return true;
}

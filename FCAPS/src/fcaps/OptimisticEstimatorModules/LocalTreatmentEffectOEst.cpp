// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/OptimisticEstimatorModules/LocalTreatmentEffectOEst.h>
#include <fcaps/Extent.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/document.h>

#include <boost/math/special_functions/beta.hpp>
#include <sstream>
#include <math.h>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(...) #__VA_ARGS__

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
				"kDelta":{
					"description": "The weight of distance between confidence intervals. The distance is normalized to the maximal possible distance",
					"type": "number",
					"minimum": 0
				},
				"kN":{
					"description": "The weight number of objects in the pattern. The number of objects is normalized to the total number of objects in the dataset",
					"type": "number",
					"minimum": 0
				},
				"ConfidenceIntervalMode":{
					"description": "How to compute the confidence interval. Options are: 'median', '2sigmas', and 'ttest'.",
					"type": "string"
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
	zp(2.56),
	minObjNum(100),
	controlSize(0),
	delta0Max(0),
	delta0(0),
	alpha(1),
	beta(1),
	confIntMode(CIM_Median)
{
	
}

void CLocalTreatmentEffectOEst::GetValue(const IExtent* ext, COEstValue& val ) const
{
	assert(ext!=0);
	if( !extractObjValues(ext) ) {
		val.Value = 0;
		val.BestSubsetEstimate = 0;
		return;
	};
	assert(objValues.Test.size() + objValues.Cntrl.size() == ext->Size());
	
	val.Value = getValue();
	val.BestSubsetEstimate = getBestSubsetEstimate();
}

JSON CLocalTreatmentEffectOEst::GetJsonQuality(const IExtent* ext) const
{
	std::stringstream rslt;
	if( !extractObjValues(ext) ) {
		return "{\"Error\":\"Too small extent\"}";
	};

	const double delta = objValues.TestConfLowBound.front()
		- objValues.CntrlConfUpperBound.back();
	rslt << "{"
		<< "\"Delta\":" << delta << ","
		<< "\"Delta0\":" << delta0 << ","
		<< "\"Delta0Max\":" << delta0Max << ","
		<< "\"N0\":" << objValues.Cntrl.size() << ","
		<< "\"N1\":" << objValues.Test.size() << ","
		<< "\"TotalSize\":" << objY.size() << ","
		<< "\"Value\":" << getValue()
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
		objTrt[i]=cntrlLevels.find(trt[i].GetString()) == cntrlLevels.end();
		objY[i]=y[i].GetDouble();
	}
	if(p.HasMember("MinObjNum") && p["MinObjNum"].IsInt()) {
		minObjNum=p["MinObjNum"].GetInt();
	}
	if(p.HasMember("kDelta") && p["kDelta"].IsNumber()) {
		const double a =p["kDelta"].GetDouble();
		alpha=a;
	}
	if(p.HasMember("kN") && p["kN"].IsNumber()) {
		const double b =p["kN"].GetDouble();
		beta=b;
	}
	if(p.HasMember("ConfidenceIntervalMode") && p["ConfidenceIntervalMode"].IsString()) {
		const string mode =p["ConfidenceIntervalMode"].GetString();
		if( mode == "median" || mode == "m") {
			confIntMode = CIM_Median;
		} else if( mode == "2sigmas" || mode == "2s") {
			confIntMode = CIM_2Sigma;
		} else if( mode == "ttest" || mode == "tt") {
			confIntMode = CIM_TTest;
		}
	}
	if(p.HasMember("SignificanceLevel") && p["SignificanceLevel"].IsNumber()) {
		const double sl =p["SignificanceLevel"].GetDouble();
		if( 0 < sl && sl < 1) {
			signifLevel=sl;
		}
	}
	
	setZP();
	computeSignificantObjectNumbers();
	buildOrder();
	computeDelta0();
	computeDelta0Max();
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
		            .AddMember("kDelta",rapidjson::Value().SetDouble(alpha), alloc)
		            .AddMember("kN",rapidjson::Value().SetDouble(beta), alloc)
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
	const bool res = cmp(a,b);
	assert( !res || res != cmp(b,a));
	return res;
}
inline bool CLocalTreatmentEffectOEst::cmp(int a, int b) const
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

// Computes the multiplier of SIGMA for the correct quantile
void CLocalTreatmentEffectOEst::setZP()
{
	if(signifLevel > 0.3) {
		zp = 1;
	} else if(signifLevel > 0.2-1e-10) {
		zp = 1.282;
	} else if(signifLevel > 0.1-1e-10) {
		zp = 1.645;
	} else if(signifLevel > 0.05-1e-10) {
		zp = 1.96;
	} else if(signifLevel > 0.02-1e-10) {
		zp = 2.326;
	} else if(signifLevel > 0.01-1e-10) {
		zp = 2.576;
	} else if(signifLevel > 0.005-1e-10) {
		zp = 2.807;
	} else if(signifLevel > 0.002-1e-10) {
		zp = 3.09;
	} else if(signifLevel > 0.001-1e-10) {
		zp = 3.29;
	} else if(signifLevel > 0.0001-1e-10) {
		zp = 3.89;
	} else if(signifLevel > 0.00001-1e-10) {
		zp = 4.417;
	} else if(signifLevel > 0.000001-1e-10) {
		zp = 4.891;
	} else {
		zp = 5.327; // For 1e-6
	}
}

// For every number of objects compute the object number that corresponds to a boundary of the median with the desired significance level
void CLocalTreatmentEffectOEst::computeSignificantObjectNumbers()
{
	assert(objY.size() == objTrt.size());
	const int n = objY.size();
	signifObjectNum.resize(n);
	signifObjectNum[0]=-1;
	signifObjectNum[1]=-1;
	int signifMinObjNum = -1;
	for( int i = 2; i < n; ++i) {
		int j = signifObjectNum[i-1]+1;
		for( ; j < i; ++j ) {
			if( incompleteBeta(j+1, i - j) < 1 -signifLevel) {
				break;
			}
		} 
		assert(j < i);
		signifObjectNum[i]= j - 1;
		if(j == 0) {
			signifMinObjNum = i+1;	
		}
	}
	minObjNum = max(signifMinObjNum, minObjNum);
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
	std::sort(order.begin(), order.end(), *this );
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
	assert(controlSize < signifObjectNum.size());
	assert(objY.size() - controlSize < signifObjectNum.size());
	const int sObjNumCntrl = signifObjectNum[controlSize];
	const int sObjNumTest = signifObjectNum[objY.size() - controlSize];

	assert(0 <= sObjNumCntrl && sObjNumCntrl < controlSize);
	assert(0 <= sObjNumTest && sObjNumTest < objY.size() - controlSize);
	const double cntrlY = objY[order[sObjNumCntrl]];
	const double testY = objY[order[objY.size() - sObjNumTest - 1]];
   
	delta0=testY-cntrlY;
}

// Computes maximal delta for the whole dataset and the minimal number of observations in Trt and Ctrl
void CLocalTreatmentEffectOEst::computeDelta0Max()
{
	// TODO change function for dealing with other measures than median confidence interval
	const int sObjNum = signifObjectNum[minObjNum];
	assert(0 <= sObjNum && sObjNum < minObjNum);
	assert(sObjNum < controlSize);
	assert(sObjNum < objY.size() - controlSize);
	const double cntrlY = objY[order[minObjNum - 1 - sObjNum + 1]];
	const double testY = objY[order[objY.size() - minObjNum + sObjNum - 1]];
   
	delta0Max=testY-cntrlY;
}

// Extracts test and control group for the extent
bool CLocalTreatmentEffectOEst::extractObjValues(const IExtent* ext) const
{
	assert(ext!=0);
	if( ext->Size() < 2 * minObjNum ) {
		return false;
	}
	
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
	if( nCntrl < minObjNum || nTest < minObjNum ) {
		return false;
	}

	switch(confIntMode) {
	case CIM_Median:
		computeMedianConfidenceIntervalBounds();
		break;
	case CIM_2Sigma:
	case CIM_TTest:
		compute2SigmasConfidenceIntervalBounds();
		break;
	default:
		assert(false);
		computeMedianConfidenceIntervalBounds();
	}
	return true;
}

// Computes the confidence interval bounds for Test and Control sets
void CLocalTreatmentEffectOEst::computeMedianConfidenceIntervalBounds() const
{
	objValues.TestConfLowBound.resize(objValues.Test.size());
	objValues.CntrlConfUpperBound.resize(objValues.Cntrl.size());

	// Computing low bonuds for the Test set
	for( int testStart = objValues.Test.size() - minObjNum; testStart >= 0; --testStart ) {
		const int objNum = signifObjectNum[objValues.Test.size() - testStart];
		assert(0 <= objNum);
		const int currConfIntObj = testStart + objNum -1;
		objValues.TestConfLowBound[testStart] = objValues.Test[currConfIntObj];
	}
	for( int testStart = objValues.Test.size() - minObjNum + 1; testStart < objValues.Test.size(); ++testStart ) {
		objValues.TestConfLowBound[testStart] = max(objValues.Test[testStart],objValues.TestConfLowBound[objValues.Test.size() - minObjNum]);
	}

	// Computing upper bound for Control set
	for( int cntrlSize = minObjNum; cntrlSize <= objValues.Cntrl.size(); ++cntrlSize ) {
		const int objNum = signifObjectNum[cntrlSize];
		assert(0 <= objNum);
		const int currConfIntObj = cntrlSize - objNum;
		objValues.CntrlConfUpperBound[cntrlSize - 1] = objValues.Cntrl[currConfIntObj];
	}
	for( int cntrlSize = minObjNum - 1; cntrlSize > 0; --cntrlSize ) {
		objValues.CntrlConfUpperBound[cntrlSize - 1] = min(objValues.Cntrl[cntrlSize - 1], objValues.CntrlConfUpperBound[minObjNum - 1]);
	}
}
#define SQR(x)((x)*(x))
void CLocalTreatmentEffectOEst::compute2SigmasConfidenceIntervalBounds() const
{
	objValues.TestConfLowBound.resize(objValues.Test.size());
	objValues.CntrlConfUpperBound.resize(objValues.Cntrl.size());
	// Computing sigmas in linear times.
	// The idea is the following.
	// Let a_n, sd_n be average and sd2 for n points then
	// sd_n = SUM (Xi-an)^2 = SUM (Xi-a{n-1}+a{n-1}-an)^2
	//      = SUM (Xi-a{n-1})^2 + (N-1)(a{n-1}-an)^2 + 2 * SUM (Xi - a_{n-1}) (a_{n-1}-a_n) + (Xn-an)^2
	//      = sd_{n-1} + (N-1)(a{n-1}-an)^2 + 2 * SUM Xi (a_{n-1}-a_n) + (Xn-an)^2

	// Computing low bonuds for the Test set
	double sd2sum = 0;
	double valSum = objValues.Test[objValues.Test.size() - 1];
	objValues.TestConfLowBound[objValues.Test.size() - 1] = valSum;
	for(int n = 2; n <= objValues.Test.size(); ++n ) {
		const int i = objValues.Test.size() - n;
		// First adding the new member to sd with the old mean
		const double meanPrev = valSum / (n-1);
		sd2sum += SQR(objValues.Test[i] - meanPrev );
		// Computing the mean shift
		const double meanDiff = meanPrev - (valSum + objValues.Test[i]) / n; 
		// Changing the sum to the all values
		valSum += objValues.Test[i];

		// Changing the mean
		sd2sum += n * meanDiff * meanDiff + 2 * (valSum - n * meanPrev) * meanDiff;
		// TODO add t-test here, for now only quantiles of norm distribution
		objValues.TestConfLowBound[i] = max(objValues.Test[i],valSum / n - sqrt(sd2sum / n) * zp);
	}
	
	sd2sum = 0;
	valSum = objValues.Cntrl[0];
	objValues.CntrlConfUpperBound[0]=valSum;
	for(int n = 2; n <= objValues.Cntrl.size(); ++n ) {
		const int i = n-1;
		// First adding the new member to sd with the old mean
		const double meanPrev = valSum / (n-1);
		sd2sum += SQR(objValues.Cntrl[i] - meanPrev);
		// Computing the mean shift
		const double meanDiff = meanPrev - (valSum + objValues.Cntrl[i]) / n; 
		// Changing the sum to the all values
		valSum += objValues.Cntrl[i];

		// Changing the mean
		sd2sum += n * meanDiff * meanDiff + 2 * (valSum-n*meanPrev) * meanDiff;
		// TODO add t-test here, for now only quantiles of norm distribution
		objValues.CntrlConfUpperBound[i] = min(objValues.Cntrl[i],valSum / n + sqrt(sd2sum / n) * zp);
	}
}

// Return the best subset estimate based on objValues object
double CLocalTreatmentEffectOEst::getBestSubsetEstimate() const
{
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
			if(testY - delta0 > cntrlY) { // delta0 is the minimal difference to be achieved
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
		const double diff = getDeltaValue(
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
		if(testStart == -1) {
			break;
		}
		assert( objValues.CntrlConfUpperBound[i] < objValues.TestConfLowBound[testStart] );
		const double currDiff =
			getDeltaValue(objValues.CntrlConfUpperBound[minObjNum - 1] - objValues.CntrlConfUpperBound[i],i - minObjNum + 1)
			+ objValues.BestTestValueChange[testStart];
		assert(abs(minObjNumValue + currDiff - getBestValueForSubsets(i,testStart)) < delta0Max * 1e-10);
		bestDiff = max(bestDiff, currDiff);
	}

	return bestDiff + minObjNumValue;
}

// Returns the exact value for a set based on objValues object
double CLocalTreatmentEffectOEst::getValue() const
{
	const double delta = objValues.TestConfLowBound.front()
		- objValues.CntrlConfUpperBound.back();
	if( delta - delta0 <= 1e-10 ) {
		return 0;
	} else {
		return getValue(delta,objValues.Test.size() + objValues.Cntrl.size()); // Only linear functions are supported
	}
}

// Computes a linear function of delta and size
double CLocalTreatmentEffectOEst::getValue(const double& delta, int size) const
{
	return alpha * (delta-delta0)/delta0Max  + beta * size/objY.size();
}
// Computes a change of the Value if the delta and size are changed by ddelta and dsize
double CLocalTreatmentEffectOEst::getDeltaValue(const double& ddelta, int dsize) const
{
	return alpha * ddelta/delta0Max  + beta * dsize/objY.size();
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
		if( testPos == -1 && objValues.CntrlConfUpperBound[i] < objValues.TestConfLowBound[objValues.Test.size() - minObjNum] - delta0 ) {
			// -1 pos means that it is not possible to find any larger value in the Test set respecting the minimal size of the tidset
			return false;
		}
		if( testPos == -1 ) {
			if( i < objValues.CntrlToTestPosition.size()-1 && objValues.CntrlToTestPosition[i+1] != -1 ) {
				return false;
			}
			continue;
		}
		assert( i < objValues.Cntrl.size() );
		assert( testPos < objValues.Test.size() );
		if( objValues.CntrlConfUpperBound[i] >= objValues.TestConfLowBound[testPos]-delta0) {
			// The returned value should be lager
			return false;
		}
		if( testPos > 0 && objValues.CntrlConfUpperBound[i] + delta0 < objValues.TestConfLowBound[testPos-1]) {
			// The testPos should be the minimal available number
			return false;
		}
	}

	// objValues.BestTestValueChange
	// ??

	return true;
}

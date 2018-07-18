// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/OptimisticEstimatorModules/LocalTreatmentEffectOEst.h>
#include <fcaps/Extent.h>

#include <ModuleJSONTools.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/document.h>

#include <boost/math/special_functions/gamma.hpp>
#include <sstream>
#include <math.h>

using namespace std;

////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CLocalTreatmentEffectOEst> CLocalTreatmentEffectOEst::registrar(
	                 OptimisticEstimatorModuleType, LocalTreatmentEffectOptimisticEstimator );

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
	const double delta = objValues.Test[signifObjectNum[objValues.Test.size()]-1]
		- objValues.Cntrl[objValues.Cntrl.size() - signifObjectNum[objValues.Cntrl.size()]];

	if( delta <= 0 ) {
		return 0;
	}
	return getValue(delta,ext->Size()); // Only linear functions are supported
}

double CLocalTreatmentEffectOEst::GetBestSubsetEstimate(const IExtent* ext) const
{
	assert(ext!=0);

	extractObjValues(ext);

	objValues.CntrlValueChange.resize(objValues.Cntrl.size() - signifObjectNum[objValues.Cntrl.size()]+1);
	objValues.CntrlToTestPosition.resize(objValues.Cntrl.size() - signifObjectNum[objValues.Cntrl.size()]+1);
	objValues.BestTestValueChange.resize(objValues.Test.size() - signifObjectNum[objValues.Test.size()]+1);
	std::fill(objValues.CntrlValueChange.begin(),objValues.CntrlValueChange.end(),0.0);
	std::fill(objValues.CntrlToTestPosition.begin(),objValues.CntrlToTestPosition.end(),0);
	std::fill(objValues.BestTestValueChange.begin(),objValues.BestTestValueChange.end(),0.0);

	// First for every position of the confidence interval we find the gain/loss w.r.t. minObjNum.
	const int minObjNumConfIntObj = minObjNum - signifObjectNum[minObjNum];
	int prevConfIntObj = minObjNumConfIntObj;
	for( int i = minObjNum+1; i < objValues.Cntrl.size(); ++i ) {
		const int confIntObj = i - signifObjectNum[i];
		// Diff is computed by getValue since getValue is a linear function
		const double diff = getValue(
			objValues.Cntrl[confIntObj] - objValues.Cntrl[minObjNumConfIntObj], i - minObjNum);
		objValues.CntrlValueChange[confIntObj]=diff;
		prevConfIntObj = confIntObj;
	}

	// For every position of the cntrl, we find the clossest position in the test set
	int curTestObj=0;
	for(int i = minObjNum; i < objValues.CntrlToTestPosition.size(); ++i) {
		const int confIntObj = i - signifObjectNum[i];
		const double cntrlY = objValues.Cntrl[confIntObj];
		while(curTestObj < objValues.Test.size()-minObjNum) {
			const int testConfIntObj=curTestObj + signifObjectNum[objValues.Test.size()-curTestObj]-1;
			const double testY = objValues.Test[testConfIntObj];
			if(testY > cntrlY) {
				break;
			}
			++curTestObj;
		}
		const int testConfIntObj=curTestObj + signifObjectNum[objValues.Test.size()-curTestObj]-1;
		objValues.CntrlToTestPosition[confIntObj] = curTestObj < objValues.Test.size()-minObjNum ? testConfIntObj : -1;
	}

	// Now for the test set we should know for every object the following.
	//  If the confidence interval is limited by the object what is the optimal size
	const int testMinObjNumConfIntObj=minObjNum + signifObjectNum[objValues.Test.size()-minObjNum]-1;
	double lastBestQ = 0;
	for(int testStart = objValues.Test.size()-minObjNum - 1; testStart >= 0; --testStart ) {
		const int testConfIntObj=testStart + signifObjectNum[objValues.Test.size()-testStart]-1;
		const double diff = getValue(
			-(objValues.Test[testConfIntObj] - objValues.Test[testMinObjNumConfIntObj]), objValues.Test.size() - minObjNum -testStart);
		lastBestQ = max(lastBestQ, diff);
		objValues.BestTestValueChange[testConfIntObj]=lastBestQ;
	}

	assert(checkObjValues());

	//  Now just iterate over possible conf interval position in cntrl set and finds the corresponding the best value in the test set
	const double minObjNumValue = getValue(objValues.Test[testMinObjNumConfIntObj] - objValues.Cntrl[minObjNum - signifObjectNum[minObjNum]], 2 * minObjNum); 
	double bestDiff = 0;
	for(int i = minObjNum; i < objValues.CntrlToTestPosition.size(); ++i) {
		const int confIntObj = i - signifObjectNum[i];
		const int testConfIntObj = objValues.CntrlToTestPosition[confIntObj];
		assert( objValues.Cntrl[confIntObj] < objValues.Test[testConfIntObj] );
		const double currDiff = objValues.CntrlValueChange[confIntObj] + objValues.BestTestValueChange[testConfIntObj];
		assert(abs(minObjNumValue + currDiff - getValueByConfInterval(testConfIntObj,confIntObj)) < delta0 * 1e-10);
		bestDiff = max(bestDiff, currDiff);
	}

	return bestDiff + minObjNumValue;
}

JSON CLocalTreatmentEffectOEst::GetJsonQuality(const IExtent* ext) const
{
	std::stringstream rslt;
	extractObjValues(ext);
	const double delta = objValues.Test[signifObjectNum[objValues.Test.size()]-1]
		- objValues.Cntrl[objValues.Cntrl.size() - signifObjectNum[objValues.Cntrl.size()]];
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
	if(p.HasMember("SignificanceLevel") && p["SignificanceLevel"].IsDouble()) {
		const double sl =p["SignificanceLevel"].GetDouble();
		if( 0 < sl && sl < 1) {
			signifLevel=sl;
		}
	}

	computeSignificantObjectNumbers();
	buildOrder();
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
	assert( 0 <= a && a <= objY.size());
	assert( 0 <= b && b <= objY.size());
	if(objTrt[a] < objTrt[b]) {
		return true;
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
	sort(order.begin(),order.end(),*this);
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
	ext->GetExtent(img);

	currentObjects.resize(objY.size());
	std::fill(currentObjects.begin(),currentObjects.end(),false);

	// Marking currentObjects in the sorting order
	int cntrlSize=0;
	for( int i = 0; i < img.ImageSize; ++i) {
		assert(0 <= img.Objects[i] && img.Objects[i] < posToOrder.size());
		assert(0 <= posToOrder[img.Objects[i]] && posToOrder[img.Objects[i]] < currentObjects.size());
		assert(!currentObjects[posToOrder[img.Objects[i]]]);
		currentObjects[posToOrder[img.Objects[i]]]=true;
		cntrlSize += objTrt[img.Objects[i]];
		assert((objTrt[img.Objects[i]]) == (posToOrder[img.Objects[i]]<controlSize));
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
			objValues.Cntrl[nCntrl] = objY[order[i]];
			++nCntrl;
		} else {
			objValues.Test[nTest] = objY[order[i]];
			++nTest;
		}
	}
}

// Computes a linear function of delta and size
double CLocalTreatmentEffectOEst::getValue(const double& delta, int size) const
{
	assert( delta >= 0 );
	assert( size > 0 );
	return alpha * delta/delta0  + beta * size/objY.size();
}
// Computes the value from position of the confidence interval
double CLocalTreatmentEffectOEst::getValueByConfInterval(int testConfIntObj, int cntrlConfIntObj) const
{
	// Finding delta
	const double delta = objValues.Test[testConfIntObj] - objValues.Cntrl[cntrlConfIntObj];
	if( delta < 0 ) {
		// Confidence intervals are intersected
		return 0;
	}
	// Finding the size of the test set
	int testStart = min(testConfIntObj, static_cast<int>(objValues.Test.size()) - minObjNum);
assert(objValues.Test.size() - minObjNum + signifObjectNum[minObjNum]-1 <= testConfIntObj);
	for( ; testStart >= 0; --testStart ) {
		const int currConfIntObj = testStart + signifObjectNum[objValues.Test.size() - testStart]-1;
		if( currConfIntObj < testConfIntObj ) {
			break;
		}
	}
	assert(testStart > 0);
	++testStart;
	assert(testStart + signifObjectNum[objValues.Test.size() - testStart]-1 == testConfIntObj);
	const int nTest = objValues.Test.size() - testStart;

	// Finding the size of the cntrl set
	int cntrlSize = max(minObjNum,cntrlConfIntObj);
	for( ; cntrlSize < objValues.Cntrl.size(); ++cntrlSize ) {
		const int currConfIntObj = cntrlSize - signifObjectNum[cntrlSize];
		if( currConfIntObj > cntrlConfIntObj) {
			break;
		}
	}
	assert( cntrlSize < objValues.Cntrl.size());
	--cntrlSize;
	assert( cntrlSize - signifObjectNum[cntrlSize] == cntrlConfIntObj);
	const int nCntrl = cntrlSize;
	
	return getValue(delta, nTest + nCntrl);
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
	}
	// objValues.Cntrl
	prevValue = objValues.Cntrl[0];
	for(int i = 1; i < objValues.Cntrl.size(); ++i) {
		if(objValues.Cntrl[i] < prevValue) {
			// should be sorted
			return false;
		}
	}
	// objValues.CntrlValueChange
	// ??
	// objValues.CntrlToTestPosition
	for( int i = 0; i < objValues.CntrlToTestPosition.size(); ++i ) {
		const int testPos = objValues.CntrlToTestPosition[i];
		if( testPos == -1 && objValues.Cntrl[i] < objValues.Test.back() ) {
			// -1 pos means that it is not possible to find any larger value in the Test set.
			return false;
		}
		assert( i < objValues.Cntrl.size() );
		assert( testPos < objValues.Test.size() );
		if( objValues.Cntrl[i] >= objValues.Test[testPos]) {
			// The returned value should be lager
			return false;
		}
		if(testPos > 0 && objValues.Cntrl[i] < objValues.Test[testPos-1]) {
			// The testPos should be the minimal available number
			return false;
		}
	}

	// objValues.BestTestValueChange
	// ??

	return true;
}

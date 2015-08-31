// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/modules/CAEPByDongClassifier.h>

#include <fcaps/storages/VectorIntentStorage.h>
#include <fcaps/PatternManager.h>

#include <fcaps/ModuleJSONTools.h>

#include <PowerfulSaxJson.h>

#include <JSONTools.h>
#include <StdTools.h>

#include <rapidjson/document.h>
#include <rapidjson/reader.h>

using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////

class CCAEPPatternAdder : public CSaxJsonDefaultTemplate {
public:
	CCAEPPatternAdder( CCAEPByDongClassifier& _cl) :
		cl(_cl), status( S_Begin ), objectCounter(0) {}

	// Methods of CSaxJsonDefaultTemplate
	TPowerfulSaxJsonResults Key(const char* str, size_t length, bool copy) {
		switch(status) {
			case S_Object:
				if( string(str) == "Nodes" ) {
					status=S_NodesArray;
					return PSJR_Iterate;
				} else {
					return PSJR_Skip;
				}
			default:
				return PSJR_Error;
		}
	}

	TPowerfulSaxJsonResults StartObject() {
		switch(status) {
			case S_Array:
				if(objectCounter < 1) {
					++objectCounter;
					return PSJR_Skip;
				} else {
					status=S_Object;
					return PSJR_Iterate;
				}
			case S_NodesArray:
				++objectCounter;
				return PSJR_Load;
			default:
				return PSJR_Error;
		}

	}
	TPowerfulSaxJsonResults StartArray() {
		switch(status) {
			case S_Begin:
				status=S_Array;
				objectCounter=0;
				return PSJR_Iterate;
			case S_NodesArray:
				objectCounter=0;
				return PSJR_Iterate;
			default:
				return PSJR_Error;
		}
	}
	bool EndArray(size_t /*elementCount*/) {
		if( status == S_NodesArray ) {
			status=S_End;
		}
		return true;
	}

	bool Load( const string& json ) {
		static char place[]="CCAEPByDongClassifier::PatternReading";
		rapidjson::Document patternJson;
		CJsonError errorText;
		if( !ReadJsonString( json, patternJson, errorText ) ) {
			throw new CJsonException( place, errorText );
		}
		if( !patternJson.HasMember("Int") || !patternJson.HasMember("Ext") ) {
			throw new CTextException( place, "Extent or Intent of pattert has not been found in pattern " + StdExt::to_string(objectCounter) );
		}
		if( patternJson["Int"].IsString() && string(patternJson["Int"].GetString()) == "BOTTOM" ) {
			// Useless pattern
			return true;
		}
		JSON intJson;
		CreateStringFromJSON( patternJson["Int"], intJson );
		if( !patternJson["Ext"].IsObject()
			|| !patternJson["Ext"].HasMember("Names") || !patternJson["Ext"]["Names"].IsArray() )
		{
			throw new CTextException( place, "Wrong Extent (e.g., no 'Names' member) in pattern " + StdExt::to_string(objectCounter) );
		}
		const rapidjson::Value& objNames = patternJson["Ext"]["Names"];

		vector<string> names;
		for( int i = 0; i < objNames.Size(); ++i ) {
			if( !objNames[i].IsString() ) {
				throw new CTextException( place, "Wrong object name in the extent in pattern " + StdExt::to_string(objectCounter) );
			}
			names.push_back(objNames[i].GetString());
		}
		cl.AddPattern( names, intJson );

		return true;
	}

	void ProcessFile( const string& path ) {
		static char place[]="CCAEPByDongClassifier::PatternReading";

		char buffer[4096];

		FILE* fp = fopen( path.c_str(), "r");
		if( fp == 0 ) {
			throw new CTextException( place, "File not found (" + path + ')' );
		}
		rapidjson::FileReadStream is( fp, buffer, sizeof(buffer) );
		rapidjson::Reader reader;
		CPowerfulSaxJson<CCAEPPatternAdder> handler(*this);
		reader.Parse(is, handler);
	}

private:
	enum TStatus {
		S_Begin = 0,
		S_Array,
		S_Object,
		S_NodesArray,
		S_End,

		S_EnumCount
	};
private:
	CCAEPByDongClassifier& cl;
	TStatus status;
	int objectCounter;
};


////////////////////////////////////////////////////////////////////

const CModuleRegistrar<CCAEPByDongClassifier> CCAEPByDongClassifier::registrar( ClassifierModuleType, CAEPByDongClassifier );

CCAEPByDongClassifier::CCAEPByDongClassifier() :
	cmp( new CVectorIntentStorage ),
	emThld( 1.01 )
{
	//ctor
}

const char jsonEmergencyThld[] = "EmergencyThld";

void CCAEPByDongClassifier::LoadParams( const JSON& json )
{
	static const char place[] = "CCAEPByDongClassifier::LoadParams";

	rapidjson::Document dParams;
	CJsonError error;
	if( !ReadJsonString( json, dParams, error ) ) {
		throw new CJsonException( place, error );
	}

	if( !dParams.HasMember("Params") || !dParams["Params"].IsObject() ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for PatternManager, ClassesPath, and TrainPath";
		throw new CJsonException(place, error);
	}
	const rapidjson::Value& params = dParams["Params"];
	if( !params.HasMember("ClassesPath") || !params["ClassesPath"].IsString() ) {
		error.Data = json;
		error.Error = "THIS.Params.ClassesPath is not found.";
		throw new CJsonException(place, error);
	}
	classesPath = params["ClassesPath"].GetString();
	JsonClassifierClasses::Load( classesPath, classes );

	if( !params.HasMember("TrainPath") || !params["TrainPath"].IsString() ) {
		error.Data = json;
		error.Error = "THIS.Params.TrainPath is not found.";
		throw new CJsonException(place, error);
	}
	trainPath = params["TrainPath"].GetString();

	if( !params.HasMember("PatternManager") || !params["PatternManager"].IsObject() ) {
		error.Data = json;
		error.Error = "THIS.Params.PatternManager is not found.";
		throw new CJsonException(place, error);
	}
	string errorText;
	pm.reset( CreateModuleFromJSON<IPatternManager>(params["PatternManager"],errorText) );
	if( pm == 0 ) {
		throw new CJsonException( place, CJsonError( json, errorText ) );
	}
	cmp->Initialize( pm );

	if( params.HasMember(jsonEmergencyThld) && params[jsonEmergencyThld].IsNumber()) {
		const double emThldJson = params[jsonEmergencyThld].GetDouble();
		if( emThldJson < 1.01 ) {
			emThld = 1.01;
		} else {
			emThld = emThldJson;
		}
	}
}
JSON CCAEPByDongClassifier::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", ClassifierModuleType, alloc )
		.AddMember( "Name", rapidjson::Value().SetString( rapidjson::StringRef( CAEPByDongClassifier ) ), alloc )
		.AddMember( "Params", rapidjson::Value().SetObject(), alloc );
	params["Params"]
		.AddMember( "ClassesPath", rapidjson::Value().SetString( rapidjson::StringRef( classesPath.c_str() ) ), alloc )
		.AddMember( "TrainPath", rapidjson::Value().SetString( rapidjson::StringRef( trainPath.c_str() ) ), alloc )
		.AddMember( jsonEmergencyThld, rapidjson::Value().SetDouble( emThld ), alloc );

	IModule* m = dynamic_cast<IModule*>(pm.get());
	assert( m!=0);
	if( m != 0 ) {
		JSON pmParams = m->SaveParams();
		rapidjson::Document pmParamsDoc;
		CJsonError error;
		const bool rslt = ReadJsonString( pmParams, pmParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("PatternManager", pmParamsDoc.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

void CCAEPByDongClassifier::PassDescriptionParams( const JSON& json )
{
	assert( pm != 0 );
	IModule& pmModule = dynamic_cast<IModule&>(*pm);
	const JSON params = string("{") +
		"\"Type\":\"" + pmModule.GetType() + "\","
		"\"Name\":\"" + pmModule.GetName() + "\","
		"\"Params\":" + json +
		"}";
	pmModule.LoadParams( params );
}

void CCAEPByDongClassifier::Prepare()
{
	CCAEPPatternAdder pAdder( *this );
	pAdder.ProcessFile( trainPath );
	computeAgregateScores();
}

string CCAEPByDongClassifier::Classify( const JSON& ptrn ) const
{
	const TIntentId intId = cmp->LoadObject( ptrn );

	unordered_map<string,double> rank;
	CStdIterator<vector<CEmergingPattern>::const_iterator> ep( eps );
	for( ; !ep.IsEnd(); ++ep ) {
		const TCompareResult rslt = cmp->Compare( (*ep).Id, intId, CR_MoreOrEqual );
		if( rslt == CR_Incomparable ) {
			continue;
		}
		CStdIterator<unordered_map<string,double>::iterator> fnd( rank.find((*ep).Class), rank.end() );
		if( fnd.IsEnd() ) {
			fnd.Reset(rank.insert(pair<string,double>((*ep).Class,0.0)).first, rank.end());
		}
		(*fnd).second += (*ep).Accuracy;
	}

	double bestScore = 0;
	string bestClass;

	CStdIterator<unordered_map<string,double>::const_iterator> cl( rank );
	for( ; !cl.IsEnd(); ++cl ) {
		const double score = (*cl).second / baseScores.at((*cl).first);
		if( score > bestScore ) {
			bestClass = (*cl).first;
			bestScore = score;
		}
	}

	if( bestClass.empty() ) {
		return CL_NotEnoughData;
	} else {
		return bestClass;
	}
}

void CCAEPByDongClassifier::AddPattern( const std::vector<std::string>& extent, const JSON& intent )
{
	unordered_map<std::string,int> classToSupport;
	CStdIterator<vector<string>::const_iterator> itr( extent );
	int totalNumber = extent.size();
	for( ; !itr.IsEnd(); ++itr ) {
		CStdIterator<unordered_map<string,string>::const_iterator> clFnd( classes.find(*itr),classes.end() );
		if( clFnd.IsEnd() ) {
			--totalNumber;
			continue;
		}
		const string& objClass = (*clFnd).second;

		// Inserting object into coverage table
		CStdIterator<unordered_map<string,int>::iterator> fnd( objToPtrnCoverage.find( *itr ), objToPtrnCoverage.end() );
		if( fnd.IsEnd() ) {
			fnd.Reset( objToPtrnCoverage.insert(pair<string,int>(*itr,0)).first, objToPtrnCoverage.end() );
		}

		// Counting class of object into support
		fnd.Reset( classToSupport.find( objClass ), classToSupport.end() );
		if( fnd.IsEnd() ) {
			fnd.Reset( classToSupport.insert(pair<string,int>(objClass,0)).first, classToSupport.end() );
		}
		++(*fnd).second;
	}

	if( totalNumber == 0 ) {
		// totally unknown pattern
		assert(false);
		return;
	}

	CStdIterator<unordered_map<string,int>::iterator> cl( classToSupport );
	string patternClass;
	double acc = 0;
	for( ; !cl.IsEnd(); ++cl ) {
		if( (*cl).second > emThld * (totalNumber - (*cl).second) ) {
			patternClass = (*cl).first;
			acc = ((double)(*cl).second) / totalNumber;
			break;
		}
	}
	if( patternClass.empty() ) {
		// Not an emerging pattern.
		return;
	}

	const DWORD ptrnId = cmp->LoadPattern( intent );
	eps.push_back(CEmergingPattern(ptrnId, patternClass, acc));

	itr.Reset( extent );
	for( ; !itr.IsEnd(); ++itr ) {
		CStdIterator<unordered_map<string,string>::const_iterator> clFnd( classes.find(*itr),classes.end() );
		if( clFnd.IsEnd() || (*clFnd).second != patternClass ) {
			continue;
		}

		CStdIterator<unordered_map<string,int>::iterator> fnd( objToPtrnCoverage.find( *itr ), objToPtrnCoverage.end() );
		if( fnd.IsEnd() ) {
			// should be done in the previous loop
			assert(false);
			fnd.Reset( objToPtrnCoverage.insert(pair<string,int>(*itr,0)).first, objToPtrnCoverage.end() );
		}
		++(*fnd).second;
	}
}

void CCAEPByDongClassifier::computeAgregateScores()
{
	unordered_map< string,vector<int> > classToUsage;
	CStdIterator<unordered_map<string,int>::const_iterator> itr(objToPtrnCoverage);
	for( ; !itr.IsEnd(); ++itr ) {
		const string& cl = classes.at((*itr).first);
		assert( !cl.empty() );
		classToUsage[cl].push_back((*itr).second);
	}

	CStdIterator<unordered_map< string,vector<int> >::iterator> ctu( classToUsage );
	for( ; !ctu.IsEnd(); ++ctu ) {
		vector<int>& arr = (*ctu).second;
		sort( arr.begin(), arr.end() );
		assert( arr.size() != 0 );
		baseScores[(*ctu).first]=arr[arr.size() / 2];
	}
	objToPtrnCoverage.clear();
}

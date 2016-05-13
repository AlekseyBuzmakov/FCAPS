// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include "ClassifierContextProcessor.h"

#include <fcaps/Classifier.h>
#include <ModuleJSONTools.h>

#include <JSONTools.h>

using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////

const CModuleRegistrar<CClassifierContextProcessor> CClassifierContextProcessor::registrar(
	ContextProcessorModuleType, ClassifierContextProcessorModule );

CClassifierContextProcessor::CClassifierContextProcessor() :
	callback( 0 )
{
	//ctor
}

const std::vector<std::string>& CClassifierContextProcessor::GetObjNames() const
{
	return objectNames;
}
void CClassifierContextProcessor::SetObjNames( const std::vector<std::string>& names )
{
	objectNames = names;
}
void CClassifierContextProcessor::PassDescriptionParams( const JSON& json )
{
	assert( classifier != 0 );
	classifier->PassDescriptionParams( json );
}
void CClassifierContextProcessor::Prepare()
{
	assert( classifier != 0 );
	classifier->Prepare();
}
void CClassifierContextProcessor::AddObject( DWORD objectNum, const JSON& intent )
{
	assert( objectNum < objectNames.size() );
	const string resultClass = classifier->Classify( intent );
	predictions[objectNames[objectNum]]=resultClass;
}
void CClassifierContextProcessor::ProcessAllObjectsAddition()
{
	// Nothing to do for the moment
}
void CClassifierContextProcessor::SaveResult( const std::string& path )
{
	CDestStream out(path);
	unordered_map<pair<string,string>, int> conf;
	int predictionNum = 0;
	int truePredictionNum = 0;
	if( outParams.OutAccuracy || outParams.OutConfusion ) {
		computeConfusion( conf, truePredictionNum , predictionNum );
	}
	out << "{";
	if( outParams.OutAccuracy ) {
		out << "\t\"Accuracy\":" << double(truePredictionNum) / predictionNum << ",\n";
		out << "\t\"Predicted\":" << predictionNum << ",\n";
		out << "\t\"Total\":" << predictions.size() << ",\n";
	}
	if( outParams.OutConfusion ) {
		out << "\t\"Confusion\":[\n";
		CStdIterator<unordered_map<pair<string,string>, int>::iterator> confEl( conf );
		for( ; !confEl.IsEnd(); ++confEl ) {
			out << "\t\t{";
			out << "\"Right\":\"" << (*confEl).first.first << "\",";
			out << "\"Predicted\":\"" << (*confEl).first.second << "\",";
			out << "\"Card\":\"" << (*confEl).second << "\",";
			out << "},\n";
		}
		out << "\t{}],\n";
	}
	if( outParams.OutObjectClassification ) {
		out << "\t\"Classification\":[\n";
		CStdIterator<unordered_map<string,string>::const_iterator> itr(predictions);
		for( ; !itr.IsEnd(); ++itr ) {
			out << "\t\t{";
			const string& name = (*itr).first;
			const string& predCl = (*itr).second;
			out << "\"N\":\"" << name << "\",";
			out << "\"Pred\":\"" << predCl << "\"";
			CStdIterator<unordered_map<string,string>::const_iterator> fnd( classes.find(name), classes.end() );
			if( !fnd.IsEnd() ) {
				const string& rightCl = (*fnd).second;
				out << ",\"Right\":\"" << (*fnd).second << "\"," << "\"Dcs\":\"";
				if( predCl == CL_ContradictoryData || predCl == CL_NotEnoughData ) {
					out << "~";
				} else if ( predCl == rightCl ) {
					out << "+";
				} else {
					out << "-";
				}
				out << "\"";
			}

			out << "},\n";
		}
		out << "\t{}]\n";
	}
	out << "}";
}

void CClassifierContextProcessor::LoadParams( const JSON& json )
{
	static char place[]="CClassifierContextProcessor::LoadParams";
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( place, error );
	}
	assert( string( params["Type"].GetString() ) == ContextProcessorModuleType );
	assert( string( params["Name"].GetString() ) == ClassifierContextProcessorModule );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for Classifier";
		throw new CJsonException(place, error);
	}
	const rapidjson::Value& p = params["Params"];

	if( !p.HasMember( "Classifier") || !p["Classifier"].IsObject() ) {
		error.Data = json;
		error.Error = "THIS.Params.Classifier is not found.";
		throw new CJsonException(place, error);
	}
	const rapidjson::Value& pm = params["Params"]["Classifier"];
	string errorText;
	classifier.reset( CreateModuleFromJSON<IClassifier>(pm,errorText) );
	if( classifier == 0 ) {
		throw new CJsonException( place, CJsonError( json, errorText ) );
	}
	if( !p.HasMember( "ClassesPath") || !p["ClassesPath"].IsString() ) {
		error.Data = json;
		error.Error = "THIS.Params.ClassesPath is not found.";
		throw new CJsonException(place, error);
	}
	classesPath = p["ClassesPath"].GetString();
	JsonClassifierClasses::Load( classesPath, classes );

	if( p.HasMember( "OutputParams") && p["OutputParams"].IsObject() ) {
		const rapidjson::Value& opJson = p["OutputParams"];
#define	getFromJson( name, opp ) \
		if( opJson.HasMember( #name ) && opJson[#name].Is##opp() ) { \
			outParams.name = opJson[#name].Get##opp(); \
		}
		getFromJson( OutAccuracy, Bool );
		getFromJson( OutConfusion, Bool );
		getFromJson( OutObjectClassification, Bool );
#undef	getFromJson
	}

}
JSON CClassifierContextProcessor::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();

#define	addToJson( name, opp ) \
		.AddMember( #name , rapidjson::Value().Set##opp( outParams.name ), alloc )
	params.SetObject()
		.AddMember( "Type", ContextProcessorModuleType, alloc )
		.AddMember( "Name", ClassifierContextProcessorModule, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "ClassesPath", rapidjson::Value().SetString(rapidjson::StringRef(classesPath.c_str())), alloc)
			.AddMember( "OutputParams", rapidjson::Value().SetObject()
				addToJson( OutAccuracy, Bool )
				addToJson( OutConfusion, Bool )
				addToJson( OutObjectClassification, Bool ),
			alloc ),
		alloc );
#undef addToJson

	IModule* m = dynamic_cast<IModule*>(classifier.get());
	assert( m!=0);
	if( m != 0 ) {
		JSON clParams = m->SaveParams();
		rapidjson::Document clParamsDoc;
		CJsonError error;
		const bool rslt = ReadJsonString( clParams, clParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("Classifier", clParamsDoc.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

void CClassifierContextProcessor::computeConfusion( unordered_map<pair<string,string>, int>& conf, int& truePredictionNum , int& predictionNum ) const
{
	predictionNum = 0;
	truePredictionNum = 0;
	conf.clear();

	CStdIterator<unordered_map<string,string>::const_iterator> itr(predictions);
	for( ; !itr.IsEnd(); ++itr ) {
		const string& name = (*itr).first;
		const string& predCl = (*itr).second;
		if(predCl == CL_NotEnoughData || predCl == CL_ContradictoryData ) {
			continue;
		}
		CStdIterator<unordered_map<string,string>::const_iterator> fnd( classes.find(name), classes.end() );
		if( fnd.IsEnd() ) {
			// Right class is not known
			continue;
		}
		const string& rightCl = (*fnd).second;
		++predictionNum;
		if( predCl == rightCl ) {
			++truePredictionNum;
		}
		const pair<string,string> confElKey( rightCl, predCl );
		CStdIterator<unordered_map<pair<string,string>, int>::iterator> confEl( conf.find(confElKey), conf.end() );
		if( confEl.IsEnd() ) {
			confEl.Reset(
				conf.insert(pair<pair<string,string>,int>( confElKey, 0 ) ).first,
				conf.end() );
		}
		++(*confEl).second;
	}
}


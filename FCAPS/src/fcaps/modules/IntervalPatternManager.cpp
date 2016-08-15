// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/modules/IntervalPatternManager.h>

#include <Exception.h>
#include <StdTools.h>
#include <JSONTools.h>

#include <rapidjson/document.h>

#include <boost/functional/hash.hpp>

using namespace std;

////////////////////////////////////////////////////////////////////

// #define DEBUG_PRINT true
#ifdef DEBUG_PRINT
#define COUT(a) std::cout << a << "\n"
#else
#define COUT(a)
#endif
#define DEBUG_RETURN(a) COUT(#a << " (" << a << ")"); return a

////////////////////////////////////////////////////////////////////

size_t CIntervalPatternDescriptor::Hash() const
{
	size_t hashVal = 0;
	boost::hash<double> doubleHash;
	for( int i = 0; i < ints.size(); ++i ) {
		hashVal ^= doubleHash(ints[i].first + ints[i].second);
		hashVal ^= doubleHash(ints[i].first - ints[i].second);
	}
	return hashVal;
}

////////////////////////////////////////////////////////////////////

const CModuleRegistrar<CIntervalPatternManager> CIntervalPatternManager::registrar(
	PatternManagerModuleType, IntervalPatternManagerModule );

CIntervalPatternManager::CIntervalPatternManager() :
	intsCount(-1)
{
	//ctor
}

const CIntervalPatternDescriptor* CIntervalPatternManager::LoadObject( const JSON& json )
{
	auto_ptr<CIntervalPatternDescriptor> ptrn( NewPattern() );
	JsonIntervalPattern::LoadPattern(json,ptrn->Intervals());
	if( intsCount == -1 ) {
		intsCount = ptrn->Intervals().size();
	} else if( intsCount != ptrn->Intervals().size() ) {
		throw new CTextException( "CIntervalPatternManager::LoadObject", "Number of intervals in a pattern is different from " + StdExt::to_string(intsCount) );
	}
	return ptrn.release();
}
JSON CIntervalPatternManager::SavePattern( const IPatternDescriptor* p ) const
{
	return JsonIntervalPattern::SavePattern( Pattern(p).Intervals() );
}
const CIntervalPatternDescriptor* CIntervalPatternManager::LoadPattern( const JSON& json )
{
	auto_ptr<CIntervalPatternDescriptor> ptrn( NewPattern() );
	JsonIntervalPattern::LoadPattern(json,ptrn->Intervals());
	if( intsCount == -1 ) {
		intsCount = ptrn->Intervals().size();
	} else if( intsCount != ptrn->Intervals().size() ) {
		throw new CTextException( "CIntervalPatternManager::LoadObject", "Number of intervals in a pattern is different from " + StdExt::to_string(intsCount) );
	}
	return ptrn.release();
}

const CIntervalPatternDescriptor* CIntervalPatternManager::CalculateSimilarity(
	const IPatternDescriptor* first, const IPatternDescriptor* second )
{
	const CIntervalPatternDescriptor& p1 = Pattern(first);
	const CIntervalPatternDescriptor& p2 = Pattern(second);
	assert(p1.Intervals().size() == p2.Intervals().size() );
	auto_ptr<CIntervalPatternDescriptor> res(NewPattern());
	res->Intervals().resize(p1.Intervals().size());
	for( DWORD i = 0; i < p1.Intervals().size(); ++i ) {
		res->Intervals()[i].first = min( p1.Intervals()[i].first, p2.Intervals()[i].first );
		res->Intervals()[i].second = max( p1.Intervals()[i].second, p2.Intervals()[i].second );
	}
	return res.release();
}
TCompareResult CIntervalPatternManager::Compare(
	const IPatternDescriptor* first, const IPatternDescriptor* second,
	DWORD interestingResults, DWORD possibleResults )
{
	possibleResults &= interestingResults | CR_Incomparable;

	const CIntervalPatternDescriptor& p1 = Pattern(first);
	const CIntervalPatternDescriptor& p2 = Pattern(second);
	assert(p1.Intervals().size() == p2.Intervals().size() );
	COUT(SavePattern(first));
	COUT(SavePattern(second));
	
	bool areEqual = true;

	for( DWORD i = 0; i < p1.Intervals().size(); ++i ) {
		if( p1.Intervals()[i].first < p2.Intervals()[i].first ) {
			possibleResults &= CR_MoreGeneral | CR_Incomparable;
			areEqual = false;
		} else if( p1.Intervals()[i].first > p2.Intervals()[i].first ) {
			possibleResults &= CR_LessGeneral | CR_Incomparable;
			areEqual = false;
		}
		if( p1.Intervals()[i].second > p2.Intervals()[i].second ) {
			possibleResults &= CR_MoreGeneral | CR_Incomparable;
			areEqual = false;
		} else if( p1.Intervals()[i].second < p2.Intervals()[i].second ) {
			possibleResults &= CR_LessGeneral | CR_Incomparable;
			areEqual = false;
		}
		if( possibleResults == CR_Incomparable ) {
			DEBUG_RETURN(CR_Incomparable);
		}
	}
	if( areEqual && HasAllFlags(possibleResults, CR_Equal ) ) {
		DEBUG_RETURN(CR_Equal);
	}
	if( areEqual ) {
		DEBUG_RETURN(CR_Incomparable);
	}

	if( HasAllFlags(possibleResults,CR_MoreGeneral) ) {
		assert(!HasAllFlags(possibleResults,CR_LessGeneral));
		DEBUG_RETURN(CR_MoreGeneral);
	}
	if( HasAllFlags(possibleResults,CR_LessGeneral) ) {
		assert(!HasAllFlags(possibleResults,CR_MoreGeneral));
		DEBUG_RETURN(CR_LessGeneral);
	}
	DEBUG_RETURN(CR_Incomparable);
}

void CIntervalPatternManager::FreePattern( const IPatternDescriptor * p )
{
	delete p;
}

void CIntervalPatternManager::Write( const IPatternDescriptor* pattern, std::ostream& dst ) const
{
	dst << SavePattern( pattern );
}

void CIntervalPatternManager::LoadParams( const JSON& json )
{
	CJsonError errorText;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "CIntervalPatternManager::LoadParams", errorText );
	}
	assert( string( params["Type"].GetString() ) == PatternManagerModuleType );
	assert( string( params["Name"].GetString() ) == IntervalPatternManagerModule );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		return;
	}
	const rapidjson::Value& paramsObj = params["Params"];
	if( paramsObj.HasMember( "IntervalsCount" ) && paramsObj["IntervalsCount"].IsUint() ) {
		intsCount = paramsObj["IntervalsCount"].GetUint();
	}
}
JSON CIntervalPatternManager::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", PatternManagerModuleType, alloc )
		.AddMember( "Name", IntervalPatternManagerModule, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "IntervalsCount", rapidjson::Value().SetUint( intsCount ), alloc ),
		alloc );

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}


CIntervalPatternDescriptor* CIntervalPatternManager::NewPattern()
{
	return new CIntervalPatternDescriptor;
}
const CIntervalPatternDescriptor& CIntervalPatternManager::Pattern( const IPatternDescriptor* p )
{
	assert( p != 0 );
	assert( p->GetType() == PT_Intervals );
	return debug_cast<const CIntervalPatternDescriptor&>(*p);
}

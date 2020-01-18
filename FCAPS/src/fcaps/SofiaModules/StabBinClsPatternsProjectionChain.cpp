// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/SofiaModules/StabBinClsPatternsProjectionChain.h>

#include <JSONTools.h>

#include <rapidjson/document.h>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(...) #__VA_ARGS__

const char description[] =
STR(
	{
	"Name":"Binary Projection Chain for Delta Measure",
	"Description":"This projection chain is used by sofia for dealing with binary data. It defines projection that corresponds to the first n attributes of the dataset. The projection is didicated to finding DELTA-stable patterns.",
	"Params": {
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of Binary Projection Chain for Delta Measure",
			"type": "object",
			"properties": {
				"alpha":{
					"description": "The probability from Robsutness (Tatti 2014) that an object is removed from the dataset. Smaller number indicates more focus towards large subdatasets",
					"type":"number",
					"minimum":0,
					"exclusiveMinimum": true,
					"maximum":1,
					"exclusiveMaximum": true
				},
				"IntentWritingMode" :{
					"description": "The option specify how the intent should be written. As indices, as names or both.",
					"type":"string",
					"enum":["Indices","Names","Both"]
				},
				"AttrNames":{
					"description": "A set of attribute names in the corresponding order",
					"type":"array",
					"items":{
						"type":"string"
					}
				},
				"AttrOrder" :{
					"description": "The order of attribute addition by the projection: random, none (initial order), and ascending and descending order in terms of attribute support. Descending order is typically the most efficient.",
					"type":"string",
					"enum":["desc","asc","none","rand"]
				}
			}
		}
	}
);

////////////////////////////////////////////////////////////////////

CModuleRegistrar<CStabBinClsPatternsProjectionChain> CStabBinClsPatternsProjectionChain::registar;

const char* const CStabBinClsPatternsProjectionChain::Desc()
{
	return description;
}
////////////////////////////////////////////////////////////////////

CStabBinClsPatternsProjectionChain::CStabBinClsPatternsProjectionChain() :
	stabApprox( ExtCmp() )
{
	OrderType()=AO_DescendingSize;
	Thld()=5;
	stabApprox.SetIgnoreIfNotClose( true );
}

void CStabBinClsPatternsProjectionChain::UpdateInterestThreshold( const double& thld )
{
	CBinClsPatternsProjectionChain::UpdateInterestThreshold(thld);
	stabApprox.SetStableThreshold( thld );
}
double CStabBinClsPatternsProjectionChain::GetPatternInterest( const IPatternDescriptor* p )
{
	const CStabPatternDescription& ptrn = StabPattern(Pattern(p));
	if( ptrn.StabAttrNum() == CurrAttr()
		|| ptrn.StabAttrNum() == Order().size() - 1 )
	{
		return ptrn.Stability();
	} else if( CurrAttr() >= ptrn.GlobMinAttr() ) {
		ptrn.Stability() = min( ptrn.Stability(), (double)ptrn.GlobMinValue() );
		ptrn.StabAttrNum() = Order().size() - 1;
		return ptrn.Stability();
	} else {
		stabApprox.InitComputation( ptrn.Extent(), ptrn.Intent() );
		stabApprox.ComputeUpperBound();

		// Not more than concept.Extent->Size() ?
		ptrn.Stability()=stabApprox.GetStabilityRightLimit(),
		ptrn.StabAttrNum()=CurrAttr();
		ptrn.MinAttr() = stabApprox.GetMinDiffAttr();
		return ptrn.Stability();
	}
}
void CStabBinClsPatternsProjectionChain::ComputeZeroProjection( CPatternList& ptrns )
{
	CBinClsPatternsProjectionChain::ComputeZeroProjection(ptrns);
	const CStabPatternDescription& p = StabPattern(Pattern(ptrns.Front()));
	p.MinAttr()= -1;
	p.Stability() = ObjCount();
	p.StabAttrNum() = CurrAttr();

	stabApprox.SetContext( AttrToTidsetMap() );
	stabApprox.SetAttrOrder( &Order() );
}
bool CStabBinClsPatternsProjectionChain::NextProjection()
{
	const bool result = CBinClsPatternsProjectionChain::NextProjection();
	stabApprox.SetMaxAttrNumber( CurrAttr() + 1 );
	return result;
}
void CStabBinClsPatternsProjectionChain::Preimages( const IPatternDescriptor* d, CPatternList& preimages )
{
	assert(CurrAttr() < Order().size());

	const CStabPatternDescription& p = StabPattern(Pattern(d));
	CPatternDescription* preimage=Preimage( Pattern(d));
	if( preimage == 0 ){
		if( CurrAttr() >= p.GlobMinAttr() ) {
			p.Stability() = min( p.Stability(), (double)p.GlobMinValue() );
			p.StabAttrNum() = Order().size() - 1;
		}
		return;
	}

	std::unique_ptr<const CStabPatternDescription> newPtrn(
		&StabPattern( *preimage ) );
	assert(newPtrn.get() != 0 );

	const DWORD extDiff = p.Extent().Size() - newPtrn->Extent().Size();
	assert( extDiff != 0 );

	if( canBeStable( newPtrn->Extent(), p.MinAttr() ) ) {
		if( GetPatternInterest( newPtrn.get() ) >= Thld() ) {
			NewConceptCreated( *newPtrn );
			preimages.PushBack( newPtrn.release() );
		}
	}
	p.StabAttrNum() = CurrAttr();
	if( p.Stability() > extDiff ) {
		p.Stability() = extDiff;
		p.MinAttr() = CurrAttr();
	}
}

void CStabBinClsPatternsProjectionChain::LoadParams( const JSON& json)
{
	CJsonError errorText;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, errorText ) ) {
		throw new CJsonException( "CStabBinClsPatternsProjectionChain::LoadParams", errorText );
	}
	assert( string( params["Type"].GetString() ) == ProjectionChainModuleType );
	assert( string( params["Name"].GetString() ) == StabBinClsPatternsProjectionChain );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		return;
	}
	const rapidjson::Value& paramsObj = params["Params"];
	JSON paramsObjStr;
	CreateStringFromJSON( paramsObj, paramsObjStr );
	LoadCommonParams( paramsObjStr );

	if( paramsObj.HasMember("alpha") && paramsObj["alpha"].IsNumber() ) {
		const double alpha = paramsObj["alpha"].GetDouble();
		stabApprox.SetBase( 1.0 / (1-alpha) ); // Robustness Tatti2014
	} else {
		stabApprox.SetBase( 2.0 ); // Classical stability case
	}
}
JSON CStabBinClsPatternsProjectionChain::SaveParams() const
{

	CJsonError errorText;
	rapidjson::Document commonParams;
	const bool res = ReadJsonString( SaveCommonParams(), commonParams, errorText );
    assert( res );
	assert(commonParams.IsObject() );

	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", ProjectionChainModuleType, alloc )
		.AddMember( "Name", StabBinClsPatternsProjectionChain, alloc )
		.AddMember( "Params", commonParams, alloc );
	rapidjson::Value& p = params["Params"];

	p.AddMember( "alpha", rapidjson::Value().SetDouble( 1 - 1 / stabApprox.GetBase() ), alloc );

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

const CStabBinClsPatternsProjectionChain::CStabPatternDescription&
CStabBinClsPatternsProjectionChain::StabPattern( const CPatternDescription& d)
{
	return debug_cast<const CStabPatternDescription&>(d);
}

void CStabBinClsPatternsProjectionChain::ReportAttrSimilarity(
	const CPatternDescription& p, DWORD i, const CVectorBinarySetDescriptor& res )
{
	const CStabPatternDescription& ptrn = StabPattern( p );
	if( ptrn.GlobMinValue() > p.Extent().Size() - res.Size() ) {
		ptrn.SetGlobMinAttr( i, p.Extent().Size() - res.Size() );
	}
}

bool CStabBinClsPatternsProjectionChain::canBeStable(
	const CVectorBinarySetDescriptor& extent, DWORD attr )
{
	if( attr == NotFound ) {
		return true;
	}
	//return true;
	CSharedPtr<const CVectorBinarySetDescriptor> tmp(
		ExtCmp().CalculateSimilarity( extent, *AttrToTidsetMap()[Order()[attr]] ), ExtDeleter() );

	const DWORD diff = tmp->Size() - extent.Size();
	return diff >= Thld();
}

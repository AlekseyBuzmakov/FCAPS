// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8

#include <fcaps/LocalProjectionChainModules/StabilityCbOLocalProjectionChain.h>

#include <fcaps/PatternDescriptor.h>
#include <fcaps/PatternEnumerator.h>

#include <JSONTools.h>
#include <ModuleJSONTools.h>
#include <StdTools.h>

using namespace std;

////////////////////////////////////////////////////////////////////
#define STR(x...) #x

const char description[] =
STR(
	{
	"Name":"Stability Close-by-One Local Projection Chain",
	"Description":"Local Projection Chain that is based on Close-by-One (also known as LCM) canonical order and incorporate computation of DELTA-stability.",
	"Params": {
			"$schema": "http://json-schema.org/draft-04/schema#",
			"title": "Params of Stability Close-by-One Local Projection Chain",
			"type": "object",
			"properties": {
				"PatternEnumerator":{
					"description": "The module that return attributes one by one by their extent",
					"type": "@PatternEnumeratorModules"
				}			}
		}
	}
);


////////////////////////////////////////////////////////////////////
const CModuleRegistrar<CStabilityCbOLocalProjectionChain> CStabilityCbOLocalProjectionChain::registrar;

const char* const CStabilityCbOLocalProjectionChain::Desc()
{
	return description;
}

////////////////////////////////////////////////////////////////////

class CPattern : public IExtent, public IPatternDescriptor {
public:
	CPattern( const IPatternDescriptor* e, const IPatternDescriptor* i, int nextAttr ) :
		extent(e), intent(i), nextAttribute(nextAttr) {}

	const IPatternDescriptor& Extent() const
		{return *extent;}
	const IPatternDescriptor& Intent() const
		{return *intent;}
	int NextAttribute() const
		{return nextAttribute;}
	DWORD Delta() const
		{return delta;}

private:
	CSharedPtr<const IPatternDescriptor> extent;
	CSharetPtr<const IPatternDescriptor> intent;
	// The number of attrbutes that can be added to the pattern.
	// This number also gives the reference to the "core" of the pattern in the CbO canonical order
	int nextAttribute;
	// The delta measure of the pattern w.r.t. the projection not including the @var nextAttribute attribute
	DWORD delta;
};

////////////////////////////////////////////////////////////////////

CStabilityCbOLocalProjectionChain::CStabilityCbOLocalProjectionChain()
{
}

void CStabilityCbOLocalProjectionChain::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CStabilityCbOLocalProjectionChain::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == Type() );
	assert( string( params["Name"].GetString() ) == Name() );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for Pattern Enumerator object";
		throw new CJsonException("CStabilityCbOLocalProjectionChain::LoadParams", error);
	}

	const rapidjson::Value& p = params["Params"];

	if(!(p.HasMember("PatternEnumerator") && (p["PatternEnumerator"].IsObject()))) {
		error.Data = json;
		error.Error = "Params.PatternEnumerator is not found or is not an object.";
		throw new CJsonException("CStabilityCbOLocalProjectionChain::LoadParams", error);
	}
	const rapidjson::Value& pe = params["Params"]["PatternEnumerator"];
	string errorText;
	enumerator.reset( CreateModuleFromJSON<IPatternEnumerator>(pe,errorText) );
	if( pe == 0 ) {
		throw new CJsonException( "CStabilityCbOLocalProjectionChain::LoadParams", CJsonError( json, errorText ) );
	}
}

JSON CStabilityCbOLocalProjectionChain::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", rapidjson::StringRef(Type()), alloc )
		.AddMember( "Name", rapidjson::StringRef(Name()), alloc )
		.AddMember( "Params", rapidjson::Value().SetObject(), alloc );

	IModule* m = dynamic_cast<IModule*>(enumerator.get());

	JSON peParams;
	rapidjson::Document peParamsDoc;
    assert( m!=0);
	if( m != 0 ) {
		peParams = m->SaveParams();
		CJsonError error;
		const bool rslt = ReadJsonString( peParams, peParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("PatternEnumerator", peParamsDoc.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

int CStabilityCbOLocalProjectionChain::GetObjectNumber() const
{
	assert(enumerator != 0);
	return enumerator->GetObjectNumber();
}
double CStabilityCbOLocalProjectionChain::GetInterestThreshold() const
{
	return thld;
}
void CStabilityCbOLocalProjectionChain::UpdateInterestThreshold( const double& _thld )
{
	thld = thld;
}
double CStabilityCbOLocalProjectionChain::GetPatternInterest( const IPatternDescriptor* p )
{

}
bool CStabilityCbOLocalProjectionChain::IsTopoSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const
{

}
void CStabilityCbOLocalProjectionChain::FreePattern(const IPatternDescriptor* p ) const
{

}
void CStabilityCbOLocalProjectionChain::ComputeZeroProjection( CPatternList& ptrns )
{

}
bool CStabilityCbOLocalProjectionChain::Preimages( const IPatternDescriptor* d, CPatternList& preimages )
{

}
int CStabilityCbOLocalProjectionChain::GetExtentSize( const IPatternDescriptor* d ) const
{

}
JSON CStabilityCbOLocalProjectionChain::SaveExtent( const IPatternDescriptor* d ) const
{

}
JSON CStabilityCbOLocalProjectionChain::SaveIntent( const IPatternDescriptor* d ) const
{

}

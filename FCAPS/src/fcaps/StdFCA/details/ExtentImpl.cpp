// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <fcaps/modules/details/ExtentImpl.h>

#include <JSONTools.h>

#include <boost/ptr_container/ptr_deque.hpp>

using namespace std;
using namespace rapidjson;

////////////////////////////////////////////////////////////////////

static bool addObject( DWORD objectID, CList<DWORD>& ext)
{
	if( ext.IsEmpty() ) {
		ext.PushBack( objectID );
		return true;
	}
	if( objectID == ext.Back() ) {
		return false;
	}
	if( objectID > ext.Back() ) {
		ext.PushBack( objectID );
		return true;
	}
	CStdIterator<CList<DWORD>::CIterator, false> itr( ext );
	for( ; !itr.IsEnd(); ++itr ) {
		if( *itr == objectID ) {
			return false;
		} else if ( objectID < *itr ) {
			ext.Insert( itr, objectID );
			return true;
		}
	}
	assert( false );
	return false;
}

static void write( const CList<DWORD>& ext, std::ostream& dst )
{
	dst << "{ ";
	CStdIterator<CList<DWORD>::CConstIterator, false> obj( ext );
	for( ; !obj.IsEnd(); ++obj ) {
		dst << *obj << " ";
	}
	dst << "}";
}

////////////////////////////////////////////////////////////////////

CDequeExtentStorage::CDequeExtentStorage()
{
}

bool CDequeExtentStorage::AddObject( DWORD objectID, DWORD extentID )
{
	assert( 0 <= extentID && extentID < extents.size() );

	CExtent& ext = extents[extentID];
	return addObject( objectID, ext );
}

DWORD CDequeExtentStorage::Clone( DWORD extentID )
{
	if( extentID == -1 ) {
		extents.push_back( new CExtent );
	} else {
		extents.push_back( new CExtent( extents[extentID] ) );
	}
	return extents.size() - 1;
}

DWORD CDequeExtentStorage::Size( DWORD extentID ) const
{
	if( extentID == -1 ) {
		return 0;
	}
	assert( 0 <= extentID && extentID < extents.size() );
	return extents[extentID].Size();
}

DWORD CDequeExtentStorage::GetLastAddedObject( DWORD extentID ) const
{
	assert( 0 <= extentID && extentID < extents.size() );
	return extents[extentID].Back();
}

void CDequeExtentStorage::ListObjects( DWORD extentID, CList<DWORD>& list ) const
{
	if( extentID == -1 ) {
		list.Clear();
		return;
	}
	assert( 0 <= extentID && extentID < extents.size() );
	list = extents[extentID];
}

void CDequeExtentStorage::Write( DWORD extentID, std::ostream& dst ) const
{
	if( extentID == -1 ) {
		dst << "{}";
		return;
	}
	assert( 0 <= extentID && extentID < extents.size() );
	const CExtent& ext = extents[extentID];

	write( ext, dst );
}

DWORD CDequeExtentStorage::Load( const JSON& json )
{
	rapidjson::Document extentJSON;
	extentJSON.Parse( json.c_str() );
	if( !extentJSON.IsObject() || !extentJSON.HasMember( "Inds" ) || !extentJSON["Inds"].IsArray() ) {
		return -1;
	}
	const Value& inds = extentJSON["Inds"];

	extents.push_back( new CExtent );
	for( size_t i = 0; i < inds.Size(); ++i ) {
		if( !inds[i].IsUint() ) {
			extents.pop_back();
			return -1;
		}
		addObject( inds.GetUint(), extents.back() );
	}
	return extents.size() - 1;
}
JSON CDequeExtentStorage::Save( DWORD id ) const
{
	if( id == -1 ) {
		return JSON( "{\"Count\":0,\"Inds\":[]}" );
	}
	assert( 0 <= id && id < extents.size() );

	const CExtent& ext = extents[id];
	Document result;
	MemoryPoolAllocator<>& alloc = result.GetAllocator();
	result.SetObject()
		.AddMember( "Count", Value().SetUint( ext.Size() ), alloc )
		.AddMember( "Inds", Value().SetArray(), alloc );
	Value& inds = result["Inds"];

	CStdIterator<CExtent::CConstIterator, false> itr( ext );
	for( ; !itr.IsEnd(); ++itr ) {
		inds.PushBack( Value().SetUint( *itr ), alloc );
	}

	if( !names.empty() ) {
		result.AddMember("Names", Value().SetArray(), alloc );
		Value& namesArray = result["Names"];
		itr.Reset( ext );
		for( ; !itr.IsEnd(); ++itr ) {
			string name = StdExt::to_string(*itr);
			if( *itr < names.size() ) {
				name = names[*itr];
			}
			namesArray.PushBack( Value().SetString(name.c_str(), alloc ), alloc );
		}
	}

	JSON rslt;
	CreateStringFromJSON( result, rslt );
	return rslt;
}

////////////////////////////////////////////////////////////////////

CDequeExtentStorageWithClass::CDequeExtentStorageWithClass(  CObjectToClassMap& _objToClassMap ) :
	objToClassMap( _objToClassMap )
{
	extents.push_back( new CExtent );
}

bool CDequeExtentStorageWithClass::AddObject( DWORD objectID, DWORD extentID )
{
	assert( 0 <= extentID && extentID < extents.size() );
	assert( extentID != 0 );

	CExtent& ext = extents[extentID];
	const bool result = addObject( objectID, ext.Extent );

	CObjectToClassMap::const_iterator fnd = objToClassMap.find( objectID );
	const string objectClass = fnd == objToClassMap.end() ? "" : fnd->second;

	if( objectClass.empty() ) {
		return result;
	}
	if( !ext.IsInit ){
		ext.IsInit = true;
		ext.Class = objectClass;
		return result;
	}

	if( ext.Class != objectClass ) {
		ext.Class = "";
	}
	return result;
}

DWORD CDequeExtentStorageWithClass::Clone( DWORD extentID )
{
	assert( extentID == -1 || 0 <= extentID && extentID < extents.size() );
	if( extentID == -1) {
		extents.push_back( new CExtent );
	} else {
		extents.push_back( new CExtent( extents[extentID] ) );
	}
	return extents.size() - 1;
}

DWORD CDequeExtentStorageWithClass::Size( DWORD extentID ) const
{
	if( extentID == -1 ) {
		return 0;
	}
	assert( 0 <= extentID && extentID < extents.size() );
	return extents[extentID].Extent.Size();
}

DWORD CDequeExtentStorageWithClass::GetLastAddedObject( DWORD extentID ) const
{
	assert( 0 <= extentID && extentID < extents.size() );
	return extents[extentID].Extent.Back();
}

void CDequeExtentStorageWithClass::ListObjects( DWORD extentID, CList<DWORD>& list ) const
{
	if( extentID == -1 ) {
		list.Clear();
		return;
	}
	assert( 0 <= extentID && extentID < extents.size() );
	list = extents[extentID].Extent;
}

void CDequeExtentStorageWithClass::Write( DWORD extentID, std::ostream& dst ) const
{
	if( extentID == -1 ) {
		dst << "{}:--";
		return;
	}
	assert( 0 <= extentID && extentID < extents.size() );
	const CExtent& ext = extents[extentID];

	write( ext.Extent, dst );
	dst << ":\"" << ext.Class << "\"";
}

DWORD CDequeExtentStorageWithClass::Load( const JSON& json )
{
	rapidjson::Document extentJSON;
	extentJSON.Parse( json.c_str() );
	if( !extentJSON.IsObject() || !extentJSON.HasMember( "Inds" ) || !extentJSON["Inds"].IsArray() ) {
		return -1;
	}
	const Value& inds = extentJSON["Inds"];

	extents.push_back( new CExtent );
	for( size_t i = 0; i < inds.Size(); ++i ) {
		if( !inds[i].IsUint() ) {
			extents.pop_back();
			return -1;
		}
		addObject( inds.GetUint(), extents.back().Extent );
	}
	return extents.size() - 1;
}
JSON CDequeExtentStorageWithClass::Save( DWORD id ) const
{
	if( id == -1 ) {
		return JSON( "{\"Count\":0,\"Inds\":[]}" );
	}
	assert( 0 <= id && id < extents.size() );

	const CExtent& ext = extents[id];
	Document result;
	MemoryPoolAllocator<>& alloc = result.GetAllocator();
	result.SetObject()
		.AddMember( "Count", Value().SetUint( ext.Extent.Size() ), alloc )
		.AddMember( "Inds", Value().SetArray(), alloc );
	Value& inds = result["Inds"];
	CStdIterator<CList<DWORD>::CConstIterator, false> itr( ext.Extent );
	for( ; !itr.IsEnd(); ++itr ) {
		inds.PushBack( Value().SetUint( *itr ), alloc );
	}

	JSON rslt;
	CreateStringFromJSON( result, rslt );
	return rslt;
}

const string&  CDequeExtentStorageWithClass::GetExtentCommonClass( DWORD extentID ) const
{
	assert( 0 <= extentID && extentID < extents.size() );
	const CExtent& ext = extents[extentID];
	return ext.Class;
}

////////////////////////////////////////////////////////////////////

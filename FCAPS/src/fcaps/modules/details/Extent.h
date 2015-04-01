#ifndef EXTENT_H_INCLUDED
#define EXTENT_H_INCLUDED

#include <common.h>

#include <fcaps/BasicTypes.h>

#include <ListWrapper.h>

#include <vector>

// A storage for extents, works with their ids
//  id == 0 is empty read-only extent
interface IExtentStorage : public virtual IObject {
	// Add object to extent
	//  Returns true if the object was really added (if there is no such object before the addition)
	virtual bool AddObject( DWORD objectID, DWORD extentID ) = 0;
	// Clone given extent
	virtual DWORD Clone( DWORD extentID ) = 0;
	// Get size of the extent given by id
	virtual DWORD Size( DWORD extentID ) const = 0;
	// Get last added object ot the extent
	virtual DWORD GetLastAddedObject( DWORD extentID ) const = 0;
	// Enumerate all objectIds in a given extent
	virtual void ListObjects( DWORD extentId, CList<DWORD>& list ) const = 0;
	// Write the extent to the stream.
	virtual void Write( DWORD extentID, std::ostream& dst ) const = 0;
	// Load/Save pattern from/to JSON
	virtual DWORD Load( const JSON& ) = 0;
	virtual JSON Save( DWORD id ) const = 0;

	// Get/Set object names
	virtual const std::vector<std::string>& GetNames() const = 0;
	virtual void SetNames(const std::vector<std::string>& n) = 0;
};

#endif // EXTENT_H_INCLUDED

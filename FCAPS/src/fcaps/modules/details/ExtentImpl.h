#ifndef CEXTENTIMPL_H
#define CEXTENTIMPL_H

#include <fcaps/modules/details/Extent.h>

#include <ListWrapper.h>

#include <boost/ptr_container/ptr_deque.hpp>
#include <tr1/unordered_map>
#include <vector>

////////////////////////////////////////////////////////////////////

class CDequeExtentStorage : public IExtentStorage {
public:
	typedef CList<DWORD> CExtent;
public:
	CDequeExtentStorage();

	// Methods of IExtentStorage
	virtual bool AddObject( DWORD objectID, DWORD extentID );
	virtual DWORD Clone( DWORD extentID );
	virtual DWORD Size( DWORD extentID ) const;
	virtual DWORD GetLastAddedObject( DWORD extentID ) const;
	virtual void ListObjects( DWORD extentId, CList<DWORD>& list ) const;
	virtual void Write( DWORD extentID, std::ostream& dst ) const;
	virtual DWORD Load( const JSON& json );
	virtual JSON Save( DWORD id ) const;
	virtual const std::vector<std::string>& GetNames() const
		{return names;}
	virtual void SetNames(const std::vector<std::string>& n)
		{names = n;}

	// Get extent by id
	const CExtent& GetExtent( DWORD extentID ) const
		{ assert( extentID < extents.size() ); return extents[extentID]; }
	// Remove extent from the set
	void Delete( DWORD extentID )
		{ extents.replace( extentID, 0 ); }

private:
	boost::ptr_deque< boost::nullable<CExtent> > extents;
	std::vector<std::string> names;
};

////////////////////////////////////////////////////////////////////

class CDequeExtentStorageWithClass : public IExtentStorage {
public:
	typedef std::tr1::unordered_map<DWORD, std::string> CObjectToClassMap;
public:
	CDequeExtentStorageWithClass( CObjectToClassMap& _objToClassMap );

	// Methods of IExtentStorage
	virtual bool AddObject( DWORD objectID, DWORD extentID );
	virtual DWORD Clone( DWORD extentID );
	virtual DWORD Size( DWORD extentID ) const;
	virtual DWORD GetLastAddedObject( DWORD extentID ) const;
	virtual void ListObjects( DWORD extentId, CList<DWORD>& list ) const;
	virtual void Write( DWORD extentID, std::ostream& dst ) const;
	virtual DWORD Load( const JSON& json );
	virtual JSON Save( DWORD id ) const;

	// Methods of the class
	// Get extent common class
	const std::string& GetExtentCommonClass( DWORD extentID ) const;

private:
	struct CExtent{
		CList<DWORD> Extent;
		std::string Class;
		bool IsInit;
		CExtent() :
			IsInit( false ) {}
	};
private:
	boost::ptr_deque< CExtent > extents;
	const CObjectToClassMap& objToClassMap;
};

////////////////////////////////////////////////////////////////////
#endif // CEXTENTIMPL_H

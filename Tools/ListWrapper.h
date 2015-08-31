// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef ListWrapper_H_
#define ListWrapper_H_

#include <common.h>
#include <StdTools.h>
#include <list>
////////////////////////////////////////////////////////////////////

template <typename T>
class CList {
public:
	typedef typename std::list<T>::iterator CIterator;
	typedef typename std::list<T>::const_iterator CConstIterator;
	typedef typename std::list<T>::reverse_iterator CReverseIterator;
	typedef typename std::list<T>::const_reverse_iterator CConstReverseIterator;

	typedef CList<T> _Self;

public:
	CList() : size( 0 ) {}

	bool IsEmpty() const
		{ return size == 0; }
	void Clear()
		{ list.clear(); size = 0; }
	CIterator Erase( CIterator& itr )
		{ --size; return list.erase( itr ); }

	void PushBack( const T& node )
		{ list.push_back( node ); ++size; }
	void PopBack()
		{ assert( !IsEmpty() ); list.pop_back(); --size; }
	T& Back()
		{ assert( !IsEmpty() ); return list.back(); }
	const T& Back() const
		{ assert( !IsEmpty() ); return list.back(); }
	void PushFront( const T& node )
		{ list.push_front( node ); ++size; }
	T& Front()
		{ assert( !IsEmpty() ); return list.front(); }
	const T& Front() const
		{ assert( !IsEmpty() ); return list.front(); }
	void PopFront()
		{ assert( !IsEmpty() ); list.pop_front(); --size; }
	void Insert( CIterator& itr, const T& node )
		{ list.insert( itr, node); ++size; }
	void Sort()
		{ list.sort(); }

	DWORD Size() const { return size; }

	CIterator Begin() { return list.begin(); }
	CConstIterator Begin() const { return list.begin(); }
	CIterator End() { return list.end(); }
	CConstIterator End() const { return list.end(); }

	CReverseIterator RBegin() { return list.rbegin(); }
	CConstReverseIterator RBegin() const { return list.rbegin(); }
	CReverseIterator REnd() { return list.rend(); }
	CConstReverseIterator REnd() const { return list.rend(); }

	void CopyTo( _Self& other ) const { StdExt::CopyList( other.list, list ); other.size = size; }

private:
	std::list<T> list;
	DWORD size;

	void assertCheck() const
		{ return; assert( size == list.size() ); }
};

///////////////////////////////////////////////////////////////
#endif // ListWrapper_H_

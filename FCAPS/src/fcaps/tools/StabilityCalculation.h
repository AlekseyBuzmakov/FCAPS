// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef STABILITYCALCULATION_H
#define STABILITYCALCULATION_H

#include <common.h>
#include <boost/container/deque.hpp>

template<typename T>
class CList;

class CVectorBinarySetDescriptor;
class CVectorBinarySetJoinComparator;

typedef boost::container::deque< CSharedPtr<CVectorBinarySetDescriptor> > CBinarySetCollection;
typedef boost::container::deque< CSharedPtr<const CVectorBinarySetDescriptor> > CConstBinarySetCollection;

// Add a new vertical line to the table, i.e. add the columnNum to every horisontal line in values.
void AddColumnToCollection( CSharedPtr<CVectorBinarySetJoinComparator>& cmp,
	DWORD columnNum, const CList<DWORD>& values, CBinarySetCollection& table );

#endif // STABILITYCALCULATION_H


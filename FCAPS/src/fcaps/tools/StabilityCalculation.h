#ifndef STABILITYCALCULATION_H
#define STABILITYCALCULATION_H

#include <boost/container/deque.hpp>

class CVectorBinarySetDescriptor;

typedef boost::container::deque< CSharedPtr<CVectorBinarySetDescriptor> > CBinarySetCollection;
typedef boost::container::deque< CSharedPtr<const CVectorBinarySetDescriptor> > CConstBinarySetCollection;

#endif // STABILITYCALCULATION_H


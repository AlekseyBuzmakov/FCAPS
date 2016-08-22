// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CJSONINTERVALPATTERNPROCESSOR_H
#define CJSONINTERVALPATTERNPROCESSOR_H

#include <fcaps/BasicTypes.h>

#include <vector>

namespace JsonIntervalPattern {
////////////////////////////////////////////////////////////////////////

typedef std::vector< std::pair<double,double> > CPattern;

void LoadPattern( const JSON& json, CPattern& result );
JSON SavePattern( const CPattern& result );

////////////////////////////////////////////////////////////////////////
} // namespace JsonIntervalPattern


#endif // CJSONINTERVALPATTERNPROCESSOR_H

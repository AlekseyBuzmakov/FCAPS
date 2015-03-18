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

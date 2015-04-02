#ifndef JSONCLASSIFIERCLASSES_H
#define JSONCLASSIFIERCLASSES_H

#include <boost/unordered_map.hpp>

namespace JsonClassifierClasses {
////////////////////////////////////////////////////////////////////////

typedef boost::unordered_map<std::string,std::string> CObjToClassesMap;

void Load( const std::string& classesPath, CObjToClassesMap& );

////////////////////////////////////////////////////////////////////////
} // namespace JsonClassifierClasses

#endif // JSONCLASSIFIERCLASSES_H

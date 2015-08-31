// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

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

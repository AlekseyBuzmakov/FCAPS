// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef JSONBINARYPATTERN_H
#define JSONBINARYPATTERN_H

#include <fcaps/BasicTypes.h>

template<typename T>
class CList;

namespace JsonBinaryPattern {
////////////////////////////////////////////////////////////////////

typedef CList<DWORD> CIndices;
typedef CList<std::string> CNames;

// Load/Saves a set pattern. Set can be given by indices and/or by names.
void LoadPattern( const JSON& json, CIndices& inds, CNames& names );
JSON SavePattern( CIndices& inds, CNames& names );

////////////////////////////////////////////////////////////////////
} // namespace JsonBinaryPattern

#endif // JSONBINARYPATTERN_H

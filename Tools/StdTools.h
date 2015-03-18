#ifndef STDLISTTOOLS_H_INCLUDED
#define STDLISTTOOLS_H_INCLUDED

#include <list>
#include <boost/lexical_cast.hpp>

namespace StdExt {
////////////////////////////////////////////////////////////////////////

template <typename T>
void CopyList( std::list<T>& dst, const std::list<T>& src );

template <typename T>
std::string to_string( const T& value ) {
	return boost::lexical_cast<std::string>(value);
}

////////////////////////////////////////////////////////////////////////
} // namespace StdExt

#include <StdTools.inl>

#endif // STDLISTTOOLS_H_INCLUDED

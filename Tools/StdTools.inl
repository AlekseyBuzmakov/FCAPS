#ifndef STDLISTTOOLS_INL_INCLUDED
#define STDLISTTOOLS_INL_INCLUDED

namespace StdExt {
////////////////////////////////////////////////////////////////////////

template <typename T>
void CopyList( std::list<T>& dst, const std::list<T>& src )
{
    dst.clear();
    typename std::list<T>::const_iterator i;
    for( i = src.begin(); i != src.end(); ++i ) {
        dst.push_back( *i );
    }
}

////////////////////////////////////////////////////////////////////////
} // namespace StdExt


#endif // STDLISTTOOLS_INL_INCLUDED

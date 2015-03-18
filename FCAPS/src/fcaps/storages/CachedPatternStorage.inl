////////////////////////////////////////////////////////////////////

template<typename THasher>
CCachedPatternStorage<THasher>::CCachedPatternStorage()
{
	//ctor
}

template<typename THasher>
void CCachedPatternStorage<THasher>::Initialize( const THasher& h )
{
	Clear();
	hasher = h;
	patterns.reset(new CPatternSet( 3571, hasher, hasher ) );
}

template<typename THasher>
void CCachedPatternStorage<THasher>::Reserve( size_t size )
{
	if( patterns == 0 ) {
		return;
	}
	const size_t desiredSize = size / patterns->max_load_factor() + 1;
	if( desiredSize > patterns->bucket_count() ) {
		patterns->rehash(desiredSize);
	}
}
template<typename THasher>
const IPatternDescriptor* CCachedPatternStorage<THasher>::AddPattern( const IPatternDescriptor* p )
{
	assert( patterns != 0 );
	assert( p != 0 );
	std::pair<typename CPatternSet::iterator, bool> newP = patterns->insert(p);
	if( !newP.second ) {
		hasher.Free(p);
	}
	return *newP.first;
}
template<typename THasher>
bool CCachedPatternStorage<THasher>::HasPattern( const IPatternDescriptor* p ) const
{
	assert( patterns != 0 );
	return patterns->find(p) != patterns->end();
}

template<typename THasher>
void CCachedPatternStorage<THasher>::RemovePattern(const IPatternDescriptor* p)
{
	assert( patterns != 0 );
	patterns->erase( p );
	hasher.Free(p);
}

template<typename THasher>
void CCachedPatternStorage<THasher>::RemovePattern(const CIterator& itr)
{
	assert( patterns != 0 );
	const IPatternDescriptor* p = *itr;
	patterns->erase(itr);
	hasher.Free(p);
}

template<typename THasher>
typename CCachedPatternStorage<THasher>::CIteratorWrapper CCachedPatternStorage<THasher>::Iterator()
{
	return CIteratorWrapper( *patterns );
}
template<typename THasher>
typename CCachedPatternStorage<THasher>::CConstIteratorWrapper CCachedPatternStorage<THasher>::Iterator() const
{
	return CConstIteratorWrapper( *patterns );
}

template<typename THasher>
void CCachedPatternStorage<THasher>::Clear()
{
	if( patterns == 0 ) {
		return;
	}
	CStdIterator<CIterator,true> itr( *patterns ), itr2;
	for( ; !itr.IsEnd(); ) {
		itr2=itr;
		++itr;
		RemovePattern(itr2);
	}
}

////////////////////////////////////////////////////////////////////

inline bool CPatternComparatorHasher::operator()(
	const IPatternDescriptor* x, const IPatternDescriptor* y) const
{
	assert( x != 0 && y != 0 );
	return cmp->Compare( x, y, CR_Equal ) == CR_Equal;
}

inline size_t CPatternComparatorHasher::operator()( const IPatternDescriptor* x) const
{
	assert( x != 0 );
	return x->Hash();
}
inline void CPatternComparatorHasher::Free( const IPatternDescriptor* x)
{
	cmp->FreePattern(x);
}

////////////////////////////////////////////////////////////////////

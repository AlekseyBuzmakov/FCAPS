#ifndef STDITERATORWRAPPER_H_INCLUDED
#define STDITERATORWRAPPER_H_INCLUDED

template <typename TIterator, typename Wrapper, typename Reference, typename Pointer>
class CStdIteratorBase {
public:
	typedef CStdIteratorBase<TIterator, Wrapper, Reference, Pointer> _Self;

public:
	CStdIteratorBase() :
		endItr( itr ) {}
	CStdIteratorBase( TIterator _itr, TIterator _endItr ) :
		itr( _itr ), endItr( _endItr ) {}
	CStdIteratorBase( const _Self& other ) :
		itr( other.itr ), endItr( other.endItr ) {}

	void Reset( const TIterator& _itr, const TIterator& _endItr )
		{ itr = _itr; endItr = _endItr; }

	Reference operator*()
		{ return *itr; }

	Pointer operator->()
		{ return &*itr; }
	operator Pointer()
		{ return &*itr; }
	operator TIterator&()
		{ return itr; }

	Wrapper& operator++()
		{ ++itr; return getWrapper(); }

	Wrapper operator++(int)
		{ Wrapper tmp = getWrapper(); ++itr; return tmp; }

	Wrapper& operator--()
		{ --itr; return getWrapper(); }

	Wrapper operator--(int)
		{ Wrapper tmp = getWrapper(); --itr; return tmp; }

	bool operator==( const Wrapper& other ) const
		{ return itr == other.itr; }
	bool operator!=( const Wrapper& other ) const
		{ return itr != other.itr; }

	Wrapper& operator=( const TIterator& other )
		{ itr = other; return getWrapper(); }
	Wrapper& Update( const TIterator& other )
		{ itr = other; return getWrapper(); }

	TIterator Get() const
		{ return itr; }

	bool IsEnd() const
		{ return itr == endItr; }

private:
	TIterator itr;
	TIterator endItr;

	Wrapper& getWrapper() { return static_cast<Wrapper&>( *this ); }
};

template <typename TIterator, bool IsStd = true>
class CStdIterator : public CStdIteratorBase<TIterator, CStdIterator<TIterator>, typename TIterator::reference, typename TIterator::pointer> {
public:
	CStdIterator()
		{}
	CStdIterator( const TIterator& itr, const TIterator& endItr ) :
		_Base( itr, endItr ) {}
	CStdIterator( const CStdIterator<TIterator,IsStd>& other ) :
		_Base( other ) {}
	template<typename TObject>
	explicit CStdIterator( TObject& obj ) :
		_Base( obj.begin(), obj.end() ) {}
	explicit CStdIterator( const std::pair<TIterator,TIterator>& pair ) :
		_Base( pair.first, pair.second ) {}

	void Reset( const TIterator& itr, const TIterator& endItr )
		{ _Base::Reset( itr, endItr ); }
	template<typename TObject>
	void Reset( TObject& obj )
		{ _Base::Reset( obj.begin(), obj.end()); }
	void Reset( const std::pair<TIterator,TIterator>& pair )
		{ _Base::Reset( pair.first, pair.second ); }


private:
	typedef CStdIteratorBase<TIterator, CStdIterator<TIterator>, typename TIterator::reference, typename TIterator::pointer> _Base;
};

template <typename TIterator>
class CStdIterator<TIterator, false> : public CStdIteratorBase<TIterator, CStdIterator<TIterator, false>, typename TIterator::reference, typename TIterator::pointer> {
public:
	CStdIterator()
		{}
	CStdIterator( const TIterator& itr, const TIterator& endItr ) :
		_Base( itr, endItr ) {}
	CStdIterator( const CStdIterator<TIterator, false>& other ) :
		_Base( other ) {}
	template<typename TObject>
	explicit CStdIterator( TObject& obj ) :
		_Base( obj.Begin(), obj.End() ) {}
	explicit CStdIterator( const std::pair<TIterator,TIterator>& pair ) :
		_Base( pair.first, pair.second ) {}

	void Reset( const TIterator& itr, const TIterator& endItr )
		{ _Base::Reset( itr, endItr ); }
	template<typename TObject>
	void Reset( TObject& obj )
		{ _Base::Reset( obj.Begin(), obj.End()); }
	void Reset( const std::pair<TIterator,TIterator>& pair )
		{ _Base::Reset( pair.first, pair.second ); }

private:
	typedef CStdIteratorBase<TIterator, CStdIterator<TIterator, false>, typename TIterator::reference, typename TIterator::pointer> _Base;
};




#endif // STDITERATORWRAPPER_H_INCLUDED

// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CRMQ_H
#define CRMQ_H

#include <common.h>
#include <boost/shared_array.hpp>

#include <vector>

#include <math.h>

////////////////////////////////////////////////////////////////////

template<typename T, typename CIndexType>
class CRmqAlgorithm {
public:
	// Does not copy the array, saves the link
	CRmqAlgorithm( std::vector<T>& _array ) :
		array( _array ) {}

	// Initialization by the array
	//  Given array should not be changed after this function
	void Initialize();

	// Query array
	CIndexType GetMinIndexOnRange( CIndexType i, CIndexType j ) const;

	// Get array
	const std::vector<T>& GetArray() const
		{ return array; }

private:
	// Initial Array
	const std::vector<T>& array;
	// Precomputted logariphms array
	std::vector<CIndexType> logs;
	// Precomputted min indexes index[i,k] on interval [i, i+2^k]
	std::vector< boost::shared_array<CIndexType> > index;

	// Number of elements in index for k line
	CIndexType getIndexSize( CIndexType k ) const
		{ return array.size() - (1 << k) + 1; }
};

template<typename T, typename CIndexType>
inline CIndexType CRmqAlgorithm<T,CIndexType>::GetMinIndexOnRange( CIndexType i, CIndexType j ) const
{
	assert( i <= j );
	assert( j <= array.size() );

	if( i == j ) {
		return i;
	}

	assert( j-i + 1 < logs.size() );

	const CIndexType& k = logs[j-i+1];
	assert( i <= j - (1 << k) + 1 );
	assert( j - (1 << k) + 1 <= getIndexSize(k) );
	const CIndexType& firstIndex = index[k][i];
	const CIndexType& secondIndex = index[k][j - (1 << k) + 1];
	return array[firstIndex] < array[secondIndex] ? firstIndex : secondIndex;
}

template<typename T, typename CIndexType>
void CRmqAlgorithm<T,CIndexType>::Initialize()
{
	assert( array.size() <= (1 << (8*sizeof(CIndexType)-1)) -1 + (1 << (8*sizeof(CIndexType)-1)));
	const CIndexType size = array.size();
	logs.resize( size + 1 );
	for( CIndexType i = 1; i < logs.size(); ++i ) {
		logs[i] = floor( log2( i ) );
	}

	index.resize( logs[size] + 1 );
	for( CIndexType k = 0; k <= logs[size]; ++k ) {
		const CIndexType currIndexSize = getIndexSize(k);
		index[k].reset( new CIndexType[getIndexSize(k)] );
		if( k == 0 ) {
			for( CIndexType i = 0; i < currIndexSize; ++i ) {
				index[k][i]= i;
			}
		} else {
			for( CIndexType i = 0; i < currIndexSize; ++i ) {
				const CIndexType firstIndex = index[k-1][i];
				assert( i + (1<<(k-1)) < getIndexSize( k - 1 ) );
				const CIndexType secondIndex = index[k-1][i + ( 1 << ( k - 1 ) )];
				index[k][i]= array[firstIndex] < array[secondIndex] ? firstIndex : secondIndex;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////

#define RMQ(prefix, FUNC) \
	assert( rmqAlgo != 0 ); \
	switch( type ) { \
			case RAT_Byte: \
				prefix static_cast< CRmqAlgorithm<T,unsigned char> >(rmqAlgo).FUNC; \
			case RAT_Short: \
				prefix static_cast< CRmqAlgorithm<T,unsigned short> >(rmqAlgo).FUNC; \
			case RAT_DWORD: \
				prefix static_cast< CRmqAlgorithm<T,DWORD> >(rmqAlgo).FUNC; \
			default: \
				assert(false); \
		}

template<typename T>
class CRmqAlgorithmAuto {
public:
	// Does not copy the array, saves the link
	CRmqAlgorithmAuto( std::vector<T>& _array ) :
		type( getAlgoType( _array.size() ) ),
		rmqAlgo( createAlgo( _array ) ) {}
	~CRmqAlgorithmAuto()
	{
		switch( type ) {
			case RAT_Byte:
				delete &getAlgo<unsigned char>();
				break;
			case RAT_Short:
				delete &getAlgo<unsigned short>();
				break;
			case RAT_DWORD:
				delete &getAlgo<DWORD>();
				break;
			default:
				assert(false);
		}
	}

	// Initialization by the array
	//  Given array should not be changed after this function
	void Initialize()
	{
		switch( type ) {
			case RAT_Byte:
				getAlgo<unsigned char>().Initialize();
				return;
			case RAT_Short:
				getAlgo<unsigned short>().Initialize();
				return;
			case RAT_DWORD:
				getAlgo<DWORD>().Initialize();
				return;
			default:
				assert(false);
		}
	}

	// Query array
	DWORD GetMinIndexOnRange( DWORD i, DWORD j ) const
	{
		switch( type ) {
			case RAT_Byte:
				assert( i < 256 && j < 256 );
				return getAlgo<unsigned char>()
					.GetMinIndexOnRange( static_cast<unsigned char>(i),static_cast<unsigned char>(j));
			case RAT_Short:
				assert( i < (1<<16) && j < (1<<16) );
				return getAlgo<unsigned short>()
					.GetMinIndexOnRange( static_cast<unsigned short>(i),static_cast<unsigned short>(j));
			case RAT_DWORD:
				return getAlgo<DWORD>().GetMinIndexOnRange(i,j);
			default:
				assert(false);
				return getAlgo<DWORD>().GetMinIndexOnRange(i,j);
		}
	}

	// Get array
	const std::vector<T>& GetArray() const
	{
		switch( type ) {
			case RAT_Byte:
				return getAlgo<unsigned char>().GetArray();
			case RAT_Short:
				return getAlgo<unsigned short>().GetArray();
			case RAT_DWORD:
				return getAlgo<DWORD>().GetArray();
			default:
				assert(false);
				return getAlgo<DWORD>().GetArray();
		}
	}

private:
	enum TRmqAlgoType {
		RAT_Byte = 0,
		RAT_Short,
		RAT_DWORD,

		RAT_EnumCount
	};

private:
	const TRmqAlgoType type;
	void* const rmqAlgo;

	static TRmqAlgoType getAlgoType( size_t size )
	{
		if( size < 256 ){
			return RAT_Byte;
		} else if( size < (1 << 16) ) {
			return RAT_Short;
		} else {
			assert(size < (1<<31)-1 + (1<<31)) ;
			return RAT_DWORD;
		}
	}
	static void* createAlgo( std::vector<T>& array )
	{
		switch( getAlgoType(array.size()) ) {
			case RAT_Byte:
				return new CRmqAlgorithm<T,unsigned char>(array);
			case RAT_Short:
				return new CRmqAlgorithm<T,unsigned short>(array);
			case RAT_DWORD:
				return new CRmqAlgorithm<T,DWORD>(array);
			default:
				assert(false);
				return 0;
		}
	}
	template<typename IndexType>
	CRmqAlgorithm<T,IndexType>& getAlgo()
	{
		assert( rmqAlgo != 0 );
		return *static_cast<CRmqAlgorithm<T,IndexType>*>(rmqAlgo);
	}
	template<typename IndexType>
	const CRmqAlgorithm<T,IndexType>& getAlgo() const
	{
		assert( rmqAlgo != 0 );
		return *static_cast<const CRmqAlgorithm<T,IndexType>*>(rmqAlgo);
	}

};

#endif // CRMQ_H

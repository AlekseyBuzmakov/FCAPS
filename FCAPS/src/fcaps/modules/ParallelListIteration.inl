// Macros for parallel iteration of two sorted lists

#define PARALLEL_LIST_ITERATION_BEGIN( TList, TListItr, firstList, lastList, firstIteratorName, lastIteratorName ) \
	const TList& __firstListRef = firstList;\
	const TList& __lastListRef = lastList;\
	TListItr firstIteratorName = __firstListRef.Begin(); \
	TListItr lastIteratorName = __lastListRef.Begin(); \
	while( firstIteratorName != __firstListRef.End() && lastIteratorName != __lastListRef.End() ) {

#define PARALLEL_LIST_ITERATION_END( firstIteratorName, lastIteratorName ) \
		if( *firstIteratorName < *lastIteratorName ) {\
			++firstIteratorName;\
		} else if( *firstIteratorName > *lastIteratorName ) {\
			++lastIteratorName;\
		} else {\
			++firstIteratorName;\
			++lastIteratorName;\
		}\
	}

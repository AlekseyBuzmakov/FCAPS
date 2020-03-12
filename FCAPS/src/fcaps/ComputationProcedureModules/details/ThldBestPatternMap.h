// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2020, v0.8
// A small map that for a given unsupervised quality of a pattern returns the best supervised quality

#ifndef THLDBESTPATTERNMAP_H
#define THLDBESTPATTERNMAP_H

#include <map>

////////////////////////////////////////////////////////////////////
// Value is suppesed to implemente operator() returning a vulue supporting < and > opeartions

template<typename T>
class CThldBestPatternKeyLess{
public:
	bool operator()(const T& a, const T& b ) const
	{
		return a < b;
	}
};
template<>
bool CThldBestPatternKeyLess<double>::operator()(const double& a, const double& b ) const
{
	return a < b - std::numeric_limits<double>::epsilon(); // for the double type
}
template<>
bool CThldBestPatternKeyLess<float>::operator()(const float& a, const float& b ) const
{
	return a < b - std::numeric_limits<float>::epsilon(); // for the double type
}

template<typename Key, typename Value>
class CThldBestPatternMap {
public:
	typedef std::map< Key,Value,CThldBestPatternKeyLess<Key> > Storage;
	typedef typename Storage::const_iterator const_iterator;
public:
	CThldBestPatternMap() :
		minAcceptQ(-1e10) {}
	// Insert a new element (if not new, w.r.t. KEY, just ignoring) to the set
	// returns if the element has been introduced
	bool Insert(const Key& k, Value v) {
		const bool res = insert(k,v);
		assert(check());
		return res;
	}
	// Returns the currently known quality for Key >= k
	const double& GetQuality(const Key& k) const {
		auto moreItr = map.upper_bound(k);
		if( moreItr == End() ) {
			return minAcceptQ;
		} else {
			return moreItr->second();
		}
	}
	// Removes all element with Key smaller than the given value
	void SetMinKey(const Key& k) {
		auto res = map.upper_bound(k);
		if( res != Begin()) {
			map.erase(Begin(),res);
		}
		assert(check());
	}
	// Get the front quality
	const double& GetFrontQuality() const {
		if( map.size() == 0 ) {
			return minAcceptQ;
		} else {
			return map.begin()->second();
		}
	}
	// Get/set the minimal interesting quality
	const double& GetMinAcceptableQuality() {
		return minAcceptQ;
	}
	const void SetMinAcceptableQuality(const double& q) {
		assert(map.size() == 0);
		minAcceptQ = q;
	}
	// Iteration
	const_iterator Begin() const
		{return map.begin();}
	const_iterator End() const
		{return map.end();}

	// Checks if has some elements
	bool HasValues() const { return !map.empty(); }

private:
	// The storage
	Storage map;
	// The minimal quality of a value that can be accepted
	double minAcceptQ;

	// A functions that insert an element with no checking
	bool insert(const Key& k, Value v);
	// A functions that check the correctness of the internal structure
	bool check();
};

template<typename Key, typename Value>
bool CThldBestPatternMap<Key,Value>::insert(const Key& k, Value v) {
	if( v() < minAcceptQ ) {
		// too bad
		return false;
	}
	if( Begin() == End() ) {
		// empty storage, just inserting
		map.insert(Begin(), std::pair<Key,Value>(k,v));
		return true;
	}
	auto inserted=End();
	auto moreEqItr = map.lower_bound(k);
	assert(moreEqItr == End() || moreEqItr->first >= k);
	if(moreEqItr == End()) {
		// no such large numbers, should be inserted
		inserted = map.insert(End(), std::pair<Key,Value>(k,v));
	} else if(moreEqItr->first == k) { // ? floating point operations ?
		if(moreEqItr->second() < v()) {
			// new pattern is better
			moreEqItr->second=v;
			inserted = moreEqItr;
		} else {
			return false;
		}
	} else {
		if( moreEqItr->second() < v() ) {
			// inserting to position
			inserted = map.insert(moreEqItr, std::pair<Key,Value>(k,v));
		} else {
			// pattern is useless, since better pattern has better quality
			return false;
		}
	}
	
	assert( inserted != End() );
	assert(v() == inserted->second());

	// now it can have better quality than some worser patterns, they should be removed
	auto itr = inserted;
	for(;;) {
		if( itr == Begin()) {
			break;
		}
		--itr;
		if( itr->second() > v() ) {
			++itr;
			break;
		}
	}
	if( itr != inserted) {
		map.erase(itr,inserted);
	}

	return true;
}


template<typename Key, typename Value>
bool CThldBestPatternMap<Key,Value>::check()
{
	if( map.empty() ) {
		return true;
	}
	auto itr = Begin();
	assert(Begin() != End());
	Key prevK = itr->first;
	double prevQ = itr->second();
	for(++itr; itr != End(); ++itr) {
		assert( prevK < itr->first );
		assert( prevQ > itr->second() );
		prevK = itr->first;
		prevQ = itr->second();
	}
	return true;
}
#endif // THLDBESTPATTERNMAP_H

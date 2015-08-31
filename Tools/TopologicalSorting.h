// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CTOPOLOGICALSORTING_H
#define CTOPOLOGICALSORTING_H

#include <vector>

// Class takes size of array and array of edges. Returns the places where every node should be placed.
class CTopologicalSorting {
public:
	CTopologicalSorting( int _size, const std::vector< std::pair<int,int> >& _edges ) :
		size(_size),edges(_edges)
			{}
	void Compute();
	const std::vector<int>& GetPlaces() const
		{ return places; }
private:
	const int size;
	const std::vector< std::pair<int,int> >& edges;
	std::vector<int> places;
	std::vector<int> inputEdges;

	void init();
};

#endif // CTOPOLOGICALSORTING_H

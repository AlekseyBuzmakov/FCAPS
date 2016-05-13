// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <TopologicalSorting.h>

#include <list>
#include <assert.h>

using namespace std;

////////////////////////////////////////////////////////////////////

void CTopologicalSorting::Compute()
{
	init();
	list<int> noInputNodes;
	for( int i = 0; i < inputEdges.size(); ++i ) {
		if( inputEdges[i] == 0 ) {
			noInputNodes.push_back(i);
		}
	}
	assert(!noInputNodes.empty());
	int nextPosition = 0;
	while( !noInputNodes.empty() ) {
		const int nextNode = noInputNodes.front();
		noInputNodes.pop_front();
		places[nextNode]=nextPosition++;

		for( int i = 0; i < edges.size(); ++i ) {
			if(edges[i].first != nextNode ) {
				continue;
			}
			const int numEdges = --inputEdges[edges[i].second];
			if( numEdges == 0 ) {
				noInputNodes.push_back(edges[i].second);
			}
		}
	}
	assert( nextPosition == size );
}

void CTopologicalSorting::init()
{
	places.resize(size,-1);
	inputEdges.resize(size,0);

	for( int i = 0; i < edges.size(); ++i ) {
		++inputEdges[edges[i].second];
	}
}


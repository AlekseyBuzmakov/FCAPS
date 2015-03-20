#ifndef CSTABILITYCHILDRENAPPROXIMATION_H
#define CSTABILITYCHILDRENAPPROXIMATION_H

#include <common.h>

#include <fcaps/PatternManager.h>
#include <fcaps/tools/StabilityCalculation.h>

#include <ListWrapper.h>

#include <deque>
#include <vector>

class CVectorBinarySetJoinComparator;
class CVectorBinarySetDescriptor;

class CStabilityChildrenApproximation {
public:
	CStabilityChildrenApproximation(
		CVectorBinarySetJoinComparator& _cmp,
		bool isLog = true,
		bool _isDebug  = false );
	~CStabilityChildrenApproximation();

	// Get/Set context on which we are working
	const CBinarySetCollection& GetContext() const
		{ return *attrToTidsetMap; }
	void SetContext( const CBinarySetCollection& c)
		{ attrToTidsetMap = &c; }

	// Get/Set order of attributes in the context
	const std::vector<DWORD>* GetAttrOrder() const
		{ return attrOrder; }
	void SetAttrOrder( const std::vector<DWORD>* order )
		{ attrOrder = order; }

	// Get/Set threshold for the stable concepts
	//  depends on isLog parameter
	const double& GetStableThreshold() const
		{ return threshold; }
	void SetStableThreshold( double value );

	// Get/Set maximal number of processed attributes
	DWORD GetMaxAttrNumber() const
		{ return attrNumLimit; }
	void SetMaxAttrNumber( DWORD n )
		{ attrNumLimit = n; }

	// Init computation of stability estimate
	void InitComputation(
		const CVectorBinarySetDescriptor& extent, const CList<DWORD>& intent );
	// Compute corresponding bound
	void ComputeLowerBound();
	void ComputeUpperBound();

	// Get left and right limits of stability for the concept
	const double& GetStabilityLeftLimit() const
		{ return leftLimit; }
	const double& GetStabilityRightLimit() const
		{ return rightLimit; }
	// Get the attribute generating the minimal difference for the concept
	DWORD GetMinDiffAttr() const
		{ return minDiffAttr; }

private:
	CVectorBinarySetJoinComparator& cmp;
	CPatternDeleter deleter;
	const CBinarySetCollection* attrToTidsetMap;
	const std::vector<DWORD>* attrOrder;

	// threshold for stable concepts
	double threshold;

	// The size of the extent in computation.
	DWORD extentSize;
	// The minimal difference for the concept.
	DWORD minDiff;
	// The attribute id providing the minimal difference
	DWORD minDiffAttr;
	// Maximal number of attributes in consideration
	DWORD attrNumLimit;
	// Set of extent for children of that concept
	std::deque< CSharedPtr<const CVectorBinarySetDescriptor> > children;
	// left limit of stability
	double leftLimit;
	// right limit of stability;
	double rightLimit;
	// Has it been detected the pattern unstable?
	bool isUnstable;
	// Is logariphmic scale?
	const bool isLog;
	// Is under debug?
	const bool isDebug;

	void clear();
//	void computeAttrToTidsetMap( const CBinarySetCollection& _attrToTidsetMap );

	DWORD getAttrCount() const;
	const CVectorBinarySetDescriptor& getAttrByNum( DWORD num ) const;

	const CVectorBinarySetDescriptor* computeIntersection(
		DWORD colNum, const CVectorBinarySetDescriptor& extent ) const;
	CSharedPtr<const CVectorBinarySetDescriptor> computeExtent(
		const CList<DWORD>& intent ) const;
	bool checkIntent(
		const CVectorBinarySetDescriptor& extent, const CList<DWORD>& intent ) const;
	DWORD getAttrNum() const;
};

#endif // CSTABILITYCHILDRENAPPROXIMATION_H

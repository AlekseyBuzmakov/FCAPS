// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CINTERVALCLSPATTERNSPROJECTIONCHAIN_H
#define CINTERVALCLSPATTERNSPROJECTIONCHAIN_H

#include <fcaps/ProjectionChain.h>

#include <fcaps/modules/VectorBinarySetDescriptor.h>

#include <fcaps/modules/details/JsonIntervalPattern.h>

#include <deque>

////////////////////////////////////////////////////////////////////

class CIntervalClsPatternsProjectionChain : public IProjectionChain {
public:
	CIntervalClsPatternsProjectionChain();

	// Methdos of IProjectionChain
	virtual const std::vector<std::string>& GetObjNames() const;
	virtual void SetObjNames( const std::vector<std::string>& );

	virtual void AddObject( DWORD objectNum, const JSON& intent );

	virtual void UpdateInterestThreshold( const double& thld );

	virtual bool AreEqual(const IPatternDescriptor* p, const IPatternDescriptor* q) const;
	virtual bool IsSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const;
	virtual bool IsTopoSmaller(const IPatternDescriptor* p, const IPatternDescriptor* q) const;
	virtual void FreePattern(const IPatternDescriptor* p ) const;

	virtual void ComputeZeroProjection( CPatternList& ptrns );
	virtual bool NextProjection();
	virtual double GetProgress() const;
	virtual void Preimages( const IPatternDescriptor* d, CPatternList& preimages );

	virtual JSON SaveExtent( const IPatternDescriptor* d ) const;
	virtual JSON SaveIntent( const IPatternDescriptor* d ) const;

protected:
	typedef std::vector< std::pair<DWORD,DWORD> > CIntent;
	typedef std::vector<CIntent> CContext;

	class CPatternDescription : public IPatternDescriptor {
	public:
		CPatternDescription( const CSharedPtr<const CVectorBinarySetDescriptor>& e ) :
			extent(e) {}
		// Methdos of IPatternDescriptor
		virtual TPatternType GetType() const { return PT_Other; }
		virtual bool IsMostGeneral() const { /*TODO?*/return false; };
		virtual size_t Hash() const { return extent->Hash(); }

		// Methods of this class
		const CVectorBinarySetDescriptor& Extent() const { return *extent;}

		CIntent& Intent() const {return intent;}

	private:
		CSharedPtr<const CVectorBinarySetDescriptor> extent;
		mutable CIntent intent;
	};

	struct CCurrState {
		enum TState {
			S_Left = 0,
			S_Right,
			S_End,

			S_EnumCount
		} State;
		DWORD AttrNum;
		DWORD ValueNum;

		CCurrState() : State(S_Left), AttrNum(0), ValueNum(0) {}
	};

protected:
	virtual CPatternDescription* NewPattern( const CSharedPtr<const CVectorBinarySetDescriptor>& ext)
		 { return new CPatternDescription(ext);}
	const CPatternDescription& Pattern( const IPatternDescriptor* p) const
		{ return debug_cast<const CPatternDescription&>(*p); }

	double& Thld() {return thld;}
	const double& Thld() const {return thld;}
	double& PropPrecisionThld() {return propPrecisionThld;}
	const double& PropPrecisionThld() const {return propPrecisionThld;}
	std::vector<double>& Precisions() {return precisions;}
	const std::vector<double>& Precisions() const {return precisions;}
	double& Precision() {return precision;}
	const double& Precision() const {return precision;}

	CVectorBinarySetJoinComparator& ExtCmp() const
		 {return *extCmp;}
	CPatternDeleter& ExtDeleter() {return extDeleter;}

	const CContext& Context() const
		 {return context;}
	const std::vector<DWORD>& AttrOrder() const
		 {return attrOrder;}
	const CCurrState& State() const
		{return state;}

	DWORD ValuesNumber( DWORD attr ) const
		{ assert( attr < values.size()); return values[attr].size(); }

	// A version of preimages function without a threshold verification.
	void JustPreimages( const CPatternDescription& p, CPatternList& preimages );

private:
	typedef std::deque<JsonIntervalPattern::CPattern> CTempContext;
	typedef std::vector< std::vector<double> > CAttrValues;

private:
	// Comparator for extents
	CSharedPtr<CVectorBinarySetJoinComparator> extCmp;
	CPatternDeleter extDeleter;
	// The threshlod for a measure.
	double thld;
	// Sets of every attribute value.
	CAttrValues values;
	// A representation of the context (used in conjunction with values).
	CContext context;
	std::vector<DWORD> attrOrder;
	// The vector of precisions for every attribute.
	double precision;
	std::vector<double> precisions;
	// Auto finding of precisions. The value [0,1] and the thld is set as maxDiff * thld.
	double propPrecisionThld;
	// A binary attribute corresponding to the corrent projection.
	CSharedPtr<CVectorBinarySetDescriptor> stateObjs;

	// State of the projection
	CCurrState state;

	// Temprory context
	CTempContext tmpContext;

	void convertContext();
	void computeAttrOrder();

	void leftPreimages( const CPatternDescription& p, CPatternList& preimages );
	void rightPreimages( const CPatternDescription& p, CPatternList& preimages );
	const CVectorBinarySetDescriptor& getStateObjs();
};

#endif // CINTERVALCLSPATTERNSPROJECTIONCHAIN_H

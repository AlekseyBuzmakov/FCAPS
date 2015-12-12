// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef BINCLSPATTERNSPROJECTIONCHAIN_H
#define BINCLSPATTERNSPROJECTIONCHAIN_H

#include <fcaps/ProjectionChain.h>

#include <fcaps/tools/StabilityCalculation.h>

#include <fcaps/modules/VectorBinarySetDescriptor.h>

#include <deque>

interface CBinarySetDescriptorsComparator;

////////////////////////////////////////////////////////////////////

class CBinClsPatternsProjectionChain : public IProjectionChain {
public:
	enum TAttributeOrder {
		// No order
		AO_None = 0,
		// Ascending/Descending order by size of the extent
		AO_AscesndingSize,
		AO_DescendingSize,
		// Random order of attributes
		AO_Random,

		AO_EnumCount
	};

public:
	CBinClsPatternsProjectionChain();

	// Methods of IProjectionChain
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
	virtual double GetProgress() const
		{ return double(currAttr)/attrOrder.size(); }
	virtual void Preimages( const IPatternDescriptor* d, CPatternList& preimages );

	virtual JSON SaveExtent( const IPatternDescriptor* d ) const;
	virtual JSON SaveIntent( const IPatternDescriptor* d ) const;

	// Methdos of the class
	// Get minimal support of a pattern. If less the pattern is suppressed.
	virtual double GetMinSupport() const
		{ return Thld();}

protected:
	class CPatternDescription : public IPatternDescriptor {
	public:
		CPatternDescription( const CSharedPtr<const CVectorBinarySetDescriptor>& e ) :
			extent(e), nextOptimAttr(-1) {}
		// Methdos of IPatternDescriptor
		virtual TPatternType GetType() const { return PT_Other; }
		virtual bool IsMostGeneral() const { /*TODO?*/return false; };
		virtual size_t Hash() const { return extent->Hash(); }

		// Methods of this class
		const CVectorBinarySetDescriptor& Extent() const { return *extent;}
		CList<DWORD>& Intent() const
			{return intent;}

		CList<DWORD>& AttrsInClosure() const
			{return attrsInClosure;}
		CList<DWORD>& EmptyAttrs() const
			{return emptyAttrs;}

		// The next attribute optimal for intersection
		DWORD& NextOptimAttr() const
			{ return nextOptimAttr; }

	private:
		CSharedPtr<const CVectorBinarySetDescriptor> extent;
		mutable CList<DWORD> intent;
		mutable CList<DWORD> attrsInClosure;
		mutable CList<DWORD> emptyAttrs;

		mutable DWORD nextOptimAttr;
	};

protected:
	// The threshlod for a measure.
	double& Thld() { return thld; }
	const double& Thld() const { return thld; }
	// The order in which attributes should be added to projections.
	TAttributeOrder& OrderType() { return order; }
	TAttributeOrder OrderType() const { return order; }

    // Load saves common params from json
	void LoadCommonParams( const JSON& );
	JSON SaveCommonParams() const;

	static const CPatternDescription& Pattern( const IPatternDescriptor* d )
		{ assert( d!= 0 ); return debug_cast<const CPatternDescription&>(*d); }

	// Creationg a new pattern
	virtual CPatternDescription* NewPattern(const CSharedPtr<const CVectorBinarySetDescriptor>& ext);
	// Given a pattern computes its possible preimage
	CPatternDescription* Preimage( const CPatternDescription& p );
	// Add new concept into consideration
	void NewConceptCreated( const CPatternDescription& p );
	// Report the intersection of pattern p with  the i-th attribute
	virtual void ReportAttrSimilarity( const CPatternDescription& p, DWORD i, const CVectorBinarySetDescriptor& res )
		{}


	// Getters of certain variables
	//  Return current processing attribute
	DWORD CurrAttr() const
		{ return currAttr; }
	//  Comparator of extents.
	CVectorBinarySetJoinComparator& ExtCmp()
		{ return *extCmp;}
	CPatternDeleter& ExtDeleter()
		{ return extDeleter; }
	DWORD ObjCount() const
		{return objCount;}
	const CBinarySetCollection& AttrToTidsetMap() const
		{return attrToTidsetMap; }
	const CVectorBinarySetDescriptor& GetTidset( DWORD attr ) const
		{ assert(attr <= CurrAttr()); return *attrToTidsetMap[attrOrder[attr]]; }
	const std::vector<DWORD>& Order() const
		{return attrOrder;}

private:
	// Comparator for extents
	CSharedPtr<CVectorBinarySetJoinComparator> extCmp;
	CPatternDeleter extDeleter;
	// Comparator for intents;
	CSharedPtr<CBinarySetDescriptorsComparator> intCmp;
	// The context in the form from attrs to objects
	CBinarySetCollection attrToTidsetMap;
	// The threshlod for a measure.
	double thld;
	// The number of objects
	DWORD objCount;
	DWORD currAttr;
	// The order in which attributes should be added to projections.
	TAttributeOrder order;
	std::vector<DWORD> attrOrder;

	// Temrorarily context
	std::deque< CList<DWORD> > tmpContext;

	// Sholud use conditional DB
	bool useConditionalDB;
	// Should turn on the conditional DB
	bool turnOnConditionalDB;
	// A flag checks if in current attrs there is a new concept added.
	bool hasNewConcept;
	// Counts the consequent number of projections without new concepts
	DWORD noNewConceptProjectionCount;

	void convertContext();
	void addColumnToTable(
		DWORD columnNum, const CList<DWORD>& values, CBinarySetCollection& table );
	void initAttrOrder();
	void updateConditionalDB(const CPatternDescription& p);
	void computeIntent( const CPatternDescription& d, CList<DWORD>& intent ) const;
};

#endif // BINCLSPATTERNSPROJECTIONCHAIN_H

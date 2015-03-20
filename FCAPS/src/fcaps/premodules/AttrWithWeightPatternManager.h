#ifndef ATTRIBUTEWITHWEIGHTPATTERNDESCRIPTOR_H
#define ATTRIBUTEWITHWEIGHTPATTERNDESCRIPTOR_H

#include <PatternManager.h>

class CAttrWithWeightPatternDescriptor : public IPatternDescriptor {
public:
	CAttrWithWeightPatternDescriptor() :
		attr( -1 ), weight( 0 ) {}
	// Methods of IPatternDescriptor
	virtual TPatternType GetType() const
		{ return PT_AttrWithWeight; }
	virtual bool IsMostGeneral() const
		{ return weight == 0; }
	virtual size_t Hash() const
		{ return attr ^ (weight << 24);}

	// Methods of Class
	// Get/Set attribute num
	DWORD GetAttr() const
		{ return attr; }
	void SetAttr( DWORD value )
		{ attr = value; }

	// Get/Set attribute weight
	DWORD GetWeight() const
		{ return weight; }
	void SetWeight( DWORD value )
		{ weight = value; }

private:
	DWORD attr;
	DWORD weight;
};

class CAttrWithWeightPatternComparator : public IPatternManager {
public:
	// Methods of IPatternManager
	virtual TPatternType GetPatternsType() const
		{ return PT_AttrWithWeight; }
	virtual void PreprocessObjectDescription( const IPatternDescriptor* desc ) const;

protected:
	// Methods of IPatternManager
	virtual IPatternDescriptor* CalculateSimilarity(
		const IPatternDescriptor* first, const IPatternDescriptor* second ) const;
	virtual TCompareResult Compare(
		const IPatternDescriptor* first, const IPatternDescriptor* second,
		DWORD interestingResults, DWORD possibleResults ) const;
	virtual void Write( const IPatternDescriptor* pattern, std::ostream& dst ) const;
	virtual CAttrWithWeightPatternDescriptor* NewPattern() const
		{ return new CAttrWithWeightPatternDescriptor; }
};

#endif // ATTRIBUTEWITHWEIGHTPATTERNDESCRIPTOR_H

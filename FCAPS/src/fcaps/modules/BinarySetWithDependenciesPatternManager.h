#ifndef BINARYSETDESCRIPTORSWITHDEPENDENCIESCOMPARATOR_H
#define BINARYSETDESCRIPTORSWITHDEPENDENCIESCOMPARATOR_H

#include <fcaps/modules/BinarySetPatternManager.h>
#include <boost/ptr_container/ptr_vector.hpp>

const char BinarySetDescriptorsWithDependenciesComparator[] = "BinarySetPatternManagerWithDependenciesModules";

class CBinarySetDescriptorsWithDependenciesComparator : public CBinarySetDescriptorsComparator {
public:
	CBinarySetDescriptorsWithDependenciesComparator();
	virtual ~CBinarySetDescriptorsWithDependenciesComparator();

	// Methods of CBinarySetDescriptorsComparator
	virtual void LoadParams( const JSON& json );
	virtual JSON SaveParams() const;

	virtual const CBinarySetPatternDescriptor* LoadObject( const JSON& json );
	virtual JSON SavePattern( const IPatternDescriptor* ptrn ) const;
	virtual const CBinarySetPatternDescriptor* LoadPattern( const JSON& json );

	virtual const CBinarySetPatternDescriptor* CalculateSimilarity(
		const IPatternDescriptor* first, const IPatternDescriptor* second );

private:
	static const CModuleRegistrar<CBinarySetDescriptorsWithDependenciesComparator> registrar;
	static const char jsonAttrPosetPath[];

	// Path to JSON file with the poset
	std::string posetPath;
	// Conversion form old to new and vice versa.
	std::vector<int> toOldAttrs;
	std::vector<int> toNewAttrs;
	// Dependencies
	boost::ptr_vector<CBinarySetPatternDescriptor> dependencies;
	std::vector< std::list<int> > attrsToDependencies;


	void loadPoset();
	void addMappings( DWORD oldAttr, DWORD newAttr );
	static void convertAttributes( CBinarySetPatternDescriptor& p, const std::vector<int>& mapping );
	void projectDescription( CBinarySetPatternDescriptor& p ) const;
};

#endif // BINARYSETDESCRIPTORSWITHDEPENDENCIESCOMPARATOR_H

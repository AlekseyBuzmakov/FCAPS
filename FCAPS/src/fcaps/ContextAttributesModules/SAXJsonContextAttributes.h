// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, GPL v2 license, 2018, v0.7

#ifndef SAXJSONATTRIBUTECONTEXT_H
#define SAXJSONATTRIBUTECONTEXT_H

#include <common.h>

#include <fcaps/ContextAttributes.h>
#include <fcaps/Module.h>
#include <fcaps/Extent.h>

#include <ModuleTools.h>

#include <unordered_set>
#include <deque>

////////////////////////////////////////////////////////////////////

const char SAXJsonContextAttributes[] = "SAXJsonContextAttributesModule";

////////////////////////////////////////////////////////////////////

class CSAXJsonContextAttributes : public IContextAttributes, public IModule {
public:
	enum TAttrOrderMode {
		AO_None = 0, // initial order
		AO_Desc, // Descending order
		AO_Asc, // Ascending order
		AO_Random, // Random order

		AO_Count
	};

	enum TWrittingMode {
		// Only names of attributes are written
		WM_Names = 0,
		// Only indices of attributes are written
		WM_Indices,
		// Names and indices of attributes are written
		WM_Both,

		WM_EnumCount
	};

	struct CAttribute {
		CPatternImage Image;
		std::string Name;
	};
	typedef std::deque<CAttribute> TAttributes;
public:
	CSAXJsonContextAttributes();
	~CSAXJsonContextAttributes();

	// Methods of IContextAttributes
	virtual int GetObjectNumber() const
		{ return objectNum; }
	virtual bool HasAttribute(int a)
		{assert( attrOrder.size() == attributes.size());return 0 <= a && a < attributes.size();}
	virtual void GetAttribute( int a, CPatternImage& pattern )
	{ assert(HasAttribute(a)); pattern = attributes[attrOrder[a]].Image;}
	virtual void ClearMemory( CPatternImage& pattern )
		{return;} // memory is not allocated and controled in the destructor}

	virtual int GetNextAttribute(int a)
		{return a+1;}
	virtual int GetNextNonChildAttribute(int a)
		{return a+1;}
	virtual JSON DescribeAttributeSet(int* attrsSet, int attrsCount);

 	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); }
	virtual const char* const GetName() const
		{ return Name(); }
	// For CModuleRegistrar
	static const char* const Type()
		{ return ContextAttributesModuleType;}
	static const char* const Name()
		{ return SAXJsonContextAttributes; }
	static const char* const Desc();

private:
	typedef std::vector<int> TAttributeOrder;

private:
	static const CModuleRegistrar<CSAXJsonContextAttributes> registrar;
	// The path to the file
	std::string filePath;
	// Number of objects in the context
	DWORD objectNum;
	// The set of attributes
	TAttributes attributes;
	// The order of attributes
	TAttributeOrder attrOrder;
	// The order of the attributes
	TAttrOrderMode order;
	// Writing mode of attributes
	TWrittingMode mode;

	void loadContext();
};

#endif // SAXJSONATTRIBUTECONTEXT_H

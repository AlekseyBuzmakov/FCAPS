// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, GPL v2 license, 2018, v0.7

#ifndef JSONATTRIBUTECONTEXT_H
#define JSONATTRIBUTECONTEXT_H

#include <common.h>

#include <fcaps/ContextAttributes.h>
#include <fcaps/Module.h>
#include <fcaps/Extent.h>

#include <ModuleTools.h>

#include <unordered_set>

////////////////////////////////////////////////////////////////////

const char JsonContextAttributes[] = "JsonContextAttributesModule";

////////////////////////////////////////////////////////////////////

class CJsonContextAttributes : public IContextAttributes, public IModule {
public:
	enum TAttributeOrder {
		AO_None = 0, // initial order
		AO_Desc, // Descending order
		AO_Asc, // Ascending order
		AO_Random, // Random order

		AO_Count
	};
public:
	CJsonContextAttributes();
	~CJsonContextAttributes();

	// Methods of IContextAttributes
	virtual int GetObjectNumber() const
		{ return objectNum; }
	virtual bool HasAttribute(int a)
		{return 0 <= a && a < attributes.size();}
	virtual void GetAttribute( int a, CPatternImage& pattern )
		{ assert(HasAttribute(a)); pattern = attributes[a].Image;}
	virtual void ClearMemory( CPatternImage& pattern )
		{return;} // memory is not allocated and controled in the destructor}

	virtual int GetNextAttribute(int a)
		{return a+1;}
	virtual int GetNextNonChildAttribute(int a)
		{return a+1;}

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
		{ return JsonContextAttributes; }
	static const char* const Desc();

private:
	struct CAttribute {
		CPatternImage Image;
	};
private:
	static const CModuleRegistrar<CJsonContextAttributes> registrar;
	// The path to the file
	std::string filePath;
	// Number of objects in the context
	DWORD objectNum;
	// The set of attributes
	std::vector<CAttribute> attributes;
	// The order of the attributes
	TAttributeOrder order;

	void loadContext();
};

#endif // JSONATTRIBUTECONTEXT_H

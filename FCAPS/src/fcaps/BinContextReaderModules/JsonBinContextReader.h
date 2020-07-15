// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, GPL v2 license, 2018, v0.7

#ifndef JSONBINCONTEXTREADER_H
#define JSONBINCONTEXTREADER_H

#include <common.h>

#include <fcaps/BinContextReader.h>
#include <fcaps/Module.h>
#include <fcaps/Extent.h>

#include <ModuleTools.h>

#include <unordered_set>
#include <deque>

////////////////////////////////////////////////////////////////////

class CSAXAttributeReader;

////////////////////////////////////////////////////////////////////

const char JsonBinContextReader[] = "JsonBinContextReaderModule";

////////////////////////////////////////////////////////////////////

class CJsonBinContextReader : public IBinContextReader, public IModule {
public:
	enum TAttrOrderMode {
		AO_None, // initial order
		AO_Desc, // Descending order
		AO_Asc, // Ascending order
		AO_Random, // Random order

		AO_Count
	};

	struct CAttribute {
		JSON Name;
		int Support;

		CAttribute() : Support(0) {}
	};
	typedef std::deque<CAttribute> TAttributes;
public:
	CJsonBinContextReader();

	virtual int GetObjectNumber() const
		{return objectNum; }
	virtual bool HasAttribute(TAttributeID a) const
		{ assert( 0 <= a && attributes.size() == attrOrder.size() ); return 0 <= a && a < attributes.size();}
	virtual JSON GetAttributeName(TAttributeID a) const
		{assert( 0 <= a && a < attributes.size() && attributes.size() == attrOrder.size() ); return attributes[attrOrder[a]].Name; }
    virtual int GetSupport(TAttributeID a) const
		{assert( 0 <= a && a < attributes.size() && attributes.size() == attrOrder.size() ); return attributes[attrOrder[a]].Support; }

    void Start();
    int GetNextObjectIntentSize();
    void GetNextObject(CObjectIntent& intent);

 	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;
	virtual const char* const GetType() const
		{ return Type(); }
	virtual const char* const GetName() const
		{ return Name(); }
	// For CModuleRegistrar
	static const char* const Type()
		{ return BinContextReaderModuleType;}
	static const char* const Name()
		{ return JsonBinContextReader; }
	static const char* const Desc();

private:
	typedef std::vector<int> TAttributeOrder;

private:
	static const CModuleRegistrar<CJsonBinContextReader> registrar;
	// The path to the file
	std::string filePath;
	// Number of objects in the context
	int objectNum;
	// The set of attributes
	TAttributes attributes;
	// The order of attributes (for every i shows the internal attribute number)
	TAttributeOrder attrOrder;
	// The reverse order of attributes (every i is the internal attribute number, the i-th value hows the external attribute value)
	TAttributeOrder reverseAttrOrder;
	
	// The order of the attributes
	TAttrOrderMode order;

	// SaxReader
	CPtrOwner<CSAXAttributeReader> saxReader;

	// Saves the next object to be reported
	int nextObjectAttrCount;
	const int* nextObjectData; 

	void loadContext();
};

#endif // JSONBINCONTEXTREADER_H

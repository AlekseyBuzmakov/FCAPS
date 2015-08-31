// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#ifndef CEXCEPTION_H
#define CEXCEPTION_H

#include <common.h>

////////////////////////////////////////////////////////////////////

class CException {
public:
	CException( const std::string& _place ) :
		place( _place ) {}
	const std::string& GetPlace() const
		{ return place; }
	const std::string& GetText() const
		{ return text; }

protected:
	void SetText( const std::string& newText )
		{ text = newText; }
private:
	std::string place;
	std::string text;
};

////////////////////////////////////////////////////////////////////

class CTextException : public CException {
public:
	CTextException( const std::string& place, const std::string& error ) :
		CException( place ) { SetText( error ); }
};

////////////////////////////////////////////////////////////////////

#endif // CEXCEPTION_H

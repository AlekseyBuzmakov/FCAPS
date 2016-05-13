// Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

#include <ConsoleApplication.h>

#include <Exception.h>
#include <StdTools.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

CConsoleApplication::CConsoleApplication( int _argc, char **_argv ) :
	argc( _argc ),
	argv( _argv ),
	isSilent( false ),
	isJustHelp( false ),
	runTimeLimit(-1)
{
	string baseName;
	RelativePathes::BaseName(argv[0], baseName);
	searchPathHolder.SetPath( baseName );

	startTime = time( NULL );
}

int CConsoleApplication::Run()
{
	assert( argc >= 1 );

	for( int i = 1; i < argc; ++i ) {
		processArg( argv[i] );
	}
	if( !outParamFile.empty() ) {
		CDestStream dst( outParamFile );
		dst << paramFileContent;
	}

	if( !FinalizeParams() || !AreParamsCorrect() || isJustHelp ) {
		if( !isJustHelp ) {
			GetErrorStream() << "(!) Wrong Parameters\n";
		}
		if( isJustHelp || !isSilent ) {
			GetInfoStream() << "Usage:\n";
			GetInfoStream() << GetCmdLineDescription( argv[0] ) << "\n";
			GetInfoStream() << "General Params:\n";
			GetInfoStream() << "\t--help - out only help, all other params are ignored\n";
			GetInfoStream() << "\t--infoLogging:{PATH} - print INFO in a file\n";
			GetInfoStream() << "\t--silent - minimaze output\n";
			GetInfoStream() << "\t--paramfile:{PATH} - path to file with one param per one line\n";
			GetInfoStream() << "\t--writeparamfile:{PATH} - write down the file with all params\n\n";
		}
		return isJustHelp ? 0 : 1;
	}
	int result = 1;
	try{
		result = Execute();
	} catch( CException * e ) {
		GetErrorStream() << "Unhandled exception:\n"
			<< e->GetPlace() << "\n"
			<< e->GetText() << "\n";
		delete e;
		return 1;
	} catch( ... ) {
		GetErrorStream() << "!!! Unknown exception\n";
		return 2;
	}
	return result;
}

bool CConsoleApplication::ProcessParam( const std::string& param, const std::string& value )
{
	if( param == "--paramfile" ) {
		processParamFile( value );
	} else if ( param == "--writeparamfile" ) {
		outParamFile = value;
	} else if ( param == "--infoLogging" ) {
		infoStream.open( value.c_str() );
	} else if ( param == "--timeLimit" ) {
		runTimeLimit = boost::lexical_cast<DWORD>( value );
	} else {
		return false;
	}
	return true;
}

bool CConsoleApplication::ProcessOption( const std::string& option )
{
	if( option == "--help" ) {
		isJustHelp = true;
	} else if( option == "--silent" ) {
		isSilent = true;
	} else {
		return false;
	}
	return true;
}

ostream& CConsoleApplication::GetStatusStream() const
{
	checkRunningTime();
	if( !isSilent ) {
		return cout;
	} else {
		static ostream nullStream(0);
		return nullStream;
	}
}
ostream& CConsoleApplication::GetInfoStream() const
{
	checkRunningTime();
	if( !infoStream.is_open() ) {
		return cout;
	} else {
		return infoStream;
	}
}

ostream& CConsoleApplication::GetWarningStream() const
{
	checkRunningTime();
	if( !isSilent ) {
		return cerr;
	} else {
		static ostream nullStream(0);
		return nullStream;
	}
}

ostream& CConsoleApplication::GetErrorStream() const
{
	checkRunningTime();
	return cerr;
}

void CConsoleApplication::processArg( const std::string& arg )
{
	if( arg.find( "--writeparamfile:" ) == string::npos ) {
		paramFileContent += arg + "\n";
	}

	bool result = false;
	if( arg[0] != '-' ) {
		result = ProcessString( arg );
	} else {
		const size_t optionPos = arg.find( ":" );
		if( optionPos != string::npos ) {
			const string param = arg.substr( 0, optionPos );
			const string value = arg.substr( optionPos + 1 );
			result = ProcessParam( param, value );
		} else {
			const string option = arg;
			result = ProcessOption( option );
		}
	}
	if( !result ) {
		cout << "(!) bad param '"<< arg << "' -- ignored\n";
	}
}

void CConsoleApplication::processParamFile( const std::string& path )
{
	CSourceStream src( path );
	string str;
	while( !src.eof() ) {
		getline( src, str );
		trim( str );
		if( str.empty() ) {
			continue;
		}
		if( str[0] == '#' ) {
			// A comment line
			continue;
		}
		processArg( str );
	}
}

void CConsoleApplication::checkRunningTime() const
{
	if( runTimeLimit == (DWORD)-1 ) {
		return;
	}
	const time_t elapsedTime= time( NULL ) - startTime;
	if( elapsedTime > runTimeLimit ) {
		throw CTextException( "CConsoleApplication::checkRunningTime",
			"Too long execution (more than " + StdExt::to_string( runTimeLimit ) + ")" );
	}
}


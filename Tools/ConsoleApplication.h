#ifndef CCONSOLEAPPLICATION_H
#define CCONSOLEAPPLICATION_H

#include <common.h>
#include <string>

class CConsoleApplication {
public:
	CConsoleApplication( int _argc, char** _argv );
	virtual ~CConsoleApplication()
		{}

	// Run application
	int Run();

protected:
	// Get description for command line
	virtual std::string GetCmdLineDescription( const std::string& progName ) const = 0;

	// Process new found param '-param:{param-value}'
	virtual bool ProcessParam( const std::string& param, const std::string& value );
	// Process new found option '-option'
	virtual bool ProcessOption( const std::string& option );
	// Process other string parameters
	virtual bool ProcessString( const std::string& str )
		{ return false; }

	// Finalize params processing: allparams where processed.
	virtual bool FinalizeParams()
		{ return true; }
	// Check if all obligitory parameters are mention
	virtual bool AreParamsCorrect() const
		{ return true; }

	// Execute the program
	virtual int Execute() = 0;

	// Service methods
	// Stream for output important info. It is never mute.
	std::ostream& GetInfoStream() const;
	// Stream for output status info. It is mute for silent mode.
	std::ostream& GetStatusStream() const;
	// Stream for warnings. It is mute in silent mode.
	std::ostream& GetWarningStream() const;
	// Stream for errors.
	std::ostream& GetErrorStream() const;

private:
	const int argc;
	const char*const*const argv;
	bool isSilent;
	bool isJustHelp;
	std::string outParamFile;
	std::string paramFileContent;
	mutable std::ofstream infoStream;

	void processArg( const std::string& arg );
	void processParamFile( const std::string& path );
};

#endif // CCONSOLEAPPLICATION_H

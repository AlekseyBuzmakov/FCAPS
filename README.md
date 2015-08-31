# FCAPS

## Compilation
You will need
* GNU make
* git
* boost (http://sourceforge.net/projects/boost/files/boost/1.55.0/)

Please put boost in the _root_ of the git folder under the name __boost__, you can put a symbolic (or a hard) link.

Run __make__ that will fetch _rapidjson_ from https://github.com/miloyip/rapidjson.git and apply a patch, then it compiles the application.

The result can be found in __Sofia-PS/bin__.

*If the program cannot be linked try to remove '-s' key in line 'LDFLAGS_RELEASE = $(LDFLAGS) -s' from Sofia-PS/makefile*


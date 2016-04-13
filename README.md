# FCAPS

## License and copyright

Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, GPL v2 license, 2011-2015, v0.7

## Compilation
You will need
* git
* [boost](http://www.boost.org/)
* [premake4](https://premake.github.io/download.html)

Boost libraries should be in the _root_ of the git folder under the name __boost__. 
You can put a symbolic (or a hard) link.

> $ ln -s "path/to/boost" boost \# Linux or MacOS

> $ mklink /D boost "path/to/boost" \# Windows

Some boost libraries (regex, thread, system, filesystem) should be compiled:
* Go to boost folder and run 

> $ ./bootstrap.

* Then build all libraries

> $ ./b2 release stage

* _If you want a faster compiling please read the instructions from the **Boost** library._

The next step is to convert the project to your most loved envirement. For that run

> $ premake4 {configuration}

The most widely used configurations are 

> $ premake4 vs2005|vs2008|vs2010|vs2012 \# for diferent version of visual studio **sln** file

> $ premake4 gmake \# for GNU make file

> $ premake4 codeblocks \# for cbp file of code::blocks IDE

> $ premake4 clean \# for removing the created files

Basically this script fetches [rapidjson](https://github.com/miloyip/rapidjson.git) and apply the file rapidjson.patch.
Then it converts the description from the file __premake4.lua__ to the format of your IDE in love.
Then, I guess, you know what to do with the resulting file.

* *For the moment, on some configurations, e.g., Windows and MS Visual Studio, a manual naming of static linked libraries is needed. This libraries are found in boost/stage/libs, but their names are not fixed. They are named like "lib{BOOST LIB NAME}-bla-bla-bla".{a|lib}, for example 'libboost_regex-gcc34-mt-d-1_36.a'*
* *Be careful, the boost libraries should be compiled by the same toolchain as the main program. For instance it is not possible to compile the boost libraries by the MSVS2015 toolchain and the program by the MSVS2013 toolchain. Errors of linkage occur in this case.* 

The result can be found in __bin/__.

*There is a dependency on file __../LibGastonForSofia/inc__. It can be found [here](https://github.com/AlekseyBuzmakov/LibGastonForSofia/blob/master/inc/LibGastonForSofia.h).
We will remove this dependency as soon as possible.*

<!--*If the program cannot be linked try to remove '-s' key in line 'LDFLAGS_RELEASE = $(LDFLAGS) -s' from Sofia-PS/makefile*-->

